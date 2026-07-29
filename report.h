/*
 * report.h
 * Report generation: summary, pending, resolved, category,
 * priority, monthly and export-to-file.
 */
#ifndef REPORT_H
#define REPORT_H

#include "common.h"

void   generateSummaryReport(void);
void   generatePendingReport(void);
void   generateResolvedReport(void);
void   generateCategoryReport(void);
void   generatePriorityReport(void);
void   generateMonthlyReport(void);
Result exportReportToFile(const char *filename);

#endif /* REPORT_H */
