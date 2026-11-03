
#include "actions.h"
#include "attendance.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define ATTENDANCE_FILE "attendance.csv"

/* Load existing attendance records.
   If the CSV does not exist, start with an empty record list. */
static int load_attendance_records(
    AttendanceRecord records[],
    size_t *count)
{
    FILE *file;

    file = fopen(ATTENDANCE_FILE, "r");

    if (file == NULL) {
        if (errno == ENOENT) {
            *count = 0;
            return 1;
        }
        return 0;
    }

    fclose(file);

    return attendance_load_csv(
        ATTENDANCE_FILE,
        records,
        ATTENDANCE_RECORDS_MAX,
        count
    );
}

/* Read attendance status from the user. */
static int read_attendance_status(int *present)
{
    char status[16];

    if (!ui_read_line(
            "Attendance (P = Present / A = Absent):",
            status,
            sizeof status)) {
        return 0;
    }

    if (strcmp(status, "P") == 0 ||
        strcmp(status, "p") == 0 ||
        strcmp(status, "1") == 0) {
        *present = 1;
        return 1;
    }

    if (strcmp(status, "A") == 0 ||
        strcmp(status, "a") == 0 ||
        strcmp(status, "0") == 0) {
        *present = 0;
        return 1;
    }

    ui_error("Invalid attendance status. Enter P or A.");
    return 0;
}

/* FR-15: Record student attendance. */
void action_record_attendance(const Session *s)
{
    static AttendanceRecord records[ATTENDANCE_RECORDS_MAX];

    AttendanceRecord record;
    size_t count = 0;

    if (s == NULL ||
        !has_permission(s->role, PERM_RECORD_ATTENDANCE)) {
        ui_error("You do not have permission to record attendance.");
        return;
    }

    if (!load_attendance_records(records, &count)) {
        ui_error("Unable to load attendance records.");
        return;
    }

    memset(&record, 0, sizeof record);

    ui_banner("Record Student Attendance");

    if (!ui_read_line("Enter Student ID:",
                      record.student_id,
                      sizeof record.student_id)) {
        return;
    }

    if (!ui_read_line("Enter Date (YYYY-MM-DD):",
                      record.date,
                      sizeof record.date)) {
        return;
    }

    if (!read_attendance_status(&record.present)) {
        return;
    }

    if (!attendance_validate(&record)) {
        ui_error("Invalid student ID, date or attendance details.");
        return;
    }

    if (!attendance_add(
            records,
            &count,
            ATTENDANCE_RECORDS_MAX,
            &record)) {
        ui_error("Duplicate attendance or maximum capacity reached.");
        return;
    }

    if (!attendance_save_csv(
            ATTENDANCE_FILE,
            records,
            count)) {
        ui_error("Unable to save attendance records.");
        return;
    }

    ui_success("Student attendance recorded successfully.");
}

/* FR-16: Update existing attendance. */
void action_update_attendance(const Session *s)
{
    static AttendanceRecord records[ATTENDANCE_RECORDS_MAX];

    AttendanceRecord updated;
    size_t count = 0;
    size_t i;
    int found = 0;

    if (s == NULL ||
        !has_permission(s->role, PERM_UPDATE_ATTENDANCE)) {
        ui_error("You do not have permission to update attendance.");
        return;
    }

    if (!load_attendance_records(records, &count)) {
        ui_error("Unable to load attendance records.");
        return;
    }

    ui_banner("Update Student Attendance");

    memset(&updated, 0, sizeof updated);

    if (!ui_read_line("Enter Student ID:",
                      updated.student_id,
                      sizeof updated.student_id)) {
        return;
    }

    if (!ui_read_line("Enter Date (YYYY-MM-DD):",
                      updated.date,
                      sizeof updated.date)) {
        return;
    }

    if (!read_attendance_status(&updated.present)) {
        return;
    }

    if (!attendance_validate(&updated)) {
        ui_error("Invalid attendance details.");
        return;
    }

    for (i = 0; i < count; i++) {
        if (strcmp(records[i].student_id,
                   updated.student_id) == 0 &&
            strcmp(records[i].date,
                   updated.date) == 0) {

            records[i].present = updated.present;
            found = 1;
            break;
        }
    }

    if (!found) {
        ui_warning("No attendance record found for this student and date.");
        return;
    }

    if (!attendance_save_csv(
            ATTENDANCE_FILE,
            records,
            count)) {
        ui_error("Unable to save updated attendance.");
        return;
    }

    ui_success("Attendance updated successfully.");
}

/* FR-17: View student attendance. */
void action_view_attendance(const Session *s)
{
    static AttendanceRecord records[ATTENDANCE_RECORDS_MAX];
    static AttendanceRecord results[ATTENDANCE_RECORDS_MAX];

    size_t count = 0;
    size_t found = 0;

    char student_id[ATTENDANCE_STUDENT_ID_MAX + 1];

    if (s == NULL ||
        !has_permission(s->role, PERM_VIEW_ATTENDANCE)) {
        ui_error("You do not have permission to view attendance.");
        return;
    }

    ui_banner("View Student Attendance");

    /* Students can view only their own attendance. */
    if (s->role == ROLE_STUDENT) {

        if (s->student_id[0] == '\0') {
            ui_error("Student ID is not linked to your account.");
            return;
        }

        if (!can_access_student(s, s->student_id)) {
            ui_error("Access denied.");
            return;
        }

        snprintf(student_id, sizeof student_id,
                 "%s", s->student_id);
    }

    /* Faculty can view attendance of a selected student. */
    else if (s->role == ROLE_FACULTY) {

        if (!ui_read_line("Enter Student ID:",
                          student_id,
                          sizeof student_id)) {
            return;
        }

        if (!can_access_student(s, student_id)) {
            ui_error("Access denied.");
            return;
        }
    }

    else {
        ui_error("Invalid role.");
        return;
    }

    if (!load_attendance_records(records, &count)) {
        ui_error("Unable to load attendance records.");
        return;
    }

    found = attendance_get_student_records(
        records,
        count,
        student_id,
        results,
        ATTENDANCE_RECORDS_MAX
    );

    if (found == 0) {
        ui_info("No attendance records found for this student.");
        return;
    }

    ui_banner("Student Attendance Report");

    attendance_display(results, found, stdout);
}
