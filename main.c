/*
 * main.c
 * Entry point for the Complaint Handling Management System (CHMS).
 * Drives the top-level menu, the logged-in user menu, and delegates
 * to the admin menu for administrator sessions.
 */
#include "common.h"
#include "utils.h"
#include "user.h"
#include "complaint.h"
#include "tracking.h"
#include "admin.h"
#include "report.h"
#include "notification.h"
#include "feedback.h"
#include "security.h"

static void userMenu(Session *session)
{
    int choice = -1;

    while (choice != 0 && session->isLoggedIn != 0) {
        printf("\n===== User Menu (%s) =====\n", session->username);
        printf(" 1. Register Complaint\n");
        printf(" 2. View My Complaints\n");
        printf(" 3. Track Complaint\n");
        printf(" 4. View Notifications\n");
        printf(" 5. Submit Feedback\n");
        printf(" 6. Change Password\n");
        printf(" 7. Search Complaints\n");
        printf(" 0. Logout\n");

        if (safeInputInt("Enter choice: ", 0, 7, &choice) == 0) {
            printf("Invalid choice.\n");
            continue;
        }

        switch (choice) {
            case 1: (void)registerComplaint(session->userId); break;
            case 2: viewComplaintsForUser(session->userId);   break;
            case 3: trackComplaint();                         break;
            case 4: viewNotifications(session->userId);       break;
            case 5: (void)submitFeedback(session->userId);    break;
            case 6: (void)changePassword(session);            break;
            case 7: searchComplaints();                       break;
            case 0: logoutUser(session);                      break;
            default: break;
        }
    }
}

static void mainMenu(void)
{
    Session session;
    memset(&session, 0, sizeof(session));

    int choice = -1;

    while (choice != 4) {
        printf("\n===================================\n");
        printf(" Complaint Handling Management System\n");
        printf("===================================\n");
        printf(" 1. Register User\n");
        printf(" 2. User Login\n");
        printf(" 3. Admin Login\n");
        printf(" 4. Exit\n");
        printf(" 5. Forgot Password\n");

        if (safeInputInt("Enter choice: ", 1, 5, &choice) == 0) {
            printf("Invalid choice. Please try again.\n");
            continue;
        }

        switch (choice) {
            case 1:
                (void)registerUser();
                break;
            case 2:
                if (loginUser(&session) == RESULT_SUCCESS) {
                    userMenu(&session);
                }
                break;
            case 3:
                if (loginAdmin(&session) == RESULT_SUCCESS) {
                    adminMenu(&session);
                }
                break;
            case 4:
                printf("Thank you for using CHMS. Goodbye!\n");
                break;
            case 5:
                (void)forgotPassword();
                break;
            default:
                break;
        }
    }
}

int main(void)
{
    logAudit("CHMS application started");
    mainMenu();
    logAudit("CHMS application exited");
    return 0;
}
