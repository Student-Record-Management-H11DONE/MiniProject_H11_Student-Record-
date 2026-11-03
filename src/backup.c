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

        /* 2. Replace the destination without ever deleting the only good copy.
     * Move the old file aside, move the new file in, then delete the old one.
     * If anything fails, the old file is put back. */
    {
        char old_path[512];
        FILE *probe;
        int had_old = 0;

        snprintf(old_path, sizeof old_path, "%s.old", path);
        remove(old_path);

        probe = fopen(path, "r");
        if (probe != NULL) {
            fclose(probe);
            had_old = 1;
            if (rename(path, old_path) != 0) {
                remove(temp_path);
                return 0;                    /* original untouched */
            }
        }
        if (rename(temp_path, path) != 0) {
            if (had_old) {
                rename(old_path, path);      /* put the original back */
            }
            remove(temp_path);
            return 0;
        }
        if (had_old) {
            remove(old_path);
        }
    }

    return 1;
}
