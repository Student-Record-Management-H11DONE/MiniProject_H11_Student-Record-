#ifndef AUTH_H
#define AUTH_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ROLE_NONE = 0,
    ROLE_ADMINISTRATOR,
    ROLE_FACULTY,
    ROLE_STUDENT
} Role;

typedef enum {
    PERM_ADD_STUDENT,
    PERM_VIEW_STUDENT,
    PERM_VIEW_ALL_STUDENTS,
    PERM_SEARCH_STUDENT,
    PERM_UPDATE_STUDENT,
    PERM_DELETE_STUDENT,
    PERM_RECORD_GRADE,
    PERM_UPDATE_GRADE,
    PERM_VIEW_GRADES,
    PERM_RECORD_ATTENDANCE,
    PERM_UPDATE_ATTENDANCE,
    PERM_VIEW_ATTENDANCE,
    PERM_FILTER_RECORDS,
    PERM_EXIT
} Permission;

typedef struct {
    char username[64];
    Role role;
    char student_id[32];   /* only set for the Student role */
} Session;

typedef struct {
    int  success;
    Role role;
    char student_id[32];
    char message[96];
} AuthResult;

/* Credential store: users.csv  ->  username,passwordHash,role,studentId */
int ensure_default_credentials(const char *path); /* creates demo users if file missing */
int load_credentials(const char *path);

/* FR-01 / FR-02 */
AuthResult login(const char *username, const char *password);

/* FR-03 / NFR-08 / NFR-12 / SR-02 */
int has_permission(Role role, Permission perm);
int can_access_student(const Session *s, const char *student_id); /* Student: own record only */
const char *role_name(Role role);

#ifdef __cplusplus
}
#endif

#endif