/*
 * utils.h
 * General purpose utility helpers: safe input, string handling,
 * date/time, logging and secure memory clearing.
 */
#ifndef UTILS_H
#define UTILS_H

#include "common.h"

void   trimString(char *str);
void   toLowerString(char *str);

void   safeInputString(const char *prompt, char *buffer, size_t bufferSize);
int    safeInputInt(const char *prompt, int minValue, int maxValue, int *outValue);
void   clearInputBuffer(void);

void   getCurrentDateTime(DateTime *dt);
void   formatDateTime(const DateTime *dt, char *buffer, size_t bufferSize);
void   addDaysToDate(const DateTime *source, int daysToAdd, DateTime *result);

void   logAudit(const char *message);
void   logErrorMessage(const char *message);

void   secureZeroMemory(void *ptr, size_t length);

int    isNullOrEmpty(const char *str);

#endif /* UTILS_H */
