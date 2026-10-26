#ifndef INTEGRITY_H
#define INTEGRITY_H

#include <stddef.h>
#include "student_record.h"

#ifdef __cplusplus
extern "C" {
#endif

/* NFR-03 / issue #29. All functions are read-only. */

/* 1 if the ID is 1..STUDENT_ID_MAX letters or digits (no spaces, commas). */
int integrity_id_format_valid(const char *id);

/* 1 if an ID already exists in the list (case-insensitive: S001 == s001). */
int integrity_id_exists(const StudentRecord records[], size_t count,
                        const char *id);

/* Index of the first record whose ID repeats an earlier one, or -1. */
int integrity_find_duplicate(const StudentRecord records[], size_t count);

/* 1 if every record is well-formed AND there are no duplicate IDs. */
int integrity_records_consistent(const StudentRecord records[], size_t count);

#ifdef __cplusplus
}
#endif

#endif
