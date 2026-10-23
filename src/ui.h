#ifndef UI_H
#define UI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_banner(const char *title);
void ui_info(const char *msg);
void ui_success(const char *msg);
void ui_warning(const char *msg);
void ui_error(const char *msg);

/* Reads a trimmed line. Returns 1 on success, 0 on end-of-input. */
int ui_read_line(const char *prompt, char *out, size_t size);
/* Returns a number in [lo, hi], or -1 on end-of-input. */
int ui_read_choice(const char *prompt, int lo, int hi);
/* y/n question. Returns 1 for yes, 0 for no. */
int ui_confirm(const char *question);
void ui_pause(void);

#ifdef __cplusplus
}
#endif

#endif