
#ifndef GRADES_H
#define GRADES_H

#include <stddef.h>
#include <stdio.h>

/* Maximum field sizes */
#define GRADE_STUDENT_ID_MAX 31
#define GRADE_SUBJECT_MAX 99
#define GRADE_LABEL_MAX 15
#define GRADE_RECORDS_MAX 5000

/*
 * Grade record structure.
 * Each record represents one subject for one student.
 */
typedef struct {
    char student_id[GRADE_STUDENT_ID_MAX + 1];
    char subject[GRADE_SUBJECT_MAX + 1];
    double marks;
    char grade[GRADE_LABEL_MAX + 1];
} GradeRecord;

/*
 * Load grade records from a CSV file.
 * Returns 1 on success, 0 on failure.
 */
int grades_load_csv(
    const char *path,
    GradeRecord records[],
    size_t capacity,
    size_t *count
);

/*
 * Retrieve records belonging to a particular student.
 * Returns the number of records retrieved.
 */
size_t grades_get_student_records(
    const GradeRecord records[],
    size_t count,
    const char *student_id,
    GradeRecord results[],
    size_t capacity
);

/*
 * Display subject-wise grades.
 */
void grades_display(
    const GradeRecord records[],
    size_t count,
    FILE *output
);

#endif
