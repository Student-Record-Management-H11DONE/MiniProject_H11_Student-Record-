# Deviations from SRS / SAD / Test Plan

## Sprint-1 Deviations

1. Implemented in C (allowed by MTR-01 "C/C++"); SAD module names .cpp/.h became .c/.h. Headers carry extern "C" guards so C++ modules can still link against them.
2. SAD says auth reads credentials via storage loadRecords(). Storage is a separate sprint item, so auth.c reads its own users.csv (passwords stored as hashes).
3. UI helpers were split from main.c into ui.c / ui.h to guarantee consistent messages (NFR-10).
4. authenticate() is internal to auth.c; login() returns AuthResult with an added student_id field (needed for Student own-record access).
5. A Logout option was added to the main menu (not listed in SRS).
6. Feature handlers for other sprint items are placeholders in actions_stub.c until teammates replace them.
7. Demo credentials are auto-created in users.csv on first run (admin/faculty1/student1).

## Sprint-2 Deviations & Implementation Notes

8. **NFR-07 Safe File Replacement on Windows:** In standard ISO C (`<stdio.h>`), `rename()` fails on Windows if the target destination file already exists. Therefore, `student_storage_save_atomic()` performs `remove(path)` immediately before `rename(temp_path, path)`. This provides safe temp-file replacement (protecting the original file if data validation or writing fails midway), though on Windows the file replacement is quasi-atomic due to runtime library behavior.
9. **NFR-06 & NFR-07 Module Integration:** The backup (`backup_create`, `backup_restore`) and safe atomic save functions are currently implemented as a modular C library in `src/backup.c` and exercised through automated unit tests (`tests/test_backup.c`), rather than being wired into the interactive `main.c` menu.
10. **Dual Implementation Track (Web App vs. C Core):** In Sprint-2, the team integrated a Flask/MySQL web application (`backend/` and `web/` merged in PR #51) for graphical browser workflows, while the C console implementation serves as the core system and automated NFR verification track.
