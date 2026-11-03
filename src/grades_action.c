
#include "actions.h"
#include "grades.h"
#include "ui.h"
#include <stdio.h>

#define GRADES_FILE "grades.csv"

void action_view_grades(const Session *s)
{
    static GradeRecord records[GRADE_RECORDS_MAX];
    static GradeRecord results[GRADE_RECORDS_MAX];

    size_t count = 0;
    size_t found = 0;

    char student_id[GRADE_STUDENT_ID_MAX + 1];

    if (s == NULL) {
        ui_error("Invalid session.");
        return;
    }

    /* Check permission before accessing grade records. */
    if (!has_permission(s->role, PERM_VIEW_GRADES)) {
        ui_error("You do not have permission to view grades.");
        return;
    }

    /* Students can only view their own grades. */
    if (s->role == ROLE_STUDENT) {
        if (s->student_id[0] == '\0') {
            ui_error("Your Student ID is not linked to your account.");
            return;
        }

        if (!can_access_student(s, s->student_id)) {
            ui_error("Access denied.");
            return;
        }

        snprintf(student_id, sizeof student_id,
                 "%s", s->student_id);
    }

    /* Faculty can select a student. */
    else if (s->role == ROLE_FACULTY) {
        if (!ui_read_line("Enter Student ID:",
                          student_id,
                          sizeof student_id)) {
            return;
        }

        if (student_id[0] == '\0') {
            ui_error("Student ID cannot be empty.");
            return;
        }

        if (!can_access_student(s, student_id)) {
            ui_error("Access denied.");
            return;
        }
    }

    else {
        ui_error("Invalid role for grade viewing.");
        return;
    }

    /* Load grade records from CSV. */
    if (!grades_load_csv(GRADES_FILE,
                         records,
                         GRADE_RECORDS_MAX,
                         &count)) {
        ui_error("Unable to load grades.csv.");
        return;
    }

    /* Retrieve the requested student's grades. */
    found = grades_get_student_records(
        records,
        count,
        student_id,
        results,
        GRADE_RECORDS_MAX
    );

    if (found == 0) {
        ui_info("No grade records found for this student.");
        return;
    }

    /* Display the grade report. */
    ui_banner("Student Grade Report");
    grades_display(results, found, stdout);
}
