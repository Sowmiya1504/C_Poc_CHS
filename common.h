/*
 * common.h
 * Shared macros, enums and structures used across the whole
 * Complaint Handling Management System (CHMS).
 *
 * This header contains ONLY declarations / definitions of
 * constants, enums and structs - no function implementations,
 * per project coding standards.
 */
#ifndef COMMON_H
#define COMMON_H

/* Expose POSIX.1-2008 functions (e.g. localtime_r) under strict C11 mode
 * on Linux/macOS. Harmless no-op on Windows/MinGW. Must be defined before
 * any system header is included. */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* -------------------------------------------------------------- */
/* Buffer size / limit macros (no magic numbers in .c files)      */
/* -------------------------------------------------------------- */
#define MAX_NAME_LEN        50
#define MAX_USERNAME_LEN    30
#define MAX_PASSWORD_LEN    64
#define MAX_HASH_LEN        65
#define MAX_EMAIL_LEN       60
#define MAX_PHONE_LEN       15
#define MAX_DESC_LEN        500
#define MAX_REMARKS_LEN     250
#define MAX_DATE_LEN        32
#define MAX_STAFF_LEN       50
#define MAX_FEEDBACK_LEN    300
#define MAX_MESSAGE_LEN      200
#define MAX_LINE_LEN        512

#define MAX_LOGIN_ATTEMPTS  3

/* -------------------------------------------------------------- */
/* Data file names                                                */
/* -------------------------------------------------------------- */
#define USERS_FILE          "users.dat"
#define COMPLAINTS_FILE     "complaints.dat"
#define NOTIFICATIONS_FILE  "notifications.dat"
#define FEEDBACK_FILE       "feedback.dat"
#define AUDIT_LOG_FILE      "audit.log"
#define ERROR_LOG_FILE      "error.log"

/* -------------------------------------------------------------- */
/* Built-in administrator credentials (demo defaults)             */
/* -------------------------------------------------------------- */
#define ADMIN_USERNAME      "admin"
#define ADMIN_DEFAULT_PASSWORD "Admin@123"

/* -------------------------------------------------------------- */
/* Generic operation result                                       */
/* -------------------------------------------------------------- */
typedef enum {
    RESULT_FAILURE = 0,
    RESULT_SUCCESS = 1
} Result;

/* -------------------------------------------------------------- */
/* Complaint domain enums                                         */
/* -------------------------------------------------------------- */
typedef enum {
    CAT_ELECTRICAL = 1,
    CAT_WATER_SUPPLY,
    CAT_ROAD_DAMAGE,
    CAT_GARBAGE,
    CAT_DRAINAGE,
    CAT_STREET_LIGHT,
    CAT_PUBLIC_TRANSPORT,
    CAT_INTERNET,
    CAT_HEALTH,
    CAT_OTHERS
} ComplaintCategory;

typedef enum {
    PRIORITY_LOW = 1,
    PRIORITY_MEDIUM,
    PRIORITY_HIGH,
    PRIORITY_CRITICAL
} ComplaintPriority;

typedef enum {
    CSTATUS_REGISTERED = 1,
    CSTATUS_ASSIGNED,
    CSTATUS_IN_PROGRESS,
    CSTATUS_RESOLVED,
    CSTATUS_CLOSED,
    CSTATUS_REJECTED
} ComplaintStatus;

/* -------------------------------------------------------------- */
/* Date / time structure                                          */
/* -------------------------------------------------------------- */
typedef struct {
    int day;
    int month;
    int year;
    int hour;
    int minute;
    int second;
} DateTime;

/* -------------------------------------------------------------- */
/* Core entity structures                                         */
/* -------------------------------------------------------------- */
typedef struct {
    int      userId;
    char     username[MAX_USERNAME_LEN];
    char     passwordHash[MAX_HASH_LEN];
    char     fullName[MAX_NAME_LEN];
    char     email[MAX_EMAIL_LEN];
    char     phone[MAX_PHONE_LEN];
    DateTime registeredOn;
    int      isActive;
} User;

typedef struct {
    int                complaintId;
    int                userId;
    ComplaintCategory  category;
    ComplaintPriority  priority;
    char               description[MAX_DESC_LEN];
    ComplaintStatus    status;
    char               assignedStaff[MAX_STAFF_LEN];
    DateTime           createdDate;
    DateTime           updatedDate;
    char               remarks[MAX_REMARKS_LEN];
} Complaint;

typedef struct {
    int      notificationId;
    int      userId;
    int      complaintId;
    char     message[MAX_MESSAGE_LEN];
    DateTime createdOn;
    int      isRead;
} Notification;

typedef struct {
    int      feedbackId;
    int      complaintId;
    int      userId;
    int      rating;
    char     feedbackText[MAX_FEEDBACK_LEN];
    DateTime submittedOn;
} Feedback;

/* -------------------------------------------------------------- */
/* Session structure - tracks who is currently logged in          */
/* -------------------------------------------------------------- */
typedef struct {
    int  isLoggedIn;
    int  userId;
    char username[MAX_USERNAME_LEN];
    int  isAdmin;
    int  loginAttempts;
} Session;

#endif /* COMMON_H */
