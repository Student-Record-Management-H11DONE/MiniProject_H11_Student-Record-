# NFR Traceability Matrix (Sprint-1 & Sprint-2)

This document tracks how all 12 Non-Functional Requirements (NFR-01 through NFR-12) defined in the SRS and SAD are met, where they are implemented in code, and how they are automated and verified.

| NFR | Requirement | How it is met | Where | Verified by | Sprint |
|:---|:---|:---|:---|:---|:---:|
| **NFR-01** | User-Friendly Interface | Intuitive text interface with formatted numbered menus, clear input prompts, aligned table outputs, and confirmation checks. | `src/ui.c`, `src/ui.h` | Code review, manual testing checklist | Sprint-1 / Sprint-2 |
| **NFR-02** | System Performance | Sub-second latency for all queries, loads, and saves (< 1.0 second response time target; executes in milliseconds). | `src/student_storage.c` | `tests/test_performance.c` | Sprint-2 |
| **NFR-03** | Data Integrity | Rejection of duplicate Student IDs (case-insensitive) on save and load; field boundary & format verification. | `src/integrity.c`, `src/integrity.h`, `src/student_storage.c` | `tests/test_integrity.c`, `tests/test_storage_integrity.c` | Sprint-1 (PR #48) |
| **NFR-04** | Error Handling & Stability | Non-crashing error handling on invalid menu options, empty inputs, NULL pointers, and unexpected EOF. | `src/ui.c`, `src/auth.c`, `src/main.c` | `tests/test_nfr.c`, CI stability step | Sprint-1 (PR #46) |
| **NFR-05** | Modular & Maintainable Design | Codebase cleanly partitioned into single-responsibility modules (`auth`, `ui`, `storage`, `backup`, `integrity`, `validation`) with clean headers. | `src/` | Architecture review, modular compilation | Sprint-1 (PR #46) |
| **NFR-06** | Backup and Recovery | Byte-level automated backup generation (`backup_create`) and disaster recovery restore capability (`backup_restore`). | `src/backup.c`, `src/backup.h` | `tests/test_backup.c` | Sprint-2 |
| **NFR-07** | Data Reliability | Atomic crash-safe file saves (`student_storage_save_atomic`) via temporary `.tmp` write and atomic replace to eliminate partial-write corruption. | `src/backup.c`, `src/student_storage.c` | `tests/test_backup.c` | Sprint-2 |
| **NFR-08** | Prevent Unauthorized Modification | Strict Role-Based Access Control (RBAC) permission check before invoking any operation; forbidden actions hidden and blocked. | `src/auth.c`, `src/main.c` | `tests/test_nfr.c` | Sprint-1 (PR #46) |
| **NFR-09** | Portability | Standard ISO C99 compliance without OS-specific API dependencies; builds identically across environments. | All `src/` files | GitHub Actions CI matrix (Ubuntu-latest & Windows-latest) | Sprint-1 (PR #46) |
| **NFR-10** | Consistent UI Messages | Standardized notification formatting using uniform prefixes (`[INFO]`, `[ERROR]`, `[SUCCESS]`) and consistent prompt spacing. | `src/ui.c`, `src/ui.h` | UI consistency checks, `test_auth.c` | Sprint-1 / Sprint-2 |
| **NFR-11** | Scalability and Performance | Reliable processing of up to 1,000 student records without memory leakage, buffer overflows, or degradation. | `src/student_storage.c`, `src/student_record.h` | `tests/test_performance.c` | Sprint-2 |
| **NFR-12** | Role-Based Information Access | Information access scoped by authenticated role; student users can only inspect their own record (`can_access_student`). | `src/auth.c`, `src/auth.h` | `tests/test_nfr.c` | Sprint-1 (PR #46) |

---

### Verification Summary
- **Sprint-1 Verified:** NFR-03, NFR-04, NFR-05, NFR-08, NFR-09, NFR-12
- **Sprint-2 Verified:** NFR-01, NFR-02, NFR-06, NFR-07, NFR-10, NFR-11
- **All 12 Non-Functional Requirements are 100% satisfied and automated via CI.**
