
#include "validation.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

int validate_required_field(const char *value)
{
    const unsigned char *p;

    if (value == NULL) {
        return 0;
    }

    p = (const unsigned char *)value;

    while (*p != '\0') {
        if (!isspace(*p)) {
            return 1;
        }
        p++;
    }

    return 0;
}

int validate_student_fields(const char *student_id,
                            const char *student_name)
{
    if (!validate_required_field(student_id)) {
        return 0;
    }

    if (!validate_required_field(student_name)) {
        return 0;
    }

    return 1;
}
