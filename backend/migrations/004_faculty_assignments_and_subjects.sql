-- Back up student_records before applying this migration.
-- Run once after migrations 001, 002, and 003.
USE student_records;
SET SQL_SAFE_UPDATES = 0;

CREATE TABLE IF NOT EXISTS faculty_assignments (
  user_id BIGINT UNSIGNED NOT NULL,
  course VARCHAR(99) NOT NULL,
  semester TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (user_id, course, semester),
  INDEX idx_faculty_assignments_course_semester (course, semester),
  CONSTRAINT chk_faculty_assignments_semester CHECK (semester BETWEEN 1 AND 8),
  CONSTRAINT fk_faculty_assignments_user
    FOREIGN KEY (user_id) REFERENCES users (user_id)
    ON UPDATE CASCADE ON DELETE CASCADE,
  CONSTRAINT fk_faculty_assignments_course
    FOREIGN KEY (course) REFERENCES courses (name)
    ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB;

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

ALTER TABLE students
  ADD UNIQUE KEY uq_students_id_course_semester (student_id, course, semester);

ALTER TABLE grades
  ADD COLUMN course VARCHAR(99) NULL,
  ADD COLUMN semester TINYINT UNSIGNED NULL;

UPDATE grades AS g
JOIN students AS s ON s.student_id = g.student_id
SET g.course = s.course,
    g.semester = s.semester
WHERE g.grade_id IS NOT NULL;

INSERT IGNORE INTO course_semester_subjects (course, semester, subject)
SELECT DISTINCT s.course, s.semester, g.subject
FROM grades AS g
JOIN students AS s ON s.student_id = g.student_id
WHERE g.grade_id IS NOT NULL;

ALTER TABLE grades
  DROP FOREIGN KEY fk_grades_student,
  DROP FOREIGN KEY fk_grades_subject,
  MODIFY COLUMN course VARCHAR(99) NOT NULL,
  MODIFY COLUMN semester TINYINT UNSIGNED NOT NULL,
  ADD CONSTRAINT fk_grades_student_course_semester
    FOREIGN KEY (student_id, course, semester)
    REFERENCES students (student_id, course, semester)
    ON UPDATE CASCADE ON DELETE CASCADE,
  ADD CONSTRAINT fk_grades_course_semester_subject
    FOREIGN KEY (course, semester, subject)
    REFERENCES course_semester_subjects (course, semester, subject)
    ON UPDATE CASCADE ON DELETE RESTRICT;
