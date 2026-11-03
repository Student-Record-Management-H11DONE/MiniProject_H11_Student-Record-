-- Back up student_records before applying this migration.
-- Run once after migration 004. Existing semester-level faculty permissions
-- are cleared so an administrator can assign only the subjects each person teaches.
USE student_records;

ALTER TABLE faculty_assignments
  ADD COLUMN subject VARCHAR(99) NOT NULL DEFAULT '';

DELETE FROM faculty_assignments;

ALTER TABLE faculty_assignments
  DROP PRIMARY KEY,
  ALTER COLUMN subject DROP DEFAULT,
  ADD PRIMARY KEY (user_id, course, semester, subject),
  ADD CONSTRAINT fk_faculty_assignments_subject
    FOREIGN KEY (course, semester, subject)
    REFERENCES course_semester_subjects (course, semester, subject)
    ON UPDATE RESTRICT ON DELETE CASCADE;
