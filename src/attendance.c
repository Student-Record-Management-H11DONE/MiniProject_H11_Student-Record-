
#include "attendance.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define CSV_LINE_SIZE 256

/* Check whether a year is a leap year. */
static int is_leap_year(int year)
{
    return (year % 400 == 0) ||
           (year % 4 == 0 && year % 100 != 0);
}

/* Validate date in YYYY-MM-DD format. */
static int validate_date(const char *date)
{
    int year, month, day;
    int days_in_month[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (date == NULL || strlen(date) != 10)
        return 0;

    if (date[4] != '-' || date[7] != '-')
        return 0;

    if (!isdigit((unsigned char)date[0]) ||
        !isdigit((unsigned char)date[1]) ||
        !isdigit((unsigned char)date[2]) ||
        !isdigit((unsigned char)date[3]) ||
        !isdigit((unsigned char)date[5]) ||
        !isdigit((unsigned char)date[6]) ||
        !isdigit((unsigned char)date[8]) ||
        !isdigit((unsigned char)date[9]))
        return 0;

    year = (date[0] - '0') * 1000 +
           (date[1] - '0') * 100 +
           (date[2] - '0') * 10 +
           (date[3] - '0');

    month = (date[5] - '0') * 10 +
            (date[6] - '0');

    day = (date[8] - '0') * 10 +
          (date[9] - '0');

    if (year < 1 || month < 1 || month > 12)
        return 0;

    if (is_leap_year(year))
        days_in_month[1] = 29;

    return day >= 1 && day <= days_in_month[month - 1];
}

/* Validate attendance record. */
int attendance_validate(const AttendanceRecord *record)
{
    size_t i;

    if (record == NULL)
        return 0;

    if (record->student_id[0] == '\0')
        return 0;

    if (strlen(record->student_id) >
        ATTENDANCE_STUDENT_ID_MAX)
        return 0;

    for (i = 0; record->student_id[i] != '\0'; i++) {
        if (isspace((unsigned char)record->student_id[i]))
            return 0;
        if (record->student_id[i] == ',')
            return 0;
    }

    if (!validate_date(record->date))
        return 0;

    if (record->present != 0 && record->present != 1)
        return 0;

    return 1;
}

/* Load attendance records from CSV. */
int attendance_load_csv(
    const char *path,
    AttendanceRecord records[],
    size_t capacity,
    size_t *count)
{
    FILE *file;
    char line[CSV_LINE_SIZE];
    size_t loaded = 0;
    int first_line = 1;

    if (count == NULL)
        return 0;

    *count = 0;

    if (path == NULL ||
        (records == NULL && capacity != 0) ||
        capacity > ATTENDANCE_RECORDS_MAX)
        return 0;

    file = fopen(path, "r");

    if (file == NULL)
        return 0;

    while (fgets(line, sizeof line, file) != NULL) {
        AttendanceRecord record;
        char *first, *second;
        char *end;
        long value;
        size_t length = strlen(line);

        if (length > 0 &&
            line[length - 1] != '\n' &&
            !feof(file)) {
            fclose(file);
            return 0;
        }

        line[strcspn(line, "\r\n")] = '\0';

        if (first_line) {
            first_line = 0;

            if (strcmp(line,
                "student_id,date,present") != 0) {
                fclose(file);
                return 0;
            }

            continue;
        }

        if (line[0] == '\0')
            continue;

        first = strchr(line, ',');

        if (first == NULL) {
            fclose(file);
            return 0;
        }

        second = strchr(first + 1, ',');

        if (second == NULL ||
            strchr(second + 1, ',') != NULL) {
            fclose(file);
            return 0;
        }

        *first = '\0';
        *second = '\0';

        if (strlen(line) > ATTENDANCE_STUDENT_ID_MAX ||
            strlen(first + 1) > ATTENDANCE_DATE_MAX) {
            fclose(file);
            return 0;
        }

        snprintf(record.student_id,
                 sizeof record.student_id, "%s", line);

        snprintf(record.date,
                 sizeof record.date, "%s", first + 1);

        errno = 0;
        value = strtol(second + 1, &end, 10);

        if (errno == ERANGE ||
            end == second + 1 ||
            *end != '\0' ||
            value < 0 || value > 1) {
            fclose(file);
            return 0;
        }

        record.present = (int)value;

        if (!attendance_validate(&record)) {
            fclose(file);
            return 0;
        }

        if (loaded >= capacity ||
            loaded >= ATTENDANCE_RECORDS_MAX) {
            fclose(file);
            return 0;
        }

        records[loaded++] = record;
    }

    if (ferror(file) || first_line) {
        fclose(file);
        return 0;
    }

    if (fclose(file) != 0)
        return 0;

    *count = loaded;
    return 1;
}

/* Save attendance records to CSV. */
int attendance_save_csv(
    const char *path,
    const AttendanceRecord records[],
    size_t count)
{
    FILE *file;
    size_t i, j;

    if (path == NULL ||
        (records == NULL && count != 0) ||
        count > ATTENDANCE_RECORDS_MAX)
        return 0;

    for (i = 0; i < count; i++) {
        if (!attendance_validate(&records[i]))
            return 0;

        for (j = 0; j < i; j++) {
            if (strcmp(records[i].student_id,
                       records[j].student_id) == 0 &&
                strcmp(records[i].date,
                       records[j].date) == 0)
                return 0;
        }
    }

    file = fopen(path, "w");

    if (file == NULL)
        return 0;

    if (fprintf(file, "student_id,date,present\n") < 0) {
        fclose(file);
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (fprintf(file, "%s,%s,%d\n",
                    records[i].student_id,
                    records[i].date,
                    records[i].present) < 0) {
            fclose(file);
            return 0;
        }
    }

    return fclose(file) == 0;
}

/* Add a new attendance record. */
int attendance_add(
    AttendanceRecord records[],
    size_t *count,
    size_t capacity,
    const AttendanceRecord *record)
{
    size_t i;

    if (records == NULL ||
        count == NULL ||
        record == NULL ||
        *count > capacity ||
        capacity > ATTENDANCE_RECORDS_MAX)
        return 0;

    if (!attendance_validate(record))
        return 0;

    if (*count >= capacity)
        return 0;

    /* Prevent duplicate attendance for same student and date. */
    for (i = 0; i < *count; i++) {
        if (strcmp(records[i].student_id,
                   record->student_id) == 0 &&
            strcmp(records[i].date,
                   record->date) == 0)
            return 0;
    }

    records[*count] = *record;
    (*count)++;

    return 1;
}

/* Retrieve attendance records for a student. */
size_t attendance_get_student_records(
    const AttendanceRecord records[],
    size_t count,
    const char *student_id,
    AttendanceRecord results[],
    size_t capacity)
{
    size_t i, found = 0;

    if (records == NULL ||
        student_id == NULL ||
        results == NULL)
        return 0;

    for (i = 0; i < count && found < capacity; i++) {
        if (strcmp(records[i].student_id,
                   student_id) == 0) {
            results[found++] = records[i];
        }
    }

    return found;
}

/* Display attendance records. */
void attendance_display(
    const AttendanceRecord records[],
    size_t count,
    FILE *output)
{
    size_t i;

    if (output == NULL)
        return;

    if (records == NULL || count == 0) {
        fprintf(output, "No attendance records available.\n");
        return;
    }

    fprintf(output, "\n");
    fprintf(output, "---------------------------------------------\n");
    fprintf(output, "          STUDENT ATTENDANCE REPORT\n");
    fprintf(output, "---------------------------------------------\n");

    fprintf(output, "%-15s %-15s %-12s\n",
            "Student ID", "Date", "Attendance");

    fprintf(output, "---------------------------------------------\n");

    for (i = 0; i < count; i++) {
        fprintf(output, "%-15s %-15s %-12s\n",
                records[i].student_id,
                records[i].date,
                records[i].present ? "Present" : "Absent");
    }

    fprintf(output, "---------------------------------------------\n");
}
