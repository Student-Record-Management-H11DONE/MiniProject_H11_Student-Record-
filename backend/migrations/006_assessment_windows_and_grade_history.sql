-- Back up student_records before applying this migration.
-- Run once after migration 005. Existing grade marks are retained as ISA1 records.
USE student_records;

ALTER TABLE grades
  ADD COLUMN component ENUM('ISA1', 'ISA2', 'ESA') NOT NULL DEFAULT 'ISA1',
  DROP INDEX uq_grades_student_subject,
  ADD UNIQUE KEY uq_grades_student_subject_component
    (student_id, subject, component),
  ALTER COLUMN component DROP DEFAULT;

CREATE TABLE grade_windows (
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

CREATE TABLE grade_change_history (
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
