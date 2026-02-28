#include "sRingBuffer.h"
#include "string.h"

void sRing_init(sRingTypeDef *ringBuffer, uint8_t *buffer, uint32_t bufferSize)
{
    ringBuffer->buffer = buffer;
    ringBuffer->bufferSize = bufferSize;
    ringBuffer->head = 0;
    ringBuffer->tail = 0;
}

sRingStatus sRing_push(sRingTypeDef *ringBuffer, uint8_t *data, sRingSize_t dataLen)
{
    uint32_t i = 0;
    uint32_t len = dataLen;
    uint32_t emptySize = 0;
    uint32_t tailPos = ringBuffer->tail;
    uint32_t headPos = ringBuffer->head;
    if (headPos <= tailPos)
    {
        emptySize = headPos + ringBuffer->bufferSize - tailPos;
    }
    else
    {
        emptySize = headPos - tailPos;
    }

    if (emptySize <= len + sizeof(sRingSize_t))
    {
        return SRING_FULL;
    }

    for (i = 0; i < sizeof(sRingSize_t); i++)
    {
        ringBuffer->buffer[tailPos] = *((uint8_t *)&len + i);
        tailPos = ((tailPos + 1) % ringBuffer->bufferSize);
    }

    if ((ringBuffer->bufferSize - tailPos) >= len)
    {
        memcpy((ringBuffer->buffer + tailPos), data, len);
    }
    else
    {
        memcpy((ringBuffer->buffer + tailPos), data, (ringBuffer->bufferSize - tailPos));
        memcpy(ringBuffer->buffer, (data + ringBuffer->bufferSize - tailPos), (len + tailPos - ringBuffer->bufferSize));
    }

    ringBuffer->tail = (tailPos + len) % ringBuffer->bufferSize;

    return SRING_SUCCESS;
}

uint32_t sRing_pop(sRingTypeDef *ringBuffer, uint8_t *data, sRingSize_t maxDataLen)
{
    uint32_t dataLen = 0;
    uint32_t i = 0;
    uint32_t nextHead = 0;
    uint32_t tailPos = ringBuffer->tail;
    uint32_t headPos = ringBuffer->head;

    if (headPos == tailPos)
    {
        return 0;
    }

    for (i = 0; i < sizeof(sRingSize_t); i++)
    {
        *((uint8_t *)&dataLen + i) = ringBuffer->buffer[headPos];
        headPos = ((headPos + 1) % ringBuffer->bufferSize);
    }

    nextHead = (headPos + dataLen) % ringBuffer->bufferSize;

    dataLen = dataLen > maxDataLen ? maxDataLen : dataLen;

    if ((ringBuffer->bufferSize - headPos) >= dataLen)
    {
        memcpy(data, (ringBuffer->buffer + headPos), dataLen);
    }
    else
    {
        memcpy(data, (ringBuffer->buffer + headPos), (ringBuffer->bufferSize - headPos));
        memcpy((data + ringBuffer->bufferSize - headPos), ringBuffer->buffer, (dataLen + headPos - ringBuffer->bufferSize));
    }

    ringBuffer->head = nextHead;

    return dataLen;
}

sRingStatus sRing_add(sRingTypeDef *ringBuffer, uint8_t *data, sRingSize_t dataLen)
{
    uint32_t i = 0;
    uint32_t len = dataLen;
    uint32_t emptySize = 0;
    uint32_t tailPos = ringBuffer->tail;
    uint32_t headPos = ringBuffer->head;

    emptySize = sRing_get_empty(ringBuffer);

    if (emptySize <= len)
    {
        return SRING_FULL;
    }

    if ((ringBuffer->bufferSize - tailPos) >= len)
    {
        memcpy((ringBuffer->buffer + tailPos), data, len);
    }
    else
    {
        memcpy((ringBuffer->buffer + tailPos), data, (ringBuffer->bufferSize - tailPos));
        memcpy(ringBuffer->buffer, (data + ringBuffer->bufferSize - tailPos), (len + tailPos - ringBuffer->bufferSize));
    }

    ringBuffer->tail = (tailPos + len) % ringBuffer->bufferSize;

    return SRING_SUCCESS;
}
uint32_t sRing_get(sRingTypeDef *ringBuffer, uint8_t *data, sRingSize_t maxDataLen)
{
    uint32_t dataLen = 0;
    uint32_t i = 0;
    uint32_t nextHead = 0;
    uint32_t tailPos = ringBuffer->tail;
    uint32_t headPos = ringBuffer->head;

    if (headPos == tailPos)
    {
        return 0;
    }

    dataLen = sRing_get_used(ringBuffer);

    dataLen = dataLen > maxDataLen ? maxDataLen : dataLen;

    nextHead = (headPos + dataLen) % ringBuffer->bufferSize;

    if ((ringBuffer->bufferSize - headPos) >= dataLen)
    {
        memcpy(data, (ringBuffer->buffer + headPos), dataLen);
    }
    else
    {
        memcpy(data, (ringBuffer->buffer + headPos), (ringBuffer->bufferSize - headPos));
        memcpy((data + ringBuffer->bufferSize - headPos), ringBuffer->buffer, (dataLen + headPos - ringBuffer->bufferSize));
    }

    ringBuffer->head = nextHead;

    return dataLen;
}

uint32_t sRing_get_used(sRingTypeDef *ringBuffer)
{
    uint32_t used = 0;
    uint32_t tailPos = ringBuffer->tail;
    uint32_t headPos = ringBuffer->head;
    if (headPos <= tailPos)
    {
        used = tailPos - headPos;
    }
    else
    {
        used = headPos + ringBuffer->bufferSize - tailPos;
    }
    return used;
}

uint32_t sRing_get_empty(sRingTypeDef *ringBuffer)
{
    uint32_t emptySize = 0;
    uint32_t tailPos = ringBuffer->tail;
    uint32_t headPos = ringBuffer->head;
    if (headPos <= tailPos)
    {
        emptySize = headPos + ringBuffer->bufferSize - tailPos;
    }
    else
    {
        emptySize = headPos - tailPos;
    }
    return emptySize;
}
