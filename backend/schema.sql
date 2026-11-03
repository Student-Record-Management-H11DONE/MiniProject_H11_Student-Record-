CREATE DATABASE IF NOT EXISTS student_records
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_0900_ai_ci;

USE student_records;

CREATE TABLE IF NOT EXISTS courses (
  name VARCHAR(99) NOT NULL,
  PRIMARY KEY (name)
) ENGINE=InnoDB;

INSERT IGNORE INTO courses (name) VALUES
  ('Business Administration'),
  ('Computer Science'),
  ('Electronics'),
  ('Information Technology');

CREATE TABLE IF NOT EXISTS course_semester_subjects (
  course VARCHAR(99) NOT NULL,
  semester TINYINT UNSIGNED NOT NULL,
  subject VARCHAR(99) NOT NULL,
  PRIMARY KEY (course, semester, subject),
  CONSTRAINT chk_course_semester_subjects_semester CHECK (semester BETWEEN 1 AND 8),
  CONSTRAINT fk_course_semester_subjects_course
    FOREIGN KEY (course) REFERENCES courses (name)
    ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS students (
  student_id VARCHAR(31) NOT NULL,
  name VARCHAR(99) NOT NULL,
  contact_details VARCHAR(255) NOT NULL,
  course VARCHAR(99) NOT NULL,
  semester TINYINT UNSIGNED NOT NULL,
  created_at DATE NOT NULL DEFAULT (CURRENT_DATE),
  PRIMARY KEY (student_id),
  INDEX idx_students_name (name),
  INDEX idx_students_course_semester (course, semester),
  UNIQUE KEY uq_students_id_course_semester (student_id, course, semester),
  CONSTRAINT chk_students_semester CHECK (semester BETWEEN 1 AND 8),
  CONSTRAINT fk_students_course
    FOREIGN KEY (course) REFERENCES courses (name)
    ON UPDATE CASCADE ON DELETE RESTRICT
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS users (
  user_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  username VARCHAR(63) NOT NULL,
  password_hash VARCHAR(255) NOT NULL,
  role ENUM('admin', 'faculty', 'student') NOT NULL,
  student_id VARCHAR(31) NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (user_id),
  UNIQUE KEY uq_users_username (username),
  UNIQUE KEY uq_users_student (student_id),
  CONSTRAINT fk_users_student
    FOREIGN KEY (student_id) REFERENCES students (student_id)
    ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS faculty_assignments (
  user_id BIGINT UNSIGNED NOT NULL,
  course VARCHAR(99) NOT NULL,
  semester TINYINT UNSIGNED NOT NULL,
  subject VARCHAR(99) NOT NULL,
  PRIMARY KEY (user_id, course, semester, subject),
  INDEX idx_faculty_assignments_course_semester (course, semester),
  CONSTRAINT chk_faculty_assignments_semester CHECK (semester BETWEEN 1 AND 8),
  CONSTRAINT fk_faculty_assignments_user
    FOREIGN KEY (user_id) REFERENCES users (user_id)
    ON UPDATE CASCADE ON DELETE CASCADE,
  CONSTRAINT fk_faculty_assignments_course
    FOREIGN KEY (course) REFERENCES courses (name)
    ON UPDATE CASCADE ON DELETE CASCADE,
  CONSTRAINT fk_faculty_assignments_subject
    FOREIGN KEY (course, semester, subject)
    REFERENCES course_semester_subjects (course, semester, subject)
    ON UPDATE RESTRICT ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS grades (
  grade_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  student_id VARCHAR(31) NOT NULL,
  course VARCHAR(99) NOT NULL,
  semester TINYINT UNSIGNED NOT NULL,
  subject VARCHAR(99) NOT NULL,
  component ENUM('ISA1', 'ISA2', 'ESA') NOT NULL,
  marks TINYINT UNSIGNED NOT NULL,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (grade_id),
  UNIQUE KEY uq_grades_student_subject_component (student_id, subject, component),
  CONSTRAINT chk_grades_marks CHECK (marks BETWEEN 0 AND 100),
  CONSTRAINT fk_grades_student_course_semester
    FOREIGN KEY (student_id, course, semester)
    REFERENCES students (student_id, course, semester)
    ON UPDATE CASCADE ON DELETE CASCADE,
  CONSTRAINT fk_grades_course_semester_subject
    FOREIGN KEY (course, semester, subject)
    REFERENCES course_semester_subjects (course, semester, subject)
    ON UPDATE CASCADE ON DELETE RESTRICT
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS grade_windows (
  course VARCHAR(99) NOT NULL,
  semester TINYINT UNSIGNED NOT NULL,
  component ENUM('ISA1', 'ISA2', 'ESA') NOT NULL,
  start_date DATE NOT NULL,
  end_date DATE NOT NULL,
  updated_by BIGINT UNSIGNED NOT NULL,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (course, semester, component),
  CONSTRAINT chk_grade_windows_semester CHECK (semester BETWEEN 1 AND 8),
  CONSTRAINT chk_grade_windows_dates CHECK (start_date <= end_date),
  CONSTRAINT fk_grade_windows_course
    FOREIGN KEY (course) REFERENCES courses (name)
    ON UPDATE CASCADE ON DELETE CASCADE,
  CONSTRAINT fk_grade_windows_admin
    FOREIGN KEY (updated_by) REFERENCES users (user_id)
    ON UPDATE CASCADE ON DELETE RESTRICT
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS grade_change_history (
  change_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  grade_id BIGINT UNSIGNED NOT NULL,
  old_marks TINYINT UNSIGNED NOT NULL,
  new_marks TINYINT UNSIGNED NOT NULL,
  reason VARCHAR(500) NOT NULL,
  changed_by BIGINT UNSIGNED NULL,
  changed_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (change_id),
  INDEX idx_grade_change_history_grade (grade_id, changed_at),
  CONSTRAINT chk_grade_change_history_old_marks CHECK (old_marks BETWEEN 0 AND 100),
  CONSTRAINT chk_grade_change_history_new_marks CHECK (new_marks BETWEEN 0 AND 100),
  CONSTRAINT fk_grade_change_history_grade
    FOREIGN KEY (grade_id) REFERENCES grades (grade_id)
    ON UPDATE CASCADE ON DELETE CASCADE,
  CONSTRAINT fk_grade_change_history_user
    FOREIGN KEY (changed_by) REFERENCES users (user_id)
    ON UPDATE CASCADE ON DELETE SET NULL
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS attendance (
  attendance_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  student_id VARCHAR(31) NOT NULL,
  subject VARCHAR(99) NOT NULL,
  attendance_date DATE NOT NULL,
  status ENUM('Present', 'Absent', 'Late') NOT NULL,
  PRIMARY KEY (attendance_id),
  UNIQUE KEY uq_attendance_student_subject_date
    (student_id, subject, attendance_date),
  CONSTRAINT fk_attendance_student
    FOREIGN KEY (student_id) REFERENCES students (student_id)
    ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB;
