#include <stdio.h>
#include <string.h>
#include <time.h>
#include "student_storage.h"
#include "integrity.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

#define PERF_RECORD_COUNT 1000

static StudentRecord records_in[PERF_RECORD_COUNT];
static StudentRecord records_out[PERF_RECORD_COUNT];

int main(void)
{
    const char *path = "test_performance_1000.csv";
    size_t i;
    size_t count = 0;
    clock_t start, end;
    double time_save, time_load, time_integrity, time_search;

    remove(path);

    /* Generate 1,000 unique records (NFR-11: Scalability) */
    for (i = 0; i < PERF_RECORD_COUNT; i++) {
        snprintf(records_in[i].student_id, sizeof(records_in[i].student_id), "STU%05zu", i + 1);
        snprintf(records_in[i].student_name, sizeof(records_in[i].student_name), "Student Name %zu", i + 1);
        snprintf(records_in[i].course, sizeof(records_in[i].course), "Computer Science");
    }

    /* 1. Benchmark: Save 1,000 records (NFR-02 & NFR-11) */
    start = clock();
    CHECK(student_storage_save(path, records_in, PERF_RECORD_COUNT));
    end = clock();
    time_save = (double)(end - start) / CLOCKS_PER_SEC;
    printf("[BENCHMARK] Save 1,000 records: %.4f seconds (Target: < 1.00s)\n", time_save);
    CHECK(time_save < 1.0);

    /* 2. Benchmark: Load 1,000 records (NFR-02 & NFR-11) */
    start = clock();
    CHECK(student_storage_load(path, records_out, PERF_RECORD_COUNT, &count));
    end = clock();
    time_load = (double)(end - start) / CLOCKS_PER_SEC;
    printf("[BENCHMARK] Load 1,000 records: %.4f seconds (Target: < 1.00s)\n", time_load);
    CHECK(count == PERF_RECORD_COUNT);
    CHECK(time_load < 1.0);

    /* 3. Benchmark: Data Integrity check on 1,000 records (NFR-02 & NFR-03) */
    start = clock();
    CHECK(integrity_records_consistent(records_out, count));
    end = clock();
    time_integrity = (double)(end - start) / CLOCKS_PER_SEC;
    printf("[BENCHMARK] Consistency check 1,000 records: %.4f seconds (Target: < 1.00s)\n", time_integrity);
    CHECK(time_integrity < 1.0);

    /* 4. Benchmark: Search across 1,000 records */
    start = clock();
    for (i = 0; i < 100; i++) {
        size_t j;
        int found = 0;
        char target_id[32];
        snprintf(target_id, sizeof(target_id), "STU%05zu", (i * 9 + 7) % PERF_RECORD_COUNT + 1);

        for (j = 0; j < count; j++) {
            if (strcmp(records_out[j].student_id, target_id) == 0) {
                found = 1;
                break;
            }
        }
        CHECK(found);
    }
    end = clock();
    time_search = (double)(end - start) / CLOCKS_PER_SEC;
    printf("[BENCHMARK] 100 lookups over 1,000 records: %.4f seconds (Target: < 1.00s)\n", time_search);
    CHECK(time_search < 1.0);

    remove(path);

    printf("%s\n", fails ? "PERFORMANCE & SCALABILITY TESTS FAILED" : "ALL PERFORMANCE & SCALABILITY TESTS PASSED (NFR-02, NFR-11)");
    return fails;
}
