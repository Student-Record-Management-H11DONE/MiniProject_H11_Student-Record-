#include "auth.h"

#include <stdio.h>
#include <string.h>

#define MAX_USERS 100
#define FIELD_LEN 128

typedef struct {
    char username[64];
    char pass_hash[40];
    Role role;
    char student_id[32];
} CredentialRecord;

static CredentialRecord g_users[MAX_USERS];
static int g_user_count = 0;

/* FNV-1a (64-bit): portable, avoids plain-text passwords in users.csv.
   Demo-grade only, not production-grade security. */
static void hash_password(const char *password, char out[40])
{
    unsigned long long h = 1469598103934665603ULL;
    const unsigned char *p = (const unsigned char *)password;

    while (*p) {
        h ^= *p++;
        h *= 1099511628211ULL;
    }
    snprintf(out, 40, "%08lx%08lx",
             (unsigned long)(h >> 32), (unsigned long)(h & 0xFFFFFFFFUL));
}

static Role parse_role(const char *s)
{
    if (strcmp(s, "Administrator") == 0) return ROLE_ADMINISTRATOR;
    if (strcmp(s, "Faculty") == 0)       return ROLE_FACULTY;
    if (strcmp(s, "Student") == 0)       return ROLE_STUDENT;
    return ROLE_NONE;
}

/* Splits a comma-separated line into up to 4 fields (keeps empty fields). */
static int split_fields(const char *line, char f[4][FIELD_LEN])
{
    int n = 0;
    size_t len = 0;
    const char *p = line;

    for (;;) {
        if (*p == ',' || *p == '\0') {
            f[n][len] = '\0';
            n++;
            len = 0;
            if (*p == '\0' || n == 4) break;
        } else if (len < FIELD_LEN - 1) {
            f[n][len++] = *p;
        }
        p++;
    }
    return n;
}

/* authenticate(): matches credentials against stored records */
static const CredentialRecord *authenticate(const char *username, const char *password)
{
    char h[40];
    int i;

    hash_password(password, h);
    for (i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].username, username) == 0 &&
            strcmp(g_users[i].pass_hash, h) == 0) {
            return &g_users[i];
        }
    }
    return NULL;
}

int ensure_default_credentials(const char *path)
{
    FILE *f = fopen(path, "r");
    char h1[40], h2[40], h3[40];

    if (f != NULL) {
        fclose(f);
        return 1;
    }
    f = fopen(path, "w");
    if (f == NULL) return 0;

    hash_password("admin123", h1);
    hash_password("faculty123", h2);
    hash_password("student123", h3);
    fprintf(f, "admin,%s,Administrator,\n", h1);
    fprintf(f, "faculty1,%s,Faculty,\n", h2);
    fprintf(f, "student1,%s,Student,S001\n", h3);
    fclose(f);
    return 1;
}

int load_credentials(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[512];

    g_user_count = 0;
    if (f == NULL) return 0;

    while (fgets(line, sizeof line, f) != NULL && g_user_count < MAX_USERS) {
        char fields[4][FIELD_LEN];
        size_t len = strlen(line);
        int n;
        Role role;
        CredentialRecord *r;

        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        n = split_fields(line, fields);
        if (n < 3) continue;
        role = parse_role(fields[2]);
        if (role == ROLE_NONE) continue;

        r = &g_users[g_user_count++];
        snprintf(r->username, sizeof r->username, "%s", fields[0]);
        snprintf(r->pass_hash, sizeof r->pass_hash, "%s", fields[1]);
        r->role = role;
        snprintf(r->student_id, sizeof r->student_id, "%s", (n > 3) ? fields[3] : "");
    }
    fclose(f);
    return g_user_count > 0;
}

AuthResult login(const char *username, const char *password)
{
    AuthResult res;
    const CredentialRecord *rec;

    memset(&res, 0, sizeof res);

    if (username == NULL || password == NULL || username[0] == '\0' || password[0] == '\0') {
        snprintf(res.message, sizeof res.message, "Username and password are required."); /* AF-02 */
        return res;
    }
    rec = authenticate(username, password);
    if (rec == NULL) {
        snprintf(res.message, sizeof res.message, "Invalid username or password.");       /* AF-01 */
        return res;
    }
    res.success = 1;
    res.role = rec->role;
    snprintf(res.student_id, sizeof res.student_id, "%s", rec->student_id);
    snprintf(res.message, sizeof res.message, "Login successful.");
    return res;
}

const char *role_name(Role role)
{
    switch (role) {
        case ROLE_ADMINISTRATOR: return "Administrator";
        case ROLE_FACULTY:       return "Faculty";
        case ROLE_STUDENT:       return "Student";
        default:                 return "None";
    }
}

/* Permission matrix taken directly from the SRS actor table. */
int has_permission(Role role, Permission p)
{
    if (p == PERM_EXIT) return role != ROLE_NONE;

    switch (role) {
        case ROLE_ADMINISTRATOR:
            return p == PERM_ADD_STUDENT || p == PERM_VIEW_STUDENT ||
                   p == PERM_VIEW_ALL_STUDENTS || p == PERM_SEARCH_STUDENT ||
                   p == PERM_UPDATE_STUDENT || p == PERM_DELETE_STUDENT ||
                   p == PERM_FILTER_RECORDS;
        case ROLE_FACULTY:
            return p == PERM_VIEW_STUDENT || p == PERM_VIEW_ALL_STUDENTS ||
                   p == PERM_SEARCH_STUDENT || p == PERM_RECORD_GRADE ||
                   p == PERM_UPDATE_GRADE || p == PERM_VIEW_GRADES ||
                   p == PERM_RECORD_ATTENDANCE || p == PERM_UPDATE_ATTENDANCE ||
                   p == PERM_VIEW_ATTENDANCE || p == PERM_FILTER_RECORDS;
        case ROLE_STUDENT:   /* own data only */
            return p == PERM_VIEW_STUDENT || p == PERM_VIEW_GRADES ||
                   p == PERM_VIEW_ATTENDANCE;
        default:
            return 0;
    }
}

int can_access_student(const Session *s, const char *student_id)
{
    if (s == NULL || student_id == NULL) return 0;
    if (s->role == ROLE_ADMINISTRATOR || s->role == ROLE_FACULTY) return 1;
    if (s->role == ROLE_STUDENT) {
        return s->student_id[0] != '\0' && strcmp(s->student_id, student_id) == 0;
    }
    return 0;
}