#include <stdio.h>
#include <string.h>
#include "backup.h"
#include "student_storage.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static void set(StudentRecord *r, const char *id, const char *name, const char *course)
{
    strcpy(r->student_id, id);
    strcpy(r->student_name, name);
    strcpy(r->course, course);
}

int main(void)
{
    const char *orig_path = "test_data_primary.csv";
    const char *backup_path = "test_data_primary.csv.bak";
    const char *restored_path = "test_data_restored.csv";
    StudentRecord r[2];
    StudentRecord out[10];
    size_t count = 0;
    FILE *f;

    set(&r[0], "S101", "Ananya", "CSE");
    set(&r[1], "S102", "Bhavesh", "ECE");

    /* Ensure test environment is clean */
    remove(orig_path);
    remove(backup_path);
    remove(restored_path);

    /* 1. NFR-07: Atomic safe save */
    CHECK(student_storage_save_atomic(orig_path, r, 2));
    CHECK(student_storage_load(orig_path, out, 10, &count));
    CHECK(count == 2);
    CHECK(strcmp(out[0].student_id, "S101") == 0);
    CHECK(strcmp(out[1].student_id, "S102") == 0);

    /* 2. NFR-06: Create a backup of the records */
    CHECK(backup_create(orig_path, backup_path));

    /* Verify backup file can be loaded directly and has identical records */
    count = 0;
    CHECK(student_storage_load(backup_path, out, 10, &count));
    CHECK(count == 2);
    CHECK(strcmp(out[0].student_id, "S101") == 0);
    CHECK(strcmp(out[1].student_id, "S102") == 0);

    /* 3. NFR-06: Simulate primary file corruption */
    f = fopen(orig_path, "w");
    CHECK(f != NULL);
    if (f != NULL) {
        fprintf(f, "CORRUPT_DATA_WITHOUT_COMMAS_AND_FIELDS\n");
        fclose(f);
    }
    /* Loading corrupt file must fail */
    count = 0;
    CHECK(!student_storage_load(orig_path, out, 10, &count));

    /* 4. NFR-06: Restore from backup to recover data */
    CHECK(backup_restore(backup_path, orig_path));

    /* Verify recovered primary file has all data back intact */
    count = 0;
    CHECK(student_storage_load(orig_path, out, 10, &count));
    CHECK(count == 2);
    CHECK(strcmp(out[0].student_id, "S101") == 0);
    CHECK(strcmp(out[1].student_id, "S102") == 0);

    /* 5. NFR-07: Crash-safe write failure resistance */
    /* Attempt to atomically save duplicate IDs (must be rejected) */
    set(&r[1], "s101", "DuplicateID", "ECE");
    CHECK(!student_storage_save_atomic(orig_path, r, 2));

    /* Verify existing orig_path was NOT corrupted or overwritten by rejected write */
    count = 0;
    CHECK(student_storage_load(orig_path, out, 10, &count));
    CHECK(count == 2);
    CHECK(strcmp(out[1].student_id, "S102") == 0); /* Still Bhavesh, not DuplicateID */

    /* Cleanup temporary test files */
    remove(orig_path);
    remove(backup_path);
    remove(restored_path);

    printf("%s\n", fails ? "BACKUP & RELIABILITY TESTS FAILED" : "ALL BACKUP & RELIABILITY TESTS PASSED (NFR-06, NFR-07)");
    return fails;
}
