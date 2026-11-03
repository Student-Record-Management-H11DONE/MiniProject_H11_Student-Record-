-- Back up student_records before applying this migration.
-- Run once against the original backend/schema.sql database.
USE student_records;
SET SQL_SAFE_UPDATES = 0;

CREATE TABLE IF NOT EXISTS courses (
  name VARCHAR(99) NOT NULL,
  PRIMARY KEY (name)
) ENGINE=InnoDB;

INSERT IGNORE INTO courses (name) VALUES
  ('Business Administration'),
  ('Computer Science'),
  ('Civil Engineering'),
  ('Electrical Engineering'),
  ('Electronics'),
  ('Information Technology'),
  ('Mechanical Engineering');

UPDATE students
SET course = CASE LOWER(TRIM(course))
  WHEN 'business administration' THEN 'Business Administration'
  WHEN 'computer science' THEN 'Computer Science'
  WHEN 'civil engineering' THEN 'Civil Engineering'
  WHEN 'electrical engineering' THEN 'Electrical Engineering'
  WHEN 'electronics' THEN 'Electronics'
  WHEN 'information technology' THEN 'Information Technology'
  WHEN 'mechanical engineering' THEN 'Mechanical Engineering'
  ELSE TRIM(course)
END;

INSERT IGNORE INTO courses (name)
SELECT DISTINCT course FROM students WHERE course IS NOT NULL;

ALTER TABLE students
  ADD COLUMN contact_details VARCHAR(255) NOT NULL DEFAULT '',
  ADD COLUMN semester TINYINT UNSIGNED NOT NULL DEFAULT 1,
  ADD CONSTRAINT chk_students_semester CHECK (semester BETWEEN 1 AND 8),
  ADD CONSTRAINT fk_students_course
    FOREIGN KEY (course) REFERENCES courses (name)
    ON UPDATE CASCADE ON DELETE RESTRICT;

CREATE TABLE IF NOT EXISTS subjects (
  name VARCHAR(99) NOT NULL,
  PRIMARY KEY (name)
) ENGINE=InnoDB;

INSERT IGNORE INTO subjects (name) VALUES
  ('Chemistry'),
  ('Data Structures'),
  ('English Communication'),
  ('Mathematics'),
  ('Physics'),
  ('Programming Fundamentals');

-- Preserve legacy subject names so existing grades can be related to subjects.
INSERT IGNORE INTO subjects (name)
SELECT DISTINCT subject FROM grades;

-- Existing marks must be whole numbers and no student may have duplicate
-- subjects before this migration can enforce the Sprint 1 grade constraints.
-- Check these before proceeding:
-- SELECT student_id, subject, COUNT(*) FROM grades
--   GROUP BY student_id, subject HAVING COUNT(*) > 1;
-- SELECT grade_id, student_id, subject, marks FROM grades
--   WHERE marks <> FLOOR(marks) OR marks < 0 OR marks > 100;
ALTER TABLE grades
  DROP CHECK chk_grades_semester,
  DROP CHECK chk_grades_marks,
  DROP INDEX uq_grades_student_subject_semester,
  DROP COLUMN semester,
  MODIFY COLUMN marks TINYINT UNSIGNED NOT NULL,
  ADD UNIQUE KEY uq_grades_student_subject (student_id, subject),
  ADD CONSTRAINT chk_grades_marks CHECK (marks BETWEEN 0 AND 100),
  ADD CONSTRAINT fk_grades_subject
    FOREIGN KEY (subject) REFERENCES subjects (name)
    ON UPDATE CASCADE ON DELETE RESTRICT;

ALTER TABLE users
  MODIFY COLUMN role VARCHAR(32) NOT NULL;

UPDATE users
SET role = CASE LOWER(role)
  WHEN 'administrator' THEN 'admin'
  WHEN 'faculty' THEN 'faculty'
  WHEN 'student' THEN 'student'
  ELSE role
END;

ALTER TABLE users
  MODIFY COLUMN role ENUM('admin', 'faculty', 'student') NOT NULL;
