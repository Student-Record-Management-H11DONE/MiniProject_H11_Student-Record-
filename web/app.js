"use strict";

const PAGE_LABELS = {
  overview: "Dashboard",
  "faculty-access": "Faculty Access",
  "grade-windows": "Grade Windows",
  departments: "Departments",
  semesters: "Semesters",
  students: "Students",
  "faculty-classes": "Students",
  detail: "Student Details",
  grades: "Grades",
  attendance: "Attendance",
  profile: "My Profile",
  "my-grades": "My Grades",
  "my-attendance": "My Attendance"
};
const ROLE_PAGES = {
  admin: [["overview", "Overview"], ["students", "Students"], ["faculty-access", "Faculty Access"], ["grade-windows", "Grade Windows"]],
  faculty: [["faculty-classes", "Students"]],
  student: [["profile", "My Profile"], ["my-grades", "My Grades"], ["my-attendance", "My Attendance"]]
};
const AVATAR_COLORS = ["avatar-color-0", "avatar-color-1", "avatar-color-2", "avatar-color-3", "avatar-color-4", "avatar-color-5"];

const loginScreen = document.querySelector("#login-screen");
const appScreen = document.querySelector("#app-screen");
const loginForm = document.querySelector("#login-form");
const loginError = document.querySelector("#login-error");
const studentDialog = document.querySelector("#student-dialog");
const studentForm = document.querySelector("#student-form");
const studentFormError = document.querySelector("#student-form-error");
const facultyDialog = document.querySelector("#faculty-dialog");
const facultyForm = document.querySelector("#faculty-form");
const deleteDialog = document.querySelector("#delete-dialog");
const searchInput = document.querySelector("#search-input");
const courseFilter = document.querySelector("#course-filter");
const semesterFilter = document.querySelector("#semester-filter");
let currentUser = null;
let csrfToken = "";
let toastTimer;
let renderSequence = 0;
let activeSearchTerm = "";
let selectedStudentId = "";
let pendingDeleteId = "";
let availableCourses = [];
let currentView = "";
let viewHistory = [];
let selectedDepartment = "";
let isDepartmentBrowse = false;
let gradeWindowCourse = "";
let gradeWindowSemester = 0;
let facultyAssignmentsEditing = false;
let facultyGradeWindows = [];
let facultyStudents = [];
let facultyAccounts = [];
let facultySubjectCatalog = [];
let selectedFacultyId = "";
let facultyClasses = [];
let selectedFacultyClass = null;

async function apiRequest(path, options = {}) {
  const method = options.method || "GET";
  const headers = { Accept: "application/json" };
  if (options.body !== undefined) headers["Content-Type"] = "application/json";
  if (method !== "GET" && method !== "HEAD" && csrfToken) headers["X-CSRF-Token"] = csrfToken;

  let response;
  try {
    response = await fetch(path, {
      method,
      headers,
      credentials: "same-origin",
      body: options.body === undefined ? undefined : JSON.stringify(options.body)
    });
  } catch (error) {
    throw new Error(`Could not reach the backend. Check that the server is running. ${error.message}`);
  }

  const payload = await response.json().catch(() => ({}));
  if (!response.ok) {
    if (response.status === 401 && currentUser) resetSession();
    throw new Error(payload.error || `The request failed (${response.status}).`);
  }
  return payload;
}

function initials(name) {
  return name.trim().split(/\s+/).slice(0, 2).map((part) => part[0]).join("").toUpperCase();
}

function formatDate(dateString) {
  return new Intl.DateTimeFormat(undefined, { month: "short", day: "numeric", year: "numeric" })
    .format(new Date(`${dateString}T12:00:00`));
}

function showToast(message, kind = "success") {
  const toast = document.querySelector("#toast");
  toast.textContent = message;
  toast.className = `toast ${kind}`;
  toast.classList.add("visible");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toast.classList.remove("visible"), 3500);
}

function showInlineMessage(element, message, kind = "error") {
  element.textContent = message;
  element.className = `inline-message ${kind}`;
  element.hidden = !message;
}

function renderFacultyClassTools() {
  const section = document.querySelector("#side-nav-tools");
  const nav = document.querySelector("#class-tool-nav");
  nav.replaceChildren();
  if (currentUser?.role !== "faculty" || !selectedFacultyClass) {
    section.hidden = true;
    return;
  }
  section.hidden = false;
  const gradeButton = document.createElement("button");
  gradeButton.className = "nav-link class-tool-link";
  gradeButton.type = "button";
  gradeButton.dataset.view = "grades";
  gradeButton.textContent = `Grades · ${selectedFacultyClass.subject}`;
  gradeButton.title = `${selectedFacultyClass.course}, Semester ${selectedFacultyClass.semester}`;
  gradeButton.addEventListener("click", () => navigate("grades"));
  nav.append(gradeButton);
  const attendanceButton = document.createElement("button");
  attendanceButton.className = "nav-link class-tool-link";
  attendanceButton.type = "button";
  attendanceButton.dataset.view = "attendance";
  attendanceButton.textContent = `Attendance · ${selectedFacultyClass.subject}`;
  attendanceButton.title = `${selectedFacultyClass.course}, Semester ${selectedFacultyClass.semester}`;
  attendanceButton.addEventListener("click", () => navigate("attendance"));
  nav.append(attendanceButton);
}

function renderNavigation() {
  const nav = document.querySelector("#side-nav");
  nav.replaceChildren();
  for (const [view, label] of ROLE_PAGES[currentUser.role]) {
    const link = document.createElement("button");
    link.className = "nav-link";
    link.type = "button";
    link.dataset.view = view;
    link.textContent = label;
    link.addEventListener("click", () => {
      if (view === "students" && currentUser.role === "admin") showAllStudents();
      else navigate(view);
    });
    nav.append(link);
  }
  renderFacultyClassTools();
  document.querySelector("#user-role").textContent = currentUser.roleLabel;
  const sidebarRole = document.querySelector("#sidebar-role");
  if (sidebarRole) sidebarRole.textContent = currentUser.roleLabel;
  document.querySelector("#user-name").textContent = currentUser.username;
  document.querySelector("#user-avatar").textContent = initials(currentUser.username);
  document.querySelector("#add-student-button").hidden = currentUser.role !== "admin";
  document.querySelector("#export-button").hidden = !["admin", "faculty"].includes(currentUser.role);
  document.querySelector("#grade-form").hidden = currentUser.role !== "faculty";
  document.querySelector("#open-departments-card").hidden = currentUser.role !== "admin";
}

function captureViewState() {
  return {
    view: currentView,
    course: courseFilter.value,
    semester: semesterFilter.value,
    isDepartmentBrowse,
    search: activeSearchTerm,
    searchInput: searchInput.value
  };
}

function navigate(view, { recordHistory = true } = {}) {
  if (currentUser?.role === "student" && view === "attendance") {
    view = "my-attendance";
  }
  const allowed = (ROLE_PAGES[currentUser.role] || []).some(([key]) => key === view);
  const auxiliaryAllowed =
    (view === "detail" && ["admin", "faculty"].includes(currentUser.role))
    || (["departments", "semesters"].includes(view) && currentUser.role === "admin")
    || (view === "students" && currentUser.role === "faculty" && selectedFacultyClass !== null)
    || (view === "grades" && currentUser.role === "faculty" && selectedFacultyClass !== null)
    || (view === "attendance" && currentUser.role === "faculty" && selectedFacultyClass !== null);
  if (!allowed && !auxiliaryAllowed) return;
  if (recordHistory && currentView && currentView !== view) {
    viewHistory.push(captureViewState());
  }
  currentView = view;
  renderSequence++;
  document.querySelectorAll(".page-view").forEach((page) => { page.hidden = true; });
  const targetView = document.querySelector(`#view-${view}`);
  if (targetView) targetView.hidden = false;
  document.querySelectorAll(".nav-link").forEach((link) => {
    const active = link.dataset.view === view
      || (view === "detail" && ["students", "faculty-classes"].includes(link.dataset.view))
      || (view === "students" && link.dataset.view === "faculty-classes");
    link.classList.toggle("active", active);
    if (active) link.setAttribute("aria-current", "page");
    else link.removeAttribute("aria-current");
  });
  document.querySelector("#breadcrumb-current").textContent = PAGE_LABELS[view] || view;
  document.querySelector("#back-button").hidden = viewHistory.length === 0;
  document.querySelector("#sidebar").classList.remove("open");
  document.querySelector("#sidebar-scrim").hidden = true;
  if (view === "overview") loadOverview();
  if (view === "faculty-access") loadFacultyAssignments();
  if (view === "grade-windows") renderGradeWindowDepartments();
  if (view === "faculty-classes") loadFacultyClasses();
  if (view === "departments") renderDepartments();
  if (view === "semesters") renderSemesters();
  if (view === "students") loadStudents();
  if (view === "profile") loadProfile();
  if (view === "grades") loadFacultyClassGrades();
  if (view === "my-grades") loadMyGrades();
  if (view === "attendance") loadFacultyClassAttendance();
  if (view === "my-attendance") loadMyAttendance();
}

function goBack() {
  const previous = viewHistory.pop();
  if (!previous) return;
  courseFilter.value = previous.course;
  semesterFilter.value = previous.semester;
  isDepartmentBrowse = previous.isDepartmentBrowse || false;
  activeSearchTerm = previous.search;
  searchInput.value = previous.searchInput;
  document.querySelector("#clear-search-button").hidden = !activeSearchTerm;
  navigate(previous.view, { recordHistory: false });
  document.querySelector("#back-button").hidden = viewHistory.length === 0;
}

function fillSemesterSelect(select, placeholder, semesters, includeAll = false) {
  select.replaceChildren(new Option(placeholder, ""));
  for (const semester of semesters) {
    select.add(new Option(`Semester ${semester}`, String(semester)));
  }
  if (includeAll) select.options[0].textContent = "All semesters";
}

function clearRoleData() {
  for (const selector of ["#student-rows", "#grade-rows", "#my-grade-rows"]) {
    document.querySelector(selector).replaceChildren();
  }
  for (const selector of [
    "#detail-id", "#detail-name", "#detail-contact", "#detail-course", "#detail-semester",
    "#profile-id", "#profile-name", "#profile-contact", "#profile-course", "#profile-semester"
  ]) {
    document.querySelector(selector).textContent = "";
  }
  document.querySelector("#total-students").textContent = "0";
  document.querySelector("#total-courses").textContent = "0";
  document.querySelector("#department-list").replaceChildren();
  document.querySelector("#semester-list").replaceChildren();
  document.querySelector("#department-card-caption").textContent = "Browse departments and their student records";
  document.querySelector("#records-count").textContent = "0";
  document.querySelector("#table-summary").textContent = "Showing 0 student records";
  document.querySelector("#grade-table-caption").textContent = "Select a student to view grades.";
  document.querySelector("#student-table").hidden = true;
  document.querySelector("#grades-table").hidden = true;
  document.querySelector("#my-grades-table").hidden = true;
  document.querySelector("#grade-student").replaceChildren(new Option("Select a student", ""));
  document.querySelector("#grade-subject").replaceChildren(new Option("Select a subject", ""));
  document.querySelector("#student-list-message").textContent = "";
  document.querySelector("#student-list-message").hidden = true;
  selectedStudentId = "";
  facultyStudents = [];
  facultyClasses = [];
  selectedFacultyClass = null;
  document.querySelector("#faculty-class-list").replaceChildren();
  document.querySelector("#student-records-card").hidden = false;
  document.querySelector("#student-view-title").textContent = "Students";
  document.querySelector("#student-view-description").textContent = "View, search, and filter student records.";
  document.querySelector("#grade-no-students").hidden = true;
  document.querySelector("#grade-form-title").textContent = "Record Grade";
  document.querySelector("#grade-form-description").textContent = "Enter marks for your assigned subject.";
  renderFacultyClassTools();
}

async function loadLookups() {
  const [{ courses }, { semesters }] = await Promise.all([
    apiRequest("/api/courses"),
    apiRequest("/api/students")
  ]);
  availableCourses = courses;
  for (const select of [courseFilter, document.querySelector("#student-course")]) {
    const placeholder = select === courseFilter ? "All courses / departments" : "Select a course / department";
    select.replaceChildren(new Option(placeholder, ""));
    courses.forEach((course) => select.add(new Option(course, course)));
  }
  fillSemesterSelect(semesterFilter, "All semesters", semesters, true);
  fillSemesterSelect(document.querySelector("#student-semester"), "Select a semester", semesters);
}

async function openApp(user, token) {
  clearRoleData();
  currentView = "";
  viewHistory = [];
  selectedDepartment = "";
  isDepartmentBrowse = false;
  document.querySelector("#back-button").hidden = true;
  currentUser = user;
  csrfToken = token;
  activeSearchTerm = "";
  selectedStudentId = "";
  searchInput.value = "";
  courseFilter.value = "";
  semesterFilter.value = "";
  document.querySelector("#clear-search-button").hidden = true;
  loginScreen.hidden = true;
  appScreen.hidden = false;
  renderNavigation();
  try {
    await loadLookups();
    const defaultView = ROLE_PAGES[user.role][0][0];
    navigate(defaultView);
  } catch (error) {
    showInlineMessage(document.querySelector("#grade-window-status"), error.message);
    showToast(error.message, "error");
  }
}

function resetSession() {
  currentUser = null;
  csrfToken = "";
  currentView = "";
  viewHistory = [];
  renderSequence++;
  clearRoleData();
  document.querySelector("#back-button").hidden = true;
  appScreen.hidden = true;
  loginScreen.hidden = false;
  loginForm.reset();
  loginError.textContent = "";
}

async function signOut() {
  if (!currentUser) return resetSession();
  try {
    await apiRequest("/api/logout", { method: "POST", body: {} });
    resetSession();
  } catch (error) {
    showToast(error.message, "error");
  }
}

function createCell(row, className) {
  const cell = document.createElement("td");
  if (className) cell.className = className;
  row.append(cell);
  return cell;
}

function renderStudentRow(student, index) {
  const row = document.createElement("tr");
  const studentCell = createCell(row);
  const identity = document.createElement("div");
  identity.className = "student-cell";
  const avatar = document.createElement("span");
  avatar.className = `student-avatar ${AVATAR_COLORS[index % AVATAR_COLORS.length]}`;
  avatar.textContent = initials(student.name);
  const name = document.createElement("span");
  name.className = "student-name";
  name.textContent = student.name;
  identity.append(avatar, name);
  studentCell.append(identity);
  createCell(row, "id-cell").textContent = student.id;
  createCell(row).textContent = student.contactDetails;
  createCell(row).textContent = student.course;
  createCell(row).textContent = `Semester ${student.semester}`;

  const actionCell = createCell(row, "action-column");
  const actions = document.createElement("div");
  actions.className = "row-actions";
  const view = document.createElement("button");
  view.className = "text-button";
  view.type = "button";
  view.textContent = "View";
  view.setAttribute("aria-label", `View ${student.name}`);
  view.addEventListener("click", () => loadStudentDetail(student.id));
  actions.append(view);
  if (currentUser.role === "admin") {
    const edit = document.createElement("button");
    edit.className = "text-button";
    edit.type = "button";
    edit.textContent = "Update";
    edit.setAttribute("aria-label", `Update ${student.name}`);
    edit.addEventListener("click", () => openStudentDialog(student));
    const remove = document.createElement("button");
    remove.className = "text-button danger-text";
    remove.type = "button";
    remove.textContent = "Delete";
    remove.setAttribute("aria-label", `Delete ${student.name}`);
    remove.addEventListener("click", () => confirmDelete(student));
    actions.append(edit, remove);
  }
  actionCell.append(actions);
  return row;
}

function buildStudentQuery() {
  const params = new URLSearchParams();
  if (courseFilter.value) params.set("course", courseFilter.value);
  if (semesterFilter.value) params.set("semester", semesterFilter.value);
  if (activeSearchTerm) params.set("q", activeSearchTerm);
  return params;
}

async function loadStudents() {
  const isFacultyClass = currentUser.role === "faculty" && selectedFacultyClass !== null;
  document.querySelector("#student-records-card").hidden = false;
  document.querySelector("#faculty-class-list").hidden = true;
  document.querySelector("#student-view-title").textContent = isFacultyClass
    ? `${selectedFacultyClass.subject} students`
    : isDepartmentBrowse
      ? `${courseFilter.value} · Semester ${semesterFilter.value}`
      : "Students";
  document.querySelector("#student-view-description").textContent = isFacultyClass
    ? `${selectedFacultyClass.course} · Semester ${selectedFacultyClass.semester}`
    : isDepartmentBrowse
      ? "Student records for this department and semester."
      : "View, search, and filter student records.";
  document.querySelector(".filters").hidden = currentUser.role === "faculty" || isDepartmentBrowse;
  const sequence = ++renderSequence;
  const message = document.querySelector("#student-list-message");
  showInlineMessage(message, "");
  const query = buildStudentQuery();
  const path = activeSearchTerm
    ? `/api/students/search?${query.toString()}`
    : `/api/students?${query.toString()}`;
  try {
    const result = await apiRequest(path);
    if (sequence !== renderSequence || !currentUser) return;
    const students = result.students;
    const rows = document.querySelector("#student-rows");
    rows.replaceChildren(...students.map(renderStudentRow));
    document.querySelector("#records-count").textContent = students.length;
    document.querySelector("#table-summary").textContent =
      `Showing ${students.length} ${students.length === 1 ? "student record" : "student records"}`;
    document.querySelector("#student-table").hidden = students.length === 0;
    document.querySelector("#student-empty-state").hidden = students.length !== 0;
    document.querySelector("#student-empty-title").textContent = activeSearchTerm
      ? "No matching student record was found."
      : "No matching records found.";
    document.querySelector("#student-empty-message").textContent = activeSearchTerm
      ? "Try a different Student ID or name."
      : "Change the course or semester filters.";
  } catch (error) {
    if (sequence !== renderSequence) return;
    if (error.message === "No matching student record was found.") {
      document.querySelector("#student-rows").replaceChildren();
      document.querySelector("#records-count").textContent = "0";
      document.querySelector("#table-summary").textContent = "Showing 0 student records";
      document.querySelector("#student-table").hidden = true;
      document.querySelector("#student-empty-state").hidden = false;
      document.querySelector("#student-empty-title").textContent = error.message;
      document.querySelector("#student-empty-message").textContent = "Try a different Student ID or name.";
      return;
    }
    showInlineMessage(message, error.message);
  }
}

async function loadFacultyClasses() {
  document.querySelector("#faculty-class-list").hidden = false;
  document.querySelector("#student-records-card").hidden = true;
  renderFacultyClassTools();
  try {
    const { classes } = await apiRequest("/api/faculty/classes");
    facultyClasses = classes;
    const classList = document.querySelector("#faculty-class-list");
    classList.replaceChildren();
    for (const assignedClass of classes) {
      const card = document.createElement("button");
      card.className = "department-card faculty-class-card";
      card.type = "button";
      const label = document.createElement("span");
      label.className = "department-card-label";
      label.textContent = `${assignedClass.course} · SEMESTER ${assignedClass.semester}`;
      const subject = document.createElement("strong");
      subject.textContent = assignedClass.subject;
      const action = document.createElement("span");
      action.className = "department-card-action";
      action.textContent = "View students";
      card.append(label, subject, action);
      card.addEventListener("click", () => openFacultyClass(assignedClass));
      classList.append(card);
    }
    if (!classes.length) {
      const emptyState = document.createElement("div");
      emptyState.className = "class-empty-state";
      const title = document.createElement("strong");
      title.textContent = "No subjects have been assigned yet.";
      const description = document.createElement("p");
      description.textContent = "Ask an administrator to assign your department, semester, and subject.";
      emptyState.append(title, description);
      classList.append(emptyState);
    }
  } catch (error) {
    showToast(error.message, "error");
  }
}

function openFacultyClass(assignedClass) {
  selectedFacultyClass = assignedClass;
  selectedStudentId = "";
  resetGradeForm();
  resetAttendanceForm();
  activeSearchTerm = "";
  searchInput.value = "";
  document.querySelector("#clear-search-button").hidden = true;
  courseFilter.value = assignedClass.course;
  semesterFilter.value = String(assignedClass.semester);
  renderFacultyClassTools();
  navigate("students");
}

async function loadOverview() {
  try {
    const { stats } = await apiRequest("/api/students");
    document.querySelector("#total-students").textContent = stats.totalStudents;
    document.querySelector("#total-courses").textContent = stats.totalCourses;
    document.querySelector("#department-card-caption").textContent =
      `Select to browse ${stats.totalCourses} departments and their student records`;
  } catch (error) {
    showToast(error.message, "error");
  }
}

function renderDepartments() {
  const departmentList = document.querySelector("#department-list");
  departmentList.replaceChildren();
  for (const course of availableCourses) {
    const department = document.createElement("button");
    department.className = "department-card";
    department.type = "button";
    const label = document.createElement("span");
    label.className = "department-card-label";
    label.textContent = "DEPARTMENT";
    const name = document.createElement("strong");
    name.textContent = course;
    const action = document.createElement("span");
    action.className = "department-card-action";
    action.textContent = "View student records";
    department.append(label, name, action);
    department.setAttribute("aria-label", `View students in ${course}`);
    department.addEventListener("click", () => {
      selectedDepartment = course;
      isDepartmentBrowse = false;
      navigate("semesters");
    });
    departmentList.append(department);
  }
  if (!availableCourses.length) {
    const emptyState = document.createElement("p");
    emptyState.className = "empty-state";
    emptyState.textContent = "No departments are currently available.";
    departmentList.append(emptyState);
  }
}

function renderSemesters() {
  const semesterList = document.querySelector("#semester-list");
  semesterList.replaceChildren();
  document.querySelector("#semester-department-title").textContent = selectedDepartment;
  for (let semester = 1; semester <= 8; semester++) {
    const card = document.createElement("button");
    card.className = "department-card";
    card.type = "button";
    const label = document.createElement("span");
    label.className = "department-card-label";
    label.textContent = "SEMESTER";
    const name = document.createElement("strong");
    name.textContent = `Semester ${semester}`;
    const action = document.createElement("span");
    action.className = "department-card-action";
    action.textContent = "View student records";
    card.append(label, name, action);
    card.setAttribute("aria-label", `View ${selectedDepartment}, Semester ${semester} students`);
    card.addEventListener("click", () => {
      viewHistory.push(captureViewState());
      courseFilter.value = selectedDepartment;
      semesterFilter.value = String(semester);
      isDepartmentBrowse = true;
      activeSearchTerm = "";
      searchInput.value = "";
      document.querySelector("#clear-search-button").hidden = true;
      navigate("students", { recordHistory: false });
    });
    semesterList.append(card);
  }
}

function showAllStudents() {
  courseFilter.value = "";
  semesterFilter.value = "";
  isDepartmentBrowse = false;
  activeSearchTerm = "";
  searchInput.value = "";
  document.querySelector("#clear-search-button").hidden = true;
  navigate("students");
}

function setStudentDetails(student, prefix) {
  document.querySelector(`#${prefix}-id`).textContent = student.id;
  document.querySelector(`#${prefix}-name`).textContent = student.name;
  document.querySelector(`#${prefix}-contact`).textContent = student.contactDetails;
  document.querySelector(`#${prefix}-course`).textContent = student.course;
  document.querySelector(`#${prefix}-semester`).textContent = `Semester ${student.semester}`;
}

async function loadStudentDetail(studentId) {
  try {
    const { student } = await apiRequest(`/api/students/${encodeURIComponent(studentId)}`);
    if (currentUser.role === "student") {
      setStudentDetails(student, "profile");
      navigate("profile");
      return;
    }
    selectedStudentId = student.id;
    document.querySelector("#detail-title").textContent = student.name;
    setStudentDetails(student, "detail");
    document.querySelector("#detail-edit-button").hidden = currentUser.role !== "admin";
    document.querySelector("#detail-edit-button").onclick = () => openStudentDialog(student);
    const isFacultyClass = currentUser.role === "faculty" && selectedFacultyClass !== null;
    const gradeBtn = document.querySelector("#detail-grade-button");
    const attBtn = document.querySelector("#detail-attendance-button");
    if (gradeBtn) {
      gradeBtn.hidden = !isFacultyClass;
      gradeBtn.onclick = () => navigate("grades");
    }
    if (attBtn) {
      attBtn.hidden = !isFacultyClass;
      attBtn.onclick = () => navigate("attendance");
    }
    navigate("detail");
  } catch (error) {
    showToast(error.message, "error");
  }
}

async function loadProfile() {
  if (!currentUser.studentId) {
    showToast("This student account is not linked to a student record.", "error");
    return;
  }
  try {
    const { student } = await apiRequest(`/api/students/${encodeURIComponent(currentUser.studentId)}`);
    setStudentDetails(student, "profile");
  } catch (error) {
    showToast(error.message, "error");
  }
}

function clearStudentErrors() {
  for (const id of ["student-id-error", "student-name-error", "student-contact-error", "student-course-error", "student-semester-error", "student-username-error", "student-password-error"]) {
    const element = document.querySelector(`#${id}`);
    element.textContent = "";
    element.hidden = true;
  }
  studentFormError.textContent = "";
}

async function openStudentDialog(student = null) {
  if (student) {
    try {
      ({ student } = await apiRequest(`/api/students/${encodeURIComponent(student.id)}`));
    } catch (error) {
      showToast(error.message, "error");
      return;
    }
  }
  studentForm.reset();
  clearStudentErrors();
  document.querySelector("#edit-original-id").value = student ? student.id : "";
  document.querySelector("#student-dialog-title").textContent = student ? "Update Student" : "Add Student";
  document.querySelector("#save-student-button").textContent = "Save";
  const usernameInput = document.querySelector("#student-username");
  const passwordInput = document.querySelector("#student-password");
  const passwordRequiredMark = document.querySelector("#student-password-required");
  const loginHelp = document.querySelector("#student-login-help");
  usernameInput.value = student ? (student.loginUsername || "") : "";
  passwordInput.required = !student || !student.loginUsername;
  passwordRequiredMark.hidden = !passwordInput.required;
  loginHelp.textContent = student
    ? (student.loginUsername
      ? "Update the username or leave the password blank to keep it unchanged."
      : "This student has no login yet. Set a username and password to create one.")
    : "Set the username and password the student will use to sign in.";
  if (student) {
    document.querySelector("#student-id").value = student.id;
    document.querySelector("#student-name").value = student.name;
    document.querySelector("#student-contact").value = student.contactDetails;
    document.querySelector("#student-course").value = student.course;
    document.querySelector("#student-semester").value = String(student.semester);
  }
  studentDialog.showModal();
  document.querySelector("#student-id").focus();
}

function showStudentValidation(message) {
  const fields = {
    "Student ID": "student-id-error",
    "Name": "student-name-error",
    "Contact details": "student-contact-error",
    "Course / department": "student-course-error",
    "Semester": "student-semester-error",
    "Login username": "student-username-error",
    "Student password": "student-password-error"
  };
  const matchedField = Object.keys(fields).find((field) => message.startsWith(`${field} `) || message === `${field} is required.`);
  if (matchedField) {
    const error = document.querySelector(`#${fields[matchedField]}`);
    error.textContent = message;
    error.hidden = false;
  } else {
    studentFormError.textContent = message;
  }
}

async function saveStudent(event) {
  event.preventDefault();
  clearStudentErrors();
  const originalId = document.querySelector("#edit-original-id").value;
  const student = {
    id: document.querySelector("#student-id").value.trim(),
    name: document.querySelector("#student-name").value.trim(),
    contactDetails: document.querySelector("#student-contact").value.trim(),
    course: document.querySelector("#student-course").value,
    semester: Number(document.querySelector("#student-semester").value),
    username: document.querySelector("#student-username").value.trim(),
    password: document.querySelector("#student-password").value
  };
  const requiredInputs = [
    ["student-id", student.id, "Student ID"],
    ["student-name", student.name, "Name"],
    ["student-contact", student.contactDetails, "Contact details"],
    ["student-course", student.course, "Course / department"],
    ["student-semester", document.querySelector("#student-semester").value, "Semester"],
    ["student-username", student.username, "Login username"]
  ];
  for (const [inputId, value, label] of requiredInputs) {
    if (!value) {
      showStudentValidation(`${label} is required.`);
      document.querySelector(`#${inputId}`).focus();
      return;
    }
  }
  if (student.id.length > 31 || student.name.length > 99 || student.contactDetails.length > 255) {
    showStudentValidation("One or more fields exceed the maximum permitted length.");
    return;
  }
  if (student.username.length > 63) {
    showStudentValidation("Login username must be at most 63 characters.");
    document.querySelector("#student-username").focus();
    return;
  }
  if ((!originalId || document.querySelector("#student-password").required) && student.password.length < 12) {
    showStudentValidation("Student password must be at least 12 characters.");
    document.querySelector("#student-password").focus();
    return;
  }
  if (student.password.length > 1024) {
    showStudentValidation("Student password must be at most 1024 characters.");
    document.querySelector("#student-password").focus();
    return;
  }
  try {
    if (originalId) {
      await apiRequest(`/api/students/${encodeURIComponent(originalId)}`, { method: "PUT", body: student });
    } else {
      await apiRequest("/api/students", { method: "POST", body: student });
    }
    studentDialog.close();
    showToast(originalId ? "Student record and login updated." : "Student record and login added.");
    if (originalId && selectedStudentId === originalId) {
      selectedStudentId = student.id;
      await loadStudentDetail(student.id);
    }
    await loadStudents();
  } catch (error) {
    showStudentValidation(error.message);
  }
}

function confirmDelete(student) {
  pendingDeleteId = student.id;
  document.querySelector("#delete-message").textContent =
    "Are you sure you want to delete this student record?";
  deleteDialog.showModal();
  document.querySelector("#confirm-delete").focus();
}

async function deleteStudent() {
  if (!pendingDeleteId) return;
  try {
    await apiRequest(`/api/students/${encodeURIComponent(pendingDeleteId)}`, { method: "DELETE", body: {} });
    deleteDialog.close();
    pendingDeleteId = "";
    showToast("Student record deleted.");
    await loadStudents();
  } catch (error) {
    showToast(error.message, "error");
  }
}

async function loadFacultyClassGrades() {
  if (!selectedFacultyClass) return;
  const { course, semester, subject } = selectedFacultyClass;
  document.querySelector("#grade-view-title").textContent = `${subject} grades`;
  document.querySelector("#grade-view-description").textContent = `${course} · Semester ${semester}`;
  document.querySelector("#grade-form-title").textContent = "Record Grade";
  document.querySelector("#grade-form-description").textContent = `Enter marks for ${subject}.`;
  const subjectSelect = document.querySelector("#grade-subject");
  subjectSelect.replaceChildren(new Option(subject, subject));
  subjectSelect.value = subject;
  subjectSelect.disabled = true;
  facultyGradeWindows = [];
  document.querySelector("#save-grade-button").disabled = true;
  showInlineMessage(document.querySelector("#grade-window-status"), "");
  try {
    const params = new URLSearchParams({ course, semester: String(semester) });
    const [{ students }, { windows }] = await Promise.all([
      apiRequest(`/api/students?${params.toString()}`),
      apiRequest(`/api/grade-windows?${params.toString()}`)
    ]);
    facultyGradeWindows = windows;
    const openComponents = new Set(
      windows.filter((window) => window.isOpen).map((window) => window.component)
    );
    for (const option of document.querySelector("#grade-component").options) {
      option.disabled = !openComponents.has(option.value);
    }
    document.querySelector("#save-grade-button").disabled = openComponents.size === 0;
    showInlineMessage(
      document.querySelector("#grade-window-status"),
      openComponents.size
        ? `Open for grade entry: ${[...openComponents].join(", ")}.`
        : "No assessment window is currently open. Contact the administrator.",
      openComponents.size ? "success" : "error"
    );
    facultyStudents = students;
    const studentSelect = document.querySelector("#grade-student");
    const previousStudent = selectedStudentId;
    studentSelect.replaceChildren(new Option("Select a student", ""));
    students.forEach((student) => {
      studentSelect.add(new Option(`${student.name} (${student.id})`, student.id));
    });
    selectedStudentId = students.some((student) => student.id === previousStudent)
      ? previousStudent
      : (students.length ? students[0].id : "");
    studentSelect.value = selectedStudentId;
    resetGradeForm();
    const componentSelect = document.querySelector("#grade-component");
    componentSelect.value = [...componentSelect.options].find((option) => !option.disabled)?.value || "";
    document.querySelector("#grade-no-students").hidden = students.length > 0;
    document.querySelector("#grade-form").hidden = students.length === 0;
    document.querySelector("#grade-empty-state").hidden = true;
    await loadGrades(selectedStudentId, true);
  } catch (error) {
    showToast(error.message, "error");
  }
}

async function loadFacultyAssignments() {
  const message = document.querySelector("#faculty-access-message");
  facultyAssignmentsEditing = false;
  showInlineMessage(document.querySelector("#faculty-assignment-save-message"), "");
  showInlineMessage(message, "");
  try {
    const result = await apiRequest("/api/faculty-assignments");
    facultyAccounts = result.faculty;
    facultySubjectCatalog = result.subjects;
    const selector = document.querySelector("#faculty-selector");
    const previousSelection = selectedFacultyId;
    selector.replaceChildren(new Option("Select a faculty member", ""));
    facultyAccounts.forEach((faculty) => {
      selector.add(new Option(faculty.username, String(faculty.id)));
    });
    selectedFacultyId = facultyAccounts.some((faculty) => String(faculty.id) === previousSelection)
      ? previousSelection
      : "";
    selector.value = selectedFacultyId;
    if (!selectedFacultyId) facultyAssignmentsEditing = false;
    renderFacultyAssignmentGrid();
  } catch (error) {
    showInlineMessage(message, error.message);
  }
}

async function createFaculty(event) {
  event.preventDefault();
  const username = document.querySelector("#faculty-username").value.trim();
  const password = document.querySelector("#faculty-password").value;
  const error = document.querySelector("#faculty-form-error");
  error.textContent = "";
  if (!username) {
    error.textContent = "Login username is required.";
    document.querySelector("#faculty-username").focus();
    return;
  }
  if (username.length > 63) {
    error.textContent = "Login username must be at most 63 characters.";
    document.querySelector("#faculty-username").focus();
    return;
  }
  if (password.length < 12 || password.length > 1024) {
    error.textContent = "Password must be between 12 and 1024 characters.";
    document.querySelector("#faculty-password").focus();
    return;
  }
  try {
    const { faculty } = await apiRequest("/api/faculty", {
      method: "POST",
      body: { username, password }
    });
    selectedFacultyId = String(faculty.id);
    facultyForm.reset();
    facultyDialog.close();
    await loadFacultyAssignments();
    showToast(`Faculty account "${faculty.username}" created. Assign their subjects in Faculty Access.`);
  } catch (requestError) {
    error.textContent = requestError.message;
  }
}

function renderFacultyAssignmentGrid() {
  const grid = document.querySelector("#faculty-assignment-grid");
  grid.replaceChildren();
  const faculty = facultyAccounts.find((account) => String(account.id) === selectedFacultyId);
  document.querySelector("#faculty-assignment-panel").hidden = !faculty;
  document.querySelector("#faculty-no-accounts").hidden = facultyAccounts.length > 0;
  if (!faculty) return;

  const assigned = new Set(faculty.assignments.map(
    ({ course, semester, subject }) => `${course}\u0000${semester}\u0000${subject}`
  ));
  for (const course of availableCourses) {
    const row = document.createElement("section");
    row.className = "assignment-department";
    const heading = document.createElement("h3");
    heading.textContent = course;
    const semesters = document.createElement("div");
    semesters.className = "assignment-semesters";
    for (let semester = 1; semester <= 8; semester++) {
      const subjects = facultySubjectCatalog
        .filter((entry) => entry.course === course && entry.semester === semester);
      if (!subjects.length) continue;
      const semesterCard = document.createElement("div");
      semesterCard.className = "assignment-semester-card";
      const semesterHeading = document.createElement("strong");
      semesterHeading.textContent = `Semester ${semester}`;
      semesterCard.append(semesterHeading);
      for (const entry of subjects) {
        const label = document.createElement("label");
        label.className = "assignment-subject";
        const checkbox = document.createElement("input");
        checkbox.type = "checkbox";
        checkbox.dataset.course = course;
        checkbox.dataset.semester = String(semester);
        checkbox.dataset.subject = entry.subject;
        checkbox.checked = assigned.has(`${course}\u0000${semester}\u0000${entry.subject}`);
        checkbox.disabled = !facultyAssignmentsEditing;
        const text = document.createElement("span");
        text.textContent = entry.subject;
        label.append(checkbox, text);
        semesterCard.append(label);
      }
      semesters.append(semesterCard);
    }
    row.append(heading, semesters);
    grid.append(row);
  }
  updateFacultyAssignmentSummary();
  updateFacultyAssignmentControls();
}

function updateFacultyAssignmentSummary() {
  const count = document.querySelectorAll("#faculty-assignment-grid input:checked").length;
  document.querySelector("#faculty-assignment-summary").textContent =
    `${count} subject${count === 1 ? "" : "s"} assigned`;
}

function updateFacultyAssignmentControls() {
  const hasFaculty = facultyAccounts.some((account) => String(account.id) === selectedFacultyId);
  document.querySelector("#edit-faculty-assignments").hidden =
    !hasFaculty || facultyAssignmentsEditing;
  document.querySelector("#cancel-faculty-assignments").hidden = !facultyAssignmentsEditing;
  document.querySelector("#save-faculty-assignments").hidden = !facultyAssignmentsEditing;
}

async function saveFacultyAssignments() {
  const message = document.querySelector("#faculty-assignment-save-message");
  const assignments = [...document.querySelectorAll("#faculty-assignment-grid input:checked")]
    .map((checkbox) => ({
      course: checkbox.dataset.course,
      semester: Number(checkbox.dataset.semester),
      subject: checkbox.dataset.subject
    }));
  try {
    await apiRequest(`/api/faculty/${encodeURIComponent(selectedFacultyId)}/assignments`, {
      method: "PUT",
      body: { assignments }
    });
    facultyAssignmentsEditing = false;
    await loadFacultyAssignments();
    showInlineMessage(message, "Faculty access assignments saved.", "success");
  } catch (error) {
    showInlineMessage(message, error.message);
  }
}

function renderGradeWindowDepartments() {
  const departmentStep = document.querySelector("#grade-window-department-step");
  departmentStep.hidden = false;
  document.querySelector("#grade-window-semester-step").hidden = true;
  document.querySelector("#grade-window-editor").hidden = true;
  departmentStep.replaceChildren();
  for (const course of availableCourses) {
    const card = document.createElement("button");
    card.className = "department-card";
    card.type = "button";
    const label = document.createElement("span");
    label.className = "department-card-label";
    label.textContent = "DEPARTMENT";
    const name = document.createElement("strong");
    name.textContent = course;
    const action = document.createElement("span");
    action.className = "department-card-action";
    action.textContent = "Set assessment dates";
    card.append(label, name, action);
    card.addEventListener("click", () => {
      gradeWindowCourse = course;
      document.querySelector("#grade-window-course-title").textContent = course;
      departmentStep.hidden = true;
      document.querySelector("#grade-window-semester-step").hidden = false;
      document.querySelector("#grade-window-editor").hidden = true;
      renderGradeWindowSemesters();
    });
    departmentStep.append(card);
  }
}

function renderGradeWindowSemesters() {
  const semesterList = document.querySelector("#grade-window-semesters");
  semesterList.replaceChildren();
  for (let semester = 1; semester <= 8; semester++) {
    const card = document.createElement("button");
    card.className = "department-card";
    card.type = "button";
    const label = document.createElement("span");
    label.className = "department-card-label";
    label.textContent = "SEMESTER";
    const name = document.createElement("strong");
    name.textContent = `Semester ${semester}`;
    const action = document.createElement("span");
    action.className = "department-card-action";
    action.textContent = "Configure ISA1, ISA2, and ESA";
    card.append(label, name, action);
    card.addEventListener("click", () => openGradeWindowEditor(semester));
    semesterList.append(card);
  }
}

async function openGradeWindowEditor(semester) {
  gradeWindowSemester = semester;
  document.querySelector("#grade-window-semester-step").hidden = true;
  document.querySelector("#grade-window-editor").hidden = false;
  document.querySelector("#grade-window-title").textContent =
    `${gradeWindowCourse} · Semester ${semester}`;
  const form = document.querySelector("#grade-window-form");
  form.reset();
  document.querySelector("#save-grade-windows").disabled = true;
  showInlineMessage(document.querySelector("#grade-window-message"), "");
  try {
    const params = new URLSearchParams({
      course: gradeWindowCourse,
      semester: String(semester)
    });
    const { windows } = await apiRequest(`/api/grade-windows?${params.toString()}`);
    for (const window of windows) {
      const prefix = window.component.toLowerCase();
      document.querySelector(`#${prefix}-start`).value = window.startDate;
      document.querySelector(`#${prefix}-end`).value = window.endDate;
    }
    document.querySelector("#save-grade-windows").disabled = false;
  } catch (error) {
    showInlineMessage(document.querySelector("#grade-window-message"), error.message);
  }
}

async function saveGradeWindows(event) {
  event.preventDefault();
  const windows = ["ISA1", "ISA2", "ESA"].map((component) => {
    const prefix = component.toLowerCase();
    return {
      component,
      startDate: document.querySelector(`#${prefix}-start`).value,
      endDate: document.querySelector(`#${prefix}-end`).value
    };
  });
  for (const window of windows) {
    if (Boolean(window.startDate) !== Boolean(window.endDate)) {
      showInlineMessage(
        document.querySelector("#grade-window-message"),
        `Enter both dates for ${window.component}, or leave both blank to close it.`
      );
      return;
    }
    if (window.startDate && window.endDate < window.startDate) {
      showInlineMessage(
        document.querySelector("#grade-window-message"),
        `The ${window.component} end date must be on or after its start date.`
      );
      return;
    }
  }
  try {
    await apiRequest("/api/grade-windows", {
      method: "PUT",
      body: {
        course: gradeWindowCourse,
        semester: gradeWindowSemester,
        windows
      }
    });
    showInlineMessage(
      document.querySelector("#grade-window-message"),
      "Assessment date windows saved.",
      "success"
    );
  } catch (error) {
    showInlineMessage(document.querySelector("#grade-window-message"), error.message);
  }
}

async function loadGrades(studentId, editable) {
  const rows = document.querySelector(editable ? "#grade-rows" : "#my-grade-rows");
  const table = document.querySelector(editable ? "#grades-table" : "#my-grades-table");
  const empty = document.querySelector(editable ? "#grade-empty-state" : "#my-grade-empty-state");
  if (!studentId) {
    rows.replaceChildren();
    table.hidden = true;
    empty.hidden = editable;
    if (editable) document.querySelector("#grade-table-caption").textContent = "Select a student to view grades.";
    return;
  }
  try {
    const { grades } = await apiRequest(`/api/students/${encodeURIComponent(studentId)}/grades`);
    const visibleGrades = editable && selectedFacultyClass
      ? grades.filter((grade) => grade.subject === selectedFacultyClass.subject)
      : grades;
    rows.replaceChildren(...visibleGrades.map((grade) => {
      const row = document.createElement("tr");
      createCell(row).textContent = grade.subject;
      createCell(row).textContent = grade.component;
      createCell(row).textContent = String(grade.marks);
      if (editable) {
        const actionCell = createCell(row, "action-column");
        const update = document.createElement("button");
        update.className = "text-button";
        update.type = "button";
        update.textContent = "Edit marks";
        update.setAttribute("aria-label", `Update ${grade.subject} ${grade.component} grade`);
        update.disabled = !isFacultyGradeWindowOpen(grade.component);
        if (update.disabled) update.title = `${grade.component} grade window is closed.`;
        update.addEventListener("click", () => {
          document.querySelector("#grade-id").value = grade.id;
          document.querySelector("#grade-subject").value = grade.subject;
          document.querySelector("#grade-component").value = grade.component;
          document.querySelector("#grade-marks").value = grade.marks;
          document.querySelector("#grade-change-reason-label").hidden = false;
          document.querySelector("#grade-form-title").textContent = "Update Grade";
          document.querySelector("#grade-form-description").textContent =
            `Update ${grade.component} marks for ${grade.subject}. Give a reason for the change.`;
          document.querySelector("#save-grade-button").textContent = "Update Grade";
          document.querySelector("#cancel-grade-edit").hidden = false;
          document.querySelector("#grade-form").scrollIntoView({ behavior: "smooth", block: "center" });
          document.querySelector("#grade-marks").focus({ preventScroll: true });
        });
        actionCell.append(update);
      }
      return row;
    }));
    table.hidden = visibleGrades.length === 0;
    empty.hidden = visibleGrades.length !== 0;
    if (editable) {
      document.querySelector("#grade-table-caption").textContent =
        `${visibleGrades.length} ${visibleGrades.length === 1 ? "grade" : "grades"} for ${selectedFacultyClass.subject}.`;
    }
  } catch (error) {
    showToast(error.message, "error");
  }
}

function isFacultyGradeWindowOpen(component) {
  return facultyGradeWindows.some((window) => window.component === component && window.isOpen);
}

async function loadMyGrades() {
  await loadGrades(currentUser.studentId, false);
}

async function saveGrade(event) {
  event.preventDefault();
  const studentId = document.querySelector("#grade-student").value;
  const subject = document.querySelector("#grade-subject").value;
  const component = document.querySelector("#grade-component").value;
  const marksInput = document.querySelector("#grade-marks");
  const marks = Number(marksInput.value);
  const message = document.querySelector("#grade-form-error");
  message.textContent = "";
  if (!studentId) {
    message.textContent = "Please select a student.";
    document.querySelector("#grade-student").focus();
    return;
  }
  if (!subject) {
    message.textContent = "Please select a subject.";
    document.querySelector("#grade-subject").focus();
    return;
  }
  if (marksInput.value === "" || !Number.isInteger(marks) || marks < 0 || marks > 100) {
    message.textContent = "Marks must be between 0 and 100.";
    return;
  }
  const gradeId = document.querySelector("#grade-id").value;
  const reason = document.querySelector("#grade-change-reason").value.trim();
  if (gradeId && !reason) {
    message.textContent = "Enter a reason for changing this grade.";
    document.querySelector("#grade-change-reason").focus();
    return;
  }
  try {
    if (gradeId) {
      await apiRequest(`/api/grades/${gradeId}`, {
        method: "PUT",
        body: { subject, component, marks, reason }
      });
    } else {
      await apiRequest(`/api/students/${encodeURIComponent(studentId)}/grades`, {
        method: "POST",
        body: { subject, component, marks }
      });
    }
    resetGradeForm();
    await loadGrades(studentId, true);
    showToast(gradeId ? "Grade updated." : "Grade recorded.");
  } catch (error) {
    message.textContent = error.message;
  }
}

function resetGradeForm() {
  document.querySelector("#grade-form").reset();
  document.querySelector("#grade-id").value = "";
  document.querySelector("#grade-form-error").textContent = "";
  document.querySelector("#grade-change-reason-label").hidden = true;
  document.querySelector("#grade-change-reason").value = "";
  document.querySelector("#save-grade-button").textContent = "Save Grade";
  document.querySelector("#cancel-grade-edit").hidden = true;
  document.querySelector("#grade-student").value = selectedStudentId;
  if (selectedFacultyClass) {
    const subjectSelect = document.querySelector("#grade-subject");
    subjectSelect.replaceChildren(new Option(selectedFacultyClass.subject, selectedFacultyClass.subject));
    subjectSelect.value = selectedFacultyClass.subject;
    subjectSelect.disabled = true;
    document.querySelector("#grade-form-title").textContent = "Record Grade";
    document.querySelector("#grade-form-description").textContent =
      `Enter marks for ${selectedFacultyClass.subject}.`;
  }
}


async function loadFacultyClassAttendance() {
  if (!selectedFacultyClass) return;
  const { course, semester, subject } = selectedFacultyClass;
  document.querySelector("#attendance-view-title").textContent = `${subject} attendance`;
  document.querySelector("#attendance-view-description").textContent = `${course} · Semester ${semester}`;
  document.querySelector("#attendance-form-title").textContent = "Record Attendance";
  document.querySelector("#attendance-form-description").textContent = `Enter attendance details for ${subject}.`;
  const subjectSelect = document.querySelector("#attendance-subject");
  subjectSelect.replaceChildren(new Option(subject, subject));
  subjectSelect.value = subject;
  subjectSelect.disabled = true;

  try {
    const params = new URLSearchParams({ course, semester: String(semester) });
    const { students } = await apiRequest(`/api/students?${params.toString()}`);
    facultyStudents = students;
    const studentSelect = document.querySelector("#attendance-student");
    const previousStudent = selectedStudentId;
    studentSelect.replaceChildren(new Option("Select a student", ""));
    students.forEach((student) => {
      studentSelect.add(new Option(`${student.name} (${student.id})`, student.id));
    });
    selectedStudentId = students.some((student) => student.id === previousStudent)
      ? previousStudent
      : (students.length ? students[0].id : "");
    studentSelect.value = selectedStudentId;
    resetAttendanceForm();
    document.querySelector("#attendance-no-students").hidden = students.length > 0;
    document.querySelector("#attendance-form").hidden = students.length === 0;
    document.querySelector("#attendance-empty-state").hidden = true;
    await loadAttendance(selectedStudentId, true);
  } catch (error) {
    showToast(error.message, "error");
  }
}

async function loadAttendance(studentId, editable) {
  const rows = document.querySelector(editable ? "#attendance-rows" : "#my-attendance-rows");
  const table = document.querySelector(editable ? "#attendance-table" : "#my-attendance-table");
  const empty = document.querySelector(editable ? "#attendance-empty-state" : "#my-attendance-empty-state");
  if (!studentId) {
    rows.replaceChildren();
    table.hidden = true;
    empty.hidden = editable;
    if (editable) document.querySelector("#attendance-table-caption").textContent = "Select a student to view attendance.";
    return;
  }
  try {
    const { attendance } = await apiRequest(`/api/students/${encodeURIComponent(studentId)}/attendance`);
    const visibleAttendance = editable && selectedFacultyClass
      ? attendance.filter((rec) => rec.subject === selectedFacultyClass.subject)
      : attendance;
    rows.replaceChildren(...visibleAttendance.map((rec) => {
      const row = document.createElement("tr");
      createCell(row).textContent = rec.subject;
      createCell(row).textContent = rec.date;
      const statusCell = createCell(row);
      const badge = document.createElement("span");
      badge.className = `status-badge ${rec.status.toLowerCase()}`;
      badge.textContent = rec.status;
      statusCell.append(badge);

      if (editable) {
        const actionCell = createCell(row, "action-column");
        const update = document.createElement("button");
        update.className = "text-button";
        update.type = "button";
        update.textContent = "Edit";
        update.setAttribute("aria-label", `Update attendance for ${rec.date}`);
        update.addEventListener("click", () => {
          document.querySelector("#attendance-id").value = rec.id;
          document.querySelector("#attendance-student").value = studentId;
          document.querySelector("#attendance-subject").value = rec.subject;
          document.querySelector("#attendance-date").value = rec.date;
          document.querySelector("#attendance-status").value = rec.status;
          document.querySelector("#attendance-form-title").textContent = "Update Attendance";
          document.querySelector("#attendance-form-description").textContent =
            `Update attendance for ${rec.subject} on ${rec.date}.`;
          document.querySelector("#save-attendance-button").textContent = "Update Attendance";
          document.querySelector("#cancel-attendance-edit").hidden = false;
          document.querySelector("#attendance-form").scrollIntoView({ behavior: "smooth", block: "center" });
          document.querySelector("#attendance-status").focus({ preventScroll: true });
        });
        actionCell.append(update);
      }
      return row;
    }));
    table.hidden = visibleAttendance.length === 0;
    empty.hidden = visibleAttendance.length !== 0;
    const totalCount = visibleAttendance.length;
      const presentCount = visibleAttendance.filter((rec) => rec.status === "Present").length;
      const absentCount = visibleAttendance.filter((rec) => rec.status === "Absent").length;
      const attendancePct = totalCount ? Math.round((presentCount / totalCount) * 100) : 0;
      if (editable) {
        document.querySelector("#attendance-table-caption").textContent =
          `${totalCount} attendance ${totalCount === 1 ? "record" : "records"} for ${selectedFacultyClass.subject} (Present: ${presentCount}, Absent: ${absentCount} · Attendance: ${attendancePct}%).`;
      } else {
        const summaryEl = document.querySelector("#my-attendance-summary");
        if (summaryEl) {
          summaryEl.textContent = totalCount
            ? `Overall Attendance: ${attendancePct}% (${presentCount} Present, ${absentCount} Absent out of ${totalCount} recorded classes).`
            : "Your recorded attendance history.";
        }
      }
  } catch (error) {
    showToast(error.message, "error");
  }
}

async function loadMyAttendance() {
  await loadAttendance(currentUser.studentId, false);
}

async function saveAttendance(event) {
  event.preventDefault();
  const studentId = document.querySelector("#attendance-student").value;
  const subject = document.querySelector("#attendance-subject").value || (selectedFacultyClass ? selectedFacultyClass.subject : "");
  const date = document.querySelector("#attendance-date").value;
  const status = document.querySelector("#attendance-status").value;
  const message = document.querySelector("#attendance-form-error");
  message.textContent = "";

  if (!studentId) {
    message.textContent = "Please select a student.";
    document.querySelector("#attendance-student").focus();
    return;
  }
  if (!subject) {
    message.textContent = "Please select a subject.";
    return;
  }
  if (!date) {
    message.textContent = "Please enter a valid date.";
    document.querySelector("#attendance-date").focus();
    return;
  }
  if (!status) {
    message.textContent = "Please select an attendance status.";
    document.querySelector("#attendance-status").focus();
    return;
  }

  const attendanceId = document.querySelector("#attendance-id").value;
  try {
    if (attendanceId) {
      await apiRequest(`/api/attendance/${attendanceId}`, {
        method: "PUT",
        body: { subject, date, status }
      });
    } else {
      await apiRequest(`/api/students/${encodeURIComponent(studentId)}/attendance`, {
        method: "POST",
        body: { subject, date, status }
      });
    }
    resetAttendanceForm();
    await loadAttendance(studentId, true);
    showToast(attendanceId ? "Attendance updated." : "Attendance recorded.");
  } catch (error) {
    message.textContent = error.message;
  }
}

function resetAttendanceForm() {
  document.querySelector("#attendance-form").reset();
  document.querySelector("#attendance-id").value = "";
  document.querySelector("#attendance-form-error").textContent = "";
  document.querySelector("#save-attendance-button").textContent = "Save Attendance";
  document.querySelector("#cancel-attendance-edit").hidden = true;
  document.querySelector("#attendance-student").value = selectedStudentId;
  document.querySelector("#attendance-date").value = new Date().toISOString().slice(0, 10);
  document.querySelector("#attendance-status").value = "Present";
  if (selectedFacultyClass) {
    const subjectSelect = document.querySelector("#attendance-subject");
    subjectSelect.replaceChildren(new Option(selectedFacultyClass.subject, selectedFacultyClass.subject));
    subjectSelect.value = selectedFacultyClass.subject;
    subjectSelect.disabled = true;
    document.querySelector("#attendance-form-title").textContent = "Record Attendance";
    document.querySelector("#attendance-form-description").textContent =
      `Enter attendance details for ${selectedFacultyClass.subject}.`;
  }
}

async function exportStudents() {
  const query = buildStudentQuery();
  let response;
  try {
    response = await fetch(`/api/students/export?${query.toString()}`, {
      credentials: "same-origin",
      headers: { Accept: "text/csv" }
    });
  } catch (error) {
    showToast(`Could not reach the backend. ${error.message}`, "error");
    return;
  }
  if (!response.ok) {
    const error = await response.json().catch(() => ({}));
    showToast(error.error || `Export failed (${response.status}).`, "error");
    return;
  }
  const url = URL.createObjectURL(await response.blob());
  const link = document.createElement("a");
  link.href = url;
  link.download = "student-records.csv";
  link.click();
  URL.revokeObjectURL(url);
  showToast("Student records exported.");
}

loginForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const username = document.querySelector("#username").value.trim();
  const password = document.querySelector("#password").value;
  if (!username || !password) {
    loginError.textContent = "Please enter your username and password.";
    return;
  }
  try {
    const result = await apiRequest("/api/login", { method: "POST", body: { username, password } });
    loginError.textContent = "";
    await openApp(result.user, result.csrfToken);
  } catch (error) {
    loginError.textContent = error.message;
  }
});

document.querySelector("#toggle-password").addEventListener("click", (event) => {
  const input = document.querySelector("#password");
  const show = input.type === "password";
  input.type = show ? "text" : "password";
  event.currentTarget.textContent = show ? "Hide" : "Show";
  event.currentTarget.setAttribute("aria-label", show ? "Hide password" : "Show password");
});
document.querySelector("#logout-button").addEventListener("click", signOut);
document.querySelector("#profile-button").addEventListener("click", signOut);
document.querySelector("#back-button").addEventListener("click", goBack);
document.querySelector("#open-students-card").addEventListener("click", showAllStudents);
document.querySelector("#open-departments-card").addEventListener("click", () => navigate("departments"));
document.querySelector("#add-student-button").addEventListener("click", () => openStudentDialog());
document.querySelector("#close-student-dialog").addEventListener("click", () => studentDialog.close());
document.querySelector("#cancel-student-dialog").addEventListener("click", () => studentDialog.close());
document.querySelector("#student-form").addEventListener("submit", saveStudent);
document.querySelector("#confirm-delete").addEventListener("click", deleteStudent);
deleteDialog.addEventListener("close", () => { pendingDeleteId = ""; });
document.querySelector("#search-button").addEventListener("click", () => {
  const search = searchInput.value.trim();
  if (!search) {
    showInlineMessage(document.querySelector("#student-list-message"), "Please enter a Student ID or name to search.");
    return;
  }
  activeSearchTerm = search;
  document.querySelector("#clear-search-button").hidden = false;
  loadStudents();
});
searchInput.addEventListener("keydown", (event) => {
  if (event.key === "Enter") {
    event.preventDefault();
    document.querySelector("#search-button").click();
  }
});
document.querySelector("#clear-search-button").addEventListener("click", () => {
  activeSearchTerm = "";
  searchInput.value = "";
  document.querySelector("#clear-search-button").hidden = true;
  showInlineMessage(document.querySelector("#student-list-message"), "");
  loadStudents();
});
courseFilter.addEventListener("change", loadStudents);
semesterFilter.addEventListener("change", loadStudents);
document.querySelector("#export-button").addEventListener("click", exportStudents);
document.querySelector("#grade-form").addEventListener("submit", saveGrade);
document.querySelector("#grade-student").addEventListener("change", async (event) => {
  selectedStudentId = event.currentTarget.value;
  document.querySelector("#grade-id").value = "";
  resetGradeForm();
  await loadGrades(selectedStudentId, true);
});
document.querySelector("#attendance-form").addEventListener("submit", saveAttendance);
document.querySelector("#cancel-attendance-edit").addEventListener("click", resetAttendanceForm);
document.querySelector("#attendance-student").addEventListener("change", async (event) => {
  selectedStudentId = event.currentTarget.value;
  document.querySelector("#attendance-id").value = "";
  resetAttendanceForm();
  await loadAttendance(selectedStudentId, true);
});

document.querySelector("#faculty-selector").addEventListener("change", (event) => {
  selectedFacultyId = event.currentTarget.value;
  facultyAssignmentsEditing = false;
  document.querySelector("#faculty-access-message").hidden = true;
  showInlineMessage(document.querySelector("#faculty-assignment-save-message"), "");
  renderFacultyAssignmentGrid();
});
document.querySelector("#faculty-assignment-grid").addEventListener("change", updateFacultyAssignmentSummary);
document.querySelector("#save-faculty-assignments").addEventListener("click", saveFacultyAssignments);
document.querySelector("#edit-faculty-assignments").addEventListener("click", () => {
  facultyAssignmentsEditing = true;
  showInlineMessage(document.querySelector("#faculty-assignment-save-message"), "");
  renderFacultyAssignmentGrid();
});
document.querySelector("#cancel-faculty-assignments").addEventListener("click", () => {
  facultyAssignmentsEditing = false;
  renderFacultyAssignmentGrid();
});
document.querySelector("#create-faculty-button").addEventListener("click", () => {
  facultyForm.reset();
  document.querySelector("#faculty-form-error").textContent = "";
  facultyDialog.showModal();
  document.querySelector("#faculty-username").focus();
});
document.querySelector("#close-faculty-dialog").addEventListener("click", () => facultyDialog.close());
document.querySelector("#cancel-faculty-dialog").addEventListener("click", () => facultyDialog.close());
facultyForm.addEventListener("submit", createFaculty);
document.querySelector("#grade-window-back-departments").addEventListener("click", renderGradeWindowDepartments);
document.querySelector("#grade-window-back-semesters").addEventListener("click", () => {
  document.querySelector("#grade-window-editor").hidden = true;
  document.querySelector("#grade-window-semester-step").hidden = false;
});
document.querySelector("#grade-window-form").addEventListener("submit", saveGradeWindows);
document.querySelector("#cancel-grade-edit").addEventListener("click", resetGradeForm);
document.querySelector("#menu-button").addEventListener("click", () => {
  document.querySelector("#sidebar").classList.add("open");
  document.querySelector("#sidebar-scrim").hidden = false;
});
document.querySelector("#sidebar-scrim").addEventListener("click", () => {
  document.querySelector("#sidebar").classList.remove("open");
  document.querySelector("#sidebar-scrim").hidden = true;
});
document.addEventListener("keydown", (event) => {
  if (event.key === "Escape") {
    document.querySelector("#sidebar").classList.remove("open");
    document.querySelector("#sidebar-scrim").hidden = true;
  }
  if (event.key === "/" && !appScreen.hidden && !["INPUT", "TEXTAREA", "SELECT"].includes(document.activeElement.tagName)) {
    event.preventDefault();
    searchInput.focus();
  }
});

async function restoreSession() {
  try {
    const session = await apiRequest("/api/session");
    if (session.authenticated) await openApp(session.user, session.csrfToken);
  } catch (error) {
    loginError.textContent = error.message;
  }
}

restoreSession();
