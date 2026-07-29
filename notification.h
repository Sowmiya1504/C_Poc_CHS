/*
 * notification.h
 * Notification creation and viewing.
 */
#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include "common.h"

Result createNotification(int userId, int complaintId, const char *message);
void   viewNotifications(int userId);
int    getNextNotificationId(void);

#endif /* NOTIFICATION_H */
