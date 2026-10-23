#include "actions.h"
#include "ui.h"

#define STUB(fn, label) \
    void fn(const Session *s) { \
        (void)s; \
        ui_info(label " is not available yet (implemented by another sprint item)."); \
    }

STUB(action_add_student,        "Add Student")
STUB(action_view_student,       "View Student")
STUB(action_view_all_students,  "View All Students")
STUB(action_search_student,     "Search Student")
STUB(action_update_student,     "Update Student")
STUB(action_delete_student,     "Delete Student")
STUB(action_record_grade,       "Record Grades")
STUB(action_update_grade,       "Update Grades")
STUB(action_view_grades,        "View Grades")
STUB(action_record_attendance,  "Record Attendance")
STUB(action_update_attendance,  "Update Attendance")
STUB(action_view_attendance,    "View Attendance")
STUB(action_filter_records,     "Filter Records")

int action_save_on_exit(void)
{
    return 1;
}