#include "backup.h"
#include "student_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096

/* Copy a file byte-for-byte from src to dst. Returns 1 on success, 0 on failure. */
static int copy_file(const char *src, const char *dst)
{
    FILE *in, *out;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    if (src == NULL || dst == NULL || strcmp(src, dst) == 0) {
        return 0;
    }

    in = fopen(src, "rb");
    if (in == NULL) {
        return 0;
    }

    out = fopen(dst, "wb");
    if (out == NULL) {
        fclose(in);
        return 0;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        if (fwrite(buffer, 1, bytes_read, out) != bytes_read) {
            fclose(in);
            fclose(out);
            remove(dst);
            return 0;
        }
    }

    fclose(in);
    if (fclose(out) != 0) {
        remove(dst);
        return 0;
    }

    return 1;
}

int backup_create(const char *source_path, const char *backup_path)
{
    return copy_file(source_path, backup_path);
}

int backup_restore(const char *backup_path, const char *dest_path)
{
    return copy_file(backup_path, dest_path);
}

int student_storage_save_atomic(const char *path,
                                const StudentRecord records[],
                                size_t count)
{
    char temp_path[512];

    if (path == NULL) {
        return 0;
    }

    /* Build temporary filename: path.tmp */
    if (snprintf(temp_path, sizeof(temp_path), "%s.tmp", path) >= (int)sizeof(temp_path)) {
        return 0;
    }

    /* 1. Write records to the temporary file first */
    if (!student_storage_save(temp_path, records, count)) {
        remove(temp_path);
        return 0;
    }

    /* 2. Atomically replace the destination file.
     * Standard portable C: remove old destination if it exists, then rename.
     */
    remove(path);
    if (rename(temp_path, path) != 0) {
        remove(temp_path);
        return 0;
    }

    return 1;
}
