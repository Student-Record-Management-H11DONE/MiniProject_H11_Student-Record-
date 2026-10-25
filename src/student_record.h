
#ifndef STUDENT_RECORD_H
#define STUDENT_RECORD_H

#define STUDENT_ID_MAX 31
#define STUDENT_NAME_MAX 99
#define STUDENT_COURSE_MAX 99
#define STUDENT_RECORDS_MAX 1000

typedef struct {
    char student_id[STUDENT_ID_MAX + 1];
    char student_name[STUDENT_NAME_MAX + 1];
    char course[STUDENT_COURSE_MAX + 1];
} StudentRecord;

#endif
