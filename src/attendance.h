
#ifndef ATTENDANCE_H
#define ATTENDANCE_H

#include <stddef.h>
#include <stdio.h>

#define ATTENDANCE_RECORDS_MAX 1000
#define ATTENDANCE_STUDENT_ID_MAX 31
#define ATTENDANCE_DATE_MAX 10

typedef struct {
    char student_id[ATTENDANCE_STUDENT_ID_MAX + 1];
    char date[ATTENDANCE_DATE_MAX + 1];
    int present;
} AttendanceRecord;

/* Record attendance validation */
int attendance_validate(const AttendanceRecord *record);

/* Load attendance records from CSV */
int attendance_load_csv(
    const char *path,
    AttendanceRecord records[],
    size_t capacity,
    size_t *count
);

/* Save attendance records to CSV */
int attendance_save_csv(
    const char *path,
    const AttendanceRecord records[],
    size_t count
);

/* Add a new attendance record */
int attendance_add(
    AttendanceRecord records[],
    size_t *count,
    size_t capacity,
    const AttendanceRecord *record
);

/* Retrieve attendance records for a student */
size_t attendance_get_student_records(
    const AttendanceRecord records[],
    size_t count,
    const char *student_id,
    AttendanceRecord results[],
    size_t capacity
);

/* Display attendance records */
void attendance_display(
    const AttendanceRecord records[],
    size_t count,
    FILE *output
);

#endif
