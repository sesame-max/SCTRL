#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "main.h"
#include "NRF24L01.h"

#define PROTOCOL_DEBUG_ENABLE 0

#if PROTOCOL_DEBUG_ENABLE
#define PROTOCOL_DEBUG(format, ...) \
    sLog_print(format, ##__VA_ARGS__)
#else
#define PROTOCOL_DEBUG(format, ...)
#endif

#define PROTOCOL_TICK_MS 1

#define PROTOCOL_HOP_FREQ_PERIOD_US 20000
#define PROTOCOL_SLOW_HOP_FREQ_PERIOD_US 36000
#define PROTOCOL_RX_SYN_SEND_PERIOD 25
#define PROTOCOL_TX_SYN_RCVD_PERIOD 15

#define PARM_ADDR 0x08007000

#define CHIPID_BASE 0x1FFFF7E0 // F10x

#define PROTOCOL_MAX_PACK_LEN 24

#define PROTOCOL_ANALOG_CH_NUM 12
#define PROTOCOL_DIGITAL_CH_NUM 16

typedef enum
{
    PROTOCOL_MASTER = 0,
    PROTOCOL_SLAVER
} protocolRole;

typedef enum
{
    PROTOCOL_BASIC_V1 = 0x01,
    PROTOCOL_LONG_V1 = 0x11,
    PROTOCOL_MULTIPLE_V1 = 0x21,
    PROTOCOL_DUPLEX_V1 = 0x31
} protocolVersion;

typedef enum
{
    PROTOCOL_TX_50HZ = 0,
    PROTOCOL_TX_100HZ,
    PROTOCOL_TX_200HZ,
    PROTOCOL_TX_400HZ
} protocolTxFreq;

typedef enum
{
    PROTOCOL_PARM_NONE = 0,
    PROTOCOL_PARM_1,
    PROTOCOL_PARM_2,
} protocolParmNum;

typedef enum
{
    PROTOCOL_INIT = 0,
    PROTOCOL_BINDING,
    PROTOCOL_CONNECTED,
    PROTOCOL_DISCONNECTED
} protocolWorkStatus;

typedef enum
{
    PROTOCOL_BIND_REQ_NONE = 0,
    PROTOCOL_BIND_REQ
} protocolBindReqState;

typedef enum
{
    PROTOCOL_BIND_NONE = 0,
    PROTOCOL_BIND
} protocolBindState;

typedef enum
{
    PROTOCOL_TX_BIND_INIT = 0,
    PROTOCOL_TX_BIND_LISTEN,
    PROTOCOL_TX_BIND_SYN_RCVD,
    PROTOCOL_TX_BIND_ESTABLISHED,
} protocolTxBindState;

typedef enum
{
    PROTOCOL_RX_BIND_INIT = 0,
    PROTOCOL_RX_BIND_SYN_SEND,
    PROTOCOL_RX_BIND_ESTABLISHED,
} protocolRxBindState;

#pragma pack(1)
typedef struct
{

} dataPackTypeDef;
#pragma pack()

#pragma pack(1)
typedef struct
{
    protocolBindState bindStatus;
    uint8_t seed;
    uint8_t power;
    uint8_t addr[5];
    uint16_t txPeriod;
    protocolTxFreq txFreq;
    protocolVersion version;
    uint8_t crc;
} parmTypeDef;
#pragma pack()

typedef struct
{
    protocolRole role;
    protocolWorkStatus workStatus;
    protocolTxBindState txBindState;
    protocolRxBindState rxBindState;

    protocolBindReqState bindReq;

    uint8_t packLen;

    uint8_t rxFlag;
    uint8_t frameCnt;
    uint8_t channelPoint;
    uint8_t channelList[127];

    uint8_t loseChannel;
    uint8_t rxCnt;
    uint8_t loseCnt;

    uint16_t hopTickCnt;
    uint8_t serialNumber;

    uint8_t bindAddr[5];
    uint8_t bindCh;

    uint8_t rxBuffer[PROTOCOL_MAX_PACK_LEN];
    uint8_t txBuffer[PROTOCOL_MAX_PACK_LEN];
    uint16_t analogChBuffer[PROTOCOL_ANALOG_CH_NUM];
    uint8_t digitalChBuffer[PROTOCOL_DIGITAL_CH_NUM];
    parmTypeDef parm;
} protocolTypeDef;

void protocol_init(protocolRole role);
void protocol_tick(void);
void protocol_irq(void);

void protocol_set_ch(uint32_t *analogCh, uint8_t *digitalCh);
void protocol_get_ch(uint32_t *analogCh, uint8_t *digitalCh);

void protocol_bind_req(void);
void protocol_clean_bind_req(void);

protocolParmNum protocol_get_parm(parmTypeDef *parm);

void protocol_timer_callback(void);

void protocol_set_tx_freq(protocolTxFreq freq);
void protocol_set_protocol_version(protocolVersion ver);
protocolVersion protocol_get_protocol_version(void);

// void protocol_write_parm(parmTypeDef *parm);
// uint8_t protocol_read_parm(parmTypeDef *parm);

#endif