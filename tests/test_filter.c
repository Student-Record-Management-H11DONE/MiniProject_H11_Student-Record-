
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../src/filter_action.h"

int main(void)
{
    FilterStudentRecord records[4] = {
        {"S001", "Asha", "Computer Science", "CSE", "1"},
        {"S002", "Rahul", "Mechanical", "ME", "2"},
        {"S003", "Priya", "Computer Science", "CSE", "3"},
        {"S004", "Arun", "Electronics", "ECE", "1"}
    };

    FilterStudentRecord results[4];
    size_t count;

    printf("FR-18 STUDENT FILTER TESTS\n");

    /* Test 1: Course filtering */
    count = filter_student_records(
        records, 4, FILTER_BY_COURSE,
        "Computer Science", results, 4
    );
    assert(count == 2);
    printf("Test 1 passed: Course filtering\n");

    /* Test 2: Department filtering */
    count = filter_student_records(
        records, 4, FILTER_BY_DEPARTMENT,
        "CSE", results, 4
    );
    assert(count == 2);
    printf("Test 2 passed: Department filtering\n");

    /* Test 3: Semester filtering */
    count = filter_student_records(
        records, 4, FILTER_BY_SEMESTER,
        "1", results, 4
    );
    assert(count == 2);
    printf("Test 3 passed: Semester filtering\n");

    /* Test 4: Case-insensitive filtering */
    count = filter_student_records(
        records, 4, FILTER_BY_DEPARTMENT,
        "cse", results, 4
    );
    assert(count == 2);
    printf("Test 4 passed: Case-insensitive filtering\n");

    /* Test 5: No matching records */
    count = filter_student_records(
        records, 4, FILTER_BY_COURSE,
        "Civil", results, 4
    );
    assert(count == 0);
    printf("Test 5 passed: No matching records\n");

    /* Test 6: Invalid filter */
    count = filter_student_records(
        records, 4, (FilterType)99,
        "CSE", results, 4
    );
    assert(count == 0);
    printf("Test 6 passed: Invalid filter handling\n");

    printf("\nAll FR-18 tests passed!\n");

    return 0;
}
