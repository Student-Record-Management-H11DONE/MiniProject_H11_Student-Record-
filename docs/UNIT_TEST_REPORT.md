# Unit Test Report

**Project:** Student Record System (Team H-11)  
**Deliverable:** Part-3 Testing & Quality Assurance  
**Execution Date:** November 6, 2026  
**Status:** All Unit Tests PASSED (100% Pass Rate)

---

## 1. Executive Summary

This report documents the execution of all unit test suites across the Student Record Management System, encompassing both the C systems modules and the Python web backend API.

| Test Suite | Module Under Test | Total Tests | Passed | Failed | Execution Time |
| :--- | :--- | :---: | :---: | :---: | :---: |
| `tests/test_auth.c` | Authentication & Password Hashing | 6 | 6 | 0 | 0.002s |
| `tests/test_validation.c` | Field Format & Mandatory Validation | 5 | 5 | 0 | 0.001s |
| `tests/test_integrity.c` | Duplicate Student ID Prevention (NFR-03) | 4 | 4 | 0 | 0.001s |
| `tests/test_student_storage.c` | Storage Read/Write Operations | 4 | 4 | 0 | 0.003s |
| `tests/test_storage_integrity.c` | Storage Duplicate ID Rejection | 3 | 3 | 0 | 0.002s |
| `tests/test_backup.c` | Backup, Disaster Recovery & Atomic Save (NFR-06, 07) | 6 | 6 | 0 | 0.005s |
| `tests/test_nfr.c` | Stability, RBAC & Role Isolation (NFR-04, 08, 12) | 5 | 5 | 0 | 0.002s |
| `tests/test_performance.c` | Performance (<1s) & 1,000 Records Scalability (NFR-02, 11) | 4 | 4 | 0 | 0.263s |
| `tests/test_backend.py` | Full Web API & Database Controller Test Suite | 16 | 16 | 0 | 10.325s |
| **TOTAL** | **Entire Codebase** | **53** | **53** | **0** | **10.604s** |

---

## 2. Detailed Unit Test Results

### 2.1 C Core Systems & NFR Modules

#### A. Authentication (`tests/test_auth.c`)
- **TC-AUTH-01:** Valid Administrator credentials (`admin`) $\rightarrow$ **PASS**
- **TC-AUTH-02:** Valid Faculty credentials (`faculty1`) $\rightarrow$ **PASS**
- **TC-AUTH-03:** Valid Student credentials (`student1`) $\rightarrow$ **PASS**
- **TC-AUTH-04:** Invalid password rejection $\rightarrow$ **PASS**
- **TC-AUTH-05:** Unknown username rejection $\rightarrow$ **PASS**
- **TC-AUTH-06:** Empty/NULL input handling $\rightarrow$ **PASS**

#### B. Data Integrity & Validation (`tests/test_integrity.c`, `test_validation.c`)
- **TC-INT-01:** Detect duplicate student ID in memory array $\rightarrow$ **PASS**
- **TC-INT-02:** Case-insensitive duplicate detection (`S001` vs `s001`) $\rightarrow$ **PASS**
- **TC-INT-03:** Storage refuses saving duplicate IDs $\rightarrow$ **PASS**
- **TC-INT-04:** Storage refuses loading corrupt file with duplicates $\rightarrow$ **PASS**
- **TC-VAL-01:** Rejection of missing required fields (ID, Name, Course, Semester) $\rightarrow$ **PASS**

#### C. Backup, Recovery & Crash-Safe Saves (`tests/test_backup.c`)
- **TC-BAK-01:** Atomic safe save creates valid file (`student_storage_save_atomic`) $\rightarrow$ **PASS**
- **TC-BAK-02:** Overwrite existing file without corruption $\rightarrow$ **PASS**
- **TC-BAK-03:** `backup_create()` creates byte-for-byte replica file $\rightarrow$ **PASS**
- **TC-BAK-04:** Detection of primary file corruption $\rightarrow$ **PASS**
- **TC-BAK-05:** `backup_restore()` fully recovers records from backup $\rightarrow$ **PASS**
- **TC-BAK-06:** Rejection of invalid records protects original file from partial overwrite $\rightarrow$ **PASS**

#### D. Performance & Scalability Benchmarks (`tests/test_performance.c`)
- **TC-PERF-01:** Save 1,000 unique records in < 1.0s (Actual: **0.0940s**) $\rightarrow$ **PASS**
- **TC-PERF-02:** Load 1,000 unique records in < 1.0s (Actual: **0.0930s**) $\rightarrow$ **PASS**
- **TC-PERF-03:** Duplicate integrity check across 1,000 records (Actual: **0.0760s**) $\rightarrow$ **PASS**
- **TC-PERF-04:** 100 search queries across 1,000 records (Actual: **< 0.0001s**) $\rightarrow$ **PASS**

---

### 2.2 Python Web Backend API (`tests/test_backend.py`)

- `test_login_roles_empty_and_invalid_credentials` $\rightarrow$ **PASS**
- `test_login_normalizes_legacy_role_labels` $\rightarrow$ **PASS**
- `test_admin_can_create_faculty_credentials` $\rightarrow$ **PASS**
- `test_admin_controls_department_semester_grade_windows` $\rightarrow$ **PASS**
- `test_admin_can_create_update_search_filter_detail_and_delete` $\rightarrow$ **PASS**
- `test_eight_semester_limit_and_four_department_sample_distribution` $\rightarrow$ **PASS**
- `test_student_cannot_access_another_profile_or_export` $\rightarrow$ **PASS**
- `test_role_and_csrf_protect_student_mutations` $\rightarrow$ **PASS**
- `test_faculty_records_updates_and_students_read_own_grades` $\rightarrow$ **PASS**
- `test_faculty_assignment_scopes_students_grades_and_subjects` $\rightarrow$ **PASS**
- `test_admin_can_assign_faculty_multiple_semesters` $\rightarrow$ **PASS**
- `test_admin_and_student_cannot_write_grades` $\rightarrow$ **PASS**
- `test_export_is_csv_for_authorized_roles` $\rightarrow$ **PASS**
- `test_role_scoped_course_and_subject_catalogs` $\rightarrow$ **PASS**
- `test_api_security_headers_and_health` $\rightarrow$ **PASS**
- `test_production_requires_a_strong_session_secret` $\rightarrow$ **PASS**

---

## 3. Conclusion
All 53 unit tests executed with **0 errors, 0 failures, and 100% pass rate**. The system meets the unit testing requirements of Deliverables Part-3.
