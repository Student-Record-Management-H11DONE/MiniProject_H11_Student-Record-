
#include <stdio.h>
#include <string.h>
#include "../src/student_storage.h"

int main(int argc, char *argv[])
{
    StudentRecord records[10];
    size_t count = 0;

    if (argc != 2) {
        printf("Usage: test_persistence save|load\n");
        return 1;
    }

    if (strcmp(argv[1], "save") == 0) {
        strcpy(records[0].student_id, "S2001");
        strcpy(records[0].student_name, "Riya");
        strcpy(records[0].course, "Nursing");

        if (!student_storage_save("persistence_test.csv", records, 1)) {
            printf("Save failed.\n");
            return 1;
        }

        printf("Record saved. Close this process and run load mode.\n");
        return 0;
    }

    if (strcmp(argv[1], "load") == 0) {
        if (!student_storage_load("persistence_test.csv",
                                  records, 10, &count)) {
            printf("Load failed.\n");
            return 1;
        }

        if (count != 1 ||
            strcmp(records[0].student_id, "S2001") != 0 ||
            strcmp(records[0].student_name, "Riya") != 0 ||
            strcmp(records[0].course, "Nursing") != 0) {
            printf("Persistence test failed.\n");
            return 1;
        }

        printf("Persistence test passed: record survived restart.\n");
        return 0;
    }

    printf("Unknown mode. Use save or load.\n");
    return 1;
}
