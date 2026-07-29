/*
 * admin.h
 * Administrator login and the admin menu loop.
 */
#ifndef ADMIN_H
#define ADMIN_H

#include "common.h"

Result loginAdmin(Session *session);
void   viewAllUsers(void);
void   adminMenu(Session *session);

#endif /* ADMIN_H */
