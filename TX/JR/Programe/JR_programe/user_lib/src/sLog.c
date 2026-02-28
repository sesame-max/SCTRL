#include "sLog.h"

#include "stdarg.h"
#include "string.h"
#include "stdio.h"

#include "sAsyncTR.h"
#include "usart.h"

#include "usbd_cdc_if.h"
#include "LCD.h"

static uint8_t sLogBuffer[SLOG_BUFFER_SIZE] = {0};

#if SLOG_ASYNC_SEND == 1
static uint8_t sLogAsyncBuffer[SLOG_RING_BUFFER_SIZE] = {0};
static sAsyncTRTypeDef sLogAsyncTR;
#endif

const static uint8_t sLogNormal[] = "\x1b[0m";
const static uint8_t sLogInfo[] = "\x1b[34mINFO: ";
const static uint8_t sLogDebug[] = "\x1b[32mDEBUG: ";
const static uint8_t sLogWarning[] = "\x1b[33mWARING: ";
const static uint8_t sLogError[] = "\x1b[31mERROR: ";

static uint32_t sLog_get_time(void)
{
    return HAL_GetTick();
}

static uint8_t sLog_send(uint8_t *data, uint32_t len)
{
#if SLOG_ASYNC_SEND == 1
    return sAsync_transmit(&sLogAsyncTR, data, len);
#else
    // return HAL_UART_Transmit(&huart1, data, len, 0xffff);
    //    LCD_print(data, len);
    //    return 0;
    return CDC_Transmit_FS(data, len);
#endif
}

#if SLOG_ASYNC_SEND == 1
void sLog_async_send_cplt_callback(void)
{
    sAsync_send_cplt_callback(&sLogAsyncTR);
}

static void sLog_async_transmit(uint8_t *data, uint32_t len)
{
    HAL_UART_Transmit_DMA(&huart1, data, (uint16_t)len);
}
#endif

static uint32_t sLog_add_time(uint32_t ms, char *buffer, uint32_t buffer_size)
{
    // 计算各个时间单位
    uint32_t milliseconds = ms % 1000;
    uint32_t total_seconds = ms / 1000;
    uint32_t seconds = total_seconds % 60;
    uint32_t total_minutes = total_seconds / 60;
    uint32_t minutes = total_minutes % 60;
    uint32_t hours = total_minutes / 60;

    // 格式化字符串
    int length = snprintf(buffer, buffer_size,
                          "\x1b[34m[%02d:%02d:%02d:%03d]\x1b[0m ",
                          hours, minutes, seconds, milliseconds);

    // 如果缓冲区不足，返回负数错误码
    if (length >= buffer_size)
    {
        // 缓冲区不足，部分写入
        return buffer_size;
    }

    return length;
}

static uint32_t sLog_add_level(sLogLevelDef level, char *buffer, uint32_t buffer_size)
{
    uint32_t levelSize = 0;
    switch (level)
    {
    case SLOG_LEVEL_NORMAL:
        levelSize = (sizeof(sLogNormal) - 1) > buffer_size ? buffer_size : (sizeof(sLogNormal) - 1);
        memcpy(buffer, sLogNormal, levelSize);
        break;
    case SLOG_LEVEL_INFO:
        levelSize = (sizeof(sLogInfo) - 1) > buffer_size ? buffer_size : (sizeof(sLogInfo) - 1);
        memcpy(buffer, sLogInfo, levelSize);
        break;
    case SLOG_LEVEL_DEBUG:
        levelSize = (sizeof(sLogDebug) - 1) > buffer_size ? buffer_size : (sizeof(sLogDebug) - 1);
        memcpy(buffer, sLogDebug, levelSize);
        break;
    case SLOG_LEVEL_WARNING:
        levelSize = (sizeof(sLogWarning) - 1) > buffer_size ? buffer_size : (sizeof(sLogWarning) - 1);
        memcpy(buffer, sLogWarning, levelSize);
        break;
    case SLOG_LEVEL_ERROR:
        levelSize = (sizeof(sLogError) - 1) > buffer_size ? buffer_size : (sizeof(sLogError) - 1);
        memcpy(buffer, sLogError, levelSize);
        break;
    default:
        levelSize = 0;
        break;
    }
    return levelSize;
}

void sLog_init(void)
{
#if SLOG_ASYNC_SEND == 1
    sAsync_tx_init(&sLogAsyncTR, sLogAsyncBuffer, sizeof(sLogAsyncBuffer), SLOG_BUFFER_SIZE, sLog_async_transmit);
    sAsync_set_tr_mode(&sLogAsyncTR, SASYNC_STREAM);
#endif
}

sLogState sLog_print(const char *__format, ...)
{
    va_list ap;
    va_start(ap, __format);
    uint32_t len = 0;

    vsnprintf((char *)(sLogBuffer), SLOG_BUFFER_SIZE, (const char *)__format, ap);
    va_end(ap);

    len = strlen((const char *)sLogBuffer);

    return (sLogState)sLog_send(sLogBuffer, len);
}
sLogState sLog_print_with_time(const char *__format, ...)
{
    va_list ap;
    va_start(ap, __format);
    uint32_t formatPos = 0;
    uint32_t len = 0;

    formatPos = sLog_add_time(sLog_get_time(), (char *)sLogBuffer, SLOG_BUFFER_SIZE);

    vsnprintf((char *)(sLogBuffer + formatPos), SLOG_BUFFER_SIZE - formatPos, (const char *)__format, ap);
    va_end(ap);
    len = strlen((const char *)sLogBuffer);

    return (sLogState)sLog_send(sLogBuffer, len);
}

sLogState sLog_print_with_level(sLogLevelDef level, const char *__format, ...)
{
    va_list ap;
    va_start(ap, __format);
    uint32_t formatPos = 0;
    uint32_t len = 0;

    formatPos = sLog_add_level(level, (char *)sLogBuffer, SLOG_BUFFER_SIZE);

    vsnprintf((char *)(sLogBuffer + formatPos), SLOG_BUFFER_SIZE - formatPos, (const char *)__format, ap);
    va_end(ap);
    len = strlen((const char *)sLogBuffer);

    return (sLogState)sLog_send(sLogBuffer, len);
}

sLogState sLog_print_with_time_level(sLogLevelDef level, const char *__format, ...)
{
    va_list ap;
    va_start(ap, __format);
    uint32_t formatPos = 0;
    uint32_t len = 0;

    formatPos = sLog_add_time(sLog_get_time(), (char *)sLogBuffer, SLOG_BUFFER_SIZE);

    formatPos += sLog_add_level(level, (char *)(sLogBuffer + formatPos), SLOG_BUFFER_SIZE - formatPos);

    vsnprintf((char *)(sLogBuffer + formatPos), SLOG_BUFFER_SIZE - formatPos, (const char *)__format, ap);
    va_end(ap);
    len = strlen((const char *)sLogBuffer);

    return (sLogState)sLog_send(sLogBuffer, len);
}