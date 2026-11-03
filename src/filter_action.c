
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "filter_action.h"
#include "auth.h"
#include "actions.h"
#include "ui.h"

#define FILTER_DATA_FILE "filter_students.csv"
#define FILTER_LINE_SIZE 512

/* Compare two strings without case sensitivity. */
static int matches_ignore_case(const char *a, const char *b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }

    while (*a && *b) {
        if (tolower((unsigned char)*a) !=
            tolower((unsigned char)*b)) {
            return 0;
        }

        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

/* Select the field according to the filter type. */
static const char *get_filter_field(
    const FilterStudentRecord *record,
    FilterType filter)
{
    if (record == NULL) {
        return NULL;
    }

    switch (filter) {
        case FILTER_BY_COURSE:
            return record->course;

        case FILTER_BY_DEPARTMENT:
            return record->department;

        case FILTER_BY_SEMESTER:
            return record->semester;

        default:
            return NULL;
    }
}

/* Filter records by course, department or semester. */
size_t filter_student_records(
    const FilterStudentRecord records[],
    size_t count,
    FilterType filter,
    const char *value,
    FilterStudentRecord results[],
    size_t capacity)
{
    size_t i;
    size_t matched = 0;

    if (records == NULL || value == NULL ||
        results == NULL || value[0] == '\0' ||
        (filter != FILTER_BY_COURSE &&
         filter != FILTER_BY_DEPARTMENT &&
         filter != FILTER_BY_SEMESTER)) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        const char *field = get_filter_field(&records[i], filter);

        if (field != NULL &&
            matches_ignore_case(field, value)) {

            if (matched >= capacity) {
                break;
            }

            results[matched++] = records[i];
        }
    }

    return matched;
}

/*
 * Load student records from CSV.
 * Format: StudentID,Name,Course,Department,Semester
 *
 * Supports Windows (CRLF) and Unix (LF) line endings.
 */
static int load_filter_records(
    const char *path,
    FilterStudentRecord records[],
    size_t capacity,
    size_t *count)
{
    FILE *file;
    char line[FILTER_LINE_SIZE];
    size_t loaded = 0;

    if (path == NULL || records == NULL || count == NULL) {
        return 0;
    }

    *count = 0;

    file = fopen(path, "r");

    if (file == NULL) {
        return 0;
    }

    while (fgets(line, sizeof line, file) != NULL) {
        FilterStudentRecord record;
        char *fields[5];
        char *start;
        char *comma;
        size_t i;
        size_t length;

        /* Detect a line that did not fit in the buffer. */
        length = strlen(line);

        if (length == sizeof line - 1 &&
            line[length - 1] != '\n' &&
            !feof(file)) {
            fclose(file);
            return 0;
        }

        /* Remove Windows and Unix line endings. */
        line[strcspn(line, "\r\n")] = '\0';

        start = line;

        /* Split the first four fields. */
        for (i = 0; i < 4; i++) {
            comma = strchr(start, ',');

            if (comma == NULL) {
                fclose(file);
                return 0;
            }

            *comma = '\0';
            fields[i] = start;
            start = comma + 1;
        }

        /* The final field must not contain another comma. */
        if (strchr(start, ',') != NULL) {
            fclose(file);
            return 0;
        }

        fields[4] = start;

        memset(&record, 0, sizeof record);

        /* Validate field lengths and copy safely. */
        for (i = 0; i < 5; i++) {
            length = strlen(fields[i]);

            if (length == 0) {
                fclose(file);
                return 0;
            }

            switch (i) {
                case 0:
                    if (length > FILTER_STUDENT_ID_MAX) {
                        fclose(file);
                        return 0;
                    }
                    strcpy(record.student_id, fields[i]);
                    break;

                case 1:
                    if (length > FILTER_STUDENT_NAME_MAX) {
                        fclose(file);
                        return 0;
                    }
                    strcpy(record.student_name, fields[i]);
                    break;

                case 2:
                    if (length > FILTER_COURSE_MAX) {
                        fclose(file);
                        return 0;
                    }
                    strcpy(record.course, fields[i]);
                    break;

                case 3:
                    if (length > FILTER_DEPARTMENT_MAX) {
                        fclose(file);
                        return 0;
                    }
                    strcpy(record.department, fields[i]);
                    break;

                case 4:
                    if (length > FILTER_SEMESTER_MAX) {
                        fclose(file);
                        return 0;
                    }
                    strcpy(record.semester, fields[i]);
                    break;
            }
        }

        if (loaded >= capacity) {
            fclose(file);
            return 0;
        }

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

/* FR-18: Interactive student record filtering. */
void action_filter_records(const Session *s)
{
    static FilterStudentRecord records[FILTER_RECORDS_MAX];
    static FilterStudentRecord results[FILTER_RECORDS_MAX];

    char value[100];
    char message[100];

    size_t count = 0;
    size_t matched;
    size_t i;

    int choice;

    /* Only Administrator and Faculty can filter records. */
    if (s == NULL ||
        (s->role != ROLE_ADMINISTRATOR &&
         s->role != ROLE_FACULTY) ||
        !has_permission(s->role, PERM_FILTER_RECORDS)) {

        ui_error("You do not have permission to filter student records.");
        return;
    }

    ui_banner("Filter Student Records");

    printf("1. Filter by Course\n");
    printf("2. Filter by Department\n");
    printf("3. Filter by Semester\n");

    choice = ui_read_choice("Select filter:", 1, 3);

    if (choice == -1) {
        return;
    }

    if (!ui_read_line(
            "Enter filter value:",
            value,
            sizeof value)) {
        return;
    }

    if (value[0] == '\0') {
        ui_error("Filter value cannot be empty.");
        return;
    }

    /* Load the sample student dataset. */
    if (!load_filter_records(
            FILTER_DATA_FILE,
            records,
            FILTER_RECORDS_MAX,
            &count)) {

        ui_error("Unable to load filter student records.");
        return;
    }

    /* Perform the selected filter. */
    matched = filter_student_records(
        records,
        count,
        (FilterType)(choice - 1),
        value,
        results,
        FILTER_RECORDS_MAX
    );

    if (matched == 0) {
        ui_info("No matching student records found.");
        return;
    }

    /* Display matching records. */
    ui_banner("Filtered Student Records");

    for (i = 0; i < matched; i++) {
        printf("\nStudent ID: %s\n",
               results[i].student_id);

        printf("Name: %s\n",
               results[i].student_name);

        printf("Course: %s\n",
               results[i].course);

        printf("Department: %s\n",
               results[i].department);

        printf("Semester: %s\n",
               results[i].semester);
    }

    snprintf(
        message,
        sizeof message,
        "%lu matching record(s) found.",
        (unsigned long)matched
    );

    ui_success(message);
}
