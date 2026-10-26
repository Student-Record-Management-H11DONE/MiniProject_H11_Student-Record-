#include "integrity.h"

#include <ctype.h>
#include <string.h>

static int equals_ignore_case(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

/* Non-empty text with no CSV delimiters or line breaks. */
static int text_field_ok(const char *s)
{
    return s != NULL && s[0] != '\0' &&
           strchr(s, ',') == NULL &&
           strchr(s, '\n') == NULL &&
           strchr(s, '\r') == NULL;
}

int integrity_id_format_valid(const char *id)
{
    size_t i, len;

    if (id == NULL) return 0;
    len = strlen(id);
    if (len == 0 || len > STUDENT_ID_MAX) return 0;
    for (i = 0; i < len; i++) {
        if (!isalnum((unsigned char)id[i])) return 0;
    }
    return 1;
}

int integrity_id_exists(const StudentRecord records[], size_t count,
                        const char *id)
{
    size_t i;

    if (records == NULL || id == NULL) return 0;
    for (i = 0; i < count; i++) {
        if (equals_ignore_case(records[i].student_id, id)) return 1;
    }
    return 0;
}

int integrity_find_duplicate(const StudentRecord records[], size_t count)
{
    size_t i, j;

    if (records == NULL) return -1;
    for (i = 1; i < count; i++) {
        for (j = 0; j < i; j++) {
            if (equals_ignore_case(records[i].student_id,
                                   records[j].student_id)) {
                return (int)i;
            }
        }
    }
    return -1;
}

int integrity_records_consistent(const StudentRecord records[], size_t count)
{
    size_t i;

    if (records == NULL) return count == 0;
    for (i = 0; i < count; i++) {
        if (!integrity_id_format_valid(records[i].student_id) ||
            !text_field_ok(records[i].student_name) ||
            !text_field_ok(records[i].course)) {
            return 0;
        }
    }
    return integrity_find_duplicate(records, count) == -1;
}
