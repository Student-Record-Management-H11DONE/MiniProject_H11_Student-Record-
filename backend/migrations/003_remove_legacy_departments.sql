-- Back up student_records before applying this migration.
-- Run once after 001_sprint1.sql. It is also safe after migration 002.
USE student_records;
SET SQL_SAFE_UPDATES = 0;

INSERT IGNORE INTO courses (name) VALUES ('Computer Science');

UPDATE students
SET course = 'Computer Science'
WHERE course IN (
  'Mechanical Engineering',
  'Civil Engineering',
  'Electrical Engineering'
);

DELETE FROM courses
WHERE name IN (
  'Mechanical Engineering',
  'Civil Engineering',
  'Electrical Engineering'
)
LIMIT 1000;
