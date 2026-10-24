
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/validation.h"
#include "../src/student_storage.h"

int main(void)
{
    StudentRecord records[10];
    StudentRecord loaded[10];
    size_t loaded_count = 0;

    /* FR-20: Required-field validation */
    assert(validate_required_field("S1001"));
    assert(!validate_required_field(""));
    assert(!validate_required_field("   "));
    assert(!validate_required_field(NULL));

    assert(validate_student_fields("S1001", "Asha"));
    assert(!validate_student_fields("", "Asha"));
    assert(!validate_student_fields("S1001", "   "));

    /* FR-22: Save a student record */
    strcpy(records[0].student_id, "S1001");
    strcpy(records[0].student_name, "Asha");
    strcpy(records[0].course, "Computer Science");

    assert(student_storage_save("test_students.csv", records, 1));

    /* FR-23: Load the saved record */
    assert(student_storage_load("test_students.csv",
                                loaded, 10, &loaded_count));
    assert(loaded_count == 1);
    assert(strcmp(loaded[0].student_id, "S1001") == 0);
    assert(strcmp(loaded[0].student_name, "Asha") == 0);
    assert(strcmp(loaded[0].course, "Computer Science") == 0);

    //remove("test_students.csv");

    puts("All student validation and storage tests passed.");
    return 0;
}
