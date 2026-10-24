
#ifndef STUDENT_STORAGE_H
#define STUDENT_STORAGE_H

#include <stddef.h>
#include "student_record.h"

/* Save all records. Returns 1 on success, 0 on failure. */
int student_storage_save(const char *path,
                         const StudentRecord records[],
                         size_t count);

/* Load records. Returns 1 on success, 0 on failure. */
int student_storage_load(const char *path,
                         StudentRecord records[],
                         size_t capacity,
                         size_t *count);

#endif
