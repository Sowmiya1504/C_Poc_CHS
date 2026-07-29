/*
 * notification.c
 * Implementation of notification creation and viewing.
 */
#include "notification.h"
#include "utils.h"

int getNextNotificationId(void)
{
    FILE *file = fopen(NOTIFICATIONS_FILE, "rb");
    if (file == NULL) {
        return 1;
    }

    Notification record;
    int maxId = 0;

    while (fread(&record, sizeof(Notification), 1U, file) == 1U) {
        if (record.notificationId > maxId) {
            maxId = record.notificationId;
        }
    }

    (void)fclose(file);
    return maxId + 1;
}

Result createNotification(int userId, int complaintId, const char *message)
{
    if (isNullOrEmpty(message)) {
        return RESULT_FAILURE;
    }

    Notification newNotification;
    memset(&newNotification, 0, sizeof(newNotification));

    newNotification.notificationId = getNextNotificationId();
    newNotification.userId = userId;
    newNotification.complaintId = complaintId;
    (void)snprintf(newNotification.message, sizeof(newNotification.message), "%s", message);
    getCurrentDateTime(&newNotification.createdOn);
    newNotification.isRead = 0;

    FILE *file = fopen(NOTIFICATIONS_FILE, "ab");
    if (file == NULL) {
        logErrorMessage("createNotification: unable to open notifications file");
        return RESULT_FAILURE;
    }

    Result writeResult = RESULT_FAILURE;
    if (fwrite(&newNotification, sizeof(Notification), 1U, file) == 1U) {
        writeResult = RESULT_SUCCESS;
    }
    (void)fclose(file);

    return writeResult;
}

void viewNotifications(int userId)
{
    FILE *file = fopen(NOTIFICATIONS_FILE, "rb");
    if (file == NULL) {
        printf("No notifications found.\n");
        return;
    }

    Notification record;
    int count = 0;
    char timeBuf[MAX_DATE_LEN];

    printf("\n--- Your Notifications ---\n");

    while (fread(&record, sizeof(Notification), 1U, file) == 1U) {
        if (record.userId == userId) {
            formatDateTime(&record.createdOn, timeBuf, sizeof(timeBuf));
            printf("[%s] (Complaint #%d) %s\n", timeBuf, record.complaintId, record.message);
            count++;
        }
    }

    (void)fclose(file);

    if (count == 0) {
        printf("No notifications yet.\n");
    }
}
