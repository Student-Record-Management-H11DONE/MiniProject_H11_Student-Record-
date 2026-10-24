
#ifndef VALIDATION_H
#define VALIDATION_H

/* Returns 1 when the value contains non-whitespace text, otherwise 0. */
int validate_required_field(const char *value);

/* Returns 1 when all required fields are valid, otherwise 0. */
int validate_student_fields(const char *student_id,
                            const char *student_name);

#endif
