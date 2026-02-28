#ifndef __SLOG_H
#define __SLOG_H

#include "main.h"

#define SLOG_ASYNC_SEND 0

#define SLOG_BUFFER_SIZE 256

#if SLOG_ASYNC_SEND == 1
#define SLOG_RING_BUFFER_SIZE 2048
#endif

typedef enum
{
    SLOG_LEVEL_NORMAL = 0,
    SLOG_LEVEL_INFO,
    SLOG_LEVEL_DEBUG,
    SLOG_LEVEL_WARNING,
    SLOG_LEVEL_ERROR,
} sLogLevelDef;

typedef enum
{
    SLOG_SUCCESS = 0,
    SLOG_FULL,
} sLogState;

#if SLOG_ASYNC_SEND == 1
void sLog_async_send_cplt_callback(void);
#endif

void sLog_init(void);
sLogState sLog_print(const char *__format, ...);
sLogState sLog_print_with_time(const char *__format, ...);
sLogState sLog_print_with_level(sLogLevelDef level, const char *__format, ...);
sLogState sLog_print_with_time_level(sLogLevelDef level, const char *__format, ...);

#endif
