#include <stdio.h>
#include <string.h>

#include "auth.h"
#include "actions.h"
#include "ui.h"

#define USERS_FILE "users.csv"

typedef struct {
    Permission perm;
    const char *label;
    const char *student_label;   /* label shown to Student role, or NULL */
    void (*handler)(const Session *);
} MenuItem;

static const MenuItem MENU[] = {
    {PERM_ADD_STUDENT,       "Add Student",            NULL,                  action_add_student},
    {PERM_VIEW_STUDENT,      "View Student Details",   "View Own Profile",    action_view_student},
    {PERM_VIEW_ALL_STUDENTS, "View All Students",      NULL,                  action_view_all_students},
    {PERM_SEARCH_STUDENT,    "Search Student",         NULL,                  action_search_student},
    {PERM_UPDATE_STUDENT,    "Update Student",         NULL,                  action_update_student},
    {PERM_DELETE_STUDENT,    "Delete Student",         NULL,                  action_delete_student},
    {PERM_FILTER_RECORDS,    "Filter Student Records", NULL,                  action_filter_records},
    {PERM_RECORD_GRADE,      "Record Grades",          NULL,                  action_record_grade},
    {PERM_UPDATE_GRADE,      "Update Grades",          NULL,                  action_update_grade},
    {PERM_VIEW_GRADES,       "View Grades",            "View Own Grades",     action_view_grades},
    {PERM_RECORD_ATTENDANCE, "Record Attendance",      NULL,                  action_record_attendance},
    {PERM_UPDATE_ATTENDANCE, "Update Attendance",      NULL,                  action_update_attendance},
    {PERM_VIEW_ATTENDANCE,   "View Attendance",        "View Own Attendance", action_view_attendance}
};
#define MENU_SIZE ((int)(sizeof MENU / sizeof MENU[0]))

/* UC-01 Login. Returns 1 if a session was established. */
static int do_login(Session *session)
{
    char user[64], pass[64], msg[160];
    AuthResult r;

    ui_banner("Login");
    if (!ui_read_line("Username: ", user, sizeof user)) return 0;
    if (!ui_read_line("Password: ", pass, sizeof pass)) return 0;

    r = login(user, pass);
    if (!r.success) {
        ui_error(r.message);              /* AF-01 / AF-02 */
        return 0;
    }
    memset(session, 0, sizeof *session);
    snprintf(session->username, sizeof session->username, "%s", user);
    session->role = r.role;
    snprintf(session->student_id, sizeof session->student_id, "%s", r.student_id);

    snprintf(msg, sizeof msg, "Welcome, %s (%s).", user, role_name(r.role));
    ui_success(msg);
    return 1;
}

/* FR-04: menu built only from permitted operations.
   Returns 1 to log out (back to login), 0 to exit the application. */
static int run_main_menu(const Session *s)
{
    for (;;) {
        int items[16];
        int n = 0, i, c;
        char title[96];

        for (i = 0; i < MENU_SIZE; i++) {
            if (has_permission(s->role, MENU[i].perm)) {
                items[n++] = i;
            }
        }

        snprintf(title, sizeof title, "Main Menu - %s", role_name(s->role));
        ui_banner(title);
        for (i = 0; i < n; i++) {
            const MenuItem *m = &MENU[items[i]];
            const char *label = (s->role == ROLE_STUDENT && m->student_label != NULL)
                                    ? m->student_label : m->label;
            printf("  %d. %s\n", i + 1, label);
        }
        printf("  %d. Logout\n", n + 1);
        printf("  %d. Exit\n", n + 2);

        c = ui_read_choice("Enter choice: ", 1, n + 2);
        if (c == -1) return 0;                         /* end of input: exit safely */
        if (c == n + 1) {
            ui_info("Logged out.");
            return 1;
        }
        if (c == n + 2) {                              /* FR-25 safe exit */
            if (!ui_confirm("Are you sure you want to exit?")) continue;
            if (!action_save_on_exit()) ui_warning("Some data could not be saved.");
            return 0;
        }
        {
            const MenuItem *m = &MENU[items[c - 1]];
            if (!has_permission(s->role, m->perm)) {   /* defence in depth (SR-02) */
                ui_error("Access denied for your role.");
                continue;
            }
            m->handler(s);
            ui_pause();
        }
    }
}

int main(void)
{
    int running = 1;

    if (!ensure_default_credentials(USERS_FILE) || !load_credentials(USERS_FILE)) {
        ui_error("Could not load user credentials (" USERS_FILE ").");
        return 1;
    }

    while (running) {
        int c;
        Session s;

        ui_banner("Student Record System");
        printf("  1. Login\n  2. Exit\n");
        c = ui_read_choice("Enter choice: ", 1, 2);
        if (c == -1) break;
        if (c == 2) {
            if (ui_confirm("Are you sure you want to exit?")) break;
            continue;
        }
        if (!do_login(&s)) continue;     /* AF-01 / AF-02: back to login prompt */
        running = run_main_menu(&s);
    }
    ui_info("Thank you. Goodbye.");
    return 0;
}