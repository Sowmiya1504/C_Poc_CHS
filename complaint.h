/*
 * complaint.h
 * Complaint registration, viewing, updating, deletion and search.
 */
#ifndef COMPLAINT_H
#define COMPLAINT_H

#include "common.h"

Result registerComplaint(int userId);
void   viewComplaintsForUser(int userId);
void   viewAllComplaints(void);
Result updateComplaintStatus(int complaintId, ComplaintStatus newStatus, const char *remarks);
Result assignComplaintStaff(int complaintId, const char *staffName);
Result deleteComplaintById(int complaintId);
void   searchComplaints(void);

Result findComplaintById(int complaintId, Complaint *outComplaint);
int    getNextComplaintId(void);

const char *categoryToString(ComplaintCategory category);
const char *priorityToString(ComplaintPriority priority);
const char *statusToString(ComplaintStatus status);

void printComplaintDetails(const Complaint *complaint);

#endif /* COMPLAINT_H */
