"""Create sample records and demo accounts in an initialized MySQL database."""

from datetime import date, timedelta

from werkzeug.security import generate_password_hash

from backend.app import DEPARTMENTS, app, database_connection


STUDENTS = [
    ("S001", "Aarav Mehta", "aarav@example.edu", "Computer Science", 2, date.today() - timedelta(days=3)),
    ("S002", "Diya Sharma", "diya@example.edu", "Information Technology", 3, date.today() - timedelta(days=6)),
    ("S003", "Kabir Nair", "kabir@example.edu", "Electronics", 1, date.today() - timedelta(days=9)),
    ("S004", "Ananya Iyer", "ananya@example.edu", "Computer Science", 4, date.today() - timedelta(days=12)),
    ("S005", "Rohan Kapoor", "rohan@example.edu", "Business Administration", 2, date.today() - timedelta(days=16)),
    ("S006", "Mira Das", "mira@example.edu", "Information Technology", 1, date.today() - timedelta(days=20)),
]

COURSES = list(DEPARTMENTS)
COURSE_CODES = {
    "Business Administration": "BA",
    "Computer Science": "CS",
    "Electronics": "EL",
    "Information Technology": "IT",
}
SUBJECTS = [
    ("Business Administration", 1, "Principles of Management"),
    ("Business Administration", 1, "Financial Accounting I"),
    ("Business Administration", 1, "Microeconomics"),
    ("Business Administration", 1, "Business Communication"),
    ("Business Administration", 2, "Organizational Behaviour"),
    ("Business Administration", 2, "Financial Accounting II"),
    ("Business Administration", 2, "Macroeconomics"),
    ("Business Administration", 2, "Business Statistics"),
    ("Business Administration", 3, "Marketing Management"),
    ("Business Administration", 3, "Cost Accounting"),
    ("Business Administration", 3, "Business Law"),
    ("Business Administration", 3, "Operations Management"),
    ("Business Administration", 4, "Human Resource Management"),
    ("Business Administration", 4, "Corporate Finance"),
    ("Business Administration", 4, "Consumer Behaviour"),
    ("Business Administration", 4, "Management Information Systems"),
    ("Business Administration", 5, "Strategic Management"),
    ("Business Administration", 5, "Management Accounting"),
    ("Business Administration", 5, "Sales Management"),
    ("Business Administration", 5, "Entrepreneurship"),
    ("Business Administration", 6, "International Business"),
    ("Business Administration", 6, "Investment Analysis"),
    ("Business Administration", 6, "Supply Chain Management"),
    ("Business Administration", 6, "Business Research Methods"),
    ("Business Administration", 7, "Business Policy"),
    ("Business Administration", 7, "Project Management"),
    ("Business Administration", 7, "Digital Marketing"),
    ("Business Administration", 7, "Corporate Governance"),
    ("Business Administration", 8, "Business Analytics"),
    ("Business Administration", 8, "Business Ethics"),
    ("Business Administration", 8, "Global Business Strategy"),
    ("Business Administration", 8, "Capstone Project"),
    ("Computer Science", 1, "Programming Fundamentals"),
    ("Computer Science", 1, "Discrete Mathematics"),
    ("Computer Science", 1, "Digital Logic Design"),
    ("Computer Science", 1, "Computing Systems"),
    ("Computer Science", 2, "Object-Oriented Programming"),
    ("Computer Science", 2, "Data Structures"),
    ("Computer Science", 2, "Computer Organization"),
    ("Computer Science", 2, "Linear Algebra for Computing"),
    ("Computer Science", 3, "Algorithms"),
    ("Computer Science", 3, "Database Systems"),
    ("Computer Science", 3, "Operating Systems"),
    ("Computer Science", 3, "Probability for Computing"),
    ("Computer Science", 4, "Computer Networks"),
    ("Computer Science", 4, "Theory of Computation"),
    ("Computer Science", 4, "Software Engineering"),
    ("Computer Science", 4, "Programming Languages"),
    ("Computer Science", 5, "Compiler Design"),
    ("Computer Science", 5, "Artificial Intelligence"),
    ("Computer Science", 5, "Distributed Systems"),
    ("Computer Science", 5, "Information Security"),
    ("Computer Science", 6, "Machine Learning"),
    ("Computer Science", 6, "Cloud Computing"),
    ("Computer Science", 6, "Computer Graphics"),
    ("Computer Science", 6, "Advanced Database Systems"),
    ("Computer Science", 7, "Deep Learning"),
    ("Computer Science", 7, "Cybersecurity"),
    ("Computer Science", 7, "Parallel Computing"),
    ("Computer Science", 7, "Technology Project Management"),
    ("Computer Science", 8, "Advanced Algorithms"),
    ("Computer Science", 8, "Data Mining"),
    ("Computer Science", 8, "Ethics in Computing"),
    ("Computer Science", 8, "Computer Science Capstone"),
    ("Electronics", 1, "Electronic Devices"),
    ("Electronics", 1, "Circuit Analysis I"),
    ("Electronics", 1, "Engineering Mathematics I"),
    ("Electronics", 1, "Electrical Workshop"),
    ("Electronics", 2, "Analog Circuits"),
    ("Electronics", 2, "Circuit Analysis II"),
    ("Electronics", 2, "Signals and Systems"),
    ("Electronics", 2, "Engineering Materials"),
    ("Electronics", 3, "Digital Electronics"),
    ("Electronics", 3, "Network Theory"),
    ("Electronics", 3, "Electromagnetic Fields"),
    ("Electronics", 3, "Electronic Measurements"),
    ("Electronics", 4, "Microprocessors"),
    ("Electronics", 4, "Communication Systems I"),
    ("Electronics", 4, "Control Systems"),
    ("Electronics", 4, "VLSI Fundamentals"),
    ("Electronics", 5, "Embedded Systems"),
    ("Electronics", 5, "Communication Systems II"),
    ("Electronics", 5, "Digital Signal Processing"),
    ("Electronics", 5, "Power Electronics"),
    ("Electronics", 6, "Wireless Communication"),
    ("Electronics", 6, "Microcontroller Applications"),
    ("Electronics", 6, "Optical Communication"),
    ("Electronics", 6, "Antenna Engineering"),
    ("Electronics", 7, "Advanced VLSI Design"),
    ("Electronics", 7, "Internet of Things"),
    ("Electronics", 7, "Radar and Satellite Systems"),
    ("Electronics", 7, "Electronic Product Design"),
    ("Electronics", 8, "5G Communication Systems"),
    ("Electronics", 8, "Robotics and Automation"),
    ("Electronics", 8, "Semiconductor Technology"),
    ("Electronics", 8, "Electronics Capstone"),
    ("Information Technology", 1, "Web Technologies I"),
    ("Information Technology", 1, "Information Systems"),
    ("Information Technology", 1, "IT Mathematics"),
    ("Information Technology", 1, "Computer Applications"),
    ("Information Technology", 2, "Web Technologies II"),
    ("Information Technology", 2, "Data Management"),
    ("Information Technology", 2, "Network Essentials"),
    ("Information Technology", 2, "Scripting for IT"),
    ("Information Technology", 3, "Web Application Development"),
    ("Information Technology", 3, "Systems Administration"),
    ("Information Technology", 3, "Database Administration"),
    ("Information Technology", 3, "IT Service Management"),
    ("Information Technology", 4, "Mobile Application Development"),
    ("Information Technology", 4, "Network Administration"),
    ("Information Technology", 4, "Enterprise Systems"),
    ("Information Technology", 4, "Information Assurance"),
    ("Information Technology", 5, "Cloud Infrastructure"),
    ("Information Technology", 5, "Full-Stack Development"),
    ("Information Technology", 5, "Data Warehousing"),
    ("Information Technology", 5, "IT Project Management"),
    ("Information Technology", 6, "Cloud Services"),
    ("Information Technology", 6, "Network Security"),
    ("Information Technology", 6, "Business Intelligence"),
    ("Information Technology", 6, "DevOps Practices"),
    ("Information Technology", 7, "Enterprise Application Integration"),
    ("Information Technology", 7, "Applied Cybersecurity"),
    ("Information Technology", 7, "Big Data Platforms"),
    ("Information Technology", 7, "IT Governance"),
    ("Information Technology", 8, "Cloud Architecture"),
    ("Information Technology", 8, "Digital Transformation"),
    ("Information Technology", 8, "IT Strategy and Audit"),
    ("Information Technology", 8, "Information Technology Capstone"),
]


def generate_sample_students():
    students = []
    for course in COURSES:
        code = COURSE_CODES[course]
        for semester in range(1, 9):
            for number in range(1, 21):
                student_id = f"DEMO-{code}-S{semester:02d}-{number:02d}"
                students.append(
                    (
                        student_id,
                        f"{course} Student {semester:02d}-{number:02d}",
                        f"{student_id.lower()}@example.edu",
                        course,
                        semester,
                        date.today(),
                    )
                )
    return students


ACCOUNTS = [
    ("admin", "admin123", "admin", None),
    ("faculty1", "faculty123", "faculty", None),
    *[
        (f"faculty{number}", f"Faculty{number}Demo!2026", "faculty", None)
        for number in range(2, 10)
    ],
    ("student1", "student123", "student", "S001"),
    ("student2", "student234", "student", "S002"),
]


def seed_demo_data():
    all_students = STUDENTS + generate_sample_students()
    with app.app_context():
        with database_connection() as connection:
            cursor = connection.cursor()
            try:
                cursor.executemany(
                    "INSERT IGNORE INTO courses (name) VALUES (%s)",
                    [(course,) for course in COURSES],
                )
                cursor.executemany(
                    "INSERT IGNORE INTO course_semester_subjects "
                    "(course, semester, subject) VALUES (%s, %s, %s)",
                    SUBJECTS,
                )
                cursor.executemany(
                    "INSERT IGNORE INTO students "
                    "(student_id, name, contact_details, course, semester, created_at) "
                    "VALUES (%s, %s, %s, %s, %s, %s)",
                    all_students,
                )
                cursor.executemany(
                    "UPDATE students SET contact_details = %s, semester = %s "
                    "WHERE student_id = %s AND contact_details = ''",
                    [(row[2], row[4], row[0]) for row in STUDENTS],
                )
                for username, password, role, student_id in ACCOUNTS:
                    cursor.execute(
                        "INSERT IGNORE INTO users (username, password_hash, role, student_id) "
                        "VALUES (%s, %s, %s, %s)",
                        (username, generate_password_hash(password), role, student_id),
                    )
                connection.commit()
            finally:
                cursor.close()


if __name__ == "__main__":
    seed_demo_data()
    print("Demo records and accounts are ready, including 20 students per department and semester.")
