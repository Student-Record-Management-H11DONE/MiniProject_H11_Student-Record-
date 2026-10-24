# NFR Traceability (Sprint-1)

| NFR | Requirement | How it is met | Where | Verified by |
|-----|-------------|---------------|-------|-------------|
| NFR-04 | Error handling and stability | Invalid menu input is rejected and re-prompted. Empty or NULL login input is handled. End-of-input exits safely. | ui.c (ui_read_choice, ui_read_line), auth.c (login), main.c | test_nfr.c (NFR-04 section), CI stability test |
| NFR-05 | Modular and maintainable design | Code is split into ui, auth, actions and main modules, each with a header file. | src/ | Code review, same build on every push |
| NFR-08 | Prevent unauthorized modification | Each role has an allowed-action list. The menu hides forbidden actions. main.c checks again before running one. | auth.c (has_permission), main.c | test_nfr.c (NFR-08 section) |
| NFR-09 | Portability | Standard C99 only, no OS-specific calls. Builds with gcc. | all src/ files | GitHub Actions builds on Ubuntu and Windows |
| NFR-12 | Role-based information access | Students see only their own record. Admin and Faculty see only what their role allows. | auth.c (has_permission, can_access_student) | test_nfr.c (NFR-12 section) |

## Not in Sprint-1 (planned for Sprint-2)

NFR-03 Data Integrity, NFR-06 Backup and Recovery, NFR-07 Data Reliability and NFR-11 Scalability and Performance depend on the storage module and will be added in Sprint-2.
