
#include "attendance.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int passed = 0;
static int failed = 0;

static void check(int condition, const char *message)
{
    if (condition) {
        printf("[PASS] %s\n", message);
        passed++;
    } else {
        printf("[FAIL] %s\n", message);
        failed++;
    }
}

int main(void)
{
    AttendanceRecord records[10];
    AttendanceRecord results[10];
    AttendanceRecord record;
    size_t count = 0;
    size_t found = 0;
    FILE *file;

    printf("\n====================================\n");
    printf("   FR-15 ATTENDANCE MODULE TESTS\n");
    printf("====================================\n\n");

    /* Test 1: Create valid CSV file */
    file = fopen("test_attendance.csv", "w");

    if (file != NULL) {
        fprintf(file, "student_id,date,present\n");
        fprintf(file, "S001,2026-11-01,1\n");
        fprintf(file, "S001,2026-11-02,0\n");
        fprintf(file, "S002,2026-11-01,1\n");
        fclose(file);
        check(1, "Create valid attendance CSV");
    } else {
        check(0, "Create valid attendance CSV");
        return 1;
    }

    /* Test 2: Load CSV */
    check(attendance_load_csv(
        "test_attendance.csv", records, 10, &count),
        "Load valid attendance records");

    check(count == 3, "Verify loaded attendance count");

    /* Test 3: Retrieve student records */
    found = attendance_get_student_records(
        records, count, "S001", results, 10);

    check(found == 2, "Retrieve attendance for one student");

    check(strcmp(results[0].student_id, "S001") == 0 &&
          results[0].present == 1,
          "Verify student attendance details");

    /* Test 4: Student without records */
    found = attendance_get_student_records(
        records, count, "S999", results, 10);

    check(found == 0, "Handle student without attendance records");

    /* Test 5: Add a valid record */
    strcpy(record.student_id, "S003");
    strcpy(record.date, "2026-11-03");
    record.present = 1;

    check(attendance_add(records, &count, 10, &record),
          "Add valid attendance record");

    check(count == 4, "Verify attendance count after adding");

    /* Test 6: Duplicate attendance */
    check(!attendance_add(records, &count, 10, &record),
          "Reject duplicate student attendance for same date");

    /* Test 7: Invalid attendance value */
    record.present = 2;

    check(!attendance_validate(&record),
          "Reject invalid attendance value");

    /* Test 8: Invalid date */
    strcpy(record.date, "2026-02-30");
    record.present = 1;

    check(!attendance_validate(&record),
          "Reject invalid calendar date");

    /* Test 9: Save attendance */
    check(attendance_save_csv(
        "test_attendance_output.csv", records, count),
        "Save attendance records to CSV");

    /* Test 10: Load saved records */
    {
        AttendanceRecord loaded[10];
        size_t loaded_count = 0;

        check(attendance_load_csv(
            "test_attendance_output.csv",
            loaded, 10, &loaded_count),
            "Reload saved attendance records");

        check(loaded_count == 4,
              "Verify saved attendance count");
    }

    /* Test 11: Missing file */
    check(!attendance_load_csv(
        "missing_attendance.csv", records, 10, &count),
        "Handle missing attendance CSV file");

    /* Test 12: Invalid CSV */
    file = fopen("invalid_attendance.csv", "w");

    if (file != NULL) {
        fprintf(file, "student_id,date,present\n");
        fprintf(file, "S001,invalid-date,5\n");
        fclose(file);

        check(!attendance_load_csv(
            "invalid_attendance.csv", records, 10, &count),
            "Reject malformed attendance records");
    }

    /* Test 13: Insufficient capacity */
    check(!attendance_load_csv(
        "test_attendance.csv", records, 1, &count),
        "Respect attendance array capacity");

    /* Test 14: Display attendance */
    file = tmpfile();

    check(file != NULL, "Create temporary report output");

    if (file != NULL) {
        attendance_display(records, 3, file);
        fclose(file);
        check(1, "Display attendance report");
    }

    printf("\n====================================\n");
    printf("Tests passed: %d\n", passed);
    printf("Tests failed: %d\n", failed);
    printf("====================================\n");

    remove("test_attendance.csv");
    remove("test_attendance_output.csv");
    remove("invalid_attendance.csv");

    return failed == 0 ? 0 : 1;
}
