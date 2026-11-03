# NFR Traceability Matrix (Sprint-1 & Sprint-2)

This document tracks how all 12 Non-Functional Requirements (NFR-01 through NFR-12) defined in the SRS and SAD are met, where they are implemented in code, and how they are automated or verified.

| NFR | Requirement | How it is met | Where | Verified by | Sprint |
|:---|:---|:---|:---|:---|:---:|
| **NFR-01** | User-Friendly Interface | Formatted numbered menus, clear input prompts, and confirmation checks. | `src/ui.c`, `src/ui.h` | Manual review of `ui.c` and menu flows | Sprint-1 / Sprint-2 |
| **NFR-02** | System Performance | Sub-second execution time for queries, loads, and saves (< 1.0s target). | `src/student_storage.c` | Measured with `clock()` in `tests/test_performance.c` | Sprint-2 |
| **NFR-03** | Data Integrity | Rejection of duplicate Student IDs (case-insensitive) on save and load; field validation. | `src/integrity.c`, `src/integrity.h`, `src/student_storage.c` | `tests/test_integrity.c`, `tests/test_storage_integrity.c` | Sprint-1 (PR #48) |
| **NFR-04** | Error Handling & Stability | Rejection of invalid menu inputs, empty strings, NULL pointers, and unexpected EOF without crashing. | `src/ui.c`, `src/auth.c`, `src/main.c` | `tests/test_nfr.c`, CI stability step | Sprint-1 (PR #46) |
| **NFR-05** | Modular & Maintainable Design | Code cleanly split into single-responsibility modules (`auth`, `ui`, `storage`, `backup`, `integrity`, `validation`) with header files. | `src/` | Architecture review, modular compilation | Sprint-1 (PR #46) |
| **NFR-06** | Backup and Recovery | Copy-based backup creation (`backup_create`) and recovery from backup upon corruption (`backup_restore`). | `src/backup.c`, `src/backup.h` | `tests/test_backup.c` | Sprint-2 |
| **NFR-07** | Data Reliability | Writes to a temporary file first and replaces original only after write succeeds (`student_storage_save_atomic`), protecting existing data if save fails midway. | `src/backup.c` | `tests/test_backup.c` | Sprint-2 |
| **NFR-08** | Prevent Unauthorized Modification | Role-based check before executing any action; unauthorized options blocked. | `src/auth.c`, `src/main.c` | `tests/test_nfr.c` (RBAC tests) | Sprint-1 (PR #46) |
| **NFR-09** | Portability | Standard ISO C99 compliance without OS-specific API calls; builds identically across platforms. | All `src/` files | GitHub Actions CI matrix (Ubuntu & Windows runners) | Sprint-1 (PR #46) |
| **NFR-10** | Consistent UI Messages | Standardized notification formatting using uniform prefixes (`[INFO]`, `[ERROR]`, `[SUCCESS]`) and consistent prompt spacing. | `src/ui.c`, `src/ui.h` | Manual review of `ui.c` | Sprint-1 / Sprint-2 |
| **NFR-11** | Scalability and Performance | Tested processing of 1,000 student records without crashing. | `src/student_storage.c`, `src/student_record.h` | `tests/test_performance.c` | Sprint-2 |
| **NFR-12** | Role-Based Information Access | Information access scoped by authenticated role; student users can only inspect their own record (`can_access_student`). | `src/auth.c`, `src/auth.h` | `tests/test_nfr.c` | Sprint-1 (PR #46) |

---

### Implementation Status
- **Sprint-1 Merged:** NFR-03, NFR-04, NFR-05, NFR-08, NFR-09, NFR-12
- **Sprint-2 Implemented:** NFR-01, NFR-02, NFR-06, NFR-07, NFR-10, NFR-11 (under branch `feature/sprint2-nfr`)
- See [`DEVIATIONS.md`](DEVIATIONS.md) for notes on platform-specific file replacement and library-level test integration.
