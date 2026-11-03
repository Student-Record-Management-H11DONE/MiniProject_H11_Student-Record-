
#include "grades.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int passed = 0;
static int failed = 0;

/* Record test results. */
static void check(int condition, const char *description)
{
    if (condition) {
        printf("[PASS] %s\n", description);
        passed++;
    } else {
        printf("[FAIL] %s\n", description);
        failed++;
    }
}

/* Create temporary CSV test files. */
static int create_test_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "w");

    if (file == NULL) {
        return 0;
    }

    fputs(content, file);

    if (fclose(file) != 0) {
        return 0;
    }

    return 1;
}

int main(void)
{
    GradeRecord records[GRADE_RECORDS_MAX];
    GradeRecord results[10];

    size_t count = 0;
    size_t found;

    const char *valid_csv =
        "student_id,subject,marks,grade\n"
        "S001,Mathematics,85,A\n"
        "S001,Physics,78,B\n"
        "S002,Mathematics,92,A+\n"
        "S002,Physics,88,A\n";

    printf("\n====================================\n");
    printf("       FR-14 GRADE MODULE TESTS\n");
    printf("====================================\n\n");

    /* Test 1: Valid CSV creation and loading. */
    check(create_test_file("test_grades.csv", valid_csv),
          "Create valid grade CSV");

    check(grades_load_csv("test_grades.csv",
                          records,
                          GRADE_RECORDS_MAX,
                          &count) == 1 && count == 4,
          "Load valid grade records");

    /* Test 2: Verify loaded data. */
    check(strcmp(records[0].student_id, "S001") == 0 &&
          strcmp(records[0].subject, "Mathematics") == 0 &&
          records[0].marks == 85.0,
          "Verify loaded student grade data");

    /* Test 3: Retrieve grades for S001. */
    found = grades_get_student_records(
        records, count, "S001", results, 10);

    check(found == 2,
          "Retrieve all grades for a student");

    /* Test 4: Check student isolation. */
    check(strcmp(results[0].student_id, "S001") == 0 &&
          strcmp(results[1].student_id, "S001") == 0,
          "Return only the requested student's grades");

    /* Test 5: Student with no grades. */
    found = grades_get_student_records(
        records, count, "S999", results, 10);

    check(found == 0,
          "Handle student without grade records");

    /* Test 6: Capacity limitation. */
    found = grades_get_student_records(
        records, count, "S001", results, 1);

    check(found == 1,
          "Respect result array capacity");

    /* Test 7: Missing CSV file. */
    remove("missing_grades.csv");
    count = 0;

    check(grades_load_csv("missing_grades.csv",
                          records, GRADE_RECORDS_MAX,
                          &count) == 0,
          "Handle missing grade CSV file");

    /* Test 8: Invalid marks. */
    create_test_file(
        "invalid_grades.csv",
        "student_id,subject,marks,grade\n"
        "S001,Mathematics,150,A\n");

    count = 0;

    check(grades_load_csv("invalid_grades.csv",
                          records, GRADE_RECORDS_MAX,
                          &count) == 0,
          "Reject marks above 100");

    /* Test 9: Negative marks. */
    create_test_file(
        "negative_grades.csv",
        "student_id,subject,marks,grade\n"
        "S001,Physics,-5,F\n");

    check(grades_load_csv("negative_grades.csv",
                          records, GRADE_RECORDS_MAX,
                          &count) == 0,
          "Reject negative marks");

    /* Test 10: Empty grade file. */
    create_test_file(
        "empty_grades.csv",
        "student_id,subject,marks,grade\n");

    count = 99;

    check(grades_load_csv("empty_grades.csv",
                          records, GRADE_RECORDS_MAX,
                          &count) == 1 && count == 0,
          "Handle empty grade records");

    /* Test 11: Malformed CSV. */
    create_test_file(
        "malformed_grades.csv",
        "student_id,subject,marks,grade\n"
        "S001,Mathematics,85\n");

    check(grades_load_csv("malformed_grades.csv",
                          records, GRADE_RECORDS_MAX,
                          &count) == 0,
          "Reject malformed CSV records");

    /* Test 12: Display grade report. */
    grades_load_csv("test_grades.csv",
                    records, GRADE_RECORDS_MAX, &count);

    printf("\n");
    grades_display(records, count, stdout);

    /* Clean up temporary test files. */
    remove("test_grades.csv");
    remove("invalid_grades.csv");
    remove("negative_grades.csv");
    remove("empty_grades.csv");
    remove("malformed_grades.csv");

    printf("\n====================================\n");
    printf("Tests passed: %d\n", passed);
    printf("Tests failed: %d\n", failed);
    printf("====================================\n");

    return failed == 0 ? 0 : 1;
}
