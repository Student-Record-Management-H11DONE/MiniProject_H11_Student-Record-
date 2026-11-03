import os
import unittest
from collections import Counter
from datetime import date, timedelta
from unittest.mock import patch

import mysql.connector
from werkzeug.security import check_password_hash, generate_password_hash

from backend.app import DEPARTMENTS, create_app
from backend.seed_demo import COURSES, generate_sample_students


class FakeDatabase:
    def __init__(self):
        self.students = [
            {
                "id": "S001",
                "name": "Aarav Mehta",
                "contactDetails": "aarav@example.edu",
                "course": "Computer Science",
                "semester": 2,
                "createdAt": "2026-10-28",
            },
            {
                "id": "S002",
                "name": "Diya Sharma",
                "contactDetails": "diya@example.edu",
                "course": "Information Technology",
                "semester": 3,
                "createdAt": "2026-10-25",
            },
        ]
        self.courses = [
            "Computer Science",
            "Information Technology",
            "Electronics",
        ]
        self.subjects = ["Mathematics", "Physics", "Data Structures"]
        self.course_subjects = [
            ("Computer Science", 2, "Mathematics"),
            ("Computer Science", 2, "Physics"),
            ("Information Technology", 3, "Data Structures"),
            ("Business Administration", 5, "Management"),
        ]
        self.faculty_assignments = [(2, "Computer Science", 2, "Mathematics")]
        self.grades = []
        self.grade_windows = [
            {
                "course": "Computer Science",
                "semester": 2,
                "component": component,
                "startDate": "2026-01-01",
                "endDate": "2027-12-31",
                "isOpen": True,
            }
            for component in ("ISA1", "ISA2", "ESA")
        ]
        self.grade_change_history = []
        self.users = {
            "admin": self.user(1, "admin", "admin123", "admin"),
            "faculty1": self.user(2, "faculty1", "faculty123", "faculty"),
            "student1": self.user(3, "student1", "student123", "student", "S001"),
            "student2": self.user(4, "student2", "student234", "student", "S002"),
        }
        self.next_grade_id = 1

    @staticmethod
    def user(user_id, username, password, role, student_id=None):
        return {
            "user_id": user_id,
            "username": username,
            "password_hash": generate_password_hash(password),
            "role": role,
            "student_id": student_id,
        }

    def connect(self):
        return FakeConnection(self)


class FakeConnection:
    def __init__(self, database):
        self.database = database

    def cursor(self, dictionary=False):
        return FakeCursor(self.database)

    def commit(self):
        pass

    def rollback(self):
        pass

    def close(self):
        pass


class FakeCursor:
    def __init__(self, database):
        self.database = database
        self.rows = []
        self.rowcount = 0
        self.lastrowid = None

    def execute(self, query, values=()):
        self.rows = []
        self.rowcount = 0
        if "FROM users WHERE username = %s AND (student_id IS NULL OR student_id <> %s)" in query:
            username, student_id = values
            account = self.database.users.get(username)
            self.rows = [(1,)] if account and account["student_id"] != student_id else []
        elif "FROM users WHERE username = %s" in query:
            account = self.database.users.get(values[0])
            self.rows = [account] if account else []
        elif "FROM users WHERE student_id = %s" in query:
            account = next(
                (account for account in self.database.users.values()
                 if account["student_id"] == values[0] and account["role"] == "student"),
                None,
            )
            self.rows = [{"username": account["username"]}] if account else []
        elif query.startswith("SELECT 1 FROM faculty_assignments"):
            if "AND subject = %s" in query:
                self.rows = [(1,)] if tuple(values) in self.database.faculty_assignments else []
            else:
                user_id, course, semester = values
                self.rows = [(1,)] if any(
                    assignment[:3] == (user_id, course, semester)
                    for assignment in self.database.faculty_assignments
                ) else []
        elif "SELECT DISTINCT course FROM faculty_assignments" in query:
            courses = sorted({
                course for user_id, course, _, _ in self.database.faculty_assignments
                if user_id == values[0]
            })
            self.rows = [(course,) for course in courses]
        elif query.startswith("SELECT DISTINCT semester FROM faculty_assignments"):
            user_id = values[0]
            course = values[1] if "course = %s" in query else None
            semesters = sorted({
                semester for assigned_user_id, assigned_course, semester, _
                in self.database.faculty_assignments
                if assigned_user_id == user_id and (course is None or assigned_course == course)
            })
            self.rows = [{"semester": semester} for semester in semesters]
        elif "SELECT user_id AS id, username FROM users" in query:
            self.rows = [
                {"id": account["user_id"], "username": account["username"]}
                for account in self.database.users.values()
                if account["role"].casefold() == "faculty"
            ]
        elif "SELECT user_id AS userId, course, semester, subject FROM faculty_assignments" in query:
            self.rows = [
                {"userId": user_id, "course": course, "semester": semester, "subject": subject}
                for user_id, course, semester, subject in self.database.faculty_assignments
            ]
        elif query.startswith("SELECT course, semester, subject FROM faculty_assignments"):
            self.rows = [
                {"course": course, "semester": semester, "subject": subject}
                for user_id, course, semester, subject in self.database.faculty_assignments
                if user_id == values[0]
            ]
        elif "SELECT course, semester, subject FROM course_semester_subjects" in query:
            self.rows = [
                {"course": course, "semester": semester, "subject": subject}
                for course, semester, subject in self.database.course_subjects
            ]
        elif "SELECT 1 FROM users WHERE user_id = %s AND role = 'faculty'" in query:
            self.rows = [(1,)] if any(
                account["user_id"] == values[0] and account["role"].casefold() == "faculty"
                for account in self.database.users.values()
            ) else []
        elif query.startswith("DELETE FROM faculty_assignments"):
            self.database.faculty_assignments = [
                assignment for assignment in self.database.faculty_assignments
                if assignment[0] != values[0]
            ]
        elif "SELECT css.subject FROM course_semester_subjects AS css" in query:
            user_id, course, semester = values
            self.rows = [
                (subject,) for assigned_user, assigned_course, assigned_semester, subject
                in self.database.faculty_assignments
                if (assigned_user, assigned_course, assigned_semester)
                == (user_id, course, semester)
            ]
        elif query == "SELECT name FROM courses ORDER BY name":
            self.rows = [(course,) for course in sorted(self.database.courses)]
        elif query == "SELECT name FROM subjects ORDER BY name":
            self.rows = [(subject,) for subject in sorted(self.database.subjects)]
        elif "COUNT(*) AS totalStudents" in query:
            students = list(self.database.students)
            if "FROM faculty_assignments fa" in query:
                user_id = values[0]
                assigned = {(user_id, course, semester) for user_id, course, semester, _
                            in self.database.faculty_assignments}
                students = [student for student in students if (
                    user_id, student["course"], student["semester"]
                ) in assigned]
            if "student_id = %s" in query:
                students = [student for student in students if student["id"] == values[0]]
            self.rows = [{
                "totalStudents": len(students),
                "totalCourses": len({student["course"] for student in students}),
                "recentlyAdded": 0,
            }]
        elif "SELECT DISTINCT course FROM students" in query:
            students = list(self.database.students)
            if "FROM faculty_assignments fa" in query:
                user_id = values[0]
                assigned = {(user_id, course, semester) for user_id, course, semester, _
                            in self.database.faculty_assignments}
                students = [student for student in students if (
                    user_id, student["course"], student["semester"]
                ) in assigned]
            if "student_id = %s" in query:
                students = [student for student in students if student["id"] == values[0]]
            self.rows = [{"course": course} for course in sorted({s["course"] for s in students})]
        elif "SELECT student_id AS id" in query and "LOCATE(LOWER(%s)" in query:
            term = values[0].lower()
            students = [
                student for student in self.database.students
                if term in student["id"].lower() or term in student["name"].lower()
            ]
            if "FROM faculty_assignments fa" in query:
                user_id = values[2]
                assigned = {(assigned_user, course, semester)
                            for assigned_user, course, semester, _
                            in self.database.faculty_assignments}
                students = [student for student in students if (
                    user_id, student["course"], student["semester"]
                ) in assigned]
            filters = values[2:]
            if "course = %s" in query:
                course = next(value for value in filters if isinstance(value, str))
                students = [student for student in students if student["course"] == course]
            if "semester = %s" in query:
                semester = next(value for value in filters if isinstance(value, int))
                students = [student for student in students if student["semester"] == semester]
            self.rows = sorted(students, key=lambda student: (student["name"], student["id"]))
        elif query.startswith("SELECT student_id AS id, name, contact_details AS contactDetails"):
            students = list(self.database.students)
            if "FROM faculty_assignments fa" in query:
                user_id = values[0]
                assigned = {(user_id, course, semester) for user_id, course, semester, _
                            in self.database.faculty_assignments}
                students = [student for student in students if (
                    user_id, student["course"], student["semester"]
                ) in assigned]
            if "WHERE student_id = %s" in query:
                students = [student for student in students if student["id"] == values[0]]
            else:
                filters = values
                if "course = %s" in query:
                    course = filters[0]
                    students = [student for student in students if student["course"] == course]
                    filters = filters[1:]
                if "semester = %s" in query:
                    semester = filters[0]
                    students = [student for student in students if student["semester"] == semester]
            self.rows = sorted(students, key=lambda student: (student["name"], student["id"]))
        elif query.startswith("SELECT course, semester FROM students WHERE student_id = %s"):
            student = next((student for student in self.database.students if student["id"] == values[0]), None)
            self.rows = [{"course": student["course"], "semester": student["semester"]}] if student else []
        elif query.startswith("SELECT student_id, course, semester FROM grades"):
            grade = next((grade for grade in self.database.grades if grade["id"] == values[0]), None)
            self.rows = [{
                "student_id": grade["student_id"],
                "course": grade["course"],
                "semester": grade["semester"],
            }] if grade else []
        elif query.startswith("SELECT student_id, course, semester, subject, component, marks FROM grades"):
            grade = next((grade for grade in self.database.grades if grade["id"] == values[0]), None)
            self.rows = [{
                "student_id": grade["student_id"],
                "course": grade["course"],
                "semester": grade["semester"],
                "subject": grade["subject"],
                "component": grade["component"],
                "marks": grade["marks"],
            }] if grade else []
        elif query.startswith("SELECT 1 FROM course_semester_subjects"):
            course, semester, subject = values
            self.rows = [(1,)] if (course, semester, subject) in self.database.course_subjects else []
        elif query.startswith("SELECT 1 FROM grade_windows"):
            self.rows = [(1,)] if any(
                (window["course"], window["semester"], window["component"]) == tuple(values)
                and window["isOpen"]
                for window in self.database.grade_windows
            ) else []
        elif query.startswith("SELECT component, DATE_FORMAT(start_date"):
            course, semester = values
            self.rows = [
                {key: window[key] for key in ("component", "startDate", "endDate", "isOpen")}
                for window in self.database.grade_windows
                if window["course"] == course and window["semester"] == semester
            ]
        elif query.startswith("SELECT student_id, name, contact_details, course, semester"):
            students = list(self.database.students)
            if "FROM faculty_assignments fa" in query:
                user_id = values[0]
                assigned = {(assigned_user, course, semester)
                            for assigned_user, course, semester, _
                            in self.database.faculty_assignments}
                students = [student for student in students if (
                    user_id, student["course"], student["semester"]
                ) in assigned]
            if "course = %s" in query:
                course = next(value for value in values if isinstance(value, str))
                students = [student for student in students if student["course"] == course]
            self.rows = [
                (student["id"], student["name"], student["contactDetails"], student["course"], student["semester"])
                for student in students
            ]
        elif query.startswith("SELECT 1 FROM courses WHERE name = %s"):
            self.rows = [(1,)] if values[0] in self.database.courses else []
        elif query.startswith("SELECT 1 FROM students WHERE student_id = %s"):
            self.rows = [(1,)] if any(student["id"] == values[0] for student in self.database.students) else []
        elif query.startswith("INSERT INTO students"):
            student_id, name, contact, course, semester = values
            if any(student["id"] == student_id for student in self.database.students):
                raise mysql.connector.IntegrityError(msg="duplicate", errno=1062)
            self.database.students.append({
                "id": student_id,
                "name": name,
                "contactDetails": contact,
                "course": course,
                "semester": semester,
                "createdAt": date.today().isoformat(),
            })
            self.rowcount = 1
        elif query.startswith("INSERT INTO users"):
            username, password_hash = values[:2]
            role = "student" if len(values) == 3 else "faculty"
            student_id = values[2] if len(values) == 3 else None
            if username in self.database.users or any(
                student_id is not None and account["student_id"] == student_id
                for account in self.database.users.values()
            ):
                raise mysql.connector.IntegrityError(msg="duplicate", errno=1062)
            user_id = max(account["user_id"] for account in self.database.users.values()) + 1
            self.database.users[username] = {
                "user_id": user_id,
                "username": username,
                "password_hash": password_hash,
                "role": role,
                "student_id": student_id,
            }
            self.lastrowid = user_id
            self.rowcount = 1
        elif query.startswith("UPDATE users SET username = %s, password_hash = %s"):
            username, password_hash, student_id = values
            if username in self.database.users and self.database.users[username]["student_id"] != student_id:
                raise mysql.connector.IntegrityError(msg="duplicate", errno=1062)
            account = next(account for account in self.database.users.values() if account["student_id"] == student_id)
            self.database.users.pop(account["username"])
            account.update(username=username, password_hash=password_hash)
            self.database.users[username] = account
            self.rowcount = 1
        elif query.startswith("UPDATE users SET username = %s"):
            username, student_id = values
            if username in self.database.users and self.database.users[username]["student_id"] != student_id:
                raise mysql.connector.IntegrityError(msg="duplicate", errno=1062)
            account = next(account for account in self.database.users.values() if account["student_id"] == student_id)
            self.database.users.pop(account["username"])
            account["username"] = username
            self.database.users[username] = account
            self.rowcount = 1
        elif query.startswith("UPDATE students SET"):
            new_id, name, contact, course, semester, original_id = values
            if any(student["id"] == new_id and student["id"] != original_id for student in self.database.students):
                raise mysql.connector.IntegrityError(msg="duplicate", errno=1062)
            for student in self.database.students:
                if student["id"] == original_id:
                    student.update(
                        id=new_id,
                        name=name,
                        contactDetails=contact,
                        course=course,
                        semester=semester,
                    )
                    self.rowcount = 1
                    break
            for account in self.database.users.values():
                if account["student_id"] == original_id:
                    account["student_id"] = new_id
        elif query.startswith("DELETE FROM students"):
            student_id = values[0]
            before = len(self.database.students)
            self.database.students = [
                student for student in self.database.students if student["id"] != student_id
            ]
            self.database.grades = [
                grade for grade in self.database.grades if grade["student_id"] != student_id
            ]
            for account in self.database.users.values():
                if account["student_id"] == student_id:
                    account["student_id"] = None
            self.rowcount = before - len(self.database.students)
        elif "FROM grades WHERE student_id = %s" in query:
            grades = [grade for grade in self.database.grades if grade["student_id"] == values[0]]
            if "AND EXISTS (SELECT 1 FROM faculty_assignments fa" in query:
                user_id = values[1]
                assigned_subjects = {
                    (course, semester, subject)
                    for assigned_user, course, semester, subject in self.database.faculty_assignments
                    if assigned_user == user_id
                }
                grades = [grade for grade in grades if (
                    grade["course"], grade["semester"], grade["subject"]
                ) in assigned_subjects]
            self.rows = [
                {
                    "id": grade["id"],
                    "subject": grade["subject"],
                    "component": grade["component"],
                    "marks": grade["marks"],
                    "lastChangeReason": next(
                        (history["reason"] for history in reversed(self.database.grade_change_history)
                         if history["grade_id"] == grade["id"]),
                        "",
                    ),
                    "updatedAt": "2026-10-31",
                }
                for grade in grades
            ]
        elif query.startswith("INSERT INTO grades"):
            student_id, course, semester, subject, component, marks = values
            if any(
                grade["student_id"] == student_id
                and grade["subject"] == subject
                and grade["component"] == component
                for grade in self.database.grades
            ):
                raise mysql.connector.IntegrityError(msg="duplicate", errno=1062)
            self.lastrowid = self.database.next_grade_id
            self.database.next_grade_id += 1
            self.database.grades.append({
                "id": self.lastrowid,
                "student_id": student_id,
                "course": course,
                "semester": semester,
                "subject": subject,
                "component": component,
                "marks": marks,
            })
        elif query.startswith("INSERT INTO grade_change_history"):
            grade_id, old_marks, new_marks, reason, changed_by = values
            self.database.grade_change_history.append({
                "grade_id": grade_id,
                "old_marks": old_marks,
                "new_marks": new_marks,
                "reason": reason,
                "changed_by": changed_by,
            })
        elif query.startswith("UPDATE grades SET"):
            marks, grade_id = values
            for grade in self.database.grades:
                if grade["id"] == grade_id:
                    grade["marks"] = marks
                    self.rowcount = 1
                    break
        elif query.startswith("DELETE FROM grade_windows"):
            course, semester, component = values
            self.database.grade_windows = [
                window for window in self.database.grade_windows
                if (window["course"], window["semester"], window["component"])
                != (course, semester, component)
            ]
        elif query.startswith("INSERT INTO grade_windows"):
            course, semester, component, start_date, end_date, _updated_by = values
            window = next(
                (window for window in self.database.grade_windows
                 if (window["course"], window["semester"], window["component"])
                 == (course, semester, component)),
                None,
            )
            updated = {
                "course": course,
                "semester": semester,
                "component": component,
                "startDate": start_date.isoformat(),
                "endDate": end_date.isoformat(),
                "isOpen": start_date <= date.today() <= end_date,
            }
            if window:
                window.update(updated)
            else:
                self.database.grade_windows.append(updated)
        elif query.startswith("SELECT 1 FROM courses WHERE name = %s"):
            self.rows = [(1,)] if values[0] in self.database.courses else []
        elif query.startswith("SELECT 1 FROM grades WHERE grade_id = %s"):
            self.rows = [(1,)] if any(grade["id"] == values[0] for grade in self.database.grades) else []
        elif query == "SELECT 1":
            self.rows = [(1,)]
        else:
            raise AssertionError(f"Unhandled test query: {query}")

    def fetchone(self):
        return self.rows.pop(0) if self.rows else None

    def fetchall(self):
        rows, self.rows = self.rows, []
        return rows

    def executemany(self, query, values):
        if query.startswith("INSERT INTO faculty_assignments"):
            self.database.faculty_assignments.extend(tuple(row) for row in values)
            self.rowcount = len(values)
            return
        raise AssertionError(f"Unhandled test executemany query: {query}")

    def close(self):
        pass


class BackendApiTests(unittest.TestCase):
    def setUp(self):
        self.database = FakeDatabase()
        self.app = create_app(
            {"TESTING": True, "SECRET_KEY": "test-secret"},
            connection_factory=self.database.connect,
        )
        self.client = self.app.test_client()

    def login(self, username, password):
        response = self.client.post(
            "/api/login",
            json={"username": username, "password": password},
        )
        self.assertEqual(response.status_code, 200, response.get_json())
        return response.get_json()["csrfToken"]

    @staticmethod
    def mutation_headers(token):
        return {"X-CSRF-Token": token}

    def test_login_roles_empty_and_invalid_credentials(self):
        empty = self.client.post("/api/login", json={"username": "", "password": ""})
        self.assertEqual(empty.status_code, 400)
        self.assertEqual(empty.get_json()["error"], "Please enter your username and password.")
        invalid = self.client.post("/api/login", json={"username": "admin", "password": "wrong"})
        self.assertEqual(invalid.status_code, 401)
        self.assertEqual(invalid.get_json()["error"], "Invalid username or password.")
        self.assertFalse(self.client.get("/api/session").get_json()["authenticated"])

        for username, password, expected_role in (
            ("admin", "admin123", "admin"),
            ("faculty1", "faculty123", "faculty"),
            ("student1", "student123", "student"),
            ("student2", "student234", "student"),
        ):
            token = self.login(username, password)
            self.assertTrue(token)
            self.assertEqual(self.client.get("/api/session").get_json()["user"]["role"], expected_role)
            self.client.post("/api/logout", json={}, headers=self.mutation_headers(token))

    def test_login_normalizes_legacy_role_labels(self):
        for username, legacy_role, expected_role in (
            ("admin", "Administrator", "admin"),
            ("faculty1", "FACULTY", "faculty"),
            ("student1", "Student", "student"),
        ):
            account = self.database.users[username]
            original_role = account["role"]
            account["role"] = legacy_role
            token = self.login(username, {
                "admin": "admin123",
                "faculty1": "faculty123",
                "student1": "student123",
            }[username])
            self.assertEqual(self.client.get("/api/session").get_json()["user"]["role"], expected_role)
            self.client.post("/api/logout", json={}, headers=self.mutation_headers(token))
            account["role"] = original_role

    def test_admin_can_create_faculty_credentials(self):
        admin_token = self.login("admin", "admin123")
        response = self.client.post(
            "/api/faculty",
            json={"username": "newfaculty", "password": "facultySecure123!"},
            headers=self.mutation_headers(admin_token),
        )
        self.assertEqual(response.status_code, 201)
        faculty = response.get_json()["faculty"]
        self.assertEqual(faculty["username"], "newfaculty")
        self.assertNotIn("password", faculty)
        account = self.database.users["newfaculty"]
        self.assertEqual(account["role"], "faculty")
        self.assertTrue(check_password_hash(account["password_hash"], "facultySecure123!"))
        faculty_token = self.login("newfaculty", "facultySecure123!")
        self.client.post("/api/logout", json={}, headers=self.mutation_headers(faculty_token))
        admin_token = self.login("admin", "admin123")

        duplicate = self.client.post(
            "/api/faculty",
            json={"username": "newfaculty", "password": "facultySecure123!"},
            headers=self.mutation_headers(admin_token),
        )
        self.assertEqual(duplicate.status_code, 409)

        short_password = self.client.post(
            "/api/faculty",
            json={"username": "shortpass", "password": "short"},
            headers=self.mutation_headers(admin_token),
        )
        self.assertEqual(short_password.status_code, 400)
        faculty_token = self.login("faculty1", "faculty123")
        forbidden = self.client.post(
            "/api/faculty",
            json={"username": "otherfaculty", "password": "facultySecure123!"},
            headers=self.mutation_headers(faculty_token),
        )
        self.assertEqual(forbidden.status_code, 403)

    def test_admin_controls_department_semester_grade_windows(self):
        admin_token = self.login("admin", "admin123")
        query = "/api/grade-windows?course=Computer+Science&semester=2"
        initial = self.client.get(query)
        self.assertEqual(initial.status_code, 200)
        self.assertEqual(
            {window["component"] for window in initial.get_json()["windows"]},
            {"ISA1", "ISA2", "ESA"},
        )
        today = date.today()
        future = today + timedelta(days=3)
        response = self.client.put(
            "/api/grade-windows",
            json={
                "course": "Computer Science",
                "semester": 2,
                "windows": [
                    {"component": "ISA1", "startDate": future.isoformat(), "endDate": future.isoformat()},
                    {"component": "ISA2", "startDate": "", "endDate": ""},
                    {"component": "ESA", "startDate": today.isoformat(), "endDate": today.isoformat()},
                ],
            },
            headers=self.mutation_headers(admin_token),
        )
        self.assertEqual(response.status_code, 200, response.get_json())
        saved = self.client.get(query).get_json()["windows"]
        statuses = {window["component"]: window["isOpen"] for window in saved}
        self.assertEqual(statuses, {"ISA1": False, "ESA": True})

        faculty_token = self.login("faculty1", "faculty123")
        self.assertEqual(self.client.get(query).status_code, 200)
        self.assertEqual(
            self.client.get(
                "/api/grade-windows?course=Business+Administration&semester=5"
            ).status_code,
            403,
        )
        closed = self.client.post(
            "/api/students/S001/grades",
            json={"subject": "Mathematics", "component": "ISA1", "marks": 80},
            headers=self.mutation_headers(faculty_token),
        )
        self.assertEqual(closed.status_code, 403)

        admin_token = self.login("admin", "admin123")
        invalid_dates = self.client.put(
            "/api/grade-windows",
            json={
                "course": "Computer Science",
                "semester": 2,
                "windows": [
                    {"component": "ISA1", "startDate": future.isoformat(), "endDate": today.isoformat()},
                    {"component": "ISA2", "startDate": "", "endDate": ""},
                    {"component": "ESA", "startDate": "", "endDate": ""},
                ],
            },
            headers=self.mutation_headers(admin_token),
        )
        self.assertEqual(invalid_dates.status_code, 400)

    def test_admin_can_create_update_search_filter_detail_and_delete(self):
        token = self.login("admin", "admin123")
        student = {
            "id": "S003",
            "name": "New Student",
            "contactDetails": "new@example.edu",
            "course": "Electronics",
            "semester": 5,
            "username": "student3",
            "password": "student12345!",
        }
        incomplete = self.client.post(
            "/api/students",
            json={
                "id": "S003",
                "name": "",
                "contactDetails": "new@example.edu",
                "course": "Electronics",
                "semester": 5,
            },
            headers=self.mutation_headers(token),
        )
        self.assertEqual(incomplete.status_code, 400)
        self.assertEqual(incomplete.get_json()["error"], "Name is required.")
        username_conflict = self.client.post(
            "/api/students",
            json=dict(student, id="S004", username="admin"),
            headers=self.mutation_headers(token),
        )
        self.assertEqual(username_conflict.status_code, 409)
        self.assertEqual(len([row for row in self.database.students if row["id"] == "S004"]), 0)
        response = self.client.post(
            "/api/students", json=student, headers=self.mutation_headers(token)
        )
        self.assertEqual(response.status_code, 201)
        self.assertEqual(response.get_json()["student"]["contactDetails"], student["contactDetails"])
        self.assertEqual(response.get_json()["student"]["loginUsername"], "student3")
        self.assertNotIn("password", response.get_json()["student"])

        duplicate = self.client.post(
            "/api/students", json=student, headers=self.mutation_headers(token)
        )
        self.assertEqual(duplicate.status_code, 409)
        self.assertEqual(duplicate.get_json()["error"], "Student ID already exists.")

        detail = self.client.get("/api/students/S003")
        self.assertEqual(detail.get_json()["student"]["semester"], 5)
        self.assertEqual(detail.get_json()["student"]["loginUsername"], "student3")
        search = self.client.get("/api/students/search?q=new")
        self.assertEqual(search.get_json()["students"][0]["id"], "S003")
        empty_search = self.client.get("/api/students/search?q=")
        self.assertEqual(
            empty_search.get_json()["error"],
            "Please enter a Student ID or name to search.",
        )
        missing_search = self.client.get("/api/students/search?q=not-a-student")
        self.assertEqual(missing_search.status_code, 200)
        self.assertEqual(missing_search.get_json()["students"], [])
        filtered = self.client.get("/api/students?course=Electronics&semester=5")
        self.assertEqual([row["id"] for row in filtered.get_json()["students"]], ["S003"])

        update = dict(student, name="Updated Student", password="")
        response = self.client.put(
            "/api/students/S003", json=update, headers=self.mutation_headers(token)
        )
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.get_json()["student"]["name"], "Updated Student")
        self.assertTrue(check_password_hash(
            self.database.users["student3"]["password_hash"], "student12345!"
        ))
        updated_credentials = dict(
            update,
            username="student3-updated",
            password="newstudent12345!",
        )
        response = self.client.put(
            "/api/students/S003",
            json=updated_credentials,
            headers=self.mutation_headers(token),
        )
        self.assertEqual(response.status_code, 200)
        self.assertNotIn("student3", self.database.users)
        self.assertTrue(check_password_hash(
            self.database.users["student3-updated"]["password_hash"], "newstudent12345!"
        ))
        student_token = self.login("student3-updated", "newstudent12345!")
        self.assertEqual(self.client.get("/api/session").get_json()["user"]["studentId"], "S003")
        self.client.post("/api/logout", json={}, headers=self.mutation_headers(student_token))
        token = self.login("admin", "admin123")
        unknown = self.client.put(
            "/api/students/UNKNOWN", json=student, headers=self.mutation_headers(token)
        )
        self.assertEqual(unknown.status_code, 404)
        self.assertEqual(unknown.get_json()["error"], "Student record not found.")

        response = self.client.delete(
            "/api/students/S003", headers=self.mutation_headers(token)
        )
        self.assertEqual(response.status_code, 200)
        self.assertEqual(len(self.database.students), 2)

    def test_eight_semester_limit_and_four_department_sample_distribution(self):
        token = self.login("admin", "admin123")
        student = {
            "id": "S009",
            "name": "Invalid Semester",
            "contactDetails": "invalid@example.edu",
            "course": "Electronics",
            "semester": 9,
        }
        response = self.client.post(
            "/api/students",
            json=student,
            headers=self.mutation_headers(token),
        )
        self.assertEqual(response.status_code, 400)
        self.assertEqual(
            response.get_json()["error"],
            "Semester must be a whole number from 1 to 8.",
        )
        listing = self.client.get("/api/students")
        self.assertEqual(listing.get_json()["semesters"], list(range(1, 9)))
        self.assertEqual(
            COURSES,
            [
                "Business Administration",
                "Computer Science",
                "Electronics",
                "Information Technology",
            ],
        )
        sample_students = generate_sample_students()
        self.assertEqual(len(sample_students), 4 * 8 * 20)
        distribution = Counter((student[3], student[4]) for student in sample_students)
        self.assertEqual(set(distribution), {(course, semester) for course in COURSES for semester in range(1, 9)})
        self.assertTrue(all(count >= 20 for count in distribution.values()))

    def test_student_cannot_access_another_profile_or_export(self):
        self.login("student1", "student123")
        own_list = self.client.get("/api/students").get_json()["students"]
        self.assertEqual([student["id"] for student in own_list], ["S001"])
        profile = self.client.get("/api/students/S001")
        self.assertEqual(profile.status_code, 200)
        forbidden = self.client.get("/api/students/S002")
        self.assertEqual(forbidden.status_code, 403)
        self.assertEqual(self.client.get("/api/students/export").status_code, 403)
        self.assertEqual(self.client.get("/api/students/search?q=Aarav").status_code, 403)

    def test_role_and_csrf_protect_student_mutations(self):
        student = {
            "id": "S003",
            "name": "New Student",
            "contactDetails": "new@example.edu",
            "course": "Electronics",
            "semester": 1,
        }
        for username, password in (("faculty1", "faculty123"), ("student1", "student123")):
            self.login(username, password)
            response = self.client.post("/api/students", json=student)
            self.assertEqual(response.status_code, 403)
            self.assertEqual(self.client.delete("/api/students/S001").status_code, 403)
        token = self.login("admin", "admin123")
        response = self.client.post("/api/students", json=student)
        self.assertEqual(response.status_code, 403)
        self.assertEqual(len(self.database.students), 2)
        self.assertTrue(token)

    def test_faculty_records_updates_and_students_read_own_grades(self):
        token = self.login("faculty1", "faculty123")
        response = self.client.post(
            "/api/students/S001/grades",
            json={"subject": "Mathematics", "component": "ISA1", "marks": 88},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(response.status_code, 201)
        grade_id = response.get_json()["id"]
        for invalid_marks in (88.5, -1, 101):
            invalid = self.client.post(
                "/api/students/S001/grades",
                json={"subject": "Mathematics", "component": "ISA1", "marks": invalid_marks},
                headers=self.mutation_headers(token),
            )
            self.assertEqual(invalid.status_code, 400)
            self.assertEqual(invalid.get_json()["error"], "Marks must be between 0 and 100.")
        missing = self.client.post(
            "/api/students/MISSING/grades",
            json={"subject": "Mathematics", "component": "ISA1", "marks": 88},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(missing.status_code, 404)
        self.assertEqual(missing.get_json()["error"], "Student or subject not found.")
        missing_subject = self.client.post(
            "/api/students/S001/grades",
            json={"subject": "Unknown Subject", "component": "ISA1", "marks": 88},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(missing_subject.get_json()["error"], "Student or subject not found.")
        duplicate = self.client.post(
            "/api/students/S001/grades",
            json={"subject": "Mathematics", "component": "ISA1", "marks": 89},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(duplicate.status_code, 409)
        missing_reason = self.client.put(
            f"/api/grades/{grade_id}",
            json={"subject": "Mathematics", "component": "ISA1", "marks": 90},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(missing_reason.status_code, 400)
        self.assertEqual(self.database.grades[0]["marks"], 88)
        updated = self.client.put(
            f"/api/grades/{grade_id}",
            json={
                "subject": "Mathematics",
                "component": "ISA1",
                "marks": 91,
                "reason": "Corrected after checking the marked answer sheet.",
            },
            headers=self.mutation_headers(token),
        )
        self.assertEqual(updated.status_code, 200)
        self.assertEqual(len(self.database.grade_change_history), 1)
        self.assertEqual(self.database.grade_change_history[0]["old_marks"], 88)
        self.assertEqual(self.database.grade_change_history[0]["new_marks"], 91)
        second_component = self.client.post(
            "/api/students/S001/grades",
            json={"subject": "Mathematics", "component": "ISA2", "marks": 76},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(second_component.status_code, 201)
        self.assertEqual(second_component.get_json()["component"], "ISA2")
        unknown_grade = self.client.put(
            "/api/grades/999",
            json={"subject": "Mathematics", "marks": 75},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(unknown_grade.get_json()["error"], "Grade record not found.")

        self.login("student1", "student123")
        grades = self.client.get("/api/students/S001/grades")
        self.assertEqual(grades.get_json()["grades"][0]["marks"], 91)
        self.assertEqual(
            {grade["component"] for grade in grades.get_json()["grades"]},
            {"ISA1", "ISA2"},
        )
        isa1 = next(grade for grade in grades.get_json()["grades"] if grade["component"] == "ISA1")
        self.assertEqual(isa1["lastChangeReason"], "Corrected after checking the marked answer sheet.")
        self.assertEqual(self.client.get("/api/students/S002/grades").status_code, 403)

    def test_faculty_assignment_scopes_students_grades_and_subjects(self):
        token = self.login("faculty1", "faculty123")
        self.assertEqual(
            self.client.get("/api/students").get_json()["semesters"],
            [2],
        )
        self.database.faculty_assignments.append(
            (2, "Business Administration", 5, "Management")
        )
        all_semesters = self.client.get("/api/students").get_json()["semesters"]
        course_semesters = self.client.get(
            "/api/students?course=Business+Administration"
        ).get_json()["semesters"]
        self.assertEqual(all_semesters, [2, 5])
        self.assertEqual(course_semesters, [5])
        self.database.faculty_assignments.remove(
            (2, "Business Administration", 5, "Management")
        )
        self.assertEqual(self.client.get("/api/students/S001").status_code, 200)
        self.assertEqual(self.client.get("/api/students/S002").status_code, 403)
        self.assertEqual(
            [student["id"] for student in self.client.get("/api/students").get_json()["students"]],
            ["S001"],
        )
        self.assertEqual(
            self.client.get("/api/students/search?q=Diya").get_json()["students"],
            [],
        )
        self.assertNotIn(
            "diya@example.edu",
            self.client.get("/api/students/export").get_data(as_text=True),
        )
        self.assertEqual(self.client.get("/api/students/S002/grades").status_code, 403)
        self.assertEqual(
            self.client.get("/api/subjects?course=Information+Technology&semester=3").status_code,
            403,
        )
        self.assertEqual(
            self.client.get("/api/subjects?course=Computer+Science&semester=2")
            .get_json()["subjects"],
            ["Mathematics"],
        )
        self.assertEqual(
            self.client.get("/api/faculty/classes").get_json()["classes"],
            [{"course": "Computer Science", "semester": 2, "subject": "Mathematics"}],
        )
        unassigned_subject = self.client.post(
            "/api/students/S001/grades",
            json={"subject": "Physics", "component": "ISA1", "marks": 80},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(unassigned_subject.status_code, 403)
        grade_response = self.client.post(
            "/api/students/S001/grades",
            json={"subject": "Mathematics", "component": "ISA1", "marks": 80},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(grade_response.status_code, 201)
        grade_id = grade_response.get_json()["id"]
        self.database.grades.append({
            "id": 99,
            "student_id": "S001",
            "course": "Computer Science",
            "semester": 2,
            "subject": "Physics",
            "component": "ISA1",
            "marks": 75,
        })
        self.assertEqual(
            [grade["subject"] for grade in self.client.get("/api/students/S001/grades")
             .get_json()["grades"]],
            ["Mathematics"],
        )
        self.assertEqual(
            self.client.put(
                "/api/grades/99",
                json={"subject": "Physics", "component": "ISA1", "marks": 90, "reason": "Correction."},
                headers=self.mutation_headers(token),
            ).status_code,
            403,
        )
        self.assertEqual(
            self.client.post(
                "/api/students/S002/grades",
                json={"subject": "Data Structures", "component": "ISA1", "marks": 80},
                headers=self.mutation_headers(token),
            ).status_code,
            403,
        )
        admin_token = self.login("admin", "admin123")
        self.assertEqual(
            self.client.put(
                "/api/faculty/2/assignments",
                json={"assignments": []},
                headers=self.mutation_headers(admin_token),
            ).status_code,
            200,
        )
        faculty_token = self.login("faculty1", "faculty123")
        self.assertEqual(
            self.client.put(
                f"/api/grades/{grade_id}",
                json={"subject": "Mathematics", "component": "ISA1", "marks": 90, "reason": "Correction."},
                headers=self.mutation_headers(faculty_token),
            ).status_code,
            403,
        )
    def test_admin_can_assign_faculty_multiple_semesters(self):
        token = self.login("admin", "admin123")
        response = self.client.get("/api/faculty-assignments")
        self.assertEqual(response.status_code, 200)
        faculty_id = response.get_json()["faculty"][0]["id"]
        response = self.client.put(
            f"/api/faculty/{faculty_id}/assignments",
            json={"assignments": [
                {"course": "Computer Science", "semester": 2, "subject": "Mathematics"},
                {"course": "Computer Science", "semester": 2, "subject": "Physics"},
            ]},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(response.status_code, 200, response.get_json())
        self.assertEqual(len(response.get_json()["assignments"]), 2)
        self.assertEqual(len(self.database.faculty_assignments), 2)
        invalid_subject = self.client.put(
            f"/api/faculty/{faculty_id}/assignments",
            json={"assignments": [
                {"course": "Computer Science", "semester": 2, "subject": "Management"},
            ]},
            headers=self.mutation_headers(token),
        )
        self.assertEqual(invalid_subject.status_code, 400)
        self.assertEqual(len(self.database.faculty_assignments), 2)

    def test_admin_and_student_cannot_write_grades(self):
        for username, password in (("admin", "admin123"), ("student1", "student123")):
            self.login(username, password)
            response = self.client.post(
                "/api/students/S001/grades",
                json={"subject": "Mathematics", "component": "ISA1", "marks": 75},
            )
            self.assertEqual(response.status_code, 403)
            self.assertEqual(self.client.put("/api/grades/1", json={"subject": "Physics", "marks": 80}).status_code, 403)
        self.assertEqual(self.database.grades, [])

    def test_export_is_csv_for_authorized_roles(self):
        self.login("faculty1", "faculty123")
        response = self.client.get("/api/students/export?course=Computer+Science")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.mimetype, "text/csv")
        self.assertIn("aarav@example.edu", response.get_data(as_text=True))

    def test_role_scoped_course_and_subject_catalogs(self):
        self.database.courses.extend(
            ["Mechanical Engineering", "Civil Engineering", "Electrical Engineering"]
        )
        self.database.students.append({
            "id": "OLD001",
            "name": "Legacy Department Student",
            "contactDetails": "legacy@example.edu",
            "course": "Mechanical Engineering",
            "semester": 1,
            "createdAt": "2026-10-31",
        })
        self.login("faculty1", "faculty123")
        self.assertEqual(self.client.get("/api/courses").status_code, 200)
        self.assertEqual(self.client.get("/api/courses").get_json()["courses"], ["Computer Science"])
        self.assertEqual(
            self.client.get("/api/subjects?course=Computer+Science&semester=2").status_code,
            200,
        )
        student_list = self.client.get("/api/students").get_json()
        self.assertEqual(student_list["courses"], ["Computer Science"])
        self.assertEqual(student_list["stats"]["totalCourses"], 1)
        self.login("student1", "student123")
        self.assertEqual(self.client.get("/api/subjects").status_code, 403)

    def test_api_security_headers_and_health(self):
        response = self.client.get("/api/health")
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.get_json()["database"], "connected")
        self.assertEqual(response.headers["X-Frame-Options"], "DENY")
        self.assertIn("frame-ancestors 'none'", response.headers["Content-Security-Policy"])

    def test_production_requires_a_strong_session_secret(self):
        with patch.dict(os.environ, {"APP_ENV": "production", "SESSION_SECRET": ""}):
            with self.assertRaises(RuntimeError):
                create_app()
        with patch.dict(os.environ, {"APP_ENV": "production", "SESSION_SECRET": "x" * 32}):
            production_app = create_app()
            self.assertTrue(production_app.config["SESSION_COOKIE_SECURE"])


if __name__ == "__main__":
    unittest.main()
