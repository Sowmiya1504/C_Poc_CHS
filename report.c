/*
 * report.c
 * Implementation of report generation for complaints.
 */
#include "report.h"
#include "complaint.h"
#include "utils.h"

#define CATEGORY_COUNT 10
#define PRIORITY_COUNT 4
#define MONTH_COUNT    12

static void writeSummaryToStream(FILE *stream)
{
    FILE *file = fopen(COMPLAINTS_FILE, "rb");
    if (file == NULL) {
        fprintf(stream, "No complaints have been registered yet.\n");
        return;
    }

    Complaint record;
    int total = 0;
    int registeredCount = 0;
    int assignedCount = 0;
    int inProgressCount = 0;
    int resolvedCount = 0;
    int closedCount = 0;
    int rejectedCount = 0;

    while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
        total++;
        switch (record.status) {
            case CSTATUS_REGISTERED:  registeredCount++; break;
            case CSTATUS_ASSIGNED:    assignedCount++;   break;
            case CSTATUS_IN_PROGRESS: inProgressCount++; break;
            case CSTATUS_RESOLVED:    resolvedCount++;   break;
            case CSTATUS_CLOSED:      closedCount++;     break;
            case CSTATUS_REJECTED:    rejectedCount++;   break;
            default: break;
        }
    }

    (void)fclose(file);

    fprintf(stream, "=== Complaint Summary Report ===\n");
    fprintf(stream, "Total Complaints : %d\n", total);
    fprintf(stream, "Registered       : %d\n", registeredCount);
    fprintf(stream, "Assigned         : %d\n", assignedCount);
    fprintf(stream, "In Progress      : %d\n", inProgressCount);
    fprintf(stream, "Resolved         : %d\n", resolvedCount);
    fprintf(stream, "Closed           : %d\n", closedCount);
    fprintf(stream, "Rejected         : %d\n", rejectedCount);
}

void generateSummaryReport(void)
{
    printf("\n");
    writeSummaryToStream(stdout);
}

static void listByStatusPredicate(const char *title, int includeRegistered, int includeAssigned,
                                   int includeInProgress, int includeResolved, int includeClosed,
                                   int includeRejected)
{
    FILE *file = fopen(COMPLAINTS_FILE, "rb");
    if (file == NULL) {
        printf("No complaints found.\n");
        return;
    }

    Complaint record;
    int count = 0;

    printf("\n--- %s ---\n", title);
    while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
        int matches = 0;
        switch (record.status) {
            case CSTATUS_REGISTERED:  matches = includeRegistered;  break;
            case CSTATUS_ASSIGNED:    matches = includeAssigned;    break;
            case CSTATUS_IN_PROGRESS: matches = includeInProgress;  break;
            case CSTATUS_RESOLVED:    matches = includeResolved;    break;
            case CSTATUS_CLOSED:      matches = includeClosed;      break;
            case CSTATUS_REJECTED:    matches = includeRejected;    break;
            default: break;
        }

        if (matches != 0) {
            printComplaintDetails(&record);
            count++;
        }
    }

    (void)fclose(file);

    if (count == 0) {
        printf("No matching complaints.\n");
    }
}

void generatePendingReport(void)
{
    listByStatusPredicate("Pending Complaints", 1, 1, 1, 0, 0, 0);
}

void generateResolvedReport(void)
{
    listByStatusPredicate("Resolved Complaints", 0, 0, 0, 1, 1, 0);
}

void generateCategoryReport(void)
{
    FILE *file = fopen(COMPLAINTS_FILE, "rb");
    if (file == NULL) {
        printf("No complaints found.\n");
        return;
    }

    int counts[CATEGORY_COUNT + 1];
    memset(counts, 0, sizeof(counts));

    Complaint record;
    while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
        if ((int)record.category >= 1 && (int)record.category <= CATEGORY_COUNT) {
            counts[(int)record.category]++;
        }
    }

    (void)fclose(file);

    printf("\n--- Category-wise Report ---\n");
    for (int category = 1; category <= CATEGORY_COUNT; category++) {
        printf("%-18s: %d\n", categoryToString((ComplaintCategory)category), counts[category]);
    }
}

void generatePriorityReport(void)
{
    FILE *file = fopen(COMPLAINTS_FILE, "rb");
    if (file == NULL) {
        printf("No complaints found.\n");
        return;
    }

    int counts[PRIORITY_COUNT + 1];
    memset(counts, 0, sizeof(counts));

    Complaint record;
    while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
        if ((int)record.priority >= 1 && (int)record.priority <= PRIORITY_COUNT) {
            counts[(int)record.priority]++;
        }
    }

    (void)fclose(file);

    printf("\n--- Priority-wise Report ---\n");
    for (int priority = 1; priority <= PRIORITY_COUNT; priority++) {
        printf("%-10s: %d\n", priorityToString((ComplaintPriority)priority), counts[priority]);
    }
}

void generateMonthlyReport(void)
{
    FILE *file = fopen(COMPLAINTS_FILE, "rb");
    if (file == NULL) {
        printf("No complaints found.\n");
        return;
    }

    /* counts[year_index][month] - simple approach: track up to 5 distinct years */
    static const int MAX_YEARS = 5;
    int years[5];
    int counts[5][MONTH_COUNT + 1];
    int yearCount = 0;
    memset(years, 0, sizeof(years));
    memset(counts, 0, sizeof(counts));

    Complaint record;
    while (fread(&record, sizeof(Complaint), 1U, file) == 1U) {
        int year = record.createdDate.year;
        int month = record.createdDate.month;
        if (month < 1 || month > MONTH_COUNT) {
            continue;
        }

        int yearIndex = -1;
        for (int i = 0; i < yearCount; i++) {
            if (years[i] == year) {
                yearIndex = i;
                break;
            }
        }
        if (yearIndex < 0 && yearCount < MAX_YEARS) {
            yearIndex = yearCount;
            years[yearIndex] = year;
            yearCount++;
        }
        if (yearIndex >= 0) {
            counts[yearIndex][month]++;
        }
    }

    (void)fclose(file);

    static const char *monthNames[13] = {
        "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    printf("\n--- Monthly Report ---\n");
    if (yearCount == 0) {
        printf("No complaints have been registered yet.\n");
        return;
    }

    for (int i = 0; i < yearCount; i++) {
        printf("Year %d:\n", years[i]);
        for (int month = 1; month <= MONTH_COUNT; month++) {
            if (counts[i][month] > 0) {
                printf("  %s : %d\n", monthNames[month], counts[i][month]);
            }
        }
    }
}

Result exportReportToFile(const char *filename)
{
    if (isNullOrEmpty(filename)) {
        return RESULT_FAILURE;
    }

    FILE *outFile = fopen(filename, "w");
    if (outFile == NULL) {
        logErrorMessage("exportReportToFile: unable to open output file");
        return RESULT_FAILURE;
    }

    DateTime now;
    char timeBuf[MAX_DATE_LEN];
    getCurrentDateTime(&now);
    formatDateTime(&now, timeBuf, sizeof(timeBuf));

    fprintf(outFile, "CHMS Report generated on %s\n\n", timeBuf);
    writeSummaryToStream(outFile);

    (void)fclose(outFile);

    char logMsg[MAX_MESSAGE_LEN];
    (void)snprintf(logMsg, sizeof(logMsg), "Report exported to %s", filename);
    logAudit(logMsg);

    printf("Report exported to %s\n", filename);
    return RESULT_SUCCESS;
}
