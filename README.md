# Student Record System

A student-record system with a C console client and a Flask/MySQL web application for student details, grades, attendance, searching, and role-based access.

## Project Information

- **Project:** Student Record System
- **Team:** H-11
- **Technology:** C, Python, Flask, MySQL, HTML, CSS, JavaScript
- **Project Phase:** Phase 1 – Requirements and Design

## Problem Statement

A record-keeping system for student details such as grades and attendance.

The system is designed to maintain student information in an organized manner and allow authorized users to add, view, search, update, and delete student records.

## Proposed Solution

The Student Record System is a menu-driven C/C++ application that provides:

- Student record management
- Grade management
- Attendance management
- Student search and filtering
- Input validation
- Persistent storage
- Authentication and role-based access
- Error handling
- Backup and recovery support

## User Roles

The system supports three types of users:

### Administrator
- Add student records
- View student records
- Search student records
- Update student information
- Delete student records
- Filter student records
- Exit the system

### Faculty
- View student records
- Search student records
- Record grades
- Update grades
- View grades
- Record attendance
- Update attendance
- View attendance
- Filter student records
- Exit the system

### Student
- View own profile
- View own grades
- View own attendance
- Exit the system

## Main Data Maintained

The system maintains:

- Student ID
- Student Name
- Contact Details
- Course / Department
- Semester
- Subject / Marks / Grades
- Attendance

## Key Features

### Authentication
Users log in using their credentials and are provided access according to their role.

### Student Management
Administrators can add, view, update, search, and delete student records.

### Grade Management
Faculty members can record, update, and view student grades.

### Attendance Management
Faculty members can record, update, and view student attendance.

### Search and Filtering
Student records can be searched using Student ID or name and filtered using course, department, or semester.

### Input Validation
The system validates required fields, Student IDs, marks, attendance values, and other user inputs before saving data.

### Persistent Storage
Student records are stored using a persistent storage mechanism such as text, CSV, binary files, or a permitted local database.

### Role-Based Access
Access to student information and operations is restricted according to the authenticated user's role.

## Architecture

The system follows a **Layered Architecture** consisting of:

1. Presentation Layer
2. Application Layer
3. Validation Layer
4. Data Layer

The architecture is divided into modular components such as:

- Authentication
- Student Management
- Academic Records
- Attendance
- Search and Reports
- Validation
- Backup
- Data Model
- Data Storage

## Project Documentation

The repository contains the following documentation:

- **Software Requirements Specification (SRS)**
- **Software Architecture & Design Specification**
- **Software Test Plan**
- **Test Case Related Update for Test Plan**

The SRS contains the functional, non-functional and technical requirements, security requirements, traceability information and use-case-related documentation. :contentReference[oaicite:1]{index=1}

The Architecture & Design Specification documents the layered architecture, component diagram, requirements traceability, security architecture, sequence diagrams, API design and error handling. :contentReference[oaicite:2]{index=2}

The Test Plan covers functional, non-functional and security testing, including authentication, student management, grades, attendance, search/filtering, validation, persistent storage and access control. :contentReference[oaicite:3]{index=3}

## Testing

Testing is organized into:

- Unit Testing (UT)
- Functional Testing (FT)
- System Testing (ST)
- Acceptance Testing (AT)
- Review-based verification (REV)

The test plan maps test cases to the requirements defined in the SRS. :contentReference[oaicite:4]{index=4}

Security validation includes:

- Authentication validation
- Role-based access control validation

## Web Application Setup (Windows)

The browser UI is served by the Flask backend. Web sign-in and student records are stored in MySQL, not in browser storage or `users.csv`. The web app and C console use separate account stores.

### 1. Prepare the MySQL database

Install **MySQL Community Server 8.0.16 or newer** and MySQL Workbench.

- If you already completed the Sprint 1 migration, back up the database and execute [`backend/migrations/002_four_departments_eight_semesters.sql`](backend/migrations/002_four_departments_eight_semesters.sql) once in Workbench.
- For a database that has only the original schema, back it up and execute [`backend/migrations/001_sprint1.sql`](backend/migrations/001_sprint1.sql), then execute migration 002. Do not rerun the original schema as a substitute for these migrations; `CREATE TABLE IF NOT EXISTS` does not update existing tables.
- For an existing database, execute migrations 003, 004, 005, and 006 in that order after migrations 001 and 002. Migration 003 removes the three legacy departments; migration 004 adds semester assignments and department/semester-scoped subjects and grades; migration 005 makes faculty assignments subject-specific; migration 006 adds separate ISA1/ISA2/ESA marks, admin-managed grade windows, and grade-change history. Back up the database first and run each migration only once. Migration 005 clears the old semester-only faculty permissions, so administrators must assign subjects again afterward. Apply 006 only after 005 has completed successfully; if migration 005 previously failed, verify the table structure before proceeding rather than rerunning it blindly. Migration 006 preserves existing marks as ISA1 entries; no grade window is open until an administrator configures dates.
- For a brand-new database, execute the current [`backend/schema.sql`](backend/schema.sql).

The four departments are Business Administration, Computer Science, Electronics, and Information Technology. Semesters are 1–8. Migration 002 keeps existing student records: students in other departments are reassigned to Computer Science, and students with semester values above 8 are set to semester 8.

If Mechanical Engineering, Civil Engineering, or Electrical Engineering still appear in an existing database, back it up and run [`backend/migrations/003_remove_legacy_departments.sql`](backend/migrations/003_remove_legacy_departments.sql) once. Students assigned to those departments are retained and moved to Computer Science before the department entries are removed.

Before migrating an existing database, check for grade records that cannot meet the new integer-mark and unique student/subject constraints, and for student accounts that need linking:

```sql
USE student_records;
SELECT student_id, subject, COUNT(*) AS grade_count
FROM grades
GROUP BY student_id, subject
HAVING COUNT(*) > 1;

SELECT grade_id, student_id, subject, marks
FROM grades
WHERE marks <> FLOOR(marks) OR marks < 0 OR marks > 100;

SELECT username, role, student_id
FROM users
WHERE (LOWER(role) = 'student' AND student_id IS NULL)
   OR (LOWER(role) <> 'student' AND student_id IS NOT NULL);
```

Resolve any returned rows before migration; the migration intentionally does not discard or silently alter existing grades or account links. It adds contact details and semester, normalizes known course-name capitalization, creates the fixed course/subject catalogs, and converts roles to `admin`, `faculty`, and `student`.

Create a dedicated database account in Workbench (do not use the MySQL root account for the app):

```sql
CREATE USER 'student_app'@'127.0.0.1' IDENTIFIED BY 'replace-with-a-strong-password';
GRANT SELECT, INSERT, UPDATE, DELETE ON student_records.* TO 'student_app'@'127.0.0.1';
```

### 2. Install and configure the backend

From the repository folder in PowerShell:

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
$env:MYSQL_HOST = "127.0.0.1"
$env:MYSQL_PORT = "3306"
$env:MYSQL_DATABASE = "student_records"
$env:MYSQL_USER = "student_app"
$env:MYSQL_PASSWORD = "the-password-you-set-in-MySQL"
$env:SESSION_SECRET = (python -c 'import secrets; print(secrets.token_hex(32))')
```

Keep the MySQL password and `SESSION_SECRET` private; do not put them in source files or commit them. These PowerShell settings last for the current terminal window. For remote MySQL, configure the provider's TLS CA file with `MYSQL_SSL_CA`; certificate verification is enabled when this setting is present.

### 3. Seed demo data, then run the app

```powershell
.\.venv\Scripts\python.exe -m backend.seed_demo
.\.venv\Scripts\python.exe -m backend.app
```

Seeding adds at least 20 sample students for each of the 4 departments in each of the 8 semesters (640 generated records total), plus the small named demo records. It is idempotent and does not reset existing accounts. For an existing database, run the seed after migrations 004 and 005 so the course/semester subject catalog is populated; on a fresh install, run it after `schema.sql`. Run the seed again after updating the app to create the eight additional demo faculty accounts. Then start the app as often as needed. Open **http://127.0.0.1:5000/**. Demo accounts are `admin` / `admin123`, `faculty1` / `faculty123`, `student1` / `student123`, and `student2` / `student234`. Additional faculty demo credentials are `faculty2`–`faculty9`, with passwords `Faculty2Demo!2026` through `Faculty9Demo!2026`, respectively. Change or remove these demo accounts before using non-demo data.

Faculty accounts have no access until an administrator assigns them specific department/semester/subject combinations in **Faculty Access**. Create additional faculty accounts from the **Create Faculty** button on that page by entering their username and password (at least 12 characters); passwords are stored only as hashes. New faculty have no class access until an administrator assigns their subjects. A faculty member can teach multiple subjects and semesters, and separate faculty members can be assigned different subjects for the same class. Faculty see class cards for their assignments, with student records and grading limited to each selected class and subject. Subject assignments open read-only; select **Edit assignments** to change them, then save at the bottom of the assignment panel.

Administrators configure ISA1, ISA2, and ESA date ranges under **Grade Windows**, selecting a department and semester. Window dates are inclusive, and leaving both dates blank closes that assessment. Faculty can record or edit marks only while that assessment's window is open. Editing an existing mark requires a reason; the old/new marks, reason, faculty account, and timestamp are retained in grade-change history. Students see separate marks for each subject and assessment.

When administrators add a student from the web app, they must set that student's sign-in username and a password of at least 12 characters. The password is stored only as a hash. In **Update Student**, administrators can change the username or set a new password; leaving the password blank keeps the current password.

Create additional users with `.\.venv\Scripts\python.exe -m backend.create_user`; it prompts for the role and password, hashes the password, and requires a student ID for student accounts. Create the student record first. To expose the server beyond this computer, use HTTPS and a production WSGI server/reverse proxy, set `APP_ENV=production`, and use a strong `SESSION_SECRET`; Flask's built-in development server is for local development only.

Deleting a student permanently cascades to that student's linked web login, grades, and attendance records.

### API overview

- `POST /api/login`, `POST /api/logout`, `GET /api/session`: signed server sessions and CSRF protection.
- `GET /api/students`, `GET /api/students/search?q=...`, `GET /api/students/<id>`: role-scoped student listing, search, and detail.
- `POST /api/students`, `PUT/DELETE /api/students/<id>`: administrator-only student management.
- `GET /api/students/export`: administrator/faculty-only filtered CSV export.
- `GET /api/courses`, `GET /api/subjects?course=...&semester=...`: authenticated, role-scoped course and subject catalogs.
- `POST /api/faculty`, `GET /api/faculty-assignments`, `PUT /api/faculty/<user_id>/assignments`: administrator-only faculty creation and subject-level access management.
- `GET /api/faculty/classes`: faculty member's assigned department/semester/subject cards.
- `GET/POST /api/students/<id>/grades`, `PUT /api/grades/<id>`: students read their own ISA1/ISA2/ESA marks; faculty record and update marks during open windows, with a reason required for changes.
- `GET/PUT /api/grade-windows`: administrators manage inclusive assessment windows per department and semester; assigned faculty can view their class windows.
- `GET /api/health`: checks the MySQL connection.

The Sprint 1 manual test checklist is in [`docs/SPRINT1_TEST_CHECKLIST.md`](docs/SPRINT1_TEST_CHECKLIST.md). It includes steps, expected outcomes, and blank actual-result/pass-fail cells.

## Security

The system implements security through:

- User authentication
- Role-based access control
- Input validation
- Restriction of unauthorized operations
- Data protection
- Backup and recovery support

## Requirements

The system requirements include:

- **25 Functional Requirements**
- **12 Non-Functional Requirements**
- **4 Technical Requirements**
- **2 Security Objectives**
- **2 Security Requirements**

## Project Structure

```text
Student-Record-System/
│
├── backend/
│   ├── app.py
│   ├── migrations/
│   │   ├── 001_sprint1.sql
│   │   ├── 002_four_departments_eight_semesters.sql
│   │   ├── 003_remove_legacy_departments.sql
│   │   ├── 004_faculty_assignments_and_subjects.sql
│   │   ├── 005_faculty_subject_assignments.sql
│   │   └── 006_assessment_windows_and_grade_history.sql
│   ├── schema.sql
│   ├── seed_demo.py
│   └── create_user.py
├── web/
│   ├── index.html
│   ├── app.js
│   └── styles.css
├── src/
│   ├── main.cpp
│   ├── auth.cpp
│   ├── student.cpp
│   ├── grades.cpp
│   ├── attendance.cpp
│   ├── search.cpp
│   ├── report.cpp
│   ├── validation.cpp
│   ├── storage.cpp
│   └── backup.cpp
│
├── include/
│   └── student.h
│
├── data/
│   └── students.dat / students.csv
│
├── docs/
│   ├── SRS.pdf
│   ├── Software Architecture & Design Specification.pdf
│   ├── Test Plan.pdf
│   └── Test Case Related Update for Test Plan.pdf
│
└── README.md
