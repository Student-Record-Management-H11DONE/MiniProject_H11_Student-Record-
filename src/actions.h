#ifndef ACTIONS_H
#define ACTIONS_H

#include "auth.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Each function below belongs to another sprint item.
   Teammate: implement it in your own .c file and DELETE the matching
   line in actions_stub.c (otherwise the linker reports a duplicate). */
void action_add_student(const Session *s);        /* FR-05, FR-06 */
void action_view_student(const Session *s);       /* FR-07 (Student: use can_access_student) */
void action_view_all_students(const Session *s);  /* FR-08 */
void action_search_student(const Session *s);     /* FR-09 */
void action_update_student(const Session *s);     /* FR-10 */
void action_delete_student(const Session *s);     /* FR-11 */
void action_record_grade(const Session *s);
void action_update_grade(const Session *s);
void action_view_grades(const Session *s);
void action_record_attendance(const Session *s);
void action_update_attendance(const Session *s);
void action_view_attendance(const Session *s);
void action_filter_records(const Session *s);

/* FR-22 / FR-25: flush data before exit. Return 1 on success, 0 on failure. */
int action_save_on_exit(void);

#ifdef __cplusplus
}
#endif

#endif