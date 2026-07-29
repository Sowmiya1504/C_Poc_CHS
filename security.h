/*
 * security.h
 * Password hashing, validation and simple session helpers.
 */
#ifndef SECURITY_H
#define SECURITY_H

#include "common.h"

void hashPassword(const char *plainPassword, char *outHash, size_t outHashSize);
int  verifyPassword(const char *plainPassword, const char *storedHash);

int  validatePasswordStrength(const char *password);
int  validateUsernameFormat(const char *username);

Result authenticateAdmin(const char *username, const char *password);

void startSession(Session *session, int userId, const char *username, int isAdmin);
void endSession(Session *session);

#endif /* SECURITY_H */
