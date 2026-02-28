#ifndef __SASYNC_TR_H
#define __SASYNC_TR_H

#include "main.h"
#include "sRingBuffer.h"

#define __SASYNC_ENTER                        \
    do                                        \
    {                                         \
        if (sAsyncTR->enter_transmit != NULL) \
        {                                     \
            sAsyncTR->enter_transmit();       \
        }                                     \
    } while (0)

#define __SASYNC_EXIT                        \
    do                                       \
    {                                        \
        if (sAsyncTR->exit_transmit != NULL) \
        {                                    \
            sAsyncTR->exit_transmit();       \
        }                                    \
    } while (0)

typedef enum
{
    SASYNC_IDEL = 0,
    SASYNC_BUSY,
    SASYNC_ERROR
} sAsyncSendStatus;

typedef enum
{
    SASYNC_SUCCESS = 0,
    SASYNC_FULL
} sAsyncSendResult;

typedef enum
{
    SASYNC_STREAM = 0,
    SASYNC_PACK
} sAsyncTRMode;

typedef struct __sAsyncTRTypeDef
{
    sRingStatus (*ringBufferAdd)(sRingTypeDef *ringBuffer, uint8_t *data, sRingSize_t dataLen);
    uint32_t (*ringBufferGet)(sRingTypeDef *ringBuffer, uint8_t *data, sRingSize_t maxDataLen);

    sRingTypeDef txRingBuffer;
    uint8_t *sendBuffer;
    uint32_t sendBufferSize;
    uint8_t sendStatus;
    void (*transmit)(uint8_t *, uint32_t);
    void (*enter_transmit)(void);
    void (*exit_transmit)(void);

    uint32_t maxWaitTime;

    sRingTypeDef rxRingBuffer;
    uint8_t *receiveBuffer;
    uint32_t receiveBufferSize;
    void (*receive)(uint8_t *, uint32_t);

} sAsyncTRTypeDef;

void sAsync_tx_init(sAsyncTRTypeDef *sAsyncTR, uint8_t *buffer, uint32_t bufferSize, uint32_t maxSendSize, void (*transmit)(uint8_t *, uint32_t));
sAsyncSendResult sAsync_transmit(sAsyncTRTypeDef *sAsyncTR, uint8_t *data, uint32_t len);
void sAsync_send_cplt_callback(sAsyncTRTypeDef *sAsyncTR);

void sAsync_rx_init(sAsyncTRTypeDef *sAsyncTR, uint8_t *buffer, uint32_t bufferSize, uint32_t maxReceiveSize, void (*receive)(uint8_t *, uint32_t));
void sAsync_receive_cplt_callback(sAsyncTRTypeDef *sAsyncTR, uint8_t *data, uint32_t len);
uint32_t sAsync_receive(sAsyncTRTypeDef *sAsyncTR, uint8_t *data, uint32_t maxLen);

void sAsync_set_max_wait_time(sAsyncTRTypeDef *sAsyncTR, uint32_t time);
void sAsync_set_tr_mode(sAsyncTRTypeDef *sAsyncTR, sAsyncTRMode mode);
void sAsync_set_enter_transmit(sAsyncTRTypeDef *sAsyncTR, void (*enter_transmit)(void));
void sAsync_set_exit_transmit(sAsyncTRTypeDef *sAsyncTR, void (*exit_transmit)(void));

#endif
