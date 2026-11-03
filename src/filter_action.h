
#ifndef FILTER_ACTION_H
#define FILTER_ACTION_H

#include <stddef.h>

#define FILTER_RECORDS_MAX 1000

#define FILTER_STUDENT_ID_MAX 31
#define FILTER_STUDENT_NAME_MAX 99
#define FILTER_COURSE_MAX 99
#define FILTER_DEPARTMENT_MAX 99
#define FILTER_SEMESTER_MAX 20

typedef struct {
    char student_id[FILTER_STUDENT_ID_MAX + 1];
    char student_name[FILTER_STUDENT_NAME_MAX + 1];
    char course[FILTER_COURSE_MAX + 1];
    char department[FILTER_DEPARTMENT_MAX + 1];
    char semester[FILTER_SEMESTER_MAX + 1];
} FilterStudentRecord;

typedef enum {
    FILTER_BY_COURSE,
    FILTER_BY_DEPARTMENT,
    FILTER_BY_SEMESTER
} FilterType;

size_t filter_student_records(
    const FilterStudentRecord records[],
    size_t count,
    FilterType filter,
    const char *value,
    FilterStudentRecord results[],
    size_t capacity
);

#endif
