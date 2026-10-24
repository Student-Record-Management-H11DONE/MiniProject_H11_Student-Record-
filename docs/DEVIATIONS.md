# Sprint-1 Deviations from SRS / SAD / Test Plan

1. Implemented in C (allowed by MTR-01 "C/C++"); SAD module names .cpp/.h became .c/.h. Headers carry extern "C" guards so C++ modules can still link against them.
2. SAD says auth reads credentials via storage loadRecords(). Storage is a separate sprint item, so auth.c reads its own users.csv (passwords stored as hashes).
3. UI helpers were split from main.c into ui.c / ui.h to guarantee consistent messages (NFR-10).
4. authenticate() is internal to auth.c; login() returns AuthResult with an added student_id field (needed for Student own-record access).
5. A Logout option was added to the main menu (not listed in SRS).
6. Feature handlers for other sprint items are placeholders in actions_stub.c until teammates replace them.
7. Demo credentials are auto-created in users.csv on first run (admin/faculty1/student1).