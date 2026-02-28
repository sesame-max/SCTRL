#include "sAsyncTR.h"
#include "string.h"

void sAsync_tx_init(sAsyncTRTypeDef *sAsyncTR, uint8_t *buffer, uint32_t bufferSize, uint32_t maxSendSize, void (*transmit)(uint8_t *, uint32_t))
{
    if (bufferSize < maxSendSize)
    {
        return;
    }
    sAsyncTR->sendBuffer = buffer;
    sAsyncTR->sendBufferSize = maxSendSize;
    sAsyncTR->sendStatus = SASYNC_IDEL;
    sAsyncTR->transmit = transmit;
    sAsyncTR->maxWaitTime = 0xFFFFFFFF;

    sAsyncTR->enter_transmit = NULL;
    sAsyncTR->exit_transmit = NULL;

    sAsyncTR->ringBufferAdd = sRing_add;
    sAsyncTR->ringBufferGet = sRing_get;

    sRing_init(&sAsyncTR->txRingBuffer, buffer + maxSendSize, bufferSize - maxSendSize);
}

void sAsync_rx_init(sAsyncTRTypeDef *sAsyncTR, uint8_t *buffer, uint32_t bufferSize, uint32_t maxReceiveSize, void (*receive)(uint8_t *, uint32_t))
{
    sAsyncTR->receiveBuffer = buffer;
    sAsyncTR->receiveBufferSize = maxReceiveSize;
    sAsyncTR->receive = receive;
    sAsyncTR->maxWaitTime = 0xFFFFFFFF;

    sRing_init(&sAsyncTR->rxRingBuffer, buffer + maxReceiveSize, bufferSize - maxReceiveSize);
}

void sAsync_receive_cplt_callback(sAsyncTRTypeDef *sAsyncTR, uint8_t *data, uint32_t len)
{
    sRing_push(&sAsyncTR->rxRingBuffer, data, len);
}

void sAsync_send_cplt_callback(sAsyncTRTypeDef *sAsyncTR)
{
    uint32_t len = 0;
    __SASYNC_EXIT;
    len = sAsyncTR->ringBufferGet(&sAsyncTR->txRingBuffer, sAsyncTR->sendBuffer, sAsyncTR->sendBufferSize);
    if (len > 0)
    {
        __SASYNC_ENTER;
        sAsyncTR->transmit(sAsyncTR->sendBuffer, len);
    }
    else
    {
        sAsyncTR->sendStatus = SASYNC_IDEL;
    }
}

sAsyncSendResult sAsync_transmit(sAsyncTRTypeDef *sAsyncTR, uint8_t *data, uint32_t len)
{
    uint32_t waitTime = 0;

    len = len > sAsyncTR->sendBufferSize ? sAsyncTR->sendBufferSize : len;

    if (sAsyncTR->sendStatus == SASYNC_BUSY)
    {
        while (sAsyncTR->ringBufferAdd(&sAsyncTR->txRingBuffer, data, len) != SRING_SUCCESS && waitTime < sAsyncTR->maxWaitTime)
        {
            waitTime++;
        }

        if (waitTime < sAsyncTR->maxWaitTime)
        {
            if (sAsyncTR->sendStatus == SASYNC_IDEL)
            {
                len = sAsyncTR->ringBufferGet(&sAsyncTR->txRingBuffer, sAsyncTR->sendBuffer, sAsyncTR->sendBufferSize);
                sAsyncTR->sendStatus = SASYNC_BUSY;
                __SASYNC_ENTER;
                sAsyncTR->transmit(sAsyncTR->sendBuffer, len);
            }
            return SASYNC_SUCCESS;
        }
        else
        {
            return SASYNC_FULL;
        }
    }
    else
    {
        memcpy(sAsyncTR->sendBuffer, data, len);
        sAsyncTR->sendStatus = SASYNC_BUSY;
        __SASYNC_ENTER;
        sAsyncTR->transmit(sAsyncTR->sendBuffer, len);
        return SASYNC_SUCCESS;
    }
}

uint32_t sAsync_get(sAsyncTRTypeDef *sAsyncTR, uint8_t *data, uint32_t maxLen)
{
    return sAsyncTR->ringBufferGet(&sAsyncTR->rxRingBuffer, data, maxLen);
}

void sAsync_set_max_wait_time(sAsyncTRTypeDef *sAsyncTR, uint32_t time)
{
    sAsyncTR->maxWaitTime = time;
}

void sAsync_set_tr_mode(sAsyncTRTypeDef *sAsyncTR, sAsyncTRMode mode)
{
    if (mode == SASYNC_STREAM)
    {
        sAsyncTR->ringBufferAdd = sRing_add;
        sAsyncTR->ringBufferGet = sRing_get;
    }
    else if (mode == SASYNC_PACK)
    {
        sAsyncTR->ringBufferAdd = sRing_push;
        sAsyncTR->ringBufferGet = sRing_pop;
    }
}
void sAsync_set_enter_transmit(sAsyncTRTypeDef *sAsyncTR, void (*enter_transmit)(void))
{
    sAsyncTR->enter_transmit = enter_transmit;
}
void sAsync_set_exit_transmit(sAsyncTRTypeDef *sAsyncTR, void (*exit_transmit)(void))
{
    sAsyncTR->exit_transmit = exit_transmit;
}
