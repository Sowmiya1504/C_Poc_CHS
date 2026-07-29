/*
 * user.c
 * Implementation of user registration, login/logout and password
 * management. All user records are persisted as fixed-size binary
 * records in USERS_FILE.
 */
#include "user.h"
#include "utils.h"
#include "security.h"

int getNextUserId(void)
{
    FILE *file = fopen(USERS_FILE, "rb");
    if (file == NULL) {
        return 1;
    }

    User record;
    int maxId = 0;

    while (fread(&record, sizeof(User), 1U, file) == 1U) {
        if (record.userId > maxId) {
            maxId = record.userId;
        }
    }

    (void)fclose(file);
    return maxId + 1;
}

int isDuplicateUsername(const char *username)
{
    User existing;
    return (findUserByUsername(username, &existing) == RESULT_SUCCESS) ? 1 : 0;
}

Result findUserByUsername(const char *username, User *outUser)
{
    if (isNullOrEmpty(username) || outUser == NULL) {
        return RESULT_FAILURE;
    }

    FILE *file = fopen(USERS_FILE, "rb");
    if (file == NULL) {
        return RESULT_FAILURE;
    }

    User record;
    Result found = RESULT_FAILURE;

    while (fread(&record, sizeof(User), 1U, file) == 1U) {
        if (strcmp(record.username, username) == 0) {
            *outUser = record;
            found = RESULT_SUCCESS;
            break;
        }
    }

    (void)fclose(file);
    return found;
}

Result findUserById(int userId, User *outUser)
{
    if (outUser == NULL) {
        return RESULT_FAILURE;
    }

    FILE *file = fopen(USERS_FILE, "rb");
    if (file == NULL) {
        return RESULT_FAILURE;
    }

    User record;
    Result found = RESULT_FAILURE;

    while (fread(&record, sizeof(User), 1U, file) == 1U) {
        if (record.userId == userId) {
            *outUser = record;
            found = RESULT_SUCCESS;
            break;
        }
    }

    (void)fclose(file);
    return found;
}

/* Overwrite an existing user record in place (by userId). */
static Result saveUserRecord(const User *updatedUser)
{
    if (updatedUser == NULL) {
        return RESULT_FAILURE;
    }

    FILE *file = fopen(USERS_FILE, "r+b");
    if (file == NULL) {
        return RESULT_FAILURE;
    }

    User record;
    Result saved = RESULT_FAILURE;
    long position = 0;

    while (fread(&record, sizeof(User), 1U, file) == 1U) {
        if (record.userId == updatedUser->userId) {
            if (fseek(file, position, SEEK_SET) != 0) {
                break;
            }
            if (fwrite(updatedUser, sizeof(User), 1U, file) == 1U) {
                saved = RESULT_SUCCESS;
            }
            break;
        }
        position = ftell(file);
    }

    (void)fclose(file);
    return saved;
}

Result registerUser(void)
{
    User newUser;
    memset(&newUser, 0, sizeof(newUser));

    char plainPassword[MAX_PASSWORD_LEN];
    char confirmPassword[MAX_PASSWORD_LEN];

    printf("\n--- User Registration ---\n");

    safeInputString("Enter desired username: ", newUser.username, sizeof(newUser.username));
    if (validateUsernameFormat(newUser.username) == 0) {
        printf("Invalid username. Use 3-%d alphanumeric/underscore characters.\n",
               MAX_USERNAME_LEN - 1);
        return RESULT_FAILURE;
    }

    if (isDuplicateUsername(newUser.username) != 0) {
        printf("Username already exists. Please choose another.\n");
        return RESULT_FAILURE;
    }

    safeInputString("Enter full name: ", newUser.fullName, sizeof(newUser.fullName));
    safeInputString("Enter email: ", newUser.email, sizeof(newUser.email));
    safeInputString("Enter phone number: ", newUser.phone, sizeof(newUser.phone));

    safeInputString("Enter password: ", plainPassword, sizeof(plainPassword));
    if (validatePasswordStrength(plainPassword) == 0) {
        printf("Weak password. Minimum 8 characters with letters and digits.\n");
        secureZeroMemory(plainPassword, sizeof(plainPassword));
        return RESULT_FAILURE;
    }

    safeInputString("Confirm password: ", confirmPassword, sizeof(confirmPassword));
    if (strcmp(plainPassword, confirmPassword) != 0) {
        printf("Passwords do not match.\n");
        secureZeroMemory(plainPassword, sizeof(plainPassword));
        secureZeroMemory(confirmPassword, sizeof(confirmPassword));
        return RESULT_FAILURE;
    }

    hashPassword(plainPassword, newUser.passwordHash, sizeof(newUser.passwordHash));
    secureZeroMemory(plainPassword, sizeof(plainPassword));
    secureZeroMemory(confirmPassword, sizeof(confirmPassword));

    newUser.userId = getNextUserId();
    newUser.isActive = 1;
    getCurrentDateTime(&newUser.registeredOn);

    FILE *file = fopen(USERS_FILE, "ab");
    if (file == NULL) {
        logErrorMessage("registerUser: unable to open users file for writing");
        return RESULT_FAILURE;
    }

    Result writeResult = RESULT_FAILURE;
    if (fwrite(&newUser, sizeof(User), 1U, file) == 1U) {
        writeResult = RESULT_SUCCESS;
    }
    (void)fclose(file);

    if (writeResult == RESULT_SUCCESS) {
        char logMsg[MAX_MESSAGE_LEN];
        (void)snprintf(logMsg, sizeof(logMsg), "New user registered: %s (ID %d)",
                        newUser.username, newUser.userId);
        logAudit(logMsg);
        printf("Registration successful. Your User ID is %d.\n", newUser.userId);
    } else {
        printf("Registration failed due to a file error.\n");
    }

    return writeResult;
}

Result loginUser(Session *session)
{
    if (session == NULL) {
        return RESULT_FAILURE;
    }

    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    User foundUser;

    printf("\n--- User Login ---\n");

    for (int attempt = 1; attempt <= MAX_LOGIN_ATTEMPTS; attempt++) {
        safeInputString("Username: ", username, sizeof(username));
        safeInputString("Password: ", password, sizeof(password));

        if (findUserByUsername(username, &foundUser) == RESULT_SUCCESS &&
            foundUser.isActive != 0 &&
            verifyPassword(password, foundUser.passwordHash) != 0) {

            secureZeroMemory(password, sizeof(password));
            startSession(session, foundUser.userId, foundUser.username, 0);
            printf("Login successful. Welcome, %s!\n", foundUser.fullName);

            char logMsg[MAX_MESSAGE_LEN];
            (void)snprintf(logMsg, sizeof(logMsg), "User login: %s (ID %d)",
                            foundUser.username, foundUser.userId);
            logAudit(logMsg);
            return RESULT_SUCCESS;
        }

        secureZeroMemory(password, sizeof(password));
        printf("Invalid credentials. Attempt %d of %d.\n", attempt, MAX_LOGIN_ATTEMPTS);
    }

    logErrorMessage("loginUser: maximum login attempts exceeded");
    printf("Maximum login attempts exceeded.\n");
    return RESULT_FAILURE;
}

void logoutUser(Session *session)
{
    if (session == NULL) {
        return;
    }

    char logMsg[MAX_MESSAGE_LEN];
    (void)snprintf(logMsg, sizeof(logMsg), "User logout: %s (ID %d)",
                    session->username, session->userId);
    logAudit(logMsg);

    endSession(session);
    printf("Logged out successfully.\n");
}

Result changePassword(const Session *session)
{
    if (session == NULL || session->isLoggedIn == 0) {
        return RESULT_FAILURE;
    }

    User currentUser;
    if (findUserById(session->userId, &currentUser) != RESULT_SUCCESS) {
        return RESULT_FAILURE;
    }

    char oldPassword[MAX_PASSWORD_LEN];
    char newPassword[MAX_PASSWORD_LEN];
    char confirmPassword[MAX_PASSWORD_LEN];

    safeInputString("Current password: ", oldPassword, sizeof(oldPassword));
    if (verifyPassword(oldPassword, currentUser.passwordHash) == 0) {
        printf("Current password is incorrect.\n");
        secureZeroMemory(oldPassword, sizeof(oldPassword));
        return RESULT_FAILURE;
    }
    secureZeroMemory(oldPassword, sizeof(oldPassword));

    safeInputString("New password: ", newPassword, sizeof(newPassword));
    if (validatePasswordStrength(newPassword) == 0) {
        printf("Weak password. Minimum 8 characters with letters and digits.\n");
        secureZeroMemory(newPassword, sizeof(newPassword));
        return RESULT_FAILURE;
    }

    safeInputString("Confirm new password: ", confirmPassword, sizeof(confirmPassword));
    if (strcmp(newPassword, confirmPassword) != 0) {
        printf("Passwords do not match.\n");
        secureZeroMemory(newPassword, sizeof(newPassword));
        secureZeroMemory(confirmPassword, sizeof(confirmPassword));
        return RESULT_FAILURE;
    }

    hashPassword(newPassword, currentUser.passwordHash, sizeof(currentUser.passwordHash));
    secureZeroMemory(newPassword, sizeof(newPassword));
    secureZeroMemory(confirmPassword, sizeof(confirmPassword));

    Result saved = saveUserRecord(&currentUser);
    if (saved == RESULT_SUCCESS) {
        printf("Password changed successfully.\n");
        logAudit("Password changed");
    } else {
        printf("Failed to update password.\n");
    }

    return saved;
}

Result forgotPassword(void)
{
    char username[MAX_USERNAME_LEN];
    char email[MAX_EMAIL_LEN];
    User foundUser;

    printf("\n--- Forgot Password ---\n");
    safeInputString("Enter username: ", username, sizeof(username));
    safeInputString("Enter registered email: ", email, sizeof(email));

    if (findUserByUsername(username, &foundUser) != RESULT_SUCCESS ||
        strcmp(foundUser.email, email) != 0) {
        printf("No matching account found.\n");
        return RESULT_FAILURE;
    }

    char newPassword[MAX_PASSWORD_LEN];
    char confirmPassword[MAX_PASSWORD_LEN];

    safeInputString("Enter new password: ", newPassword, sizeof(newPassword));
    if (validatePasswordStrength(newPassword) == 0) {
        printf("Weak password. Minimum 8 characters with letters and digits.\n");
        secureZeroMemory(newPassword, sizeof(newPassword));
        return RESULT_FAILURE;
    }

    safeInputString("Confirm new password: ", confirmPassword, sizeof(confirmPassword));
    if (strcmp(newPassword, confirmPassword) != 0) {
        printf("Passwords do not match.\n");
        secureZeroMemory(newPassword, sizeof(newPassword));
        secureZeroMemory(confirmPassword, sizeof(confirmPassword));
        return RESULT_FAILURE;
    }

    hashPassword(newPassword, foundUser.passwordHash, sizeof(foundUser.passwordHash));
    secureZeroMemory(newPassword, sizeof(newPassword));
    secureZeroMemory(confirmPassword, sizeof(confirmPassword));

    Result saved = saveUserRecord(&foundUser);
    if (saved == RESULT_SUCCESS) {
        printf("Password reset successfully. You may now log in.\n");
        logAudit("Password reset via forgot-password flow");
    } else {
        printf("Failed to reset password.\n");
    }

    return saved;
}
