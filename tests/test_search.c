
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../src/search_action.h"

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

    printf("FR-19 SEARCH RESULT TESTS\n");

    count = search_student_records(
        records, 4, "S001", results, 4
    );
    assert(count == 1);
    printf("Test 1 passed: Search by Student ID\n");

    count = search_student_records(
        records, 4, "Asha", results, 4
    );
    assert(count == 1);
    printf("Test 2 passed: Search by name\n");

    count = search_student_records(
        records, 4, "Computer Science", results, 4
    );
    assert(count == 2);
    printf("Test 3 passed: Search by course\n");

    count = search_student_records(
        records, 4, "cse", results, 4
    );
    assert(count == 2);
    printf("Test 4 passed: Case-insensitive search\n");

    count = search_student_records(
        records, 4, "Civil", results, 4
    );
    assert(count == 0);
    printf("Test 5 passed: No matching records\n");

    count = search_student_records(
        records, 4, "Computer", results, 1
    );
    assert(count == 1);
    printf("Test 6 passed: Result capacity limit\n");

    printf("\nAll FR-19 search tests passed!\n");

    return 0;
}
