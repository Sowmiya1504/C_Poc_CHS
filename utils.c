/*
 * utils.c
 * Implementation of general purpose utility helpers.
 */
#include "utils.h"

/* Remove leading/trailing whitespace (and trailing newline) in place. */
void trimString(char *str)
{
    if (str == NULL) {
        return;
    }

    size_t len = strlen(str);
    while (len > 0U && (unsigned char)str[len - 1U] <= (unsigned char)' ') {
        str[len - 1U] = '\0';
        len--;
    }

    size_t start = 0U;
    while (str[start] != '\0' && (unsigned char)str[start] <= (unsigned char)' ') {
        start++;
    }

    if (start > 0U) {
        memmove(str, str + start, strlen(str + start) + 1U);
    }
}

void toLowerString(char *str)
{
    if (str == NULL) {
        return;
    }
    for (size_t i = 0U; str[i] != '\0'; i++) {
        str[i] = (char)tolower((unsigned char)str[i]);
    }
}

int isNullOrEmpty(const char *str)
{
    return (str == NULL) || (str[0] == '\0');
}

/* Consume the rest of the current stdin line, discarding it. */
void clearInputBuffer(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        /* discard */
    }
}

/*
 * Prompt the user and read a line of text safely using fgets.
 * Guarantees NUL termination and strips the trailing newline.
 */
void safeInputString(const char *prompt, char *buffer, size_t bufferSize)
{
    if (buffer == NULL || bufferSize == 0U) {
        return;
    }

    if (prompt != NULL) {
        printf("%s", prompt);
    }

    if (fgets(buffer, (int)bufferSize, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    size_t len = strlen(buffer);
    if (len > 0U && buffer[len - 1U] == '\n') {
        buffer[len - 1U] = '\0';
    } else if (len == bufferSize - 1U) {
        /* Line longer than buffer - discard remainder of input line */
        clearInputBuffer();
    } else {
        /* nothing */
    }

    trimString(buffer);
}

/*
 * Prompt for and safely parse an integer within [minValue, maxValue].
 * Returns RESULT_SUCCESS/RESULT_FAILURE-style int (1 on success).
 */
int safeInputInt(const char *prompt, int minValue, int maxValue, int *outValue)
{
    char lineBuffer[64];
    char *endPtr = NULL;
    long parsedValue;

    if (outValue == NULL) {
        return 0;
    }

    safeInputString(prompt, lineBuffer, sizeof(lineBuffer));

    if (isNullOrEmpty(lineBuffer)) {
        return 0;
    }

    parsedValue = strtol(lineBuffer, &endPtr, 10);

    if (endPtr == lineBuffer || *endPtr != '\0') {
        return 0;
    }

    if (parsedValue < (long)minValue || parsedValue > (long)maxValue) {
        return 0;
    }

    *outValue = (int)parsedValue;
    return 1;
}

void getCurrentDateTime(DateTime *dt)
{
    if (dt == NULL) {
        return;
    }

    time_t rawTime = time(NULL);
    struct tm localTimeCopy;

#if defined(_WIN32)
    localtime_s(&localTimeCopy, &rawTime);
#else
    localtime_r(&rawTime, &localTimeCopy);
#endif

    dt->day    = localTimeCopy.tm_mday;
    dt->month  = localTimeCopy.tm_mon + 1;
    dt->year   = localTimeCopy.tm_year + 1900;
    dt->hour   = localTimeCopy.tm_hour;
    dt->minute = localTimeCopy.tm_min;
    dt->second = localTimeCopy.tm_sec;
}

void formatDateTime(const DateTime *dt, char *buffer, size_t bufferSize)
{
    if (dt == NULL || buffer == NULL || bufferSize == 0U) {
        return;
    }

    (void)snprintf(buffer, bufferSize, "%04d-%02d-%02d %02d:%02d:%02d",
                    dt->year, dt->month, dt->day,
                    dt->hour, dt->minute, dt->second);
}

/* Simple calendar-naive day addition (good enough for expected-resolution
   estimates used by the tracking module). */
void addDaysToDate(const DateTime *source, int daysToAdd, DateTime *result)
{
    static const int daysInMonth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (source == NULL || result == NULL) {
        return;
    }

    *result = *source;

    for (int i = 0; i < daysToAdd; i++) {
        int isLeap = ((result->year % 4 == 0) && (result->year % 100 != 0)) || (result->year % 400 == 0);
        int maxDay = daysInMonth[result->month];
        if (result->month == 2 && isLeap != 0) {
            maxDay = 29;
        }

        result->day++;
        if (result->day > maxDay) {
            result->day = 1;
            result->month++;
            if (result->month > 12) {
                result->month = 1;
                result->year++;
            }
        }
    }
}

static void appendLogLine(const char *filename, const char *tag, const char *message)
{
    FILE *logFile = fopen(filename, "a");
    if (logFile == NULL) {
        return;
    }

    DateTime now;
    char timeBuf[MAX_DATE_LEN];
    getCurrentDateTime(&now);
    formatDateTime(&now, timeBuf, sizeof(timeBuf));

    (void)fprintf(logFile, "[%s] %s: %s\n", timeBuf, tag, (message != NULL) ? message : "");
    (void)fclose(logFile);
}

void logAudit(const char *message)
{
    appendLogLine(AUDIT_LOG_FILE, "AUDIT", message);
}

void logErrorMessage(const char *message)
{
    appendLogLine(ERROR_LOG_FILE, "ERROR", message);
}

void secureZeroMemory(void *ptr, size_t length)
{
    if (ptr == NULL) {
        return;
    }
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (length > 0U) {
        *p = 0U;
        p++;
        length--;
    }
}
