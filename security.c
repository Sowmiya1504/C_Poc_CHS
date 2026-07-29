/*
 * security.c
 * Implementation of password hashing/validation and session helpers.
 *
 * NOTE: This project has no external crypto library available, so a
 * salted djb2-style hash is used to avoid storing plain-text
 * passwords in users.dat. This is sufficient for demonstrating
 * secure-coding *practice* (no plaintext persistence) but is NOT a
 * cryptographically strong hash and should be replaced with a
 * vetted algorithm (e.g. bcrypt/argon2) in a real deployment.
 */
#include "security.h"
#include "utils.h"

#define PASSWORD_SALT "CHMS_STATIC_SALT_v1"
#define MIN_PASSWORD_LEN 8

static unsigned long djb2Hash(const char *str)
{
    unsigned long hash = 5381UL;
    int c;

    if (str == NULL) {
        return hash;
    }

    while ((c = (unsigned char)*str) != 0) {
        hash = ((hash << 5) + hash) + (unsigned long)c; /* hash * 33 + c */
        str++;
    }
    return hash;
}

/* Produce a deterministic hex-digest hash for the given password. */
void hashPassword(const char *plainPassword, char *outHash, size_t outHashSize)
{
    char salted[MAX_PASSWORD_LEN + sizeof(PASSWORD_SALT) + 1];
    unsigned long h1;
    unsigned long h2;

    if (outHash == NULL || outHashSize == 0U) {
        return;
    }

    outHash[0] = '\0';

    if (plainPassword == NULL) {
        return;
    }

    (void)snprintf(salted, sizeof(salted), "%s%s", PASSWORD_SALT, plainPassword);
    h1 = djb2Hash(salted);

    /* Second pass over the reversed salted string increases avalanche
       so single-character password changes shift more hex digits. */
    size_t saltedLen = strlen(salted);
    for (size_t i = 0U; i < saltedLen / 2U; i++) {
        char tmp = salted[i];
        salted[i] = salted[saltedLen - 1U - i];
        salted[saltedLen - 1U - i] = tmp;
    }
    h2 = djb2Hash(salted);

    (void)snprintf(outHash, outHashSize, "%08lx%08lx", h1, h2);
}

int verifyPassword(const char *plainPassword, const char *storedHash)
{
    char computedHash[MAX_HASH_LEN];

    if (plainPassword == NULL || storedHash == NULL) {
        return 0;
    }

    hashPassword(plainPassword, computedHash, sizeof(computedHash));

    int isEqual = (strcmp(computedHash, storedHash) == 0);
    secureZeroMemory(computedHash, sizeof(computedHash));
    return isEqual;
}

int validatePasswordStrength(const char *password)
{
    if (isNullOrEmpty(password)) {
        return 0;
    }

    size_t len = strlen(password);
    if (len < (size_t)MIN_PASSWORD_LEN) {
        return 0;
    }

    int hasDigit = 0;
    int hasAlpha = 0;

    for (size_t i = 0U; i < len; i++) {
        if (isdigit((unsigned char)password[i]) != 0) {
            hasDigit = 1;
        } else if (isalpha((unsigned char)password[i]) != 0) {
            hasAlpha = 1;
        } else {
            /* punctuation/symbols are allowed but not required */
        }
    }

    return (hasDigit != 0 && hasAlpha != 0) ? 1 : 0;
}

int validateUsernameFormat(const char *username)
{
    if (isNullOrEmpty(username)) {
        return 0;
    }

    size_t len = strlen(username);
    if (len < 3U || len >= (size_t)MAX_USERNAME_LEN) {
        return 0;
    }

    for (size_t i = 0U; i < len; i++) {
        unsigned char ch = (unsigned char)username[i];
        if (isalnum(ch) == 0 && ch != (unsigned char)'_') {
            return 0;
        }
    }

    return 1;
}

Result authenticateAdmin(const char *username, const char *password)
{
    if (isNullOrEmpty(username) || isNullOrEmpty(password)) {
        return RESULT_FAILURE;
    }

    if (strcmp(username, ADMIN_USERNAME) != 0) {
        return RESULT_FAILURE;
    }

    if (strcmp(password, ADMIN_DEFAULT_PASSWORD) != 0) {
        return RESULT_FAILURE;
    }

    return RESULT_SUCCESS;
}

void startSession(Session *session, int userId, const char *username, int isAdmin)
{
    if (session == NULL) {
        return;
    }

    session->isLoggedIn = 1;
    session->userId = userId;
    session->isAdmin = isAdmin;
    session->loginAttempts = 0;

    if (username != NULL) {
        (void)snprintf(session->username, sizeof(session->username), "%s", username);
    } else {
        session->username[0] = '\0';
    }
}

void endSession(Session *session)
{
    if (session == NULL) {
        return;
    }

    session->isLoggedIn = 0;
    session->userId = 0;
    session->isAdmin = 0;
    session->loginAttempts = 0;
    session->username[0] = '\0';
}
