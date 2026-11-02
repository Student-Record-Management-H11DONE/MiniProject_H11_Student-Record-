"""Interactively add a user account without storing a plain-text password."""

import getpass

import mysql.connector
from werkzeug.security import generate_password_hash

from backend.app import app, database_connection


def create_user():
    username = input("Username: ").strip()
    if not username or len(username) > 63 or any(ord(char) < 32 for char in username):
        raise ValueError("Username must contain 1 to 63 printable characters.")

    role_options = {
        "1": "admin",
        "2": "faculty",
        "3": "student",
    }
    print("Role: 1) Administrator  2) Faculty  3) Student")
    role = role_options.get(input("Choose role: ").strip())
    if role is None:
        raise ValueError("Choose role 1, 2, or 3.")

    student_id = None
    if role == "student":
        student_id = input("Linked student ID: ").strip()
        if not student_id or len(student_id) > 31:
            raise ValueError("Student ID must contain 1 to 31 characters.")

    password = getpass.getpass("Password (at least 12 characters): ")
    if len(password) < 12:
        raise ValueError("Password must contain at least 12 characters.")
    confirmation = getpass.getpass("Confirm password: ")
    if password != confirmation:
        raise ValueError("Passwords do not match.")

    with app.app_context():
        with database_connection() as connection:
            cursor = connection.cursor()
            try:
                cursor.execute(
                    "INSERT INTO users (username, password_hash, role, student_id) "
                    "VALUES (%s, %s, %s, %s)",
                    (username, generate_password_hash(password), role, student_id),
                )
                connection.commit()
            except mysql.connector.IntegrityError as error:
                if error.errno == 1062:
                    raise ValueError("That username or student account already exists.") from None
                if error.errno == 1452:
                    raise ValueError("Create the linked student record before its account.") from None
                raise
            finally:
                cursor.close()


if __name__ == "__main__":
    try:
        create_user()
    except (ValueError, mysql.connector.Error) as error:
        raise SystemExit(str(error)) from None
    print("User account created.")
