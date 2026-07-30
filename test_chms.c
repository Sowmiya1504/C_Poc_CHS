/*
 * test_chms.c
 *
 * CUnit unit test suite for the Complaint Handling Management System.
 *
 * SCOPE
 * -----
 * This suite covers the functions that can be exercised without
 * interactive stdin input: string/date utilities, password hashing
 * and validation, and the file-persistence (CRUD) functions in
 * user.c, complaint.c, notification.c and feedback.c.
 *
 * The menu-driven wrapper functions (registerUser, loginUser,
 * registerComplaint, adminMenu, etc.) call safeInputString /
 * safeInputInt directly against stdin and are therefore integration-
 * tested manually via the running program rather than here. See the
 * note at the bottom of this file for a suggested refactor
 * (dependency injection of an input source) if fully automated
 * coverage of those flows is required later.
 *
 * Each suite runs in its own temporary working directory so that
 * users.dat / complaints.dat / notifications.dat / feedback.dat /
 * audit.log / error.log created by the functions under test never
 * touch the developer's real data files.
 *
 * BUILD (Linux - requires `sudo apt install libcunit1 libcunit1-dev`):
 *   gcc -std=c11 -Wall -Wextra -Wpedantic \
 *       test_chms.c utils.c security.c user.c complaint.c \
 *       notification.c feedback.c tracking.c report.c admin.c \
 *       -lcunit -o run_tests
 *   ./run_tests
 *
 * BUILD (Windows, MinGW - requires CUnit built/installed for MinGW,
 * e.g. via vcpkg: `vcpkg install cunit:x64-mingw-static`):
 *   gcc -std=c11 -Wall -Wextra -Wpedantic ^
 *       test_chms.c utils.c security.c user.c complaint.c ^
 *       notification.c feedback.c tracking.c report.c admin.c ^
 *       -lcunit -o run_tests.exe
 *   run_tests.exe
 *
 * IMPORTANT: do NOT link main.c into the test binary - it defines
 * its own main() and would collide with the CUnit runner's main()
 * at the bottom of this file.
 */
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include "common.h"
#include "utils.h"
#include "security.h"
#include "user.h"
#include "complaint.h"
#include "notification.h"
#include "feedback.h"

#if defined(_WIN32)
#include <direct.h>
#define TEST_MKDIR(path) _mkdir(path)
#define TEST_CHDIR(path) _chdir(path)
#else
#include <sys/stat.h>
#include <unistd.h>
#define TEST_MKDIR(path) mkdir((path), 0700)
#define TEST_CHDIR(path) chdir(path)
#endif

/* -------------------------------------------------------------- */
/* Shared test fixtures                                           */
/* -------------------------------------------------------------- */
#define TEST_WORK_DIR "chms_test_workspace"

/* Remove any data/log files left over from a previous run so every
 * test suite starts from a clean, deterministic state. */
static void removeAllDataFiles(void)
{
    (void)remove(USERS_FILE);
    (void)remove(COMPLAINTS_FILE);
    (void)remove(NOTIFICATIONS_FILE);
    (void)remove(FEEDBACK_FILE);
    (void)remove(AUDIT_LOG_FILE);
    (void)remove(ERROR_LOG_FILE);
}

/* Runs once before any suite: create and enter an isolated working
 * directory so test-generated data files never collide with real
 * project data files. */
static int globalSuiteInit(void)
{
    (void)TEST_MKDIR(TEST_WORK_DIR);
    if (TEST_CHDIR(TEST_WORK_DIR) != 0) {
        return -1;
    }
    removeAllDataFiles();
    return 0;
}

static int globalSuiteClean(void)
{
    removeAllDataFiles();
    return 0;
}

/* -------------------------------------------------------------- */
/* Helpers to build fixture records directly, bypassing the        */
/* interactive registration/complaint-filing flows.                */
/* -------------------------------------------------------------- */
static void makeSampleUser(User *user, int userId, const char *username, const char *plainPassword)
{
    memset(user, 0, sizeof(*user));
    user->userId = userId;
    (void)snprintf(user->username, sizeof(user->username), "%s", username);
    hashPassword(plainPassword, user->passwordHash, sizeof(user->passwordHash));
    (void)snprintf(user->fullName, sizeof(user->fullName), "Test User %d", userId);
    (void)snprintf(user->email, sizeof(user->email), "user%d@example.com", userId);
    (void)snprintf(user->phone, sizeof(user->phone), "9000000%03d", userId);
    getCurrentDateTime(&user->registeredOn);
    user->isActive = 1;
}

static Result appendUserRecord(const User *user)
{
    FILE *file = fopen(USERS_FILE, "ab");
    if (file == NULL) {
        return RESULT_FAILURE;
    }
    Result result = (fwrite(user, sizeof(User), 1U, file) == 1U) ? RESULT_SUCCESS : RESULT_FAILURE;
    (void)fclose(file);
    return result;
}

static void makeSampleComplaint(Complaint *complaint, int complaintId, int userId,
                                 ComplaintCategory category, ComplaintPriority priority,
                                 ComplaintStatus status)
{
    memset(complaint, 0, sizeof(*complaint));
    complaint->complaintId = complaintId;
    complaint->userId = userId;
    complaint->category = category;
    complaint->priority = priority;
    complaint->status = status;
    (void)snprintf(complaint->description, sizeof(complaint->description),
                    "Sample complaint %d description", complaintId);
    getCurrentDateTime(&complaint->createdDate);
    complaint->updatedDate = complaint->createdDate;
}

static Result appendComplaintRecord(const Complaint *complaint)
{
    FILE *file = fopen(COMPLAINTS_FILE, "ab");
    if (file == NULL) {
        return RESULT_FAILURE;
    }
    Result result = (fwrite(complaint, sizeof(Complaint), 1U, file) == 1U) ? RESULT_SUCCESS : RESULT_FAILURE;
    (void)fclose(file);
    return result;
}

static Result appendFeedbackRecord(int feedbackId, int complaintId, int userId, int rating)
{
    Feedback feedback;
    memset(&feedback, 0, sizeof(feedback));
    feedback.feedbackId = feedbackId;
    feedback.complaintId = complaintId;
    feedback.userId = userId;
    feedback.rating = rating;
    (void)snprintf(feedback.feedbackText, sizeof(feedback.feedbackText), "Feedback %d", feedbackId);
    getCurrentDateTime(&feedback.submittedOn);

    FILE *file = fopen(FEEDBACK_FILE, "ab");
    if (file == NULL) {
        return RESULT_FAILURE;
    }
    Result result = (fwrite(&feedback, sizeof(Feedback), 1U, file) == 1U) ? RESULT_SUCCESS : RESULT_FAILURE;
    (void)fclose(file);
    return result;
}

/* ================================================================ */
/* Suite: Utils                                                     */
/* ================================================================ */
static int utilsSuiteInit(void)
{
    return 0;
}

static int utilsSuiteClean(void)
{
    return 0;
}

static void test_trimString_removesLeadingAndTrailingWhitespace(void)
{
    char buffer[32];

    (void)snprintf(buffer, sizeof(buffer), "  hello world  ");
    trimString(buffer);
    CU_ASSERT_STRING_EQUAL(buffer, "hello world");

    (void)snprintf(buffer, sizeof(buffer), "no_padding");
    trimString(buffer);
    CU_ASSERT_STRING_EQUAL(buffer, "no_padding");

    (void)snprintf(buffer, sizeof(buffer), "   ");
    trimString(buffer);
    CU_ASSERT_STRING_EQUAL(buffer, "");
}

static void test_trimString_handlesNullSafely(void)
{
    /* Must not crash; defensive-programming contract. */
    trimString(NULL);
    CU_ASSERT_TRUE(1);
}

static void test_isNullOrEmpty_detectsBothCases(void)
{
    CU_ASSERT_TRUE(isNullOrEmpty(NULL) != 0);
    CU_ASSERT_TRUE(isNullOrEmpty("") != 0);
    CU_ASSERT_TRUE(isNullOrEmpty("x") == 0);
}

static void test_formatDateTime_producesExpectedLayout(void)
{
    DateTime sample;
    char buffer[MAX_DATE_LEN];

    sample.year = 2026;
    sample.month = 7;
    sample.day = 30;
    sample.hour = 9;
    sample.minute = 5;
    sample.second = 3;

    formatDateTime(&sample, buffer, sizeof(buffer));
    CU_ASSERT_STRING_EQUAL(buffer, "2026-07-30 09:05:03");
}

static void test_addDaysToDate_rollsOverMonthBoundary(void)
{
    DateTime start;
    DateTime result;

    start.year = 2026;
    start.month = 1;
    start.day = 30;
    start.hour = 0;
    start.minute = 0;
    start.second = 0;

    addDaysToDate(&start, 3, &result);

    CU_ASSERT_EQUAL(result.year, 2026);
    CU_ASSERT_EQUAL(result.month, 2);
    CU_ASSERT_EQUAL(result.day, 2);
}

static void test_addDaysToDate_handlesLeapYearFebruary(void)
{
    DateTime start;
    DateTime result;

    /* 2028 is a leap year: Feb has 29 days. */
    start.year = 2028;
    start.month = 2;
    start.day = 28;
    start.hour = 0;
    start.minute = 0;
    start.second = 0;

    addDaysToDate(&start, 1, &result);

    CU_ASSERT_EQUAL(result.month, 2);
    CU_ASSERT_EQUAL(result.day, 29);
}

/* ================================================================ */
/* Suite: Security                                                  */
/* ================================================================ */
static int securitySuiteInit(void)
{
    return 0;
}

static int securitySuiteClean(void)
{
    return 0;
}

static void test_hashPassword_isDeterministic(void)
{
    char hashA[MAX_HASH_LEN];
    char hashB[MAX_HASH_LEN];

    hashPassword("Passw0rd!", hashA, sizeof(hashA));
    hashPassword("Passw0rd!", hashB, sizeof(hashB));

    CU_ASSERT_STRING_EQUAL(hashA, hashB);
}

static void test_hashPassword_differsForDifferentInput(void)
{
    char hashA[MAX_HASH_LEN];
    char hashB[MAX_HASH_LEN];

    hashPassword("Passw0rd!", hashA, sizeof(hashA));
    hashPassword("Different1", hashB, sizeof(hashB));

    CU_ASSERT_TRUE(strcmp(hashA, hashB) != 0);
}

static void test_hashPassword_neverStoresPlainText(void)
{
    char hash[MAX_HASH_LEN];

    hashPassword("Passw0rd!", hash, sizeof(hash));
    CU_ASSERT_TRUE(strstr(hash, "Passw0rd") == NULL);
}

static void test_verifyPassword_acceptsCorrectPassword(void)
{
    char hash[MAX_HASH_LEN];

    hashPassword("Passw0rd!", hash, sizeof(hash));
    CU_ASSERT_TRUE(verifyPassword("Passw0rd!", hash) != 0);
}

static void test_verifyPassword_rejectsIncorrectPassword(void)
{
    char hash[MAX_HASH_LEN];

    hashPassword("Passw0rd!", hash, sizeof(hash));
    CU_ASSERT_TRUE(verifyPassword("WrongPass1", hash) == 0);
}

static void test_validatePasswordStrength_rejectsShortOrSimplePasswords(void)
{
    CU_ASSERT_TRUE(validatePasswordStrength("short1") == 0);      /* too short */
    CU_ASSERT_TRUE(validatePasswordStrength("alletters") == 0);   /* no digit */
    CU_ASSERT_TRUE(validatePasswordStrength("12345678") == 0);    /* no letter */
    CU_ASSERT_TRUE(validatePasswordStrength(NULL) == 0);
    CU_ASSERT_TRUE(validatePasswordStrength("") == 0);
}

static void test_validatePasswordStrength_acceptsStrongPassword(void)
{
    CU_ASSERT_TRUE(validatePasswordStrength("Passw0rd!") != 0);
}

static void test_validateUsernameFormat_rejectsInvalidUsernames(void)
{
    CU_ASSERT_TRUE(validateUsernameFormat("ab") == 0);            /* too short */
    CU_ASSERT_TRUE(validateUsernameFormat("bad name") == 0);      /* space */
    CU_ASSERT_TRUE(validateUsernameFormat("bad@name") == 0);      /* symbol */
    CU_ASSERT_TRUE(validateUsernameFormat(NULL) == 0);
    CU_ASSERT_TRUE(validateUsernameFormat("") == 0);
}

static void test_validateUsernameFormat_acceptsValidUsername(void)
{
    CU_ASSERT_TRUE(validateUsernameFormat("alice_01") != 0);
}

static void test_authenticateAdmin_correctCredentials(void)
{
    CU_ASSERT_EQUAL(authenticateAdmin(ADMIN_USERNAME, ADMIN_DEFAULT_PASSWORD), RESULT_SUCCESS);
}

static void test_authenticateAdmin_wrongCredentials(void)
{
    CU_ASSERT_EQUAL(authenticateAdmin(ADMIN_USERNAME, "wrong-password"), RESULT_FAILURE);
    CU_ASSERT_EQUAL(authenticateAdmin("not-admin", ADMIN_DEFAULT_PASSWORD), RESULT_FAILURE);
}

static void test_startSessionAndEndSession_updateStateCorrectly(void)
{
    Session session;
    memset(&session, 0, sizeof(session));

    startSession(&session, 7, "alice_01", 0);
    CU_ASSERT_EQUAL(session.isLoggedIn, 1);
    CU_ASSERT_EQUAL(session.userId, 7);
    CU_ASSERT_STRING_EQUAL(session.username, "alice_01");
    CU_ASSERT_EQUAL(session.isAdmin, 0);

    endSession(&session);
    CU_ASSERT_EQUAL(session.isLoggedIn, 0);
    CU_ASSERT_EQUAL(session.userId, 0);
    CU_ASSERT_STRING_EQUAL(session.username, "");
}

/* ================================================================ */
/* Suite: Complaint enum-to-string helpers                          */
/* ================================================================ */
static int enumSuiteInit(void)
{
    return 0;
}

static int enumSuiteClean(void)
{
    return 0;
}

static void test_categoryToString_mapsKnownValues(void)
{
    CU_ASSERT_STRING_EQUAL(categoryToString(CAT_ELECTRICAL), "Electrical");
    CU_ASSERT_STRING_EQUAL(categoryToString(CAT_OTHERS), "Others");
}

static void test_categoryToString_handlesUnknownValueSafely(void)
{
    CU_ASSERT_STRING_EQUAL(categoryToString((ComplaintCategory)999), "Unknown");
}

static void test_priorityToString_mapsKnownValues(void)
{
    CU_ASSERT_STRING_EQUAL(priorityToString(PRIORITY_LOW), "Low");
    CU_ASSERT_STRING_EQUAL(priorityToString(PRIORITY_CRITICAL), "Critical");
}

static void test_statusToString_mapsKnownValues(void)
{
    CU_ASSERT_STRING_EQUAL(statusToString(CSTATUS_REGISTERED), "Registered");
    CU_ASSERT_STRING_EQUAL(statusToString(CSTATUS_CLOSED), "Closed");
}

/* ================================================================ */
/* Suite: User persistence (user.c)                                 */
/* ================================================================ */
static int userSuiteInit(void)
{
    (void)remove(USERS_FILE);
    return 0;
}

static int userSuiteClean(void)
{
    (void)remove(USERS_FILE);
    return 0;
}

static void test_getNextUserId_startsAtOneWhenFileMissing(void)
{
    (void)remove(USERS_FILE);
    CU_ASSERT_EQUAL(getNextUserId(), 1);
}

static void test_getNextUserId_incrementsAfterRecordsExist(void)
{
    User user1;
    User user2;

    (void)remove(USERS_FILE);
    makeSampleUser(&user1, 1, "alice_01", "Passw0rd!");
    makeSampleUser(&user2, 2, "bob_02", "Passw0rd!");

    CU_ASSERT_EQUAL(appendUserRecord(&user1), RESULT_SUCCESS);
    CU_ASSERT_EQUAL(appendUserRecord(&user2), RESULT_SUCCESS);

    CU_ASSERT_EQUAL(getNextUserId(), 3);
}

static void test_findUserByUsername_findsExistingUser(void)
{
    User user1;
    User found;

    (void)remove(USERS_FILE);
    makeSampleUser(&user1, 1, "alice_01", "Passw0rd!");
    CU_ASSERT_EQUAL(appendUserRecord(&user1), RESULT_SUCCESS);

    CU_ASSERT_EQUAL(findUserByUsername("alice_01", &found), RESULT_SUCCESS);
    CU_ASSERT_EQUAL(found.userId, 1);
    CU_ASSERT_TRUE(verifyPassword("Passw0rd!", found.passwordHash) != 0);
}

static void test_findUserByUsername_returnsFailureWhenMissing(void)
{
    User found;

    (void)remove(USERS_FILE);
    CU_ASSERT_EQUAL(findUserByUsername("nobody", &found), RESULT_FAILURE);
}

static void test_findUserById_findsExistingUser(void)
{
    User user1;
    User found;

    (void)remove(USERS_FILE);
    makeSampleUser(&user1, 5, "carol_05", "Passw0rd!");
    CU_ASSERT_EQUAL(appendUserRecord(&user1), RESULT_SUCCESS);

    CU_ASSERT_EQUAL(findUserById(5, &found), RESULT_SUCCESS);
    CU_ASSERT_STRING_EQUAL(found.username, "carol_05");
}

static void test_isDuplicateUsername_detectsExistingUsername(void)
{
    User user1;

    (void)remove(USERS_FILE);
    makeSampleUser(&user1, 1, "alice_01", "Passw0rd!");
    CU_ASSERT_EQUAL(appendUserRecord(&user1), RESULT_SUCCESS);

    CU_ASSERT_TRUE(isDuplicateUsername("alice_01") != 0);
    CU_ASSERT_TRUE(isDuplicateUsername("someone_else") == 0);
}

/* ================================================================ */
/* Suite: Complaint persistence (complaint.c)                       */
/* ================================================================ */
static int complaintSuiteInit(void)
{
    (void)remove(COMPLAINTS_FILE);
    (void)remove(NOTIFICATIONS_FILE);
    return 0;
}

static int complaintSuiteClean(void)
{
    (void)remove(COMPLAINTS_FILE);
    (void)remove(NOTIFICATIONS_FILE);
    return 0;
}

static void test_getNextComplaintId_startsAtOneWhenFileMissing(void)
{
    (void)remove(COMPLAINTS_FILE);
    CU_ASSERT_EQUAL(getNextComplaintId(), 1);
}

static void test_findComplaintById_findsExistingComplaint(void)
{
    Complaint complaint1;
    Complaint found;

    (void)remove(COMPLAINTS_FILE);
    makeSampleComplaint(&complaint1, 1, 1, CAT_WATER_SUPPLY, PRIORITY_HIGH, CSTATUS_REGISTERED);
    CU_ASSERT_EQUAL(appendComplaintRecord(&complaint1), RESULT_SUCCESS);

    CU_ASSERT_EQUAL(findComplaintById(1, &found), RESULT_SUCCESS);
    CU_ASSERT_EQUAL(found.category, CAT_WATER_SUPPLY);
    CU_ASSERT_EQUAL(found.priority, PRIORITY_HIGH);
}

static void test_findComplaintById_returnsFailureWhenMissing(void)
{
    Complaint found;

    (void)remove(COMPLAINTS_FILE);
    CU_ASSERT_EQUAL(findComplaintById(999, &found), RESULT_FAILURE);
}

static void test_updateComplaintStatus_persistsNewStatusAndRemarks(void)
{
    Complaint complaint1;
    Complaint found;

    (void)remove(COMPLAINTS_FILE);
    (void)remove(NOTIFICATIONS_FILE);
    makeSampleComplaint(&complaint1, 1, 1, CAT_ROAD_DAMAGE, PRIORITY_MEDIUM, CSTATUS_REGISTERED);
    CU_ASSERT_EQUAL(appendComplaintRecord(&complaint1), RESULT_SUCCESS);

    CU_ASSERT_EQUAL(updateComplaintStatus(1, CSTATUS_IN_PROGRESS, "Crew dispatched"), RESULT_SUCCESS);

    CU_ASSERT_EQUAL(findComplaintById(1, &found), RESULT_SUCCESS);
    CU_ASSERT_EQUAL(found.status, CSTATUS_IN_PROGRESS);
    CU_ASSERT_STRING_EQUAL(found.remarks, "Crew dispatched");
}

static void test_assignComplaintStaff_setsStaffAndStatus(void)
{
    Complaint complaint1;
    Complaint found;

    (void)remove(COMPLAINTS_FILE);
    (void)remove(NOTIFICATIONS_FILE);
    makeSampleComplaint(&complaint1, 1, 1, CAT_ELECTRICAL, PRIORITY_LOW, CSTATUS_REGISTERED);
    CU_ASSERT_EQUAL(appendComplaintRecord(&complaint1), RESULT_SUCCESS);

    CU_ASSERT_EQUAL(assignComplaintStaff(1, "John Tech"), RESULT_SUCCESS);

    CU_ASSERT_EQUAL(findComplaintById(1, &found), RESULT_SUCCESS);
    CU_ASSERT_STRING_EQUAL(found.assignedStaff, "John Tech");
    CU_ASSERT_EQUAL(found.status, CSTATUS_ASSIGNED);
}

static void test_assignComplaintStaff_rejectsEmptyStaffName(void)
{
    CU_ASSERT_EQUAL(assignComplaintStaff(1, ""), RESULT_FAILURE);
    CU_ASSERT_EQUAL(assignComplaintStaff(1, NULL), RESULT_FAILURE);
}

static void test_deleteComplaintById_removesOnlyTargetRecord(void)
{
    Complaint complaint1;
    Complaint complaint2;
    Complaint found;

    (void)remove(COMPLAINTS_FILE);
    makeSampleComplaint(&complaint1, 1, 1, CAT_GARBAGE, PRIORITY_LOW, CSTATUS_REGISTERED);
    makeSampleComplaint(&complaint2, 2, 1, CAT_DRAINAGE, PRIORITY_HIGH, CSTATUS_REGISTERED);
    CU_ASSERT_EQUAL(appendComplaintRecord(&complaint1), RESULT_SUCCESS);
    CU_ASSERT_EQUAL(appendComplaintRecord(&complaint2), RESULT_SUCCESS);

    CU_ASSERT_EQUAL(deleteComplaintById(1), RESULT_SUCCESS);

    CU_ASSERT_EQUAL(findComplaintById(1, &found), RESULT_FAILURE);
    CU_ASSERT_EQUAL(findComplaintById(2, &found), RESULT_SUCCESS);
}

static void test_deleteComplaintById_returnsFailureWhenMissing(void)
{
    (void)remove(COMPLAINTS_FILE);
    CU_ASSERT_EQUAL(deleteComplaintById(42), RESULT_FAILURE);
}

/* ================================================================ */
/* Suite: Notification persistence (notification.c)                 */
/* ================================================================ */
static int notificationSuiteInit(void)
{
    (void)remove(NOTIFICATIONS_FILE);
    return 0;
}

static int notificationSuiteClean(void)
{
    (void)remove(NOTIFICATIONS_FILE);
    return 0;
}

static void test_getNextNotificationId_startsAtOneWhenFileMissing(void)
{
    (void)remove(NOTIFICATIONS_FILE);
    CU_ASSERT_EQUAL(getNextNotificationId(), 1);
}

static void test_createNotification_persistsRecordAndIncrementsId(void)
{
    (void)remove(NOTIFICATIONS_FILE);

    CU_ASSERT_EQUAL(createNotification(1, 10, "Complaint #10 registered."), RESULT_SUCCESS);
    CU_ASSERT_EQUAL(getNextNotificationId(), 2);

    CU_ASSERT_EQUAL(createNotification(1, 10, "Complaint #10 assigned."), RESULT_SUCCESS);
    CU_ASSERT_EQUAL(getNextNotificationId(), 3);
}

static void test_createNotification_rejectsEmptyMessage(void)
{
    CU_ASSERT_EQUAL(createNotification(1, 10, ""), RESULT_FAILURE);
    CU_ASSERT_EQUAL(createNotification(1, 10, NULL), RESULT_FAILURE);
}

/* ================================================================ */
/* Suite: Feedback persistence (feedback.c)                         */
/* ================================================================ */
static int feedbackSuiteInit(void)
{
    (void)remove(FEEDBACK_FILE);
    return 0;
}

static int feedbackSuiteClean(void)
{
    (void)remove(FEEDBACK_FILE);
    return 0;
}

static void test_getNextFeedbackId_startsAtOneWhenFileMissing(void)
{
    (void)remove(FEEDBACK_FILE);
    CU_ASSERT_EQUAL(getNextFeedbackId(), 1);
}

static void test_calculateAverageRating_returnsZeroWhenNoFeedback(void)
{
    (void)remove(FEEDBACK_FILE);
    CU_ASSERT_DOUBLE_EQUAL(calculateAverageRating(), 0.0, 0.0001);
}

static void test_calculateAverageRating_computesCorrectMean(void)
{
    (void)remove(FEEDBACK_FILE);

    CU_ASSERT_EQUAL(appendFeedbackRecord(1, 10, 1, 5), RESULT_SUCCESS);
    CU_ASSERT_EQUAL(appendFeedbackRecord(2, 11, 1, 3), RESULT_SUCCESS);
    CU_ASSERT_EQUAL(appendFeedbackRecord(3, 12, 2, 4), RESULT_SUCCESS);

    /* (5 + 3 + 4) / 3 = 4.0 */
    CU_ASSERT_DOUBLE_EQUAL(calculateAverageRating(), 4.0, 0.0001);
}

/* ================================================================ */
/* Test runner wiring                                               */
/* ================================================================ */
static int addUtilsSuite(void)
{
    CU_pSuite suite = CU_add_suite("UtilsSuite", utilsSuiteInit, utilsSuiteClean);
    if (suite == NULL) {
        return -1;
    }

    if ((CU_add_test(suite, "trimString removes leading/trailing whitespace",
                      test_trimString_removesLeadingAndTrailingWhitespace) == NULL) ||
        (CU_add_test(suite, "trimString handles NULL safely",
                      test_trimString_handlesNullSafely) == NULL) ||
        (CU_add_test(suite, "isNullOrEmpty detects NULL and empty string",
                      test_isNullOrEmpty_detectsBothCases) == NULL) ||
        (CU_add_test(suite, "formatDateTime produces expected layout",
                      test_formatDateTime_producesExpectedLayout) == NULL) ||
        (CU_add_test(suite, "addDaysToDate rolls over month boundary",
                      test_addDaysToDate_rollsOverMonthBoundary) == NULL) ||
        (CU_add_test(suite, "addDaysToDate handles leap year February",
                      test_addDaysToDate_handlesLeapYearFebruary) == NULL)) {
        return -1;
    }
    return 0;
}

static int addSecuritySuite(void)
{
    CU_pSuite suite = CU_add_suite("SecuritySuite", securitySuiteInit, securitySuiteClean);
    if (suite == NULL) {
        return -1;
    }

    if ((CU_add_test(suite, "hashPassword is deterministic",
                      test_hashPassword_isDeterministic) == NULL) ||
        (CU_add_test(suite, "hashPassword differs for different input",
                      test_hashPassword_differsForDifferentInput) == NULL) ||
        (CU_add_test(suite, "hashPassword never stores plain text",
                      test_hashPassword_neverStoresPlainText) == NULL) ||
        (CU_add_test(suite, "verifyPassword accepts correct password",
                      test_verifyPassword_acceptsCorrectPassword) == NULL) ||
        (CU_add_test(suite, "verifyPassword rejects incorrect password",
                      test_verifyPassword_rejectsIncorrectPassword) == NULL) ||
        (CU_add_test(suite, "validatePasswordStrength rejects weak passwords",
                      test_validatePasswordStrength_rejectsShortOrSimplePasswords) == NULL) ||
        (CU_add_test(suite, "validatePasswordStrength accepts strong password",
                      test_validatePasswordStrength_acceptsStrongPassword) == NULL) ||
        (CU_add_test(suite, "validateUsernameFormat rejects invalid usernames",
                      test_validateUsernameFormat_rejectsInvalidUsernames) == NULL) ||
        (CU_add_test(suite, "validateUsernameFormat accepts valid username",
                      test_validateUsernameFormat_acceptsValidUsername) == NULL) ||
        (CU_add_test(suite, "authenticateAdmin accepts correct credentials",
                      test_authenticateAdmin_correctCredentials) == NULL) ||
        (CU_add_test(suite, "authenticateAdmin rejects wrong credentials",
                      test_authenticateAdmin_wrongCredentials) == NULL) ||
        (CU_add_test(suite, "startSession/endSession update state correctly",
                      test_startSessionAndEndSession_updateStateCorrectly) == NULL)) {
        return -1;
    }
    return 0;
}

static int addEnumSuite(void)
{
    CU_pSuite suite = CU_add_suite("ComplaintEnumSuite", enumSuiteInit, enumSuiteClean);
    if (suite == NULL) {
        return -1;
    }

    if ((CU_add_test(suite, "categoryToString maps known values",
                      test_categoryToString_mapsKnownValues) == NULL) ||
        (CU_add_test(suite, "categoryToString handles unknown value safely",
                      test_categoryToString_handlesUnknownValueSafely) == NULL) ||
        (CU_add_test(suite, "priorityToString maps known values",
                      test_priorityToString_mapsKnownValues) == NULL) ||
        (CU_add_test(suite, "statusToString maps known values",
                      test_statusToString_mapsKnownValues) == NULL)) {
        return -1;
    }
    return 0;
}

static int addUserSuite(void)
{
    CU_pSuite suite = CU_add_suite("UserPersistenceSuite", userSuiteInit, userSuiteClean);
    if (suite == NULL) {
        return -1;
    }

    if ((CU_add_test(suite, "getNextUserId starts at 1 when file missing",
                      test_getNextUserId_startsAtOneWhenFileMissing) == NULL) ||
        (CU_add_test(suite, "getNextUserId increments after records exist",
                      test_getNextUserId_incrementsAfterRecordsExist) == NULL) ||
        (CU_add_test(suite, "findUserByUsername finds existing user",
                      test_findUserByUsername_findsExistingUser) == NULL) ||
        (CU_add_test(suite, "findUserByUsername returns failure when missing",
                      test_findUserByUsername_returnsFailureWhenMissing) == NULL) ||
        (CU_add_test(suite, "findUserById finds existing user",
                      test_findUserById_findsExistingUser) == NULL) ||
        (CU_add_test(suite, "isDuplicateUsername detects existing username",
                      test_isDuplicateUsername_detectsExistingUsername) == NULL)) {
        return -1;
    }
    return 0;
}

static int addComplaintSuite(void)
{
    CU_pSuite suite = CU_add_suite("ComplaintPersistenceSuite", complaintSuiteInit, complaintSuiteClean);
    if (suite == NULL) {
        return -1;
    }

    if ((CU_add_test(suite, "getNextComplaintId starts at 1 when file missing",
                      test_getNextComplaintId_startsAtOneWhenFileMissing) == NULL) ||
        (CU_add_test(suite, "findComplaintById finds existing complaint",
                      test_findComplaintById_findsExistingComplaint) == NULL) ||
        (CU_add_test(suite, "findComplaintById returns failure when missing",
                      test_findComplaintById_returnsFailureWhenMissing) == NULL) ||
        (CU_add_test(suite, "updateComplaintStatus persists new status and remarks",
                      test_updateComplaintStatus_persistsNewStatusAndRemarks) == NULL) ||
        (CU_add_test(suite, "assignComplaintStaff sets staff and status",
                      test_assignComplaintStaff_setsStaffAndStatus) == NULL) ||
        (CU_add_test(suite, "assignComplaintStaff rejects empty staff name",
                      test_assignComplaintStaff_rejectsEmptyStaffName) == NULL) ||
        (CU_add_test(suite, "deleteComplaintById removes only target record",
                      test_deleteComplaintById_removesOnlyTargetRecord) == NULL) ||
        (CU_add_test(suite, "deleteComplaintById returns failure when missing",
                      test_deleteComplaintById_returnsFailureWhenMissing) == NULL)) {
        return -1;
    }
    return 0;
}

static int addNotificationSuite(void)
{
    CU_pSuite suite = CU_add_suite("NotificationPersistenceSuite", notificationSuiteInit, notificationSuiteClean);
    if (suite == NULL) {
        return -1;
    }

    if ((CU_add_test(suite, "getNextNotificationId starts at 1 when file missing",
                      test_getNextNotificationId_startsAtOneWhenFileMissing) == NULL) ||
        (CU_add_test(suite, "createNotification persists record and increments id",
                      test_createNotification_persistsRecordAndIncrementsId) == NULL) ||
        (CU_add_test(suite, "createNotification rejects empty message",
                      test_createNotification_rejectsEmptyMessage) == NULL)) {
        return -1;
    }
    return 0;
}

static int addFeedbackSuite(void)
{
    CU_pSuite suite = CU_add_suite("FeedbackPersistenceSuite", feedbackSuiteInit, feedbackSuiteClean);
    if (suite == NULL) {
        return -1;
    }

    if ((CU_add_test(suite, "getNextFeedbackId starts at 1 when file missing",
                      test_getNextFeedbackId_startsAtOneWhenFileMissing) == NULL) ||
        (CU_add_test(suite, "calculateAverageRating returns 0 when no feedback",
                      test_calculateAverageRating_returnsZeroWhenNoFeedback) == NULL) ||
        (CU_add_test(suite, "calculateAverageRating computes correct mean",
                      test_calculateAverageRating_computesCorrectMean) == NULL)) {
        return -1;
    }
    return 0;
}

int main(void)
{
    if (globalSuiteInit() != 0) {
        (void)fprintf(stderr, "Failed to set up isolated test workspace.\n");
        return (int)CUE_SINIT;
    }

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return (int)CU_get_error();
    }

    if ((addUtilsSuite() != 0) ||
        (addSecuritySuite() != 0) ||
        (addEnumSuite() != 0) ||
        (addUserSuite() != 0) ||
        (addComplaintSuite() != 0) ||
        (addNotificationSuite() != 0) ||
        (addFeedbackSuite() != 0)) {
        CU_cleanup_registry();
        return (int)CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int failures = CU_get_number_of_failures();

    CU_cleanup_registry();
    (void)globalSuiteClean();

    return (failures > 0U) ? 1 : 0;
}
