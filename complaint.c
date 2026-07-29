/*
 * complaint.c
 * Implementation of complaint registration, viewing, update,
 * deletion and search. Records are persisted as fixed-size binary
 * records in COMPLAINTS_FILE.
 */
#include "complaint.h"
#include "utils.h"
#include "notification.h"

int getNextComplaintId(void)
{
    FILE *file = fopen(COMPLAINTS_FILE, "rb");
    if (file == NULL) {
        return 1;
    }

    Complaint record;
    int maxId = 0;

    while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
        if (record.complaintId > maxId) {
            maxId = record.complaintId;
        }
    }

    (void)fclose(file);
    return maxId + 1;
}

Result findComplaintById(int complaintId, Complaint *outComplaint)
{
    if (outComplaint == NULL) {
        return RESULT_FAILURE;
    }

    FILE *file = fopen(COMPLAINTS_FILE, "rb");
    if (file == NULL) {
        return RESULT_FAILURE;
    }

    Complaint record;
    Result found = RESULT_FAILURE;

    while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
        if (record.complaintId == complaintId) {
            *outComplaint = record;
            found = RESULT_SUCCESS;
            break;
        }
    }

    (void)fclose(file);
    return found;
}

static Result saveComplaintRecord(const Complaint *updatedComplaint)
{
    if (updatedComplaint == NULL) {
        return RESULT_FAILURE;
    }

    FILE *file = fopen(COMPLAINTS_FILE, "r+b");
    if (file == NULL) {
        return RESULT_FAILURE;
    }

    Complaint record;
    Result saved = RESULT_FAILURE;
    long position = 0;

    while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
        if (record.complaintId == updatedComplaint->complaintId) {
            if (fseek(file, position, SEEK_SET) != 0) {
                break;
            }
            if (fwrite(updatedComplaint, sizeof(Complaint), 1U, file) == 1U) {
                saved = RESULT_SUCCESS;
            }
            break;
        }
        position = ftell(file);
    }

    (void)fclose(file);
    return saved;
}

const char *categoryToString(ComplaintCategory category)
{
    switch (category) {
        case CAT_ELECTRICAL:        return "Electrical";
        case CAT_WATER_SUPPLY:      return "Water Supply";
        case CAT_ROAD_DAMAGE:       return "Road Damage";
        case CAT_GARBAGE:           return "Garbage";
        case CAT_DRAINAGE:          return "Drainage";
        case CAT_STREET_LIGHT:      return "Street Light";
        case CAT_PUBLIC_TRANSPORT:  return "Public Transport";
        case CAT_INTERNET:          return "Internet";
        case CAT_HEALTH:            return "Health";
        case CAT_OTHERS:            return "Others";
        default:                    return "Unknown";
    }
}

const char *priorityToString(ComplaintPriority priority)
{
    switch (priority) {
        case PRIORITY_LOW:      return "Low";
        case PRIORITY_MEDIUM:   return "Medium";
        case PRIORITY_HIGH:     return "High";
        case PRIORITY_CRITICAL: return "Critical";
        default:                return "Unknown";
    }
}

const char *statusToString(ComplaintStatus status)
{
    switch (status) {
        case CSTATUS_REGISTERED:  return "Registered";
        case CSTATUS_ASSIGNED:    return "Assigned";
        case CSTATUS_IN_PROGRESS: return "In Progress";
        case CSTATUS_RESOLVED:    return "Resolved";
        case CSTATUS_CLOSED:      return "Closed";
        case CSTATUS_REJECTED:    return "Rejected";
        default:                  return "Unknown";
    }
}

void printComplaintDetails(const Complaint *complaint)
{
    char createdBuf[MAX_DATE_LEN];
    char updatedBuf[MAX_DATE_LEN];

    if (complaint == NULL) {
        return;
    }

    formatDateTime(&complaint->createdDate, createdBuf, sizeof(createdBuf));
    formatDateTime(&complaint->updatedDate, updatedBuf, sizeof(updatedBuf));

    printf("---------------------------------------------\n");
    printf("Complaint ID   : %d\n", complaint->complaintId);
    printf("User ID        : %d\n", complaint->userId);
    printf("Category       : %s\n", categoryToString(complaint->category));
    printf("Priority       : %s\n", priorityToString(complaint->priority));
    printf("Description    : %s\n", complaint->description);
    printf("Status         : %s\n", statusToString(complaint->status));
    printf("Assigned Staff : %s\n",
           isNullOrEmpty(complaint->assignedStaff) ? "(unassigned)" : complaint->assignedStaff);
    printf("Created On     : %s\n", createdBuf);
    printf("Last Updated   : %s\n", updatedBuf);
    printf("Remarks        : %s\n",
           isNullOrEmpty(complaint->remarks) ? "(none)" : complaint->remarks);
    printf("---------------------------------------------\n");
}

static int promptCategory(void)
{
    int choice = 0;
    printf("Select Category:\n");
    printf(" 1. Electrical\n 2. Water Supply\n 3. Road Damage\n 4. Garbage\n");
    printf(" 5. Drainage\n 6. Street Light\n 7. Public Transport\n 8. Internet\n");
    printf(" 9. Health\n10. Others\n");

    if (safeInputInt("Enter choice (1-10): ", 1, 10, &choice) == 0) {
        return -1;
    }
    return choice;
}

static int promptPriority(void)
{
    int choice = 0;
    printf("Select Priority:\n 1. Low\n 2. Medium\n 3. High\n 4. Critical\n");

    if (safeInputInt("Enter choice (1-4): ", 1, 4, &choice) == 0) {
        return -1;
    }
    return choice;
}

Result registerComplaint(int userId)
{
    Complaint newComplaint;
    memset(&newComplaint, 0, sizeof(newComplaint));

    printf("\n--- Register Complaint ---\n");

    int category = promptCategory();
    if (category < 0) {
        printf("Invalid category selection.\n");
        return RESULT_FAILURE;
    }

    int priority = promptPriority();
    if (priority < 0) {
        printf("Invalid priority selection.\n");
        return RESULT_FAILURE;
    }

    safeInputString("Enter complaint description: ", newComplaint.description,
                     sizeof(newComplaint.description));
    if (isNullOrEmpty(newComplaint.description)) {
        printf("Description cannot be empty.\n");
        return RESULT_FAILURE;
    }

    newComplaint.complaintId = getNextComplaintId();
    newComplaint.userId = userId;
    newComplaint.category = (ComplaintCategory)category;
    newComplaint.priority = (ComplaintPriority)priority;
    newComplaint.status = CSTATUS_REGISTERED;
    newComplaint.assignedStaff[0] = '\0';
    newComplaint.remarks[0] = '\0';
    getCurrentDateTime(&newComplaint.createdDate);
    newComplaint.updatedDate = newComplaint.createdDate;

    FILE *file = fopen(COMPLAINTS_FILE, "ab");
    if (file == NULL) {
        logErrorMessage("registerComplaint: unable to open complaints file");
        return RESULT_FAILURE;
    }

    Result writeResult = RESULT_FAILURE;
    if (fwrite(&newComplaint, sizeof(Complaint), 1U, file) == 1U) {
        writeResult = RESULT_SUCCESS;
    }
    (void)fclose(file);

    if (writeResult == RESULT_SUCCESS) {
        char notifyMsg[MAX_MESSAGE_LEN];
        (void)snprintf(notifyMsg, sizeof(notifyMsg),
                        "Complaint #%d registered successfully.", newComplaint.complaintId);
        (void)createNotification(userId, newComplaint.complaintId, notifyMsg);

        char logMsg[MAX_MESSAGE_LEN];
        (void)snprintf(logMsg, sizeof(logMsg), "Complaint #%d registered by user %d",
                        newComplaint.complaintId, userId);
        logAudit(logMsg);

        printf("Complaint registered successfully with ID %d.\n", newComplaint.complaintId);
    } else {
        printf("Failed to register complaint due to a file error.\n");
    }

    return writeResult;
}

void viewComplaintsForUser(int userId)
{
    FILE *file = fopen(COMPLAINTS_FILE, "rb");
    if (file == NULL) {
        printf("No complaints found.\n");
        return;
    }

    Complaint record;
    int count = 0;

    printf("\n--- Your Complaints ---\n");
    while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
        if (record.userId == userId) {
            printComplaintDetails(&record);
            count++;
        }
    }

    (void)fclose(file);

    if (count == 0) {
        printf("You have not registered any complaints yet.\n");
    }
}

void viewAllComplaints(void)
{
    FILE *file = fopen(COMPLAINTS_FILE, "rb");
    if (file == NULL) {
        printf("No complaints found.\n");
        return;
    }

    Complaint record;
    int count = 0;

    printf("\n--- All Complaints ---\n");
    while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
        printComplaintDetails(&record);
        count++;
    }

    (void)fclose(file);

    if (count == 0) {
        printf("No complaints have been registered yet.\n");
    }
}

Result updateComplaintStatus(int complaintId, ComplaintStatus newStatus, const char *remarks)
{
    Complaint existing;
    if (findComplaintById(complaintId, &existing) != RESULT_SUCCESS) {
        printf("Complaint #%d not found.\n", complaintId);
        return RESULT_FAILURE;
    }

    existing.status = newStatus;
    if (remarks != NULL) {
        (void)snprintf(existing.remarks, sizeof(existing.remarks), "%s", remarks);
    }
    getCurrentDateTime(&existing.updatedDate);

    Result saved = saveComplaintRecord(&existing);
    if (saved == RESULT_SUCCESS) {
        char notifyMsg[MAX_MESSAGE_LEN];
        const char *tag = (newStatus == CSTATUS_CLOSED) ? "closed" : "updated";
        (void)snprintf(notifyMsg, sizeof(notifyMsg), "Complaint #%d has been %s (status: %s).",
                        complaintId, tag, statusToString(newStatus));
        (void)createNotification(existing.userId, complaintId, notifyMsg);

        char logMsg[MAX_MESSAGE_LEN];
        (void)snprintf(logMsg, sizeof(logMsg), "Complaint #%d status changed to %s",
                        complaintId, statusToString(newStatus));
        logAudit(logMsg);

        printf("Complaint #%d updated successfully.\n", complaintId);
    } else {
        printf("Failed to update complaint #%d.\n", complaintId);
    }

    return saved;
}

Result assignComplaintStaff(int complaintId, const char *staffName)
{
    if (isNullOrEmpty(staffName)) {
        return RESULT_FAILURE;
    }

    Complaint existing;
    if (findComplaintById(complaintId, &existing) != RESULT_SUCCESS) {
        printf("Complaint #%d not found.\n", complaintId);
        return RESULT_FAILURE;
    }

    (void)snprintf(existing.assignedStaff, sizeof(existing.assignedStaff), "%s", staffName);
    existing.status = CSTATUS_ASSIGNED;
    getCurrentDateTime(&existing.updatedDate);

    Result saved = saveComplaintRecord(&existing);
    if (saved == RESULT_SUCCESS) {
        char notifyMsg[MAX_MESSAGE_LEN];
        (void)snprintf(notifyMsg, sizeof(notifyMsg),
                        "Complaint #%d has been assigned to %s.", complaintId, staffName);
        (void)createNotification(existing.userId, complaintId, notifyMsg);

        char logMsg[MAX_MESSAGE_LEN];
        (void)snprintf(logMsg, sizeof(logMsg), "Complaint #%d assigned to %s",
                        complaintId, staffName);
        logAudit(logMsg);

        printf("Complaint #%d assigned to %s.\n", complaintId, staffName);
    } else {
        printf("Failed to assign complaint #%d.\n", complaintId);
    }

    return saved;
}

Result deleteComplaintById(int complaintId)
{
    FILE *sourceFile = fopen(COMPLAINTS_FILE, "rb");
    if (sourceFile == NULL) {
        printf("No complaints found.\n");
        return RESULT_FAILURE;
    }

    FILE *tempFile = fopen("complaints.tmp", "wb");
    if (tempFile == NULL) {
        (void)fclose(sourceFile);
        logErrorMessage("deleteComplaintById: unable to create temp file");
        return RESULT_FAILURE;
    }

    Complaint record;
    int found = 0;

    while (fread(&record, sizeof(Complaint), 1U, sourceFile) == 1U) {
        if (record.complaintId == complaintId) {
            found = 1;
            continue;
        }
        (void)fwrite(&record, sizeof(Complaint), 1U, tempFile);
    }

    (void)fclose(sourceFile);
    (void)fclose(tempFile);

    if (found == 0) {
        (void)remove("complaints.tmp");
        printf("Complaint #%d not found.\n", complaintId);
        return RESULT_FAILURE;
    }

    if (remove(COMPLAINTS_FILE) != 0) {
        logErrorMessage("deleteComplaintById: unable to remove original complaints file");
        return RESULT_FAILURE;
    }

    if (rename("complaints.tmp", COMPLAINTS_FILE) != 0) {
        logErrorMessage("deleteComplaintById: unable to rename temp complaints file");
        return RESULT_FAILURE;
    }

    char logMsg[MAX_MESSAGE_LEN];
    (void)snprintf(logMsg, sizeof(logMsg), "Complaint #%d deleted", complaintId);
    logAudit(logMsg);

    printf("Complaint #%d deleted successfully.\n", complaintId);
    return RESULT_SUCCESS;
}

void searchComplaints(void)
{
    int searchMode = 0;
    printf("\n--- Search Complaints ---\n");
    printf(" 1. By Category\n 2. By Status\n 3. By Keyword in Description\n");

    if (safeInputInt("Enter choice (1-3): ", 1, 3, &searchMode) == 0) {
        printf("Invalid choice.\n");
        return;
    }

    FILE *file = fopen(COMPLAINTS_FILE, "rb");
    if (file == NULL) {
        printf("No complaints found.\n");
        return;
    }

    int matchCount = 0;
    Complaint record;

    if (searchMode == 1) {
        int category = promptCategory();
        if (category < 0) {
            (void)fclose(file);
            printf("Invalid category.\n");
            return;
        }
        while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
            if ((int)record.category == category) {
                printComplaintDetails(&record);
                matchCount++;
            }
        }
    } else if (searchMode == 2) {
        int statusChoice = 0;
        printf(" 1. Registered\n 2. Assigned\n 3. In Progress\n 4. Resolved\n 5. Closed\n 6. Rejected\n");
        if (safeInputInt("Enter status (1-6): ", 1, 6, &statusChoice) == 0) {
            (void)fclose(file);
            printf("Invalid status.\n");
            return;
        }
        while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
            if ((int)record.status == statusChoice) {
                printComplaintDetails(&record);
                matchCount++;
            }
        }
    } else {
        char keyword[MAX_DESC_LEN];
        safeInputString("Enter keyword: ", keyword, sizeof(keyword));
        while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
            if (strstr(record.description, keyword) != NULL) {
                printComplaintDetails(&record);
                matchCount++;
            }
        }
    }

    (void)fclose(file);

    if (matchCount == 0) {
        printf("No matching complaints found.\n");
    }
}
