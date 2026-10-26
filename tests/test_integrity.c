#include <stdio.h>
#include <string.h>
#include "integrity.h"

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
    StudentRecord r[4];

    /* NFR-03: ID format */
    CHECK(integrity_id_format_valid("S001"));
    CHECK(integrity_id_format_valid("PES1UG24CS428"));
    CHECK(!integrity_id_format_valid(""));
    CHECK(!integrity_id_format_valid(NULL));
    CHECK(!integrity_id_format_valid("S 001"));
    CHECK(!integrity_id_format_valid("S,001"));
    CHECK(!integrity_id_format_valid("1234567890123456789012345678901234"));

    /* NFR-03: duplicate detection */
    set(&r[0], "S001", "Asha", "CSE");
    set(&r[1], "S002", "Ravi", "ECE");
    set(&r[2], "S003", "Meera", "ME");
    CHECK(integrity_id_exists(r, 3, "S002"));
    CHECK(integrity_id_exists(r, 3, "s002"));
    CHECK(!integrity_id_exists(r, 3, "S004"));
    CHECK(!integrity_id_exists(r, 0, "S001"));
    CHECK(!integrity_id_exists(r, 3, NULL));
    CHECK(integrity_find_duplicate(r, 3) == -1);

    set(&r[3], "s001", "Kiran", "CSE");
    CHECK(integrity_find_duplicate(r, 4) == 3);

    /* NFR-03: consistent record sets */
    CHECK(integrity_records_consistent(r, 3));
    CHECK(!integrity_records_consistent(r, 4));
    CHECK(integrity_records_consistent(NULL, 0));
    CHECK(!integrity_records_consistent(NULL, 2));

    set(&r[3], "S004", "", "CSE");
    CHECK(!integrity_records_consistent(r, 4));
    set(&r[3], "S004", "Ki,ran", "CSE");
    CHECK(!integrity_records_consistent(r, 4));
    set(&r[3], "S004", "Kiran", "CSE");
    CHECK(integrity_records_consistent(r, 4));

    printf("%s\n", fails ? "INTEGRITY TESTS FAILED" : "ALL INTEGRITY TESTS PASSED");
    return fails;
}
