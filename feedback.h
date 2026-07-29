/*
 * feedback.h
 * Post-resolution feedback: rating and free-text comments.
 */
#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "common.h"

Result submitFeedback(int userId);
void   viewRatings(void);
double calculateAverageRating(void);
void   generateFeedbackReport(void);
int    getNextFeedbackId(void);

#endif /* FEEDBACK_H */
