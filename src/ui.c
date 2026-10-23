#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void ui_banner(const char *title)
{
    printf("\n==============================================\n");
    printf("  %s\n", title);
    printf("==============================================\n");
}

void ui_info(const char *msg)    { printf("[INFO]  %s\n", msg); }
void ui_success(const char *msg) { printf("[OK]    %s\n", msg); }
void ui_warning(const char *msg) { printf("[WARN]  %s\n", msg); }
void ui_error(const char *msg)   { printf("[ERROR] %s\n", msg); }

int ui_read_line(const char *prompt, char *out, size_t size)
{
    size_t len, start;

    fputs(prompt, stdout);
    fflush(stdout);

    if (fgets(out, (int)size, stdin) == NULL) {
        return 0;
    }

    len = strlen(out);
    if (len > 0 && out[len - 1] == '\n') {
        out[len - 1] = '\0';
    } else {
        /* input longer than buffer: discard the rest of the line */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }

    /* trim trailing whitespace */
    len = strlen(out);
    while (len > 0 && isspace((unsigned char)out[len - 1])) {
        out[--len] = '\0';
    }
    /* trim leading whitespace */
    start = 0;
    while (out[start] != '\0' && isspace((unsigned char)out[start])) {
        start++;
    }
    if (start > 0) {
        memmove(out, out + start, strlen(out + start) + 1);
    }
    return 1;
}

int ui_read_choice(const char *prompt, int lo, int hi)
{
    char buf[64];

    for (;;) {
        char *end;
        long v;

        if (!ui_read_line(prompt, buf, sizeof buf)) {
            return -1;
        }
        if (buf[0] != '\0') {
            v = strtol(buf, &end, 10);
            if (*end == '\0' && v >= lo && v <= hi) {
                return (int)v;
            }
        }
        printf("[ERROR] Invalid choice. Please enter a number between %d and %d.\n", lo, hi);
    }
}

int ui_confirm(const char *question)
{
    char buf[64];
    char prompt[160];

    snprintf(prompt, sizeof prompt, "%s (y/n): ", question);
    for (;;) {
        if (!ui_read_line(prompt, buf, sizeof buf)) {
            return 1; /* end of input: proceed */
        }
        if (strcmp(buf, "y") == 0 || strcmp(buf, "Y") == 0) return 1;
        if (strcmp(buf, "n") == 0 || strcmp(buf, "N") == 0) return 0;
        ui_error("Please enter 'y' or 'n'.");
    }
}

void ui_pause(void)
{
    char buf[64];
    ui_read_line("Press Enter to continue...", buf, sizeof buf);
}