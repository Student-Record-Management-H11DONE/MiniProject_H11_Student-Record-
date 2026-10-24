
#include "student_storage.h"
#include "validation.h"

#include <stdio.h>
#include <string.h>

#define CSV_LINE_SIZE 256

/* Check that a fixed-size field contains a terminating null byte. */
static int field_length(const char *field, size_t capacity, size_t *length)
{
    const char *end;

    if (field == NULL || length == NULL) {
        return 0;
    }

    end = (const char *)memchr(field, '\0', capacity);

    if (end == NULL) {
        return 0;
    }

    *length = (size_t)(end - field);
    return 1;
}

/* CSV fields cannot contain delimiters or line breaks in this format. */
static int csv_field_valid(const char *field, size_t capacity,
                           size_t maximum, int required)
{
    size_t length;

    if (!field_length(field, capacity, &length) ||
        length > maximum ||
        (required && length == 0) ||
        strchr(field, ',') != NULL ||
        strchr(field, '\n') != NULL ||
        strchr(field, '\r') != NULL) {
        return 0;
    }

    return 1;
}

static int record_valid(const StudentRecord *record)
{
    if (record == NULL ||
        !csv_field_valid(record->student_id,
                         sizeof record->student_id,
                         STUDENT_ID_MAX, 1) ||
        !csv_field_valid(record->student_name,
                         sizeof record->student_name,
                         STUDENT_NAME_MAX, 1) ||
        !csv_field_valid(record->course,
                         sizeof record->course,
                         STUDENT_COURSE_MAX, 1)) {
        return 0;
    }

    return validate_student_fields(record->student_id,
                                    record->student_name);
}

/* FR-22: Save student records to CSV. */
int student_storage_save(const char *path,
                         const StudentRecord records[],
                         size_t count)
{
    FILE *file;
    size_t i;

    if (path == NULL ||
        (records == NULL && count != 0) ||
        count > STUDENT_RECORDS_MAX) {
        return 0;
    }

    /* Validate everything before opening the file for writing. */
    for (i = 0; i < count; i++) {
        if (!record_valid(&records[i])) {
            return 0;
        }
    }

    file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (fprintf(file, "%s,%s,%s\n",
                    records[i].student_id,
                    records[i].student_name,
                    records[i].course) < 0) {
            fclose(file);
            return 0;
        }
    }

    if (fclose(file) != 0) {
        return 0;
    }

    return 1;
}

/* FR-23: Load student records from CSV. */
int student_storage_load(const char *path,
                         StudentRecord records[],
                         size_t capacity,
                         size_t *count)
{
    FILE *file;
    char line[CSV_LINE_SIZE];
    size_t loaded = 0;

    if (count == NULL) {
        return 0;
    }

    *count = 0;

    if (path == NULL || records == NULL ||
        capacity > STUDENT_RECORDS_MAX) {
        return 0;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        return 0;
    }

    while (fgets(line, sizeof line, file) != NULL) {
        char *first, *second, *third;
        size_t length;
        int ch;
        StudentRecord record;

        length = strlen(line);

        /* Reject lines that exceed the buffer size. */
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

        /* Require exactly three CSV fields. */
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

        if (strchr(second + 1, ',') != NULL) {
            fclose(file);
            return 0;
        }

        *first = '\0';
        *second = '\0';

        third = second + 1;

        if (!csv_field_valid(line, strlen(line) + 1,
                             STUDENT_ID_MAX, 1) ||
            !csv_field_valid(first + 1, strlen(first + 1) + 1,
                             STUDENT_NAME_MAX, 1) ||
            !csv_field_valid(third, strlen(third) + 1,
                             STUDENT_COURSE_MAX, 1) ||
            !validate_student_fields(line, first + 1)) {
            fclose(file);
            return 0;
        }

        if (loaded >= capacity ||
            loaded >= STUDENT_RECORDS_MAX) {
            fclose(file);
            return 0;
        }

        strcpy(record.student_id, line);
        strcpy(record.student_name, first + 1);
        strcpy(record.course, third);

        records[loaded++] = record;
    }

    if (ferror(file)) {
        fclose(file);
        return 0;
    }

    if (fclose(file) != 0) {
        return 0;
    }

    *count = loaded;
    return 1;
}
