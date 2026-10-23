#include <stdio.h>
#include <string.h>
#include "auth.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    const char *f = "users_test.csv";
    AuthResult a, fa, st;
    Session s;

    remove(f);
    CHECK(ensure_default_credentials(f));
    CHECK(load_credentials(f));

    a = login("admin", "admin123");                     /* TC-FR-01 / SEC-01 */
    CHECK(a.success && a.role == ROLE_ADMINISTRATOR);
    fa = login("faculty1", "faculty123");
    CHECK(fa.success && fa.role == ROLE_FACULTY);
    st = login("student1", "student123");
    CHECK(st.success && st.role == ROLE_STUDENT && strcmp(st.student_id, "S001") == 0);

    CHECK(!login("admin", "wrong").success);            /* TC-FR-02 */
    CHECK(!login("nobody", "x").success);
    CHECK(!login("", "").success);                      /* AF-02 */
    CHECK(!login("admin", "").success);

    /* TC-FR-03 / TC-FR-04 / SEC-02 / NFR-08 / NFR-12 */
    CHECK(has_permission(ROLE_ADMINISTRATOR, PERM_DELETE_STUDENT));
    CHECK(!has_permission(ROLE_ADMINISTRATOR, PERM_RECORD_GRADE));
    CHECK(!has_permission(ROLE_FACULTY, PERM_DELETE_STUDENT));
    CHECK(has_permission(ROLE_FACULTY, PERM_RECORD_ATTENDANCE));
    CHECK(!has_permission(ROLE_STUDENT, PERM_UPDATE_STUDENT));
    CHECK(has_permission(ROLE_STUDENT, PERM_VIEW_GRADES));
    CHECK(!has_permission(ROLE_NONE, PERM_EXIT));

    memset(&s, 0, sizeof s);
    s.role = ROLE_STUDENT;
    snprintf(s.student_id, sizeof s.student_id, "%s", "S001");
    CHECK(can_access_student(&s, "S001"));
    CHECK(!can_access_student(&s, "S002"));

    remove(f);
    printf("%s\n", fails ? "TESTS FAILED" : "ALL TESTS PASSED");
    return fails;
}