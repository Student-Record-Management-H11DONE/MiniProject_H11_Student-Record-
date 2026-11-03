
#include "grades.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define CSV_LINE_SIZE 256

/* Remove leading and trailing whitespace from a field. */
static void trim_field(char *field)
{
    size_t len;
    char *start = field;

    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (start != field) {
        memmove(field, start, strlen(start) + 1);
    }

    len = strlen(field);

    while (len > 0 &&
           isspace((unsigned char)field[len - 1])) {
        field[--len] = '\0';
    }
}

/* Safely copy a field into a fixed-size destination. */
static int copy_field(char *destination,
                      size_t capacity,
                      const char *source)
{
    size_t length = strlen(source);

    if (length == 0 || length >= capacity) {
        return 0;
    }

    memcpy(destination, source, length + 1);
    return 1;
}

/* Validate and convert marks between 0 and 100. */
static int parse_marks(const char *text, double *marks)
{
    char *end;
    double value;

    errno = 0;
    value = strtod(text, &end);

    if (text == end || errno == ERANGE) {
        return 0;
    }

    while (isspace((unsigned char)*end)) {
        end++;
    }

    if (*end != '\0' ||
        value < 0.0 ||
        value > 100.0) {
        return 0;
    }

    *marks = value;
    return 1;
}

/* Load grade records from a CSV file. */
int grades_load_csv(const char *path,
                    GradeRecord records[],
                    size_t capacity,
                    size_t *count)
{
    FILE *file;
    char line[CSV_LINE_SIZE];
    size_t loaded = 0;
    int first_line = 1;

    if (count == NULL) {
        return 0;
    }

    *count = 0;

    if (path == NULL ||
        (records == NULL && capacity != 0) ||
        capacity > GRADE_RECORDS_MAX) {
        return 0;
    }

    file = fopen(path, "r");

    if (file == NULL) {
        return 0;
    }

    while (fgets(line, sizeof line, file) != NULL) {
        char *first;
        char *second;
        char *third;
        char *fourth;
        GradeRecord record;
        size_t length = strlen(line);
        int ch;

        /* Reject CSV lines that exceed the buffer size. */
        if (length > 0 && line[length - 1] != '\n') {
            ch = fgetc(file);

            if (ch != EOF) {
                fclose(file);
                return 0;
            }

            if (ferror(file)) {
                fclose(file);
                return 0;
            }
        }

        /* Remove line endings. */
        line[strcspn(line, "\r\n")] = '\0';

        /* Validate CSV header. */
        if (first_line) {
            first_line = 0;

            if (strcmp(line,
                       "student_id,subject,marks,grade") != 0) {
                fclose(file);
                return 0;
            }

            continue;
        }

        if (line[0] == '\0') {
            continue;
        }

        /* Require exactly four CSV fields. */
        first = strchr(line, ',');

        if (first == NULL) {
            fclose(file);
            return 0;
        }

        second = strchr(first + 1, ',');

        if (second == NULL) {
            fclose(file);
            return 0;
        }

        third = strchr(second + 1, ',');

        if (third == NULL ||
            strchr(third + 1, ',') != NULL) {
            fclose(file);
            return 0;
        }

        *first = '\0';
        *second = '\0';
        *third = '\0';

        fourth = third + 1;

        /* Remove unnecessary whitespace. */
        trim_field(line);
        trim_field(first + 1);
        trim_field(second + 1);
        trim_field(fourth);

        /* Validate and copy the record fields. */
        if (!copy_field(record.student_id,
                        sizeof record.student_id,
                        line) ||
            !copy_field(record.subject,
                        sizeof record.subject,
                        first + 1) ||
            !copy_field(record.grade,
                        sizeof record.grade,
                        fourth) ||
            !parse_marks(second + 1,
                         &record.marks)) {
            fclose(file);
            return 0;
        }

        if (loaded >= capacity ||
            loaded >= GRADE_RECORDS_MAX) {
            fclose(file);
            return 0;
        }

        records[loaded++] = record;
    }

    if (ferror(file) || first_line) {
        fclose(file);
        return 0;
    }

    if (fclose(file) != 0) {
        return 0;
    }

    *count = loaded;
    return 1;
}

/* Retrieve grade records belonging to a particular student. */
size_t grades_get_student_records(
    const GradeRecord records[],
    size_t count,
    const char *student_id,
    GradeRecord results[],
    size_t capacity)
{
    size_t i;
    size_t found = 0;

    if (records == NULL ||
        student_id == NULL ||
        results == NULL) {
        return 0;
    }

    for (i = 0; i < count && found < capacity; i++) {
        if (strcmp(records[i].student_id,
                   student_id) == 0) {
            results[found++] = records[i];
        }
    }

    return found;
}

/* Display subject-wise student grades. */
void grades_display(const GradeRecord records[],
                    size_t count,
                    FILE *output)
{
    size_t i;

    if (output == NULL) {
        return;
    }

    if (records == NULL || count == 0) {
        fprintf(output, "No grade records available.\n");
        return;
    }

    fprintf(output, "\n");
    fprintf(output,
            "-----------------------------------------------\n");
    fprintf(output,
            "             STUDENT GRADE REPORT\n");
    fprintf(output,
            "-----------------------------------------------\n");

    fprintf(output, "%-12s %-20s %-10s %-10s\n",
            "Student ID",
            "Subject",
            "Marks",
            "Grade");

    fprintf(output,
            "-----------------------------------------------\n");

    for (i = 0; i < count; i++) {
        fprintf(output,
                "%-12s %-20s %-10.2f %-10s\n",
                records[i].student_id,
                records[i].subject,
                records[i].marks,
                records[i].grade);
    }

    fprintf(output,
            "-----------------------------------------------\n");
}
