#include <stdio.h>
#include <string.h>
#include "auth.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    const char *f = "users_nfr_test.csv";
    char longname[300];
    Session s;

    remove(f);
    CHECK(ensure_default_credentials(f));
    CHECK(load_credentials(f));

    /* NFR-12: Role-Based Information Access */
    CHECK(has_permission(ROLE_ADMINISTRATOR, PERM_VIEW_ALL_STUDENTS));
    CHECK(has_permission(ROLE_FACULTY, PERM_VIEW_GRADES));
    CHECK(has_permission(ROLE_STUDENT, PERM_VIEW_ATTENDANCE));
    CHECK(!has_permission(ROLE_STUDENT, PERM_VIEW_ALL_STUDENTS));
    CHECK(!has_permission(ROLE_STUDENT, PERM_SEARCH_STUDENT));
    memset(&s, 0, sizeof s);
    s.role = ROLE_STUDENT;
    snprintf(s.student_id, sizeof s.student_id, "%s", "S001");
    CHECK(can_access_student(&s, "S001"));
    CHECK(!can_access_student(&s, "S002"));

    /* NFR-08: Prevent Unauthorized Modification */
    CHECK(!has_permission(ROLE_STUDENT, PERM_ADD_STUDENT));
    CHECK(!has_permission(ROLE_STUDENT, PERM_UPDATE_STUDENT));
    CHECK(!has_permission(ROLE_STUDENT, PERM_DELETE_STUDENT));
    CHECK(!has_permission(ROLE_STUDENT, PERM_RECORD_GRADE));
    CHECK(!has_permission(ROLE_STUDENT, PERM_UPDATE_ATTENDANCE));
    CHECK(!has_permission(ROLE_FACULTY, PERM_ADD_STUDENT));
    CHECK(!has_permission(ROLE_FACULTY, PERM_UPDATE_STUDENT));
    CHECK(!has_permission(ROLE_FACULTY, PERM_DELETE_STUDENT));
    CHECK(!has_permission(ROLE_ADMINISTRATOR, PERM_RECORD_GRADE));
    CHECK(!has_permission(ROLE_ADMINISTRATOR, PERM_UPDATE_ATTENDANCE));

    /* NFR-04: Error Handling and Stability (bad input must not crash) */
    memset(longname, 'A', sizeof longname - 1);
    longname[sizeof longname - 1] = '\0';
    CHECK(!login(longname, "x").success);
    CHECK(!login(NULL, NULL).success);
    CHECK(!login("admin", NULL).success);
    CHECK(!has_permission(ROLE_NONE, PERM_VIEW_STUDENT));
    CHECK(!can_access_student(NULL, "S001"));
    CHECK(!can_access_student(&s, NULL));

    remove(f);
    printf("%s\n", fails ? "NFR TESTS FAILED" : "ALL NFR TESTS PASSED");
    return fails;
}
