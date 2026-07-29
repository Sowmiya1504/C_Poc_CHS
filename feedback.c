/*
 * feedback.c
 * Implementation of post-resolution feedback capture and reporting.
 */
#include "feedback.h"
#include "complaint.h"
#include "utils.h"

#define MIN_RATING 1
#define MAX_RATING 5

int getNextFeedbackId(void)
{
    FILE *file = fopen(FEEDBACK_FILE, "rb");
    if (file == NULL) {
        return 1;
    }

    Feedback record;
    int maxId = 0;

    while (fread(&record, sizeof(Feedback), 1U, file) == 1U) {
        if (record.feedbackId > maxId) {
            maxId = record.feedbackId;
        }
    }

    (void)fclose(file);
    return maxId + 1;
}

static int feedbackAlreadyExists(int complaintId, int userId)
{
    FILE *file = fopen(FEEDBACK_FILE, "rb");
    if (file == NULL) {
        return 0;
    }

    Feedback record;
    int exists = 0;

    while (fread(&record, sizeof(Feedback), 1U, file) == 1U) {
        if (record.complaintId == complaintId && record.userId == userId) {
            exists = 1;
            break;
        }
    }

    (void)fclose(file);
    return exists;
}

Result submitFeedback(int userId)
{
    int complaintId = 0;

    printf("\n--- Submit Feedback ---\n");
    if (safeInputInt("Enter Complaint ID: ", 1, 1000000, &complaintId) == 0) {
        printf("Invalid Complaint ID.\n");
        return RESULT_FAILURE;
    }

    Complaint complaint;
    if (findComplaintById(complaintId, &complaint) != RESULT_SUCCESS || complaint.userId != userId) {
        printf("Complaint not found for your account.\n");
        return RESULT_FAILURE;
    }

    if (complaint.status != CSTATUS_RESOLVED && complaint.status != CSTATUS_CLOSED) {
        printf("Feedback can only be submitted after resolution.\n");
        return RESULT_FAILURE;
    }

    if (feedbackAlreadyExists(complaintId, userId) != 0) {
        printf("You have already submitted feedback for this complaint.\n");
        return RESULT_FAILURE;
    }

    Feedback newFeedback;
    memset(&newFeedback, 0, sizeof(newFeedback));

    if (safeInputInt("Enter rating (1-5): ", MIN_RATING, MAX_RATING, &newFeedback.rating) == 0) {
        printf("Invalid rating.\n");
        return RESULT_FAILURE;
    }

    safeInputString("Enter feedback comments: ", newFeedback.feedbackText,
                     sizeof(newFeedback.feedbackText));

    newFeedback.feedbackId = getNextFeedbackId();
    newFeedback.complaintId = complaintId;
    newFeedback.userId = userId;
    getCurrentDateTime(&newFeedback.submittedOn);

    FILE *file = fopen(FEEDBACK_FILE, "ab");
    if (file == NULL) {
        logErrorMessage("submitFeedback: unable to open feedback file");
        return RESULT_FAILURE;
    }

    Result writeResult = RESULT_FAILURE;
    if (fwrite(&newFeedback, sizeof(Feedback), 1U, file) == 1U) {
        writeResult = RESULT_SUCCESS;
    }
    (void)fclose(file);

    if (writeResult == RESULT_SUCCESS) {
        printf("Thank you for your feedback.\n");
        logAudit("Feedback submitted");
    } else {
        printf("Failed to save feedback.\n");
    }

    return writeResult;
}

void viewRatings(void)
{
    FILE *file = fopen(FEEDBACK_FILE, "rb");
    if (file == NULL) {
        printf("No feedback found.\n");
        return;
    }

    Feedback record;
    int count = 0;

    printf("\n--- All Feedback Ratings ---\n");
    while (fread(&record, sizeof(Feedback), 1U, file) == 1U) {
        printf("Complaint #%d | User #%d | Rating: %d/5 | Comments: %s\n",
               record.complaintId, record.userId, record.rating,
               isNullOrEmpty(record.feedbackText) ? "(none)" : record.feedbackText);
        count++;
    }

    (void)fclose(file);

    if (count == 0) {
        printf("No feedback has been submitted yet.\n");
    }
}

double calculateAverageRating(void)
{
    FILE *file = fopen(FEEDBACK_FILE, "rb");
    if (file == NULL) {
        return 0.0;
    }

    Feedback record;
    long total = 0;
    int count = 0;

    while (fread(&record, sizeof(Feedback), 1U, file) == 1U) {
        total += record.rating;
        count++;
    }

    (void)fclose(file);

    return (count > 0) ? ((double)total / (double)count) : 0.0;
}

void generateFeedbackReport(void)
{
    FILE *file = fopen(FEEDBACK_FILE, "rb");
    if (file == NULL) {
        printf("No feedback found.\n");
        return;
    }

    Feedback record;
    int ratingCounts[MAX_RATING + 1];
    memset(ratingCounts, 0, sizeof(ratingCounts));
    int total = 0;

    while (fread(&record, sizeof(Feedback), 1U, file) == 1U) {
        if (record.rating >= MIN_RATING && record.rating <= MAX_RATING) {
            ratingCounts[record.rating]++;
        }
        total++;
    }

    (void)fclose(file);

    printf("\n--- Feedback Report ---\n");
    printf("Total Feedback Entries: %d\n", total);
    for (int rating = MAX_RATING; rating >= MIN_RATING; rating--) {
        printf("%d Star: %d\n", rating, ratingCounts[rating]);
    }
    printf("Average Rating: %.2f / 5.00\n", calculateAverageRating());
}
