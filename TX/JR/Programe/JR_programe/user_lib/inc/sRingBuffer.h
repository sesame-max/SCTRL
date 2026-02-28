#ifndef __SRING_BUFFER_H
#define __SRING_BUFFER_H

#include "main.h"

typedef uint32_t sRingSize_t;

typedef struct __sRingTypeDef
{
    uint32_t head;
    uint32_t tail;
    uint32_t bufferSize;
    uint8_t *buffer;
} sRingTypeDef;

typedef enum
{
    SRING_SUCCESS = 0,
    SRING_FULL,
} sRingStatus;

void sRing_init(sRingTypeDef *ringBuffer, uint8_t *buffer, uint32_t bufferSize);
sRingStatus sRing_push(sRingTypeDef *ringBuffer, uint8_t *data, sRingSize_t dataLen);
uint32_t sRing_pop(sRingTypeDef *ringBuffer, uint8_t *data, sRingSize_t maxDataLen);
sRingStatus sRing_add(sRingTypeDef *ringBuffer, uint8_t *data, sRingSize_t dataLen);
uint32_t sRing_get(sRingTypeDef *ringBuffer, uint8_t *data, sRingSize_t maxDataLen);
uint32_t sRing_get_used(sRingTypeDef *ringBuffer);
uint32_t sRing_get_empty(sRingTypeDef *ringBuffer);

#endif
