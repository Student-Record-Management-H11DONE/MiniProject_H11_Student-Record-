# System Validation Report

**Project:** Student Record Management System  
**Team:** H-11  
**Deliverable:** Part-3 Quality Assurance & System Verification  
**Evaluation Date:** November 6, 2026  
**System Status:** Complete & Fully Validated  

---

## 1. Executive Summary

This System Validation Report provides formal verification of the **Student Record Management System** across all end-to-end user workflows, functional requirements (FR-01 through FR-25), and non-functional requirements (NFR-01 through NFR-12). 

Testing was performed on the integrated dual-track architecture:
1. **Core C Systems Engine:** Responsible for low-level memory management, atomic file persistence, data integrity validation, and backup/restore benchmarks.
2. **Web Backend & Dashboard (Flask + MySQL):** Responsible for role-based access control (Admin, Faculty, Student), multi-semester course enrollments, assessment window enforcement, grade history tracking, and student attendance calculations.

**Overall Validation Outcome:** **100% Passed (0 Failures, 0 Regressions)**.

---

## 2. Test Environment & Configuration

| Parameter | Specification |
| :--- | :--- |
| **Operating System** | Windows 11 (x86_64) |
| **C Compiler** | MinGW GCC (ISO C99 compliant) |
| **Python Environment** | Python 3.13.2 |
| **Database Server** | MySQL Community Server 8.0 / 8.4 (Port 3306) |
| **Web Framework** | Flask 3.0+ / Werkzeug |
| **Dataset Size** | 646 Active Students, 128 Subjects across 4 Departments & 8 Semesters |
| **Execution Host** | `localhost:5000` (API & Web UI) |

---

## 3. End-to-End Functional Role Validation

### 3.1 Administrator Workflows
| ID | Workflow / Test Description | Expected Behavior | Actual Behavior | Result |
| :--- | :--- | :--- | :--- | :---: |
| **VAL-ADM-01** | Administrator Login (`admin` / `admin123`) | Authenticate, issue secure session cookie, redirect to Admin dashboard. | Successfully authenticated; session cookie set; role verified as `admin`. | **PASS** |
| **VAL-ADM-02** | Create New Student Record | Validate required fields, insert into `students`, generate student user credentials. | Student inserted into MySQL; default password hashed and stored. | **PASS** |
| **VAL-ADM-03** | Prevent Duplicate Student ID | Reject insertion if Student ID already exists in storage/database. | System returned 409 Conflict with descriptive message `Student ID already exists`. | **PASS** |
| **VAL-ADM-04** | Assessment Window Control | Open or close grade entry window for specific department and semester. | Grade window updated in `grade_windows` table; status immediately reflected for faculty. | **PASS** |
| **VAL-ADM-05** | Assign Faculty to Multi-Semester Subjects | Allocate faculty members to distinct subjects and semester sections. | Recorded in `faculty_assignments`; scopes faculty visibility accordingly. | **PASS** |
| **VAL-ADM-06** | Export System Records to CSV | Generate standard CSV dump for authorized administration backup. | Returns formatted `text/csv` attachment; header properly parsed. | **PASS** |

---

### 3.2 Faculty Workflows
| ID | Workflow / Test Description | Expected Behavior | Actual Behavior | Result |
| :--- | :--- | :--- | :--- | :---: |
| **VAL-FAC-01** | Faculty Login (`faculty1` / `faculty123`) | Authenticate and restrict dashboard to assigned departments and semesters. | Successfully logged in; only assigned department/courses visible. | **PASS** |
| **VAL-FAC-02** | Enter / Update Student Grades | Allow grade modification only when assessment window is OPEN. | Grade saved to `grades` table; previous entry recorded in `grade_change_history`. | **PASS** |
| **VAL-FAC-03** | Assessment Window Enforcement | Reject grade submission when assessment window is CLOSED. | System blocked submission with HTTP 403 / 400 error: window closed. | **PASS** |
| **VAL-FAC-04** | Attendance Recording & Percentage | Input attended vs total classes; auto-calculate attendance percentage. | Attendance recorded in `attendance` table; percentage accurately computed. | **PASS** |
| **VAL-FAC-05** | Search & Filter Students | Filter student list by Department, Semester, and Name prefix. | Instant sub-second filtered response matching selected criteria. | **PASS** |

---

### 3.3 Student Workflows & Data Isolation
| ID | Workflow / Test Description | Expected Behavior | Actual Behavior | Result |
| :--- | :--- | :--- | :--- | :---: |
| **VAL-STU-01** | Student Login (`student1` / `student123`) | Authenticate and load individual student academic record. | Logged in successfully; loaded personal student dashboard. | **PASS** |
| **VAL-STU-02** | View Personal Grades & Attendance | Display enrolled subjects, grades, and attendance metrics. | Data rendered correctly matching student's enrolled courses. | **PASS** |
| **VAL-STU-03** | Cross-Student Access Protection | Student attempts to view another student's record (`/api/students/2`). | Access blocked; returned HTTP 403 Forbidden. | **PASS** |
| **VAL-STU-04** | Prevent Unauthorized Grade Modification | Student attempts POST/PUT request to mutate grade or attendance records. | Rejected with HTTP 403 Forbidden; audit record maintained. | **PASS** |
| **VAL-STU-05** | Unauthorized CSV Export Block | Student attempts to access administrative export endpoint (`/api/export`). | Rejected with HTTP 403 Forbidden; no data leaked. | **PASS** |

---

## 4. Non-Functional System Validation (NFRs)

| NFR | Specification | Validation Method & Evidence | Status |
| :--- | :--- | :--- | :---: |
| **NFR-01** | **User-Friendly Interface** | Verified structured menu navigation in C CLI (`ui.c`) and intuitive responsive Web UI (`web/index.html`). | **VERIFIED** |
| **NFR-02** | **System Performance** | Benchmarked in `test_performance.c`: 1,000 saves in **0.075s**, 1,000 loads in **0.111s**, search lookups in **<0.0001s** (Target: < 1.0s). | **PASSED** |
| **NFR-03** | **Data Integrity** | Rejects duplicate IDs both in memory (`test_integrity.c`) and disk storage (`test_storage_integrity.c`). Verified database unique constraint on `students.id`. | **PASSED** |
| **NFR-04** | **Error Handling & Stability** | Gracefully handles empty inputs, NULL strings, invalid menu choices, and corrupt files without segmentation faults (`test_nfr.c`). | **PASSED** |
| **NFR-05** | **Modular Design** | System decomposed into decoupled modules (`auth`, `ui`, `backup`, `integrity`, `validation`, `storage`) with explicit header boundaries. | **VERIFIED** |
| **NFR-06** | **Backup and Recovery** | `backup_create()` creates snapshot replicas; `backup_restore()` successfully recovers corrupted datasets (`test_backup.c`). | **PASSED** |
| **NFR-07** | **Data Reliability (Crash-Safe)** | `student_storage_save_atomic()` writes to `.tmp` first before replacing live file; original data remains intact if write is interrupted. | **PASSED** |
| **NFR-08** | **Prevent Unauthorized Mutation** | Non-admin/non-faculty roles cannot write grades or modify records. Tested in both C (`test_nfr.c`) and Python API (`test_backend.py`). | **PASSED** |
| **NFR-09** | **Cross-Platform Portability** | ISO C99 compliance tested across Windows 11 and Ubuntu via automated GitHub Actions CI pipelines. | **PASSED** |
| **NFR-10** | **Consistent UI Messages** | Enforced uniform message prefixes (`[INFO]`, `[ERROR]`, `[SUCCESS]`) and spacing in `ui.c` and web alert banners. | **VERIFIED** |
| **NFR-11** | **Scalability & Stress Test** | Scaled to 1,000 records in C engine without memory leak or crash; web database seeded with 646 students across 8 semesters. | **PASSED** |
| **NFR-12** | **Role-Based Access Control** | Strict horizontal and vertical authorization checks (`can_access_student()`, Flask session authorization). | **PASSED** |

---

## 5. Security & System Hardening Validation

1. **Password Security:** All stored user passwords in MySQL are hashed using cryptographically secure algorithms (`werkzeug.security.generate_password_hash`).
2. **CSRF & Injection Protection:** All state-changing web requests require CSRF token validation; SQL operations utilize parameterized prepared statements, preventing SQL injection.
3. **HTTP Security Headers:** Verified presence of protective response headers:
   - `X-Content-Type-Options: nosniff`
   - `X-Frame-Options: SAMEORIGIN`
   - `Content-Security-Policy: default-src 'self' ...`

---

## 6. Validation Sign-Off

The Student Record Management System has successfully satisfied all functional and non-functional validation criteria stipulated in the Software Requirements Specification (SRS) and System Architecture Document (SAD).

- **Lead QA / Validation Engineer:** Shanteri (Team H-11)  
- **Result:** **APPROVED FOR FINAL SUBMISSION**
