/*
 * tracking.c
 * Implementation of complaint tracking by Complaint ID.
 */
#include "tracking.h"
#include "complaint.h"
#include "utils.h"

#define RESOLUTION_DAYS_CRITICAL 1
#define RESOLUTION_DAYS_HIGH     3
#define RESOLUTION_DAYS_MEDIUM   7
#define RESOLUTION_DAYS_LOW      14

static int expectedResolutionDays(ComplaintPriority priority)
{
    switch (priority) {
        case PRIORITY_CRITICAL: return RESOLUTION_DAYS_CRITICAL;
        case PRIORITY_HIGH:     return RESOLUTION_DAYS_HIGH;
        case PRIORITY_MEDIUM:   return RESOLUTION_DAYS_MEDIUM;
        case PRIORITY_LOW:      return RESOLUTION_DAYS_LOW;
        default:                return RESOLUTION_DAYS_MEDIUM;
    }
}

void trackComplaint(void)
{
    int complaintId = 0;

    printf("\n--- Track Complaint ---\n");
    if (safeInputInt("Enter Complaint ID: ", 1, 1000000, &complaintId) == 0) {
        printf("Invalid Complaint ID.\n");
        return;
    }

    Complaint found;
    if (findComplaintById(complaintId, &found) != RESULT_SUCCESS) {
        printf("Complaint #%d not found.\n", complaintId);
        return;
    }

    char updatedBuf[MAX_DATE_LEN];
    char expectedBuf[MAX_DATE_LEN];
    DateTime expectedResolution;

    formatDateTime(&found.updatedDate, updatedBuf, sizeof(updatedBuf));

    if (found.status == CSTATUS_RESOLVED || found.status == CSTATUS_CLOSED) {
        printf("\nComplaint #%d has already been %s.\n", complaintId,
               (found.status == CSTATUS_RESOLVED) ? "resolved" : "closed");
    } else {
        addDaysToDate(&found.createdDate, expectedResolutionDays(found.priority), &expectedResolution);
        formatDateTime(&expectedResolution, expectedBuf, sizeof(expectedBuf));
        printf("\nExpected Resolution By: %s\n", expectedBuf);
    }

    printf("\n--- Tracking Details ---\n");
    printf("Status         : %s\n", statusToString(found.status));
    printf("Assigned Staff : %s\n",
           isNullOrEmpty(found.assignedStaff) ? "(unassigned)" : found.assignedStaff);
    printf("Last Updated   : %s\n", updatedBuf);
    printf("\n");
    printComplaintDetails(&found);
}
