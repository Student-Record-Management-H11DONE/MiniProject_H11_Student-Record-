#include <stdio.h>
#include <string.h>
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
    const char *path = "test_integrity_storage.csv";
    StudentRecord r[3];
    StudentRecord out[10];
    size_t n = 0;
    FILE *f;

    /* NFR-03: saving duplicate IDs must be refused (case-insensitive) */
    set(&r[0], "S001", "Asha", "CSE");
    set(&r[1], "s001", "Ravi", "ECE");
    CHECK(!student_storage_save(path, r, 2));

    /* NFR-03: unique IDs still save and load */
    set(&r[1], "S002", "Ravi", "ECE");
    CHECK(student_storage_save(path, r, 2));
    CHECK(student_storage_load(path, out, 10, &n));
    CHECK(n == 2);

    /* NFR-03: a file that contains duplicate IDs must be rejected on load */
    f = fopen(path, "w");
    CHECK(f != NULL);
    if (f != NULL) {
        fprintf(f, "S001,Asha,CSE\ns001,Ravi,ECE\n");
        fclose(f);
    }
    CHECK(!student_storage_load(path, out, 10, &n));
    CHECK(n == 0);

    remove(path);
    printf("%s\n", fails ? "STORAGE INTEGRITY TESTS FAILED" : "ALL STORAGE INTEGRITY TESTS PASSED");
    return fails;
}
