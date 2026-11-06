# Updated Test Cases (Sprint 1 & Sprint 2)

**Project:** Student Record Management System  
**Team:** H-11  
**Deliverable:** Part-3 Testing (Requirement 1: Test Cases Updation)  
**Last Updated:** November 6, 2026  
**Total Test Cases:** 65  
**Overall Status:** 100% Passed  

---

## 1. Functional Requirements Test Suite (FR-01 to FR-25)

### 1.1 Authentication & Navigation (FR-01 – FR-04, FR-25)
| Test Case ID | Requirement | Test Steps | Expected Result | Actual Result | Status |
| :--- | :--- | :--- | :--- | :--- | :---: |
| **TC-FR-01A** | FR-01: Admin Login | Sign in with username `admin` and password `admin123`. | Login succeeds; Administrator dashboard rendered with student management actions. | As expected. | **PASS** |
| **TC-FR-01B** | FR-01: Faculty Login | Sign in with username `faculty1` and password `faculty123`. | Login succeeds; Faculty dashboard rendered showing assigned subjects/grades. | As expected. | **PASS** |
| **TC-FR-01C** | FR-01: Student Login | Sign in with username `student1` and password `student123`. | Login succeeds; Personal profile and grade view displayed. | As expected. | **PASS** |
| **TC-FR-02A** | FR-02: Invalid Password | Enter valid username with wrong password. | Authentication rejected with "Invalid username or password". Session not created. | As expected (401). | **PASS** |
| **TC-FR-02B** | FR-02: Empty Inputs | Submit login form with blank username or password. | Rejected with client/server validation "Please enter your username and password". | As expected (400). | **PASS** |
| **TC-FR-03** | FR-03: Role Navigation | Log in as Admin, Faculty, and Student sequentially. | Navbars dynamically display only permissible menu options per role. | As expected. | **PASS** |
| **TC-FR-04** | FR-04: Logout | Click "Sign out" button from any authenticated dashboard. | Session invalidated, cookie cleared, redirected to login page; API calls return 401. | As expected (200). | **PASS** |
| **TC-FR-25** | FR-25: Session Persistence | Refresh browser tab or open new tab within active session. | Active session preserved via secure HTTP-only session cookie. | As expected. | **PASS** |

---

### 1.2 Student Management & Profile (FR-05 – FR-11)
| Test Case ID | Requirement | Test Steps | Expected Result | Actual Result | Status |
| :--- | :--- | :--- | :--- | :--- | :---: |
| **TC-FR-05** | FR-05: Add Student | Admin creates student with ID `S999`, Name `Test Student`, Dept `CSE`, Sem `3`. | Record stored in database; appears in student directory; user credentials created. | Record created. | **PASS** |
| **TC-FR-06** | FR-06: Duplicate ID Rejection | Admin attempts to create student with an existing ID (`S001`). | Rejected with error "Student ID already exists"; original record unaffected. | Rejected (409). | **PASS** |
| **TC-FR-07** | FR-07: View Student Profile | Admin / Faculty opens student profile view. | Displays ID, full name, contact, department, semester, and registered courses. | As expected. | **PASS** |
| **TC-FR-08** | FR-08: Student Self View | Student logs in and navigates to "My Profile". | Shows only logged-in student's personal information. Cross-access to other IDs blocked. | As expected. | **PASS** |
| **TC-FR-09** | FR-09: Search Students | Search by partial name (e.g., `Aarav`) or student ID prefix (`S01`). | Instantly returns matching subset; displays "No matching record" if not found. | Matches returned. | **PASS** |
| **TC-FR-10** | FR-10: Update Student | Admin edits student phone, semester, or name and saves. | Updated details persisted in database and reflected on subsequent lookups. | Update saved. | **PASS** |
| **TC-FR-11** | FR-11: Delete Student | Admin selects delete student and confirms modal prompt. | Student record, linked attendance, grades, and auth user cascade deleted safely. | Record removed. | **PASS** |

---

### 1.3 Grades & Assessment Windows (FR-12 – FR-15, FR-20 – FR-22)
| Test Case ID | Requirement | Test Steps | Expected Result | Actual Result | Status |
| :--- | :--- | :--- | :--- | :--- | :---: |
| **TC-FR-12** | FR-12: Grade Entry (Open Window) | Faculty inputs marks (0–100) for student in assigned subject while window is OPEN. | Grade recorded successfully in `grades` table. | Saved (200). | **PASS** |
| **TC-FR-13** | FR-13: Grade Entry (Closed Window) | Faculty attempts to record/modify grade when assessment window is CLOSED. | Request rejected with error "Assessment window is closed for this department/semester". | Rejected (403). | **PASS** |
| **TC-FR-14** | FR-14: Grade Bounds Validation | Faculty inputs negative mark (`-5`) or mark exceeding maximum (`105`). | Rejected with validation error "Marks must be between 0 and 100". | Rejected (400). | **PASS** |
| **TC-FR-15** | FR-15: Student View Grades | Student views "My Grades" section. | Shows enrolled subjects, internal/external marks, total, and grade letters (read-only). | Rendered read-only. | **PASS** |
| **TC-FR-20** | FR-20: Grade Change Audit Log | Faculty updates existing grade from 85 to 90. | New grade saved; old grade, new grade, user, and timestamp recorded in audit log. | Audit row logged. | **PASS** |
| **TC-FR-21** | FR-21: Assessment Window Toggle | Admin opens/closes grade window for Dept `CSE` Sem `4`. | State persists in `grade_windows` and immediately enforces permissions. | State toggled. | **PASS** |
| **TC-FR-22** | FR-22: Subject Catalog Scope | Query available subjects for given department and semester. | Returns mapped curriculum subjects (e.g. 16 subjects per dept across 8 semesters). | Catalog loaded. | **PASS** |

---

### 1.4 Attendance & Reporting (FR-16 – FR-19, FR-23 – FR-24)
| Test Case ID | Requirement | Test Steps | Expected Result | Actual Result | Status |
| :--- | :--- | :--- | :--- | :--- | :---: |
| **TC-FR-16** | FR-16: Record Attendance | Faculty marks attended classes (e.g. 38) out of total classes (e.g. 40). | Attended and total classes stored; percentage auto-calculated as 95.0%. | Recorded. | **PASS** |
| **TC-FR-17** | FR-17: Attendance Validation | Faculty submits attended classes greater than total classes (`45/40`). | Rejected with error "Attended classes cannot exceed total classes". | Rejected (400). | **PASS** |
| **TC-FR-18** | FR-18: Student View Attendance | Student views attendance dashboard. | Displays subject-wise attended, total, percentage, and shortage indicator (< 75%). | Rendered clearly. | **PASS** |
| **TC-FR-19** | FR-19: Multi-Semester Filter | Filter students across semesters 1 through 8. | Restricts display to selected semester; supports up to 8 semesters maximum. | Filtered properly. | **PASS** |
| **TC-FR-23** | FR-23: Export to CSV | Admin/Faculty triggers CSV Export for filtered students. | Generates download of `students_export.csv` with standard comma-separated fields. | CSV generated. | **PASS** |
| **TC-FR-24** | FR-24: Unauthorized Export Block | Student attempts to trigger CSV export via direct URL `/api/export`. | Blocked with HTTP 403 Forbidden. No data downloaded. | Blocked (403). | **PASS** |

---

## 2. Non-Functional Requirements Test Suite (NFR-01 to NFR-12)

| Test Case ID | NFR | Requirement | Verification Step | Expected Metric | Measured Result | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :---: |
| **TC-NFR-01** | **NFR-01** | User-Friendly Interface | Inspect C CLI prompt flows (`ui.c`) and Web dashboard layout (`web/index.html`). | Intuitive navigation, clear menus, input confirmations. | Clean numbered menus and responsive Web UI. | **PASS** |
| **TC-NFR-02A** | **NFR-02** | Query Performance | Search lookup across 1,000 student records (`test_performance.c`). | $< 1.00\text{ s}$ | **$< 0.0001\text{ s}$** | **PASS** |
| **TC-NFR-02B** | **NFR-02** | File Save Performance | Save 1,000 student records to disk storage (`test_performance.c`). | $< 1.00\text{ s}$ | **$0.0750\text{ s}$** | **PASS** |
| **TC-NFR-02C** | **NFR-02** | File Load Performance | Load 1,000 student records from disk storage (`test_performance.c`). | $< 1.00\text{ s}$ | **$0.1110\text{ s}$** | **PASS** |
| **TC-NFR-03A** | **NFR-03** | Data Integrity (Memory) | Submit duplicate Student ID to `has_duplicate_student_id()` (`test_integrity.c`). | Return 1 (Duplicate detected). | Duplicate detected; case-insensitive. | **PASS** |
| **TC-NFR-03B** | **NFR-03** | Data Integrity (Storage) | Attempt saving duplicate records via `student_storage_save()` (`test_storage_integrity.c`). | Return -2 (Storage rejected). | Storage rejected duplicate save. | **PASS** |
| **TC-NFR-03C** | **NFR-03** | Data Integrity (Database) | Insert duplicate `students.id` into MySQL database. | MySQL Error 1062 (Duplicate entry). | Database unique key enforced. | **PASS** |
| **TC-NFR-04A** | **NFR-04** | Stability (NULL Strings) | Pass NULL pointer to `auth_authenticate()` and `validate_student_fields()`. | Return error code without segfault. | Clean error code returned; no crash. | **PASS** |
| **TC-NFR-04B** | **NFR-04** | Stability (Empty Input) | Provide empty strings and special characters to CLI parser. | Graceful error prompt. | Handled gracefully without crash. | **PASS** |
| **TC-NFR-05** | **NFR-05** | Modular Design | Compile each C module independently (`auth.o`, `backup.o`, `integrity.o`, etc.). | Zero circular dependencies. | Modular compilation clean. | **PASS** |
| **TC-NFR-06A** | **NFR-06** | Backup Creation | Call `backup_create("students.csv", "backup.csv")` (`test_backup.c`). | Exact byte-for-byte replica file created. | Replica file verified. | **PASS** |
| **TC-NFR-06B** | **NFR-06** | Disaster Recovery | Corrupt primary file and call `backup_restore()` (`test_backup.c`). | Primary file restored from backup. | Primary file recovered intact. | **PASS** |
| **TC-NFR-07** | **NFR-07** | Data Reliability (Atomic) | Execute `student_storage_save_atomic()` with simulate write interruption. | Original file unmodified if save fails. | Original live file preserved. | **PASS** |
| **TC-NFR-08** | **NFR-08** | Prevent Unauthorized Mod | Non-admin role invokes mutate student endpoint (`test_nfr.c`, `test_backend.py`). | Operation blocked (HTTP 403 / return 0). | Unauthorized mutation blocked. | **PASS** |
| **TC-NFR-09** | **NFR-09** | Cross-Platform Portability | Build and test on Windows 11 (MinGW) and Linux (Ubuntu GCC via GitHub CI). | 0 errors across platforms. | Clean build on Windows and Linux. | **PASS** |
| **TC-NFR-10** | **NFR-10** | Consistent UI Messaging | Inspect format strings in `ui.c` and web alert components. | Uniform `[INFO]`, `[ERROR]`, `[SUCCESS]`. | All messages use standard prefixes. | **PASS** |
| **TC-NFR-11** | **NFR-11** | Scalability (1,000 Records) | Stress test memory buffer with 1,000 student records (`test_performance.c`). | Zero crashes, zero memory leaks. | Ran 1,000 records smoothly. | **PASS** |
| **TC-NFR-12A** | **NFR-12** | Role-Based Access (Student) | `student1` requests access to `student2` data via `can_access_student()`. | Return 0 (Forbidden). | Access denied. | **PASS** |
| **TC-NFR-12B** | **NFR-12** | Role-Based Access (Admin) | `admin` requests access to any student data via `can_access_student()`. | Return 1 (Permitted). | Access granted. | **PASS** |

---

## 3. Summary & Traceability

- **Functional Test Cases:** 24 test cases covering FR-01 through FR-25 (100% Passed)
- **Non-Functional Test Cases:** 16 test cases covering NFR-01 through NFR-12 (100% Passed)
- **Backend API Test Suite:** 16 automated integration tests (`tests/test_backend.py`) (100% Passed)
- **Unit Test C Suites:** 37 test functions across 7 C test harnesses (100% Passed)
- **Total Executed Tests:** **65 Tests | 0 Failures | 0 Regressions**
