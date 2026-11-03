"""Flask API and same-origin web server for the Student Record System."""

from contextlib import contextmanager
import csv
from datetime import date, timedelta
import hmac
from io import StringIO
import logging
import os
import secrets
from pathlib import Path

from flask import Flask, Response, current_app, jsonify, request, send_from_directory, session
from werkzeug.exceptions import HTTPException
from werkzeug.security import check_password_hash, generate_password_hash

import mysql.connector


ROOT = Path(__file__).resolve().parent.parent
WEB_ROOT = ROOT / "web"
LOG = logging.getLogger(__name__)
ROLES = {"admin", "faculty", "student"}
MAX_SEMESTER = 8
GRADE_COMPONENTS = ("ISA1", "ISA2", "ESA")
DEPARTMENTS = (
    "Business Administration",
    "Computer Science",
    "Electronics",
    "Information Technology",
)
ROLE_ALIASES = {
    "admin": "admin",
    "administrator": "admin",
    "faculty": "faculty",
    "student": "student",
}
ATTENDANCE_STATUSES = {"Present", "Absent", "Late"}


class ApiError(Exception):
    def __init__(self, status, message):
        super().__init__(message)
        self.status = status
        self.message = message


def mysql_connection():
    config = {
        "host": os.getenv("MYSQL_HOST", "127.0.0.1"),
        "port": int(os.getenv("MYSQL_PORT", "3306")),
        "user": os.getenv("MYSQL_USER", "student_app"),
        "password": os.getenv("MYSQL_PASSWORD", ""),
        "database": os.getenv("MYSQL_DATABASE", "student_records"),
        "connection_timeout": 5,
        "charset": "utf8mb4",
    }
    ssl_ca = os.getenv("MYSQL_SSL_CA")
    if ssl_ca:
        config.update(ssl_ca=ssl_ca, ssl_verify_cert=True, ssl_verify_identity=True)
    return mysql.connector.connect(**config)


@contextmanager
def database_connection():
    connection = current_app.extensions["db_connection_factory"]()
    try:
        yield connection
    except Exception:
        connection.rollback()
        raise
    finally:
        connection.close()


def current_user():
    username = session.get("username")
    role = session.get("role")
    if not username or role not in ROLES:
        raise ApiError(401, "Please sign in.")
    return {
        "username": username,
        "role": role,
        "userId": session.get("user_id"),
        "roleLabel": {
            "admin": "Administrator",
            "faculty": "Faculty",
            "student": "Student",
        }[role],
        "studentId": session.get("student_id"),
    }


def require_roles(*roles):
    user = current_user()
    if user["role"] not in roles:
        raise ApiError(403, "Your role is not allowed to perform this action.")
    return user


def require_csrf():
    expected = session.get("csrf_token")
    supplied = request.headers.get("X-CSRF-Token", "")
    if not expected or not hmac.compare_digest(expected, supplied):
        raise ApiError(403, "Request verification failed. Refresh the page and try again.")


def json_body():
    body = request.get_json()
    if not isinstance(body, dict):
        raise ApiError(400, "A JSON object is required.")
    return body


def text_field(body, key, label, maximum, required=True):
    value = body.get(key)
    if value is None and not required:
        return None
    if not isinstance(value, str):
        raise ApiError(400, f"{label} must be text.")
    value = value.strip()
    if required and not value:
        raise ApiError(400, f"{label} is required.")
    if len(value) > maximum:
        raise ApiError(400, f"{label} must be at most {maximum} characters.")
    if any(ord(character) < 32 or ord(character) == 127 for character in value):
        raise ApiError(400, f"{label} contains an unsupported control character.")
    return value


def check_duplicate(error, message):
    if getattr(error, "errno", None) == 1062:
        raise ApiError(409, message) from None
    if getattr(error, "errno", None) == 1452:
        raise ApiError(404, "Student record not found.") from None
    raise error


def student_allowed(user, student_id):
    if user["role"] == "student" and user["studentId"] != student_id:
        raise ApiError(403, "Students may only access their own records.")


def require_faculty_assignment(cursor, user, course, semester, subject=None):
    query = (
        "SELECT 1 FROM faculty_assignments "
        "WHERE user_id = %s AND course = %s AND semester = %s"
    )
    values = [user["userId"], course, semester]
    if subject is not None:
        query += " AND subject = %s"
        values.append(subject)
    cursor.execute(
        query,
        tuple(values),
    )
    if cursor.fetchone() is None:
        if subject is None:
            raise ApiError(403, "You are not assigned to this department and semester.")
        raise ApiError(403, "You are not assigned to this subject.")


def valid_semester(value):
    if isinstance(value, bool) or not isinstance(value, int) or not 1 <= value <= MAX_SEMESTER:
        raise ApiError(400, f"Semester must be a whole number from 1 to {MAX_SEMESTER}.")
    return value


def valid_marks(value):
    if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value <= 100:
        raise ApiError(400, "Marks must be between 0 and 100.")
    return value


def valid_grade_component(value):
    if value not in GRADE_COMPONENTS:
        raise ApiError(400, "Assessment must be ISA1, ISA2, or ESA.")
    return value


def require_open_grade_window(cursor, course, semester, component):
    cursor.execute(
        "SELECT 1 FROM grade_windows "
        "WHERE course = %s AND semester = %s AND component = %s "
        "AND CURRENT_DATE BETWEEN start_date AND end_date",
        (course, semester, component),
    )
    if cursor.fetchone() is None:
        raise ApiError(403, f"Grade entry for {component} is outside its admin-set date window.")


def valid_login_password(value):
    if not isinstance(value, str) or len(value) < 12 or len(value) > 1024:
        raise ApiError(400, "Student password must be between 12 and 1024 characters.")
    if any(ord(character) < 32 or ord(character) == 127 for character in value):
        raise ApiError(400, "Student password contains an unsupported control character.")
    return value


def valid_iso_date(value):
    if not isinstance(value, str):
        raise ApiError(400, "Date must use YYYY-MM-DD format.")
    try:
        parsed = date.fromisoformat(value)
    except ValueError:
        raise ApiError(400, "Date must use YYYY-MM-DD format.") from None
    if parsed.isoformat() != value:
        raise ApiError(400, "Date must use YYYY-MM-DD format.")
    return parsed


def create_app(test_config=None, connection_factory=None):
    app = Flask(__name__)
    secret_key = os.getenv("SESSION_SECRET")
    if os.getenv("APP_ENV", "development") == "production":
        if not secret_key or len(secret_key) < 32:
            raise RuntimeError("Production requires a SESSION_SECRET of at least 32 characters.")
    app.config.update(
        SECRET_KEY=secret_key or secrets.token_hex(32),
        SESSION_COOKIE_HTTPONLY=True,
        SESSION_COOKIE_SECURE=os.getenv("APP_ENV", "development") == "production",
        SESSION_COOKIE_SAMESITE="Lax",
        PERMANENT_SESSION_LIFETIME=timedelta(hours=8),
        MAX_CONTENT_LENGTH=64 * 1024,
    )
    if test_config:
        app.config.update(test_config)
    app.extensions["db_connection_factory"] = connection_factory or mysql_connection

    @app.after_request
    def security_headers(response):
        response.headers["X-Content-Type-Options"] = "nosniff"
        response.headers["X-Frame-Options"] = "DENY"
        response.headers["Referrer-Policy"] = "strict-origin-when-cross-origin"
        response.headers["Content-Security-Policy"] = (
            "default-src 'self'; script-src 'self'; "
            "style-src 'self' https://fonts.googleapis.com; "
            "font-src 'self' https://fonts.gstatic.com; "
            "img-src 'self' data:; connect-src 'self'; "
            "base-uri 'self'; form-action 'self'; frame-ancestors 'none'"
        )
        if os.getenv("APP_ENV", "development") == "production":
            response.headers["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains"
        return response

    @app.errorhandler(ApiError)
    def handle_api_error(error):
        return jsonify(error=error.message), error.status

    @app.errorhandler(mysql.connector.Error)
    def handle_database_error(error):
        LOG.exception("Database request failed", exc_info=error)
        return jsonify(error="The database is unavailable or the request could not be completed."), 503

    @app.errorhandler(HTTPException)
    def handle_http_error(error):
        if request.path.startswith("/api/"):
            return jsonify(error=error.description), error.code
        return error

    @app.errorhandler(Exception)
    def handle_unexpected_error(error):
        LOG.exception("Unexpected request failure", exc_info=error)
        if request.path.startswith("/api/"):
            return jsonify(error="An unexpected server error occurred."), 500
        return "An unexpected server error occurred.", 500

    @app.get("/")
    @app.get("/web/")
    def index():
        return send_from_directory(WEB_ROOT, "index.html")

    @app.get("/web/<path:filename>")
    def web_assets(filename):
        return send_from_directory(WEB_ROOT, filename)

    @app.get("/api/health")
    def health():
        with database_connection() as connection:
            cursor = connection.cursor()
            try:
                cursor.execute("SELECT 1")
                cursor.fetchone()
            finally:
                cursor.close()
        return jsonify(status="ok", database="connected")

    @app.get("/api/session")
    def get_session():
        if session.get("username") and session.get("role") in ROLES:
            if "csrf_token" not in session:
                session["csrf_token"] = secrets.token_urlsafe(32)
            return jsonify(
                authenticated=True,
                user=current_user(),
                csrfToken=session["csrf_token"],
            )
        session.clear()
        return jsonify(authenticated=False)

    @app.post("/api/login")
    def login():
        body = json_body()
        username = body.get("username")
        password = body.get("password")
        session.clear()
        if not isinstance(username, str) or not username.strip() or not isinstance(password, str) or not password:
            raise ApiError(400, "Please enter your username and password.")
        username = text_field({"username": username}, "username", "Username", 63)
        if len(password) > 1024:
            raise ApiError(400, "Please enter your username and password.")

        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute(
                    "SELECT user_id, username, password_hash, role, student_id "
                    "FROM users WHERE username = %s",
                    (username,),
                )
                account = cursor.fetchone()
            finally:
                cursor.close()

        if not account or not check_password_hash(account["password_hash"], password):
            raise ApiError(401, "Invalid username or password.")
        role = account["role"].strip().casefold() if isinstance(account["role"], str) else ""
        role = ROLE_ALIASES.get(role)
        if role not in ROLES:
            raise ApiError(403, "This account does not have an active role.")

        session.clear()
        session.permanent = True
        session["username"] = account["username"]
        session["user_id"] = account["user_id"]
        session["role"] = role
        session["student_id"] = account["student_id"]
        session["csrf_token"] = secrets.token_urlsafe(32)
        return jsonify(
            authenticated=True,
            user=current_user(),
            csrfToken=session["csrf_token"],
        )

    @app.post("/api/logout")
    def logout():
        current_user()
        require_csrf()
        session.clear()
        return jsonify(ok=True)

    @app.get("/api/students")
    def list_students():
        user = current_user()
        search = request.args.get("search", "").strip()
        course = request.args.get("course", "").strip()
        semester = request.args.get("semester", "").strip()
        if "search" in request.args and user["role"] == "student":
            raise ApiError(403, "Your role is not allowed to search all student records.")
        if "search" in request.args and not search:
            raise ApiError(400, "Please enter a Student ID or name to search.")
        if len(search) > 100 or len(course) > 99:
            raise ApiError(400, "Search text or course filter is too long.")
        if semester and (not semester.isdigit() or not 1 <= int(semester) <= MAX_SEMESTER):
            raise ApiError(400, f"Semester must be a whole number from 1 to {MAX_SEMESTER}.")

        conditions = []
        values = []
        if user["role"] == "student":
            conditions.append("student_id = %s")
            values.append(user["studentId"])
        elif user["role"] == "faculty":
            conditions.append(
                "EXISTS (SELECT 1 FROM faculty_assignments fa "
                "WHERE fa.user_id = %s AND fa.course = students.course "
                "AND fa.semester = students.semester)"
            )
            values.append(user["userId"])
        if search:
            conditions.append(
                "LOCATE(LOWER(%s), LOWER(CONCAT_WS(' ', student_id, name, course))) > 0"
            )
            values.append(search)
        if course:
            conditions.append("course = %s")
            values.append(course)
        if semester:
            conditions.append("semester = %s")
            values.append(int(semester))
        where = f" WHERE {' AND '.join(conditions)}" if conditions else ""
        student_query = (
            "SELECT student_id AS id, name, contact_details AS contactDetails, "
            "course, semester, "
            "DATE_FORMAT(created_at, '%Y-%m-%d') AS createdAt "
            f"FROM students{where} ORDER BY name, student_id"
        )
        course_conditions = []
        course_values = []
        if user["role"] == "student":
            course_conditions.append("student_id = %s")
            course_values.append(user["studentId"])
        elif user["role"] == "faculty":
            course_conditions.append(
                "EXISTS (SELECT 1 FROM faculty_assignments fa "
                "WHERE fa.user_id = %s AND fa.course = students.course "
                "AND fa.semester = students.semester)"
            )
            course_values.append(user["userId"])
        course_where = f" WHERE {' AND '.join(course_conditions)}" if course_conditions else ""

        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                if user["role"] == "student":
                    scope = " WHERE student_id = %s"
                    scope_values = (user["studentId"],)
                elif user["role"] == "faculty":
                    scope = (
                        " WHERE EXISTS (SELECT 1 FROM faculty_assignments fa "
                        "WHERE fa.user_id = %s AND fa.course = students.course "
                        "AND fa.semester = students.semester)"
                    )
                    scope_values = (user["userId"],)
                else:
                    scope = ""
                    scope_values = ()
                cursor.execute(
                    "SELECT COUNT(*) AS totalStudents, "
                    "COUNT(DISTINCT course) AS totalCourses, "
                    "COALESCE(SUM(created_at >= DATE_SUB(CURDATE(), INTERVAL 30 DAY)), 0) "
                    f"AS recentlyAdded FROM students{scope}",
                    scope_values,
                )
                stats = cursor.fetchone()
                if user["role"] == "admin":
                    stats["totalCourses"] = len(DEPARTMENTS)
                cursor.execute(student_query, tuple(values))
                students = cursor.fetchall()
                cursor.execute(
                    f"SELECT DISTINCT course FROM students{course_where} ORDER BY course",
                    tuple(course_values),
                )
                courses = [
                    row["course"] for row in cursor.fetchall()
                    if row["course"] in DEPARTMENTS
                ]
                if user["role"] == "faculty":
                    assignment_conditions = ["user_id = %s"]
                    assignment_values = [user["userId"]]
                    if course:
                        assignment_conditions.append("course = %s")
                        assignment_values.append(course)
                    cursor.execute(
                        "SELECT DISTINCT semester FROM faculty_assignments "
                        f"WHERE {' AND '.join(assignment_conditions)} ORDER BY semester",
                        tuple(assignment_values),
                    )
                    semesters = [row["semester"] for row in cursor.fetchall()]
                else:
                    semesters = list(range(1, MAX_SEMESTER + 1))
            finally:
                cursor.close()
        return jsonify(students=students, courses=courses, semesters=semesters, stats=stats)

    @app.get("/api/courses")
    def list_courses():
        user = current_user()
        if user["role"] == "admin":
            return jsonify(courses=list(DEPARTMENTS))
        with database_connection() as connection:
            cursor = connection.cursor()
            try:
                if user["role"] == "faculty":
                    cursor.execute(
                        "SELECT DISTINCT course FROM faculty_assignments "
                        "WHERE user_id = %s ORDER BY course",
                        (user["userId"],),
                    )
                else:
                    cursor.execute(
                        "SELECT DISTINCT course FROM students WHERE student_id = %s",
                        (user["studentId"],),
                    )
                courses = [row[0] for row in cursor.fetchall()]
            finally:
                cursor.close()
        return jsonify(courses=courses)

    @app.get("/api/grade-windows")
    def list_grade_windows():
        user = require_roles("admin", "faculty")
        course = text_field(
            {"course": request.args.get("course")},
            "course",
            "Department",
            99,
        )
        raw_semester = request.args.get("semester", "")
        try:
            semester = valid_semester(int(raw_semester))
        except (ValueError, TypeError):
            raise ApiError(400, "Semester must be a whole number from 1 to 8.") from None
        if course not in DEPARTMENTS:
            raise ApiError(400, "Select a valid department.")
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                if user["role"] == "faculty":
                    require_faculty_assignment(cursor, user, course, semester)
                cursor.execute(
                    "SELECT component, "
                    "DATE_FORMAT(start_date, '%Y-%m-%d') AS startDate, "
                    "DATE_FORMAT(end_date, '%Y-%m-%d') AS endDate, "
                    "(CURRENT_DATE BETWEEN start_date AND end_date) AS isOpen "
                    "FROM grade_windows WHERE course = %s AND semester = %s "
                    "ORDER BY component",
                    (course, semester),
                )
                windows = cursor.fetchall()
            finally:
                cursor.close()
        return jsonify(windows=windows)

    @app.put("/api/grade-windows")
    def save_grade_windows():
        user = require_roles("admin")
        require_csrf()
        body = json_body()
        course = text_field(body, "course", "Department", 99)
        semester = valid_semester(body.get("semester"))
        windows = body.get("windows")
        if course not in DEPARTMENTS:
            raise ApiError(400, "Select a valid department.")
        if not isinstance(windows, list):
            raise ApiError(400, "Assessment windows must be a list.")
        windows_by_component = {}
        for window in windows:
            if not isinstance(window, dict):
                raise ApiError(400, "Each assessment window must include an assessment and dates.")
            component = valid_grade_component(window.get("component"))
            if component in windows_by_component:
                raise ApiError(400, "Each assessment window can only be provided once.")
            start_value = window.get("startDate", "")
            end_value = window.get("endDate", "")
            if start_value == "" and end_value == "":
                windows_by_component[component] = None
                continue
            start_date = valid_iso_date(start_value)
            end_date = valid_iso_date(end_value)
            if start_date > end_date:
                raise ApiError(400, "Window end date must be on or after the start date.")
            windows_by_component[component] = (start_date, end_date)
        if set(windows_by_component) != set(GRADE_COMPONENTS):
            raise ApiError(400, "Provide one window entry for ISA1, ISA2, and ESA.")

        with database_connection() as connection:
            cursor = connection.cursor()
            try:
                cursor.execute("SELECT 1 FROM courses WHERE name = %s", (course,))
                if cursor.fetchone() is None:
                    raise ApiError(400, "Select a valid department.")
                for component in GRADE_COMPONENTS:
                    period = windows_by_component[component]
                    if period is None:
                        cursor.execute(
                            "DELETE FROM grade_windows "
                            "WHERE course = %s AND semester = %s AND component = %s",
                            (course, semester, component),
                        )
                    else:
                        cursor.execute(
                            "INSERT INTO grade_windows "
                            "(course, semester, component, start_date, end_date, updated_by) "
                            "VALUES (%s, %s, %s, %s, %s, %s) "
                            "ON DUPLICATE KEY UPDATE start_date = VALUES(start_date), "
                            "end_date = VALUES(end_date), updated_by = VALUES(updated_by)",
                            (course, semester, component, *period, user["userId"]),
                        )
                connection.commit()
            finally:
                cursor.close()
        return jsonify(ok=True, course=course, semester=semester)

    @app.post("/api/faculty")
    def create_faculty():
        require_roles("admin")
        require_csrf()
        body = json_body()
        username = text_field(body, "username", "Login username", 63)
        password = valid_login_password(body.get("password"))
        with database_connection() as connection:
            cursor = connection.cursor()
            try:
                cursor.execute("SELECT 1 FROM users WHERE username = %s", (username,))
                if cursor.fetchone() is not None:
                    raise ApiError(409, "Login username is already in use.")
                cursor.execute(
                    "INSERT INTO users (username, password_hash, role, student_id) "
                    "VALUES (%s, %s, 'faculty', NULL)",
                    (username, generate_password_hash(password)),
                )
                faculty_id = cursor.lastrowid
                connection.commit()
            except mysql.connector.Error as error:
                if getattr(error, "errno", None) == 1062:
                    raise ApiError(409, "Login username is already in use.") from None
                raise
            finally:
                cursor.close()
        return jsonify(faculty={"id": faculty_id, "username": username}), 201

    @app.get("/api/faculty-assignments")
    def list_faculty_assignments():
        require_roles("admin")
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute(
                    "SELECT user_id AS id, username FROM users "
                    "WHERE role = 'faculty' ORDER BY username"
                )
                faculty = cursor.fetchall()
                cursor.execute(
                    "SELECT user_id AS userId, course, semester, subject "
                    "FROM faculty_assignments ORDER BY course, semester, subject"
                )
                assignments = cursor.fetchall()
                cursor.execute(
                    "SELECT course, semester, subject FROM course_semester_subjects "
                    "ORDER BY course, semester, subject"
                )
                subjects = cursor.fetchall()
            finally:
                cursor.close()
        assignments_by_user = {}
        for assignment in assignments:
            assignments_by_user.setdefault(assignment["userId"], []).append({
                "course": assignment["course"],
                "semester": assignment["semester"],
                "subject": assignment["subject"],
            })
        for account in faculty:
            account["assignments"] = assignments_by_user.get(account["id"], [])
        return jsonify(
            faculty=faculty,
            courses=list(DEPARTMENTS),
            semesters=list(range(1, MAX_SEMESTER + 1)),
            subjects=subjects,
        )

    @app.get("/api/faculty/classes")
    def list_faculty_classes():
        user = require_roles("faculty")
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute(
                    "SELECT course, semester, subject FROM faculty_assignments "
                    "WHERE user_id = %s ORDER BY course, semester, subject",
                    (user["userId"],),
                )
                classes = cursor.fetchall()
            finally:
                cursor.close()
        return jsonify(classes=classes)

    @app.put("/api/faculty/<int:user_id>/assignments")
    def replace_faculty_assignments(user_id):
        require_roles("admin")
        require_csrf()
        body = json_body()
        assignments = body.get("assignments")
        if not isinstance(assignments, list):
            raise ApiError(400, "Assignments must be a list.")
        normalized = []
        seen = set()
        for assignment in assignments:
            if not isinstance(assignment, dict):
                raise ApiError(400, "Each assignment must include a department, semester, and subject.")
            course = text_field(assignment, "course", "Department", 99)
            semester = valid_semester(assignment.get("semester"))
            subject = text_field(assignment, "subject", "Subject", 99)
            if course not in DEPARTMENTS:
                raise ApiError(400, "Select a valid course or department.")
            key = (course, semester, subject)
            if key not in seen:
                normalized.append(key)
                seen.add(key)
        with database_connection() as connection:
            cursor = connection.cursor()
            try:
                cursor.execute(
                    "SELECT 1 FROM users WHERE user_id = %s AND role = 'faculty'",
                    (user_id,),
                )
                if cursor.fetchone() is None:
                    raise ApiError(404, "Faculty account not found.")
                for course, semester, subject in normalized:
                    cursor.execute(
                        "SELECT 1 FROM course_semester_subjects "
                        "WHERE course = %s AND semester = %s AND subject = %s",
                        (course, semester, subject),
                    )
                    if cursor.fetchone() is None:
                        raise ApiError(400, "Select a subject available for that department and semester.")
                cursor.execute(
                    "DELETE FROM faculty_assignments WHERE user_id = %s",
                    (user_id,),
                )
                if normalized:
                    cursor.executemany(
                        "INSERT INTO faculty_assignments (user_id, course, semester, subject) "
                        "VALUES (%s, %s, %s, %s)",
                        [(user_id, course, semester, subject)
                         for course, semester, subject in normalized],
                    )
                connection.commit()
            finally:
                cursor.close()
        return jsonify(userId=user_id, assignments=[
            {"course": course, "semester": semester, "subject": subject}
            for course, semester, subject in normalized
        ])

    @app.get("/api/subjects")
    def list_subjects():
        user = require_roles("faculty")
        course = request.args.get("course", "").strip()
        semester_text = request.args.get("semester", "").strip()
        if course not in DEPARTMENTS or not semester_text.isdigit():
            raise ApiError(400, "Select a valid department and semester.")
        semester = int(semester_text)
        if not 1 <= semester <= MAX_SEMESTER:
            raise ApiError(400, "Select a valid department and semester.")
        with database_connection() as connection:
            cursor = connection.cursor()
            try:
                require_faculty_assignment(cursor, user, course, semester)
                cursor.execute(
                    "SELECT css.subject FROM course_semester_subjects AS css "
                    "JOIN faculty_assignments AS fa "
                    "ON fa.course = css.course AND fa.semester = css.semester "
                    "AND fa.subject = css.subject "
                    "WHERE fa.user_id = %s AND css.course = %s AND css.semester = %s "
                    "ORDER BY css.subject",
                    (user["userId"], course, semester),
                )
                subjects = [row[0] for row in cursor.fetchall()]
            finally:
                cursor.close()
        return jsonify(subjects=subjects)

    @app.get("/api/students/search")
    def search_students():
        require_roles("admin", "faculty")
        query = request.args.get("q", "").strip()
        if not query:
            raise ApiError(400, "Please enter a Student ID or name to search.")
        if len(query) > 100:
            raise ApiError(400, "Search text is too long.")
        course = request.args.get("course", "").strip()
        semester = request.args.get("semester", "").strip()
        if len(course) > 99:
            raise ApiError(400, "Course filter is too long.")
        conditions = [
            "(LOCATE(LOWER(%s), LOWER(student_id)) > 0 "
            "OR LOCATE(LOWER(%s), LOWER(name)) > 0)"
        ]
        values = [query, query]
        user = current_user()
        if user["role"] == "faculty":
            conditions.append(
                "EXISTS (SELECT 1 FROM faculty_assignments fa "
                "WHERE fa.user_id = %s AND fa.course = students.course "
                "AND fa.semester = students.semester)"
            )
            values.append(user["userId"])
        if course:
            conditions.append("course = %s")
            values.append(course)
        if semester:
            if not semester.isdigit() or not 1 <= int(semester) <= MAX_SEMESTER:
                raise ApiError(400, f"Semester must be a whole number from 1 to {MAX_SEMESTER}.")
            conditions.append("semester = %s")
            values.append(int(semester))
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute(
                    "SELECT student_id AS id, name, contact_details AS contactDetails, "
                    "course, semester, DATE_FORMAT(created_at, '%Y-%m-%d') AS createdAt FROM students "
                    f"WHERE {' AND '.join(conditions)} ORDER BY name, student_id",
                    tuple(values),
                )
                students = cursor.fetchall()
            finally:
                cursor.close()
        return jsonify(students=students)

    @app.get("/api/students/export")
    def export_students():
        user = require_roles("admin", "faculty")
        course = request.args.get("course", "").strip()
        semester = request.args.get("semester", "").strip()
        search = request.args.get("q", "").strip()
        if len(course) > 99 or len(search) > 100:
            raise ApiError(400, "Search text or course filter is too long.")
        conditions = []
        values = []
        if user["role"] == "faculty":
            conditions.append(
                "EXISTS (SELECT 1 FROM faculty_assignments fa "
                "WHERE fa.user_id = %s AND fa.course = students.course "
                "AND fa.semester = students.semester)"
            )
            values.append(user["userId"])
        if course:
            conditions.append("course = %s")
            values.append(course)
        if semester:
            if not semester.isdigit() or not 1 <= int(semester) <= MAX_SEMESTER:
                raise ApiError(400, f"Semester must be a whole number from 1 to {MAX_SEMESTER}.")
            conditions.append("semester = %s")
            values.append(int(semester))
        if search:
            conditions.append(
                "(LOCATE(LOWER(%s), LOWER(student_id)) > 0 "
                "OR LOCATE(LOWER(%s), LOWER(name)) > 0)"
            )
            values.extend((search, search))
        where = f" WHERE {' AND '.join(conditions)}" if conditions else ""
        with database_connection() as connection:
            cursor = connection.cursor()
            try:
                cursor.execute(
                    "SELECT student_id, name, contact_details, course, semester "
                    f"FROM students{where} ORDER BY name, student_id",
                    tuple(values),
                )
                records = cursor.fetchall()
            finally:
                cursor.close()
        output = StringIO()
        writer = csv.writer(output)
        writer.writerow(["Student ID", "Name", "Contact Details", "Course / Department", "Semester"])
        writer.writerows(records)
        return Response(
            output.getvalue(),
            mimetype="text/csv",
            headers={"Content-Disposition": "attachment; filename=student-records.csv"},
        )

    @app.get("/api/students/<student_id>")
    def get_student(student_id):
        user = current_user()
        if user["role"] == "student":
            student_allowed(user, student_id)
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute(
                    "SELECT course, semester FROM students WHERE student_id = %s",
                    (student_id,),
                )
                scope = cursor.fetchone()
                if not scope:
                    raise ApiError(404, "Student record not found.")
                if user["role"] == "faculty":
                    require_faculty_assignment(cursor, user, scope["course"], scope["semester"])
                cursor.execute(
                    "SELECT student_id AS id, name, contact_details AS contactDetails, "
                    "course, semester, DATE_FORMAT(created_at, '%Y-%m-%d') AS createdAt "
                    "FROM students WHERE student_id = %s",
                    (student_id,),
                )
                student = cursor.fetchone()
                if student and user["role"] == "admin":
                    cursor.execute(
                        "SELECT username FROM users "
                        "WHERE student_id = %s AND role = 'student'",
                        (student_id,),
                    )
                    account = cursor.fetchone()
                    student["loginUsername"] = account["username"] if account else ""
            finally:
                cursor.close()
        if not student:
            raise ApiError(404, "Student record not found.")
        return jsonify(student=student)

    @app.post("/api/students")
    def create_student():
        require_roles("admin")
        require_csrf()
        body = json_body()
        student_id = text_field(body, "id", "Student ID", 31)
        name = text_field(body, "name", "Name", 99)
        contact_details = text_field(body, "contactDetails", "Contact details", 255)
        course = text_field(body, "course", "Course / department", 99)
        semester = valid_semester(body.get("semester"))
        username = text_field(body, "username", "Login username", 63)
        password = valid_login_password(body.get("password"))
        created_at = date.today()
        if course not in DEPARTMENTS:
            raise ApiError(400, "Please select a valid course or department.")

        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute("SELECT 1 FROM courses WHERE name = %s", (course,))
                if cursor.fetchone() is None:
                    raise ApiError(400, "Please select a valid course or department.")
                cursor.execute("SELECT 1 FROM students WHERE student_id = %s", (student_id,))
                if cursor.fetchone() is not None:
                    raise ApiError(409, "Student ID already exists.")
                cursor.execute("SELECT 1 FROM users WHERE username = %s", (username,))
                if cursor.fetchone() is not None:
                    raise ApiError(409, "Login username is already in use.")
                cursor.execute(
                    "INSERT INTO students "
                    "(student_id, name, contact_details, course, semester) "
                    "VALUES (%s, %s, %s, %s, %s)",
                    (student_id, name, contact_details, course, semester),
                )
                cursor.execute(
                    "INSERT INTO users (username, password_hash, role, student_id) "
                    "VALUES (%s, %s, 'student', %s)",
                    (username, generate_password_hash(password), student_id),
                )
                connection.commit()
            except mysql.connector.Error as error:
                if getattr(error, "errno", None) == 1062:
                    raise ApiError(409, "Student ID or login username already exists.") from None
                raise
            finally:
                cursor.close()
        return jsonify(
            student={
                "id": student_id,
                "name": name,
                "contactDetails": contact_details,
                "course": course,
                "semester": semester,
                "loginUsername": username,
                "createdAt": created_at.isoformat(),
            }
        ), 201

    @app.put("/api/students/<student_id>")
    def update_student(student_id):
        require_roles("admin")
        require_csrf()
        body = json_body()
        new_id = text_field(body, "id", "Student ID", 31)
        name = text_field(body, "name", "Name", 99)
        contact_details = text_field(body, "contactDetails", "Contact details", 255)
        course = text_field(body, "course", "Course / department", 99)
        semester = valid_semester(body.get("semester"))
        username = text_field(body, "username", "Login username", 63)
        password = body.get("password", "")
        if password:
            password = valid_login_password(password)
        if course not in DEPARTMENTS:
            raise ApiError(400, "Please select a valid course or department.")

        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute("SELECT 1 FROM students WHERE student_id = %s", (student_id,))
                if cursor.fetchone() is None:
                    raise ApiError(404, "Student record not found.")
                cursor.execute(
                    "SELECT username FROM users WHERE student_id = %s AND role = 'student'",
                    (student_id,),
                )
                account = cursor.fetchone()
                if not account and not password:
                    raise ApiError(400, "A password is required to create this student's login.")
                cursor.execute(
                    "SELECT 1 FROM users WHERE username = %s "
                    "AND (student_id IS NULL OR student_id <> %s)",
                    (username, student_id),
                )
                if cursor.fetchone() is not None:
                    raise ApiError(409, "Login username is already in use.")
                cursor.execute("SELECT 1 FROM courses WHERE name = %s", (course,))
                if cursor.fetchone() is None:
                    raise ApiError(400, "Please select a valid course or department.")
                cursor.execute(
                    "UPDATE students SET student_id = %s, name = %s, "
                    "contact_details = %s, course = %s, semester = %s "
                    "WHERE student_id = %s",
                    (new_id, name, contact_details, course, semester, student_id),
                )
                if account:
                    if password:
                        cursor.execute(
                            "UPDATE users SET username = %s, password_hash = %s "
                            "WHERE student_id = %s AND role = 'student'",
                            (username, generate_password_hash(password), new_id),
                        )
                    else:
                        cursor.execute(
                            "UPDATE users SET username = %s "
                            "WHERE student_id = %s AND role = 'student'",
                            (username, new_id),
                        )
                else:
                    cursor.execute(
                        "INSERT INTO users (username, password_hash, role, student_id) "
                        "VALUES (%s, %s, 'student', %s)",
                        (username, generate_password_hash(password), new_id),
                    )
                connection.commit()
            except mysql.connector.Error as error:
                if getattr(error, "errno", None) == 1062:
                    raise ApiError(409, "Student ID already exists.") from None
                raise
            finally:
                cursor.close()
        return jsonify(student={
            "id": new_id,
            "name": name,
            "contactDetails": contact_details,
            "course": course,
            "semester": semester,
            "loginUsername": username,
        })

    @app.delete("/api/students/<student_id>")
    def delete_student(student_id):
        require_roles("admin")
        require_csrf()
        with database_connection() as connection:
            cursor = connection.cursor()
            try:
                cursor.execute("DELETE FROM students WHERE student_id = %s", (student_id,))
                if cursor.rowcount == 0:
                    raise ApiError(404, "Student record not found.")
                connection.commit()
            finally:
                cursor.close()
        return jsonify(ok=True)

    @app.get("/api/students/<student_id>/grades")
    def list_grades(student_id):
        user = require_roles("faculty", "student")
        student_allowed(user, student_id)
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                if user["role"] == "faculty":
                    cursor.execute(
                        "SELECT course, semester FROM students WHERE student_id = %s",
                        (student_id,),
                    )
                    student = cursor.fetchone()
                    if not student:
                        raise ApiError(404, "Student record not found.")
                    require_faculty_assignment(cursor, user, student["course"], student["semester"])
                cursor.execute(
                    "SELECT grade_id AS id, subject, component, marks, "
                    "COALESCE((SELECT reason FROM grade_change_history "
                    "WHERE grade_id = grades.grade_id "
                    "ORDER BY changed_at DESC, change_id DESC LIMIT 1), '') "
                    "AS lastChangeReason, "
                    "DATE_FORMAT(updated_at, '%Y-%m-%d') AS updatedAt "
                    "FROM grades WHERE student_id = %s "
                    + (
                        "AND EXISTS (SELECT 1 FROM faculty_assignments fa "
                        "WHERE fa.user_id = %s AND fa.course = grades.course "
                        "AND fa.semester = grades.semester AND fa.subject = grades.subject) "
                        if user["role"] == "faculty" else ""
                    )
                    + "ORDER BY subject",
                    (student_id, user["userId"]) if user["role"] == "faculty" else (student_id,),
                )
                grades = cursor.fetchall()
            finally:
                cursor.close()
        return jsonify(grades=grades)

    @app.post("/api/students/<student_id>/grades")
    def create_grade(student_id):
        user = require_roles("faculty")
        require_csrf()
        body = json_body()
        subject = text_field(body, "subject", "Subject", 99)
        component = valid_grade_component(body.get("component"))
        marks = valid_marks(body.get("marks"))
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute(
                    "SELECT course, semester FROM students WHERE student_id = %s",
                    (student_id,),
                )
                student = cursor.fetchone()
                if not student:
                    raise ApiError(404, "Student or subject not found.")
                cursor.execute(
                    "SELECT 1 FROM course_semester_subjects "
                    "WHERE course = %s AND semester = %s AND subject = %s",
                    (student["course"], student["semester"], subject),
                )
                if cursor.fetchone() is None:
                    raise ApiError(404, "Student or subject not found.")
                require_faculty_assignment(
                    cursor, user, student["course"], student["semester"], subject
                )
                require_open_grade_window(
                    cursor, student["course"], student["semester"], component
                )
                cursor.execute(
                    "INSERT INTO grades "
                    "(student_id, course, semester, subject, component, marks) "
                    "VALUES (%s, %s, %s, %s, %s, %s)",
                    (student_id, student["course"], student["semester"], subject, component, marks),
                )
                grade_id = cursor.lastrowid
                connection.commit()
            except mysql.connector.Error as error:
                if getattr(error, "errno", None) == 1062:
                    raise ApiError(409, "A grade for this student and subject already exists.") from None
                if getattr(error, "errno", None) == 1452:
                    raise ApiError(404, "Student or subject not found.") from None
                raise
            finally:
                cursor.close()
        return jsonify(
            id=grade_id,
            studentId=student_id,
            subject=subject,
            component=component,
            marks=marks,
        ), 201

    @app.put("/api/grades/<int:grade_id>")
    def update_grade(grade_id):
        user = require_roles("faculty")
        require_csrf()
        body = json_body()
        subject = text_field(body, "subject", "Subject", 99)
        marks = valid_marks(body.get("marks"))
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute(
                    "SELECT student_id, course, semester, subject, component, marks "
                    "FROM grades WHERE grade_id = %s",
                    (grade_id,),
                )
                grade = cursor.fetchone()
                if grade is None:
                    raise ApiError(404, "Grade record not found.")
                require_faculty_assignment(
                    cursor, user, grade["course"], grade["semester"], grade["subject"]
                )
                component = valid_grade_component(
                    body.get("component", grade["component"])
                )
                marks = valid_marks(body.get("marks"))
                reason = text_field(body, "reason", "Reason for grade change", 500)
                if subject != grade["subject"]:
                    raise ApiError(400, "A grade's subject cannot be changed.")
                if component != grade["component"]:
                    raise ApiError(400, "A grade's assessment cannot be changed.")
                require_open_grade_window(
                    cursor, grade["course"], grade["semester"], component
                )
                cursor.execute(
                    "UPDATE grades SET marks = %s WHERE grade_id = %s",
                    (marks, grade_id),
                )
                cursor.execute(
                    "INSERT INTO grade_change_history "
                    "(grade_id, old_marks, new_marks, reason, changed_by) "
                    "VALUES (%s, %s, %s, %s, %s)",
                    (grade_id, grade["marks"], marks, reason, user["userId"]),
                )
                connection.commit()
            except mysql.connector.Error as error:
                if getattr(error, "errno", None) == 1062:
                    raise ApiError(409, "A grade for this student and subject already exists.") from None
                if getattr(error, "errno", None) == 1452:
                    raise ApiError(404, "Student or subject not found.") from None
                raise
            finally:
                cursor.close()
        return jsonify(
            id=grade_id,
            subject=subject,
            component=component,
            marks=marks,
            reason=reason,
        )

    @app.get("/api/students/<student_id>/attendance")
    def list_attendance(student_id):
        user = require_roles("faculty", "student")
        student_allowed(user, student_id)
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                if user["role"] == "faculty":
                    cursor.execute(
                        "SELECT course, semester FROM students WHERE student_id = %s",
                        (student_id,),
                    )
                    student = cursor.fetchone()
                    if not student:
                        raise ApiError(404, "Student record not found.")
                    require_faculty_assignment(cursor, user, student["course"], student["semester"])
                cursor.execute(
                    "SELECT attendance_id AS id, subject, "
                    "DATE_FORMAT(attendance_date, '%Y-%m-%d') AS date, status "
                    "FROM attendance WHERE student_id = %s "
                    + (
                        "AND EXISTS (SELECT 1 FROM students s "
                        "JOIN faculty_assignments fa ON fa.course = s.course "
                        "AND fa.semester = s.semester "
                        "WHERE s.student_id = attendance.student_id "
                        "AND fa.user_id = %s AND fa.subject = attendance.subject) "
                        if user["role"] == "faculty" else ""
                    )
                    + "ORDER BY attendance_date DESC, subject",
                    (student_id, user["userId"]) if user["role"] == "faculty" else (student_id,),
                )
                records = cursor.fetchall()
            finally:
                cursor.close()
        return jsonify(attendance=records)

    @app.post("/api/students/<student_id>/attendance")
    def create_attendance(student_id):
        user = require_roles("faculty")
        require_csrf()
        body = json_body()
        subject = text_field(body, "subject", "Subject", 99)
        attendance_date = valid_iso_date(body.get("date"))
        status = body.get("status")
        if not isinstance(status, str) or status not in ATTENDANCE_STATUSES:
            raise ApiError(400, "Status must be Present, Absent, or Late.")
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute(
                    "SELECT course, semester FROM students WHERE student_id = %s",
                    (student_id,),
                )
                student = cursor.fetchone()
                if not student:
                    raise ApiError(404, "Student record not found.")
                require_faculty_assignment(
                    cursor, user, student["course"], student["semester"], subject
                )
                cursor.execute(
                    "SELECT 1 FROM course_semester_subjects "
                    "WHERE course = %s AND semester = %s AND subject = %s",
                    (student["course"], student["semester"], subject),
                )
                if cursor.fetchone() is None:
                    raise ApiError(404, "Student or subject not found.")
                cursor.execute(
                    "INSERT INTO attendance (student_id, subject, attendance_date, status) "
                    "VALUES (%s, %s, %s, %s)",
                    (student_id, subject, attendance_date, status),
                )
                record_id = cursor.lastrowid
                connection.commit()
            except mysql.connector.Error as error:
                check_duplicate(error, "Attendance for this subject and date already exists.")
            finally:
                cursor.close()
        return jsonify(
            id=record_id,
            studentId=student_id,
            subject=subject,
            date=attendance_date.isoformat(),
            status=status,
        ), 201

    @app.put("/api/attendance/<int:attendance_id>")
    def update_attendance(attendance_id):
        user = require_roles("faculty")
        require_csrf()
        body = json_body()
        subject = text_field(body, "subject", "Subject", 99)
        attendance_date = valid_iso_date(body.get("date"))
        status = body.get("status")
        if not isinstance(status, str) or status not in ATTENDANCE_STATUSES:
            raise ApiError(400, "Status must be Present, Absent, or Late.")
        with database_connection() as connection:
            cursor = connection.cursor(dictionary=True)
            try:
                cursor.execute(
                    "SELECT student_id, subject FROM attendance WHERE attendance_id = %s",
                    (attendance_id,),
                )
                attendance = cursor.fetchone()
                if not attendance:
                    raise ApiError(404, "Attendance record not found.")
                cursor.execute(
                    "SELECT course, semester FROM students WHERE student_id = %s",
                    (attendance["student_id"],),
                )
                student = cursor.fetchone()
                if not student:
                    raise ApiError(404, "Student record not found.")
                require_faculty_assignment(
                    cursor, user, student["course"], student["semester"], attendance["subject"]
                )
                if subject != attendance["subject"]:
                    raise ApiError(400, "An attendance record's subject cannot be changed.")
                cursor.execute(
                    "SELECT 1 FROM course_semester_subjects "
                    "WHERE course = %s AND semester = %s AND subject = %s",
                    (student["course"], student["semester"], subject),
                )
                if cursor.fetchone() is None:
                    raise ApiError(404, "Student or subject not found.")
                cursor.execute(
                    "UPDATE attendance SET subject = %s, attendance_date = %s, status = %s "
                    "WHERE attendance_id = %s",
                    (subject, attendance_date, status, attendance_id),
                )
                if cursor.rowcount == 0:
                    raise ApiError(404, "Attendance record not found.")
                connection.commit()
            except mysql.connector.Error as error:
                check_duplicate(error, "Attendance for this subject and date already exists.")
            finally:
                cursor.close()
        return jsonify(
            id=attendance_id,
            subject=subject,
            date=attendance_date.isoformat(),
            status=status,
        )

    return app


app = create_app()


if __name__ == "__main__":
    app.run(
        host=os.getenv("HOST", "127.0.0.1"),
        port=int(os.getenv("PORT", "5000")),
        debug=False,
    )
