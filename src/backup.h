#ifndef BACKUP_H
#define BACKUP_H

#include <stddef.h>
#include "student_record.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NFR-06: Backup and Recovery
 * Create a byte-for-byte backup copy of source_path at backup_path.
 * Returns 1 on success, 0 on failure.
 */
int backup_create(const char *source_path, const char *backup_path);

/*
 * NFR-06: Backup and Recovery
 * Restore source database file from a known good backup.
 * Returns 1 on success, 0 on failure.
 */
int backup_restore(const char *backup_path, const char *dest_path);

/*
 * NFR-07: Data Reliability (Crash-Safe Atomic Write)
 * Writes student records to a temporary file first (<path>.tmp),
 * flushes and syncs to disk, then atomically replaces <path>.
 * Guarantees that an interrupted write will never corrupt the original file.
 * Returns 1 on success, 0 on failure.
 */
int student_storage_save_atomic(const char *path,
                                const StudentRecord records[],
                                size_t count);

#ifdef __cplusplus
}
#endif

#endif /* BACKUP_H */
