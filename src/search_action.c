
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "search_action.h"
#include "auth.h"
#include "ui.h"

#define SEARCH_DATA_FILE "search_students.csv"

/* Check whether text contains a search value, ignoring case. */
static int contains_ignore_case(const char *text, const char *search)
{
    size_t i;
    size_t j;

    if (text == NULL || search == NULL || search[0] == '\0') {
        return 0;
    }

    for (i = 0; text[i] != '\0'; i++) {
        for (j = 0; search[j] != '\0'; j++) {
            if (text[i + j] == '\0' ||
                tolower((unsigned char)text[i + j]) !=
                tolower((unsigned char)search[j])) {
                break;
            }
        }

        if (search[j] == '\0') {
            return 1;
        }
    }

    return 0;
}

/* Search student records across all available fields. */
size_t search_student_records(
    const FilterStudentRecord records[],
    size_t count,
    const char *search_value,
    FilterStudentRecord results[],
    size_t capacity)
{
    size_t i;
    size_t result_count = 0;

    if (records == NULL || search_value == NULL ||
        results == NULL || search_value[0] == '\0') {
        return 0;
    }

    for (i = 0; i < count && result_count < capacity; i++) {
        if (contains_ignore_case(records[i].student_id, search_value) ||
            contains_ignore_case(records[i].student_name, search_value) ||
            contains_ignore_case(records[i].course, search_value) ||
            contains_ignore_case(records[i].department, search_value) ||
            contains_ignore_case(records[i].semester, search_value)) {

            results[result_count] = records[i];
            result_count++;
        }
    }

    return result_count;
}

/* Load student records from the independent search CSV file. */
static int load_search_records(
    FilterStudentRecord records[],
    size_t capacity,
    size_t *count)
{
    FILE *file;
    char line[512];

    *count = 0;

    file = fopen(SEARCH_DATA_FILE, "r");

    if (file == NULL) {
        return 0;
    }

    /* Skip the CSV header. */
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return 1;
    }

    while (fgets(line, sizeof(line), file) != NULL &&
           *count < capacity) {

        FilterStudentRecord record;

        if (sscanf(line,
                   "%31[^,],%99[^,],%99[^,],%99[^,],%20[^\r\n]",
                   record.student_id,
                   record.student_name,
                   record.course,
                   record.department,
                   record.semester) == 5) {

            records[*count] = record;
            (*count)++;
        }
    }

    fclose(file);
    return 1;
}

/* FR-19: Search and display matching student records. */
void action_search_student(const Session *s)
{
    FilterStudentRecord records[FILTER_RECORDS_MAX];
    FilterStudentRecord results[FILTER_RECORDS_MAX];

    size_t count = 0;
    size_t result_count;
    size_t i;

    char search_value[100];

    /* Check user permissions. */
    if (s == NULL ||
        !has_permission(s->role, PERM_SEARCH_STUDENT)) {

        ui_error("You do not have permission to search student records.");
        return;
    }

    ui_banner("SEARCH STUDENT RECORDS");

    /* Load records. */
    if (!load_search_records(records, FILTER_RECORDS_MAX, &count)) {
        ui_error("Unable to open search_students.csv.");
        return;
    }

    /* Get search input. */
    if (!ui_read_line(
            "Enter Student ID, name, course, department or semester: ",
            search_value,
            sizeof(search_value))) {
        return;
    }

    if (search_value[0] == '\0') {
        ui_warning("Search value cannot be empty.");
        return;
    }

    /* Perform search. */
    result_count = search_student_records(
        records,
        count,
        search_value,
        results,
        FILTER_RECORDS_MAX
    );

    /* Display results. */
    if (result_count == 0) {
        ui_info("No matching student records found.");
        return;
    }

    printf("\n========================================\n");
    printf("       MATCHING STUDENT RECORDS\n");
    printf("========================================\n");
    printf("Total matching records: %zu\n", result_count);

    for (i = 0; i < result_count; i++) {
        printf("\n----------------------------------------\n");
        printf("Student ID : %s\n", results[i].student_id);
        printf("Name       : %s\n", results[i].student_name);
        printf("Course     : %s\n", results[i].course);
        printf("Department : %s\n", results[i].department);
        printf("Semester   : %s\n", results[i].semester);
    }

    printf("\n========================================\n");
}
