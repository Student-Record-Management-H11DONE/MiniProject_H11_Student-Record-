-- Back up student_records before applying this migration.
-- Run once after 001_sprint1.sql has completed.
USE student_records;
SET SQL_SAFE_UPDATES = 0;

INSERT IGNORE INTO courses (name) VALUES
  ('Business Administration'),
  ('Computer Science'),
  ('Electronics'),
  ('Information Technology');

-- Retain all student records; students in other departments move to
-- Computer Science as requested for this four-department sample dataset.
UPDATE students
SET course = CASE LOWER(TRIM(course))
  WHEN 'business administration' THEN 'Business Administration'
  WHEN 'computer science' THEN 'Computer Science'
  WHEN 'electronics' THEN 'Electronics'
  WHEN 'information technology' THEN 'Information Technology'
  ELSE 'Computer Science'
END
WHERE student_id IS NOT NULL;

UPDATE students
SET semester = 8
WHERE student_id IS NOT NULL AND semester > 8;

ALTER TABLE students
  DROP CHECK chk_students_semester,
  ADD CONSTRAINT chk_students_semester CHECK (semester BETWEEN 1 AND 8);

DELETE FROM courses
WHERE name NOT IN (
  'Business Administration',
  'Computer Science',
  'Electronics',
  'Information Technology'
)
LIMIT 1000;

ALTER TABLE users
  MODIFY COLUMN role ENUM('admin', 'faculty', 'student') NOT NULL;
