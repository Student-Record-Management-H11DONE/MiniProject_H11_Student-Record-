# Student Record System – Updated SRS

## 1. Introduction

The Student Record System is a student information management platform that supports student record maintenance, grades, attendance, search/filter operations, and role-based access. The system is implemented in two parallel tracks:

- C-based console application for local/offline record management
- Flask + MySQL web application for browser-based access

The system is designed for administrators, faculty members, and students.

## 2. Purpose

The purpose of the system is to maintain and manage academic records efficiently while ensuring secure access and accurate reporting. The system must support:

- Student profile management
- Grade recording and modification tracking
- Attendance recording
- Search and filter functionality
- Role-based access control
- Data validation and persistence
- Accountability for faculty changes

## 3. Scope

### In Scope

- Login and authentication
- Student creation, update, and deletion
- Course and semester-based student organization
- Grade entry and grade-change history
- Attendance tracking
- Faculty assignment to departments/semesters/subjects
- Search and export functionality
- Security and validation checks

### Out of Scope

- Payment processing
- Real-time messaging
- External academic ERP integration
- Multi-tenant SaaS deployment

## 4. Users and Roles

### 4.1 Administrator

The administrator manages system users and data. Responsibilities include:

- Create and manage student records
- Create faculty accounts
- Assign faculty to department/semester/subject combinations
- Configure grade windows by department and semester
- Search, filter, and export records
- Ensure data integrity and role enforcement

### 4.2 Faculty

Faculty can perform class-specific operations:

- View assigned students
- Record and update grades for authorized subjects and assessment components
- Record and update student attendance
- View grade and attendance records for assigned classes
- Search/filter student records based on assigned course and semester

### 4.3 Student

Students can:

- Log in to their own account
- View their own profile
- View own grades
- View own attendance
- Search only within their own record when applicable

## 5. Functional Requirements

### FR-01: Authentication
The system shall authenticate users using username and password.

### FR-02: Role-based access
The system shall restrict operations based on user role: admin, faculty, or student.

### FR-03: Student management
The system shall allow administrators to add, update, view, and delete student records.

### FR-04: Search and filter
The system shall allow authorized users to search by student ID/name and filter by course and semester.

### FR-05: Grade management
Faculty shall be able to enter and update grades for assigned course/semester/subject combinations using assessment components: ISA1, ISA2, and ESA.

### FR-06: Grade change tracking
Any grade modification shall store the previous value, new value, reason, and user responsible.

### FR-07: Attendance management
Faculty shall be able to add and revise attendance records for assigned subjects.

### FR-08: Faculty assignments
Administrators shall configure the set of department/semester/subject combinations assigned to each faculty member.

### FR-09: Grade windows
Administrators shall define valid date windows per course, semester, and assessment component.

### FR-10: Data export
Authorized users shall be able to export filtered student records to CSV.

### FR-11: Validation
The system shall validate inputs such as semester, marks, dates, role, student IDs, and password rules.

### FR-12: Persistence
The system shall store data persistently using MySQL in the web app and file-based storage in the C console implementation.

## 6. Non-Functional Requirements

### NFR-01: Security
The system shall enforce authentication and authorization.

### NFR-02: Data integrity
The system shall prevent invalid data and duplicate conflict values.

### NFR-03: Usability
The system shall provide clear menus and understandable prompts.

### NFR-04: Reliability
The system shall handle invalid input without crashing.

### NFR-05: Maintainability
The application shall be modularized into separate components and files.

### NFR-06: Performance
The system shall support normal academic record operations without significant delays.

### NFR-07: Portability
The system shall work on standard desktop environments and browser-based setups.

## 7. Technical Requirements

- Programming languages: C and Python
- Web framework: Flask
- Database: MySQL
- Frontend: HTML, CSS, JavaScript
- File storage: CSV/text files for local C implementation
- Validation and access enforcement: server-side logic in the web app and modular validation routines in the C app

## 8. System Architecture

The project follows a layered architecture with two execution paths:

### 8.1 Web Application Architecture

- Presentation Layer: web UI pages and browser interactions
- Application Layer: Flask API routes and business logic
- Data Access Layer: MySQL queries and database models
- Security Layer: login, session management, role checks, CSRF validation

### 8.2 Console Application Architecture

- User Interface Layer: menus and input/output handling
- Business Logic Layer: student handling, grades, attendance, search, validation
- Data/Storage Layer: file-based persistence and backup support
- Integrity Layer: validation and consistency checks

## 9. Data Model

The system stores the following primary entities:

- Student
- User account
- Course
- Semester subject mapping
- Faculty assignment
- Grade
- Grade history
- Attendance
- Grade window

## 10. Use Case Summary

### Admin Use Cases
- Log in
- Add student
- Update student
- Delete student
- Create faculty
- Assign faculty access
- Set grade windows
- Search/filter/export records

### Faculty Use Cases
- Log in
- View assigned class records
- Add/update grades
- Add/update attendance
- Search/filter students within access scope

### Student Use Cases
- Log in
- View personal profile
- View personal grades
- View personal attendance

## 11. Security Requirements

- Passwords must be hashed and stored securely.
- Only authenticated users may access restricted resources.
- Roles determine allowed actions.
- Faculty may access only assigned departments/semesters/subjects.
- Students may access only their own records.
- Inputs must be validated before storage or processing.

## 12. Assumptions and Constraints

- The project is academic in nature and does not require enterprise-scale deployment.
- The application is expected to be used in a controlled institutional environment.
- Student and faculty credentials are managed internally.
- The web app and console app are separate implementations and may not share the same data store by default.

## 13. Traceability

| Requirement ID | Covered By | Implementation Status |
|---|---|---|
| FR-01 | Login APIs / C auth module | Implemented |
| FR-02 | Role checks and access control | Implemented |
| FR-03 | Student CRUD APIs / C student storage | Implemented |
| FR-04 | Search/filter endpoints / C search logic | Implemented |
| FR-05 | Grade APIs / grades module | Implemented |
| FR-06 | Grade change history | Implemented |
| FR-07 | Attendance APIs / attendance module | Implemented |
| FR-08 | Faculty assignment APIs | Implemented |
| FR-09 | Grade windows API | Implemented |
| FR-10 | Export CSV endpoint | Implemented |
| FR-11 | Validation functions | Implemented |
| FR-12 | MySQL schema and persistent storage | Implemented |

## 14. Acceptance Criteria

The project is considered complete when:

- users can log in with correct roles
- students can access only own data
- faculty can manage only assigned classes
- admins can manage system data
- grades and attendance can be recorded and updated
- records can be searched and exported
- invalid operations are blocked by validation and authorization checks

## 15. Summary

The Student Record System provides a secure and structured way to manage academic records for administrators, faculty, and students. The current implementation combines a console application with a browser-based Flask/MySQL system, with role-based access, subject-level faculty assignment, grade windows, and grade/attendance tracking.
