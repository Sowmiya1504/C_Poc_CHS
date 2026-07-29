/*
 * admin.c
 * Implementation of administrator login, user listing and the
 * admin menu loop, tying together complaint, report and feedback
 * modules for administrative actions.
 */
#include "admin.h"
#include "utils.h"
#include "security.h"
#include "complaint.h"
#include "report.h"
#include "feedback.h"

Result loginAdmin(Session *session)
{
    if (session == NULL) {
        return RESULT_FAILURE;
    }

    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];

    printf("\n--- Admin Login ---\n");

    for (int attempt = 1; attempt <= MAX_LOGIN_ATTEMPTS; attempt++) {
        safeInputString("Admin Username: ", username, sizeof(username));
        safeInputString("Admin Password: ", password, sizeof(password));

        if (authenticateAdmin(username, password) == RESULT_SUCCESS) {
            secureZeroMemory(password, sizeof(password));
            startSession(session, 0, username, 1);
            printf("Admin login successful.\n");
            logAudit("Admin login successful");
            return RESULT_SUCCESS;
        }

        secureZeroMemory(password, sizeof(password));
        printf("Invalid admin credentials. Attempt %d of %d.\n", attempt, MAX_LOGIN_ATTEMPTS);
    }

    logErrorMessage("loginAdmin: maximum login attempts exceeded");
    printf("Maximum login attempts exceeded.\n");
    return RESULT_FAILURE;
}

void viewAllUsers(void)
{
    FILE *file = fopen(USERS_FILE, "rb");
    if (file == NULL) {
        printf("No users found.\n");
        return;
    }

    User record;
    int count = 0;
    char regBuf[MAX_DATE_LEN];

    printf("\n--- All Registered Users ---\n");
    while (fread(&record, sizeof(User), 1U, file) == 1U) {
        formatDateTime(&record.registeredOn, regBuf, sizeof(regBuf));
        printf("ID: %-4d Username: %-20s Name: %-25s Email: %-25s Registered: %s %s\n",
               record.userId, record.username, record.fullName, record.email, regBuf,
               (record.isActive != 0) ? "" : "(inactive)");
        count++;
    }

    (void)fclose(file);

    if (count == 0) {
        printf("No users have registered yet.\n");
    }
}

static void handleAssignComplaint(void)
{
    int complaintId = 0;
    char staffName[MAX_STAFF_LEN];

    if (safeInputInt("Enter Complaint ID to assign: ", 1, 1000000, &complaintId) == 0) {
        printf("Invalid Complaint ID.\n");
        return;
    }

    safeInputString("Enter staff name to assign: ", staffName, sizeof(staffName));
    if (isNullOrEmpty(staffName)) {
        printf("Staff name cannot be empty.\n");
        return;
    }

    (void)assignComplaintStaff(complaintId, staffName);
}

static void handleUpdateComplaintStatus(void)
{
    int complaintId = 0;
    int statusChoice = 0;
    char remarks[MAX_REMARKS_LEN];

    if (safeInputInt("Enter Complaint ID to update: ", 1, 1000000, &complaintId) == 0) {
        printf("Invalid Complaint ID.\n");
        return;
    }

    printf("Select new status:\n");
    printf(" 1. Registered\n 2. Assigned\n 3. In Progress\n 4. Resolved\n 5. Closed\n 6. Rejected\n");
    if (safeInputInt("Enter choice (1-6): ", 1, 6, &statusChoice) == 0) {
        printf("Invalid status.\n");
        return;
    }

    safeInputString("Enter remarks: ", remarks, sizeof(remarks));

    (void)updateComplaintStatus(complaintId, (ComplaintStatus)statusChoice, remarks);
}

static void handleDeleteComplaint(void)
{
    int complaintId = 0;
    if (safeInputInt("Enter Complaint ID to delete: ", 1, 1000000, &complaintId) == 0) {
        printf("Invalid Complaint ID.\n");
        return;
    }

    (void)deleteComplaintById(complaintId);
}

static void reportsSubMenu(void)
{
    int choice = -1;

    while (choice != 0) {
        printf("\n--- Reports Menu ---\n");
        printf(" 1. Summary Report\n");
        printf(" 2. Pending Report\n");
        printf(" 3. Resolved Report\n");
        printf(" 4. Category Report\n");
        printf(" 5. Priority Report\n");
        printf(" 6. Monthly Report\n");
        printf(" 7. Export Summary Report to File\n");
        printf(" 0. Back\n");

        if (safeInputInt("Enter choice: ", 0, 7, &choice) == 0) {
            printf("Invalid choice.\n");
            continue;
        }

        switch (choice) {
            case 1: generateSummaryReport();  break;
            case 2: generatePendingReport();  break;
            case 3: generateResolvedReport(); break;
            case 4: generateCategoryReport(); break;
            case 5: generatePriorityReport(); break;
            case 6: generateMonthlyReport();  break;
            case 7: (void)exportReportToFile("summary_report.txt"); break;
            case 0: break;
            default: break;
        }
    }
}

void adminMenu(Session *session)
{
    int choice = -1;

    while (choice != 0 && session->isLoggedIn != 0) {
        printf("\n===== Admin Menu =====\n");
        printf(" 1. View Users\n");
        printf(" 2. View Complaints\n");
        printf(" 3. Assign Complaint\n");
        printf(" 4. Update Complaint Status\n");
        printf(" 5. Delete Complaint\n");
        printf(" 6. Generate Reports\n");
        printf(" 7. View Feedback Report\n");
        printf(" 8. Search Complaints\n");
        printf(" 0. Logout\n");

        if (safeInputInt("Enter choice: ", 0, 8, &choice) == 0) {
            printf("Invalid choice.\n");
            continue;
        }

        switch (choice) {
            case 1: viewAllUsers();               break;
            case 2: viewAllComplaints();           break;
            case 3: handleAssignComplaint();       break;
            case 4: handleUpdateComplaintStatus(); break;
            case 5: handleDeleteComplaint();       break;
            case 6: reportsSubMenu();              break;
            case 7: generateFeedbackReport();      break;
            case 8: searchComplaints();            break;
            case 0:
                logAudit("Admin logout");
                endSession(session);
                printf("Admin logged out.\n");
                break;
            default: break;
        }
    }
}
