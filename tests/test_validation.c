
#include <assert.h>
#include <stdio.h>

#include "../src/validation.h"

int main(void)
{
    /* Valid required fields */
    assert(validate_required_field("S1001"));
    assert(validate_required_field("Asha"));

    /* Empty and whitespace-only values */
    assert(!validate_required_field(""));
    assert(!validate_required_field("   "));
    assert(!validate_required_field("\t\n"));

    /* Null input */
    assert(!validate_required_field(NULL));

    /* Student field combinations */
    assert(validate_student_fields("S1001", "Asha"));
    assert(!validate_student_fields("", "Asha"));
    assert(!validate_student_fields("S1001", ""));
    assert(!validate_student_fields(" ", "Asha"));
    assert(!validate_student_fields("S1001", "   "));

    puts("All invalid-input validation tests passed.");
    return 0;
}
