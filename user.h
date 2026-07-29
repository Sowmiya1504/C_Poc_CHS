/*
 * user.h
 * User registration, login/logout and password management.
 */
#ifndef USER_H
#define USER_H

#include "common.h"

Result registerUser(void);
Result loginUser(Session *session);
void   logoutUser(Session *session);
Result changePassword(const Session *session);
Result forgotPassword(void);

Result findUserByUsername(const char *username, User *outUser);
Result findUserById(int userId, User *outUser);
int    getNextUserId(void);
int    isDuplicateUsername(const char *username);

#endif /* USER_H */
