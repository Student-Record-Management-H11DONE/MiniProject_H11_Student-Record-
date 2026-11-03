
#ifndef SEARCH_ACTION_H
#define SEARCH_ACTION_H

#include <stddef.h>
#include "filter_action.h"

size_t search_student_records(
    const FilterStudentRecord records[],
    size_t count,
    const char *search_value,
    FilterStudentRecord results[],
    size_t capacity
);

#endif
