#include "protocol.h"

#include "tim.h"

#include "Morder.h"
#include "CRC8.h"

#include "sLog.h"
#include "user_led.h"

static protocolTypeDef protocol = {
    .bindAddr = {0xFF, 'A', 'H', 'H', 'A'},
    .bindCh = 110,
    .bindReq = PROTOCOL_BIND_REQ_NONE,
};
const static protocolVersion protocolSupportVersion[] = {PROTOCOL_BASIC_V1,
                                                         PROTOCOL_LONG_V1,
                                                         PROTOCOL_MULTIPLE_V1,
                                                         PROTOCOL_DUPLEX_V1};

CRC8TypeDef protocolCrc;

parmTypeDef parmSave;

NRF24L01TypeDef wirelessModule;

static void protocol_tx_interrupt(NRF24L01TypeDef *dev);
static void protocol_rx_interrupt(NRF24L01TypeDef *dev);

static void protocol_module_init(void)
{
    wirelessModule.ce_pin = (uint32_t)CE_Pin;
    wirelessModule.ce_port = (uint32_t)CE_GPIO_Port;
    wirelessModule.cs_pin = (uint32_t)CS_Pin;
    wirelessModule.cs_port = (uint32_t)CS_GPIO_Port;
    wirelessModule.spi = (uint32_t)SPI2;

    wirelessModule.rxBuffer = protocol.rxBuffer;
    wirelessModule.packLen = PROTOCOL_MAX_PACK_LEN;
    wirelessModule.rxCallback = protocol_rx_interrupt;
    wirelessModule.txCallback = protocol_tx_interrupt;
    if (NRF24L01_init(&wirelessModule) != NRF_SUCCESS)
    {
        PROTOCOL_DEBUG("loss NRF24L01\r\n");
        user_led_set_error();
        return;
    }
    else
    {
        PROTOCOL_DEBUG("Find NRF24L01\r\n"); // 模块检测成功
        user_led_loss_signal();
    }
}

static void protocol_set_power(uint8_t power)
{
    NRF24L01_set_power(&wirelessModule, (NRF24L01Power)power);
}

static void protocol_set_channel(uint8_t ch)
{
    NRF24L01_set_channel(&wirelessModule, ch);
}

static void protocol_set_tx_addr(uint8_t *addr)
{
    NRF24L01_set_tx_address(&wirelessModule, addr);
}

static void protocol_set_rx_addr(uint8_t *addr)
{
    NRF24L01_set_rx_address(&wirelessModule, addr);
}

static void protocol_set_pack_len(uint8_t len)
{
    protocol.packLen = len;
    NRF24L01_set_size(&wirelessModule, len);
}
static void protocol_enter_tx_mode(void)
{
    NRF24L01_tx_mode(&wirelessModule);
}

static void protocol_enter_rx_mode(void)
{
    NRF24L01_rx_mode(&wirelessModule);
}

static void protocol_start_tim(void)
{
    HAL_TIM_Base_Start_IT(&htim11);
}

static void protocol_stop_tim(void)
{
    HAL_TIM_Base_Stop_IT(&htim11);
}

static void protocol_set_tim(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim11, 0);
    __HAL_TIM_SET_AUTORELOAD(&htim11, us);
}

static void protocol_get_chipId(uint8_t *id)
{
    uint32_t ID_32[3];
    uint8_t ID_8[12];
    ID_32[0] = *(__IO uint32_t *)(CHIPID_BASE); // 读取芯片序列号
    ID_32[1] = *(__IO uint32_t *)(CHIPID_BASE + 4);
    ID_32[2] = *(__IO uint32_t *)(CHIPID_BASE + 8);

    id[0] = (ID_32[0] >> 24) & 0xff;
    id[1] = (ID_32[0] >> 16) & 0xff;
    id[2] = (ID_32[0] >> 8) & 0xff;
    id[3] = ID_32[0] & 0xff;

    id[4] = (ID_32[1] >> 24) & 0xff;
    id[5] = (ID_32[1] >> 16) & 0xff;
    id[6] = (ID_32[1] >> 8) & 0xff;
    id[7] = ID_32[1] & 0xff;

    id[8] = (ID_32[2] >> 24) & 0xff;
    id[9] = (ID_32[2] >> 16) & 0xff;
    id[10] = (ID_32[2] >> 8) & 0xff;
    id[11] = ID_32[2] & 0xff;
    PROTOCOL_DEBUG("chip id: %X:%X:%X\r\n", ID_32[0], ID_32[1], ID_32[2]);
}

static uint8_t protocol_seed_generate(void)
{
    uint8_t ID_8[12];

    protocol_get_chipId(ID_8);

    return CRC8_calculate(&protocolCrc, ID_8, 12); // 计算序列号的CRC值作为种子
}

static void protocol_addr_generate(void)
{
    uint8_t ID_8[12];

    protocol_get_chipId(ID_8);

    protocol.parm.addr[0] = CRC8_calculate(&protocolCrc, ID_8, 7) % 0xFF; // 避免第一个地址为0xff与对频地址冲突
    protocol.parm.addr[1] = CRC8_calculate(&protocolCrc, &ID_8[1], 7);
    protocol.parm.addr[2] = CRC8_calculate(&protocolCrc, &ID_8[2], 7);
    protocol.parm.addr[3] = CRC8_calculate(&protocolCrc, &ID_8[3], 7);
    protocol.parm.addr[4] = CRC8_calculate(&protocolCrc, &ID_8[4], 7);

    PROTOCOL_DEBUG("Addr: %hX %hX %hX %hX %hX\r\n", protocol.parm.addr[0], protocol.parm.addr[1], protocol.parm.addr[2], protocol.parm.addr[3], protocol.parm.addr[4]);
}

static void protocol_channel_generate(uint8_t seed)
{
    uint8_t i;
    uint8_t Morder[127];
    uint8_t tail;
    tail = seed % 14;
    Morder_generate(seed, seed % 9, Morder);
    for (i = 0; i < 127; i++)
    {
        protocol.channelList[Morder[i] - 1] = (i % 8) * 16 + tail;
    }
    for (i = 0; i < 127; i++)
    {
        PROTOCOL_DEBUG("%d ", protocol.channelList[i]);
    }
    PROTOCOL_DEBUG("\r\n");
}

static void protocol_set_parm(parmTypeDef *parm)
{
}

static protocolBindReqState protocol_get_bind_req(void)
{
    return protocol.bindReq;
}

static void protocol_send_data(uint8_t *data, uint8_t len)
{
    data[len - 1] = CRC8_calculate(&protocolCrc, data, len - 1); // 在数据末尾添加CRC校验
    NRF24L01_write_FIFO(&wirelessModule, data, len);             // 将发送数据写入FIFO
    protocol_enter_tx_mode();                                    // 切换到发送模式
}

static void protocol_bind_enter(void)
{
    PROTOCOL_DEBUG("enter bind\r\n");
    protocol_stop_tim();
    protocol_set_pack_len(PROTOCOL_MAX_PACK_LEN);
    protocol_set_power(0);                   // 设置低功率
    protocol_set_channel(protocol.bindCh);   // 设置通信频段
    protocol_set_tx_addr(protocol.bindAddr); // 设置接收发送地址
    protocol_set_rx_addr(protocol.bindAddr);
}

static void protocol_bind_exit(void)
{
    PROTOCOL_DEBUG("exit bind\r\n");
    protocol.txBindState = PROTOCOL_TX_BIND_INIT;
    protocol.rxBindState = PROTOCOL_RX_BIND_INIT;
    protocol_set_pack_len(protocol.packLen);
    protocol_set_power(3); // 设置高功率
    protocol_set_rx_addr(protocol.parm.addr);
    protocol_set_tx_addr(protocol.parm.addr);
    protocol_channel_generate(protocol.parm.seed);
    protocol_set_channel(protocol.channelList[0]);
    protocol_start_tim();
}

static void protocol_rx_bind(void)
{
    uint8_t i;
    uint8_t txData[PROTOCOL_MAX_PACK_LEN];
    static uint8_t readyMesgSendCnt = 0;
    static uint8_t synSendTickCnt = 0;

    switch (protocol.rxBindState)
    {
    case PROTOCOL_RX_BIND_INIT:
        readyMesgSendCnt = 0;
        synSendTickCnt = 0;
        protocol.rxBindState = PROTOCOL_RX_BIND_SYN_SEND;
        break;
    case PROTOCOL_RX_BIND_SYN_SEND:
        if (synSendTickCnt < PROTOCOL_RX_SYN_SEND_PERIOD)
        {
            synSendTickCnt += PROTOCOL_TICK_MS;
        }
        else
        {
            txData[0] = 0xFF;
            txData[1] = 0x21;
            txData[2] = 0x1A;
            txData[3] = protocol.parm.addr[0];
            txData[4] = protocol.parm.addr[1];
            txData[5] = protocol.parm.addr[2];
            txData[6] = protocol.parm.addr[3];
            txData[7] = protocol.parm.addr[4];
            protocol_send_data(txData, PROTOCOL_MAX_PACK_LEN);
            PROTOCOL_DEBUG("rx syn send\r\n");
            synSendTickCnt = 0;
        }
        break;

    case PROTOCOL_RX_BIND_ESTABLISHED:

        if (readyMesgSendCnt < 17)
        {
            txData[0] = 0xFF;
            txData[1] = 'R';
            txData[2] = 'E';
            txData[3] = 'A';
            txData[4] = 'D';
            txData[5] = 'Y';
            txData[6] = protocol.parm.seed;
            protocol_send_data(txData, PROTOCOL_MAX_PACK_LEN);
            readyMesgSendCnt++;
        }
        else
        {
            PROTOCOL_DEBUG("rx bind OK \r\nSeed: %hX Power: %d Version: %d\r\n", protocol.parm.seed, protocol.parm.power, protocol.parm.version);
            PROTOCOL_DEBUG("addr %d %d %d %d %d\r\n", protocol.parm.addr[0], protocol.parm.addr[1], protocol.parm.addr[2], protocol.parm.addr[3], protocol.parm.addr[4]);
            protocol_set_parm(&protocol.parm);
            protocol.workStatus = PROTOCOL_DISCONNECTED;
            protocol.rxBindState = PROTOCOL_RX_BIND_INIT;
            protocol.bindReq = PROTOCOL_BIND_REQ_NONE;
            user_led_loss_signal();
            protocol_set_tim(23000);
            protocol_bind_exit();
        }

        break;
    }
}

static void protocol_rx_bind_check(uint8_t *data)
{
    if ((data[0] == 0xFF) &&
        (data[1] == 0x12) &&
        (data[2] == 0x1A) &&
        (data[3] == CRC8_calculate(&protocolCrc, &protocol.parm.addr[0], 5)))
    {
        protocol.parm.seed = data[4];
        protocol.parm.power = data[5];
        protocol.parm.version = data[6];
        protocol.rxBindState = PROTOCOL_RX_BIND_ESTABLISHED;
        PROTOCOL_DEBUG("rx syn send success, jump to established\r\n");
    }
}

static void protocol_tx_bind(void)
{
    uint8_t i;
    uint8_t txData[PROTOCOL_MAX_PACK_LEN];
    static uint8_t synRcvdTickCnt = 0;
    switch (protocol.txBindState)
    {
    case PROTOCOL_TX_BIND_INIT:
        synRcvdTickCnt = 0;
        protocol.txBindState = PROTOCOL_TX_BIND_LISTEN;
        break;
    case PROTOCOL_TX_BIND_LISTEN:
        protocol.txBindState = protocol.txBindState;
        // PROTOCOL_DEBUG("tx listen\r\n");
        break;
    case PROTOCOL_TX_BIND_SYN_RCVD:
        if (synRcvdTickCnt < PROTOCOL_TX_SYN_RCVD_PERIOD)
        {
            synRcvdTickCnt += PROTOCOL_TICK_MS;
        }
        else
        {
            txData[0] = 0xFF;
            txData[1] = 0x12;
            txData[2] = 0x1A;
            txData[3] = CRC8_calculate(&protocolCrc, &protocol.parm.addr[0], 5);
            txData[4] = protocol.parm.seed;
            txData[5] = protocol.parm.power;
            txData[6] = protocol.parm.version;
            protocol_send_data(txData, PROTOCOL_MAX_PACK_LEN);
            PROTOCOL_DEBUG("tx syn rcvd\r\n");
            synRcvdTickCnt = 0;
        }
        break;
    case PROTOCOL_TX_BIND_ESTABLISHED:
        protocol_set_parm(&protocol.parm);
        PROTOCOL_DEBUG("tx bind OK \r\nSeed: %hX Power: %d Version: %d\r\n", protocol.parm.seed, protocol.parm.power, protocol.parm.version);
        PROTOCOL_DEBUG("addr %d %d %d %d %d\r\n", protocol.parm.addr[0], protocol.parm.addr[1], protocol.parm.addr[2], protocol.parm.addr[3], protocol.parm.addr[4]);
        protocol.txBindState = PROTOCOL_TX_BIND_INIT;
        protocol.workStatus = PROTOCOL_DISCONNECTED;
        protocol.bindReq = PROTOCOL_BIND_REQ_NONE;
        protocol_set_tim(protocol.parm.txPeriod);
        user_led_loss_signal();
        protocol_bind_exit();
        break;
    }
}

static void protocol_tx_bind_check(uint8_t *data)
{
    switch (protocol.txBindState)
    {
    case PROTOCOL_TX_BIND_LISTEN:
        if (data[0] == 0xFF &&
            data[1] == 0x21 &&
            data[2] == 0x1A)
        {
            protocol.parm.addr[0] = data[3];
            protocol.parm.addr[1] = data[4];
            protocol.parm.addr[2] = data[5];
            protocol.parm.addr[3] = data[6];
            protocol.parm.addr[4] = data[7];
            protocol.txBindState = PROTOCOL_TX_BIND_SYN_RCVD;
            PROTOCOL_DEBUG("tx listen success, jump to syn rcvd\r\n");
        }
        break;
    case PROTOCOL_TX_BIND_SYN_RCVD:
        if (data[0] == 0xFF &&
            data[1] == 'R' &&
            data[2] == 'E' &&
            data[3] == 'A' &&
            data[4] == 'D' &&
            data[5] == 'Y' &&
            data[6] == protocol.parm.seed)
        {
            protocol.txBindState = PROTOCOL_TX_BIND_ESTABLISHED;
            PROTOCOL_DEBUG("tx syn rcvd success, jump to established\r\n");
        }
        break;
    default:
        break;
    }
}

static void protocol_bind(void)
{
    if (protocol.role == PROTOCOL_MASTER)
    {
        protocol_tx_bind();
    }
    else if (protocol.role == PROTOCOL_SLAVER)
    {
        protocol_rx_bind();
    }
}

static void protocol_bind_check(void)
{
    if (protocol.role == PROTOCOL_MASTER)
    {
        protocol_tx_bind_check(protocol.rxBuffer);
    }
    else if (protocol.role == PROTOCOL_SLAVER)
    {
        protocol_rx_bind_check(protocol.rxBuffer);
    }
}

static void protocol_unpack_data(uint8_t type, uint8_t *payload)
{
    switch (protocol.parm.version)
    {
    case PROTOCOL_BASIC_V1:
        protocol.analogChBuffer[0] = ((uint16_t)payload[0] | ((((uint16_t)payload[1]) << 8) & 0xF00)) & 0xFFF;
        protocol.analogChBuffer[1] = (((((uint16_t)payload[1]) >> 4) & 0x0F) | ((((uint16_t)payload[2]) << 4) & 0xFF0)) & 0xFFF;
        protocol.analogChBuffer[2] = ((uint16_t)payload[3] | ((((uint16_t)payload[4]) << 8) & 0xF00)) & 0xFFF;
        protocol.analogChBuffer[3] = (((((uint16_t)payload[4]) >> 4) & 0x0F) | ((((uint16_t)payload[5]) << 4) & 0xFF0)) & 0xFFF;

        protocol.analogChBuffer[4] = ((uint16_t)payload[6] | ((((uint16_t)payload[7]) << 8) & 0x300)) & 0x3FF;
        protocol.analogChBuffer[5] = (((((uint16_t)payload[7]) >> 2) & 0x3F) | ((((uint16_t)payload[8]) << 6) & 0x3C0)) & 0x3FF;
        protocol.analogChBuffer[6] = (((((uint16_t)payload[8]) >> 4) & 0x0F) | ((((uint16_t)payload[9]) << 4) & 0x3F0)) & 0x3FF;
        protocol.analogChBuffer[7] = (((((uint16_t)payload[9]) >> 6) & 0x03) | ((((uint16_t)payload[10]) << 2) & 0xFF0)) & 0x3FF;

        protocol.analogChBuffer[8] = ((uint16_t)payload[11] | ((((uint16_t)payload[12]) << 8) & 0x300)) & 0x3FF;
        protocol.analogChBuffer[9] = (((((uint16_t)payload[12]) >> 2) & 0x3F) | ((((uint16_t)payload[13]) << 6) & 0x3C0)) & 0x3FF;
        protocol.analogChBuffer[10] = (((((uint16_t)payload[13]) >> 4) & 0x0F) | ((((uint16_t)payload[14]) << 4) & 0x3F0)) & 0x3FF;
        protocol.analogChBuffer[11] = (((((uint16_t)payload[14]) >> 6) & 0x03) | ((((uint16_t)payload[15]) << 2) & 0xFF0)) & 0x3FF;
        break;
    case PROTOCOL_LONG_V1:
        /* code */
        break;
    case PROTOCOL_MULTIPLE_V1:
        /* code */
        break;
    case PROTOCOL_DUPLEX_V1:
        /* code */
        break;
    default:
        break;
    }
}

static void protocol_receive_data(void)
{
    uint8_t type = 0;
    uint8_t serialNum = 0;
    uint16_t time = 0;
    PROTOCOL_DEBUG("RX OK\r\n");
    if (CRC8_calculate(&protocolCrc, protocol.rxBuffer, protocol.packLen - 1) == protocol.rxBuffer[protocol.packLen - 1]) // CRC校验通过
    {
        if (protocol.workStatus == PROTOCOL_BINDING) // 处于对频模式中
        {
            protocol_bind_check();
        }
        else // 接收模式
        {
            protocol.channelPoint = protocol.rxBuffer[0] & 0x7f;
            serialNum = (protocol.rxBuffer[1] & 0x0F);
            type = ((protocol.rxBuffer[1] >> 4) & 0x0F);
            time = PROTOCOL_HOP_FREQ_PERIOD_US + 400 - 5000 * (serialNum + 1) + 2500 * (((~protocol.rxBuffer[0]) >> 7) & 0x01);
            protocol_set_tim(time);
            protocol.loseCnt = 0;
            protocol.rxCnt++;
            if (protocol.workStatus == PROTOCOL_DISCONNECTED)
            {
                user_led_connected();
            }
            protocol.workStatus = PROTOCOL_CONNECTED;
            PROTOCOL_DEBUG("CH %d, SN %d, SSN %d, TIME %d\r\n", protocol.channelPoint, serialNum, (protocol.rxBuffer[0]) >> 7 & 0x01, time);
            protocol_unpack_data(type, &protocol.rxBuffer[2]);
        }
    }
}

static void protocol_pack_data(uint8_t *serialNum, uint8_t *payload)
{
    switch (protocol.parm.version)
    {
    case PROTOCOL_BASIC_V1:
        *serialNum = (*serialNum & 0x0F);
        payload[0] = protocol.analogChBuffer[0] & 0xFF;
        payload[1] = ((protocol.analogChBuffer[0] >> 8) & 0x0F) | ((protocol.analogChBuffer[1] << 4) & 0xF0);
        payload[2] = ((protocol.analogChBuffer[1] >> 4) & 0xFF);
        payload[3] = (protocol.analogChBuffer[2] & 0xFF);
        payload[4] = ((protocol.analogChBuffer[2] >> 8) & 0x0F) | ((protocol.analogChBuffer[3] << 4) & 0xF0);
        payload[5] = ((protocol.analogChBuffer[3] >> 4) & 0xFF);

        payload[6] = protocol.analogChBuffer[4] & 0xFF;
        payload[7] = ((protocol.analogChBuffer[4] >> 8) & 0x03) | ((protocol.analogChBuffer[5] << 2) & 0xFC);
        payload[8] = ((protocol.analogChBuffer[5] >> 6) & 0x0F) | ((protocol.analogChBuffer[6] << 4) & 0xF0);
        payload[9] = ((protocol.analogChBuffer[6] >> 4) & 0x3F) | ((protocol.analogChBuffer[7] << 6) & 0xC0);
        payload[10] = ((protocol.analogChBuffer[7] >> 2) & 0xFF);

        payload[11] = protocol.analogChBuffer[8] & 0xFF;
        payload[12] = ((protocol.analogChBuffer[8] >> 8) & 0x03) | ((protocol.analogChBuffer[9] << 2) & 0xFC);
        payload[13] = ((protocol.analogChBuffer[9] >> 6) & 0x0F) | ((protocol.analogChBuffer[10] << 4) & 0xF0);
        payload[14] = ((protocol.analogChBuffer[10] >> 4) & 0x3F) | ((protocol.analogChBuffer[11] << 6) & 0xC0);
        payload[15] = ((protocol.analogChBuffer[11] >> 2) & 0xFF);

        break;
    case PROTOCOL_LONG_V1:
        /* code */
        break;
    case PROTOCOL_MULTIPLE_V1:
        /* code */
        break;
    case PROTOCOL_DUPLEX_V1:
        /* code */
        break;
    default:
        break;
    }
}

static void protocol_tx_timer_callback(void)
{
    protocol.hopTickCnt += protocol.parm.txPeriod;

    if (protocol.hopTickCnt >= PROTOCOL_HOP_FREQ_PERIOD_US)
    {
        protocol.hopTickCnt = 0;
        protocol.channelPoint = (protocol.channelPoint + 1) % 127;
        protocol.txBuffer[0] = protocol.channelPoint;
        protocol_set_channel(protocol.channelList[protocol.channelPoint]);
    }

    switch (protocol.parm.txFreq)
    {
    case PROTOCOL_TX_50HZ:
        protocol.txBuffer[1] = 0;
        break;
    case PROTOCOL_TX_100HZ:
        protocol.txBuffer[1] = (protocol.hopTickCnt / protocol.parm.txPeriod) * 2;
        break;
    case PROTOCOL_TX_200HZ:
        protocol.txBuffer[1] = (protocol.hopTickCnt / protocol.parm.txPeriod);
        break;
    case PROTOCOL_TX_400HZ:
        protocol.txBuffer[1] = ((protocol.hopTickCnt / protocol.parm.txPeriod)) / 2;
        if ((protocol.hopTickCnt / protocol.parm.txPeriod) % 2 == 1)
        {
            protocol.txBuffer[0] |= 0x80;
        }
        else
        {
            protocol.txBuffer[0] &= 0x7f;
        }
        break;
    default:
        break;
    }

    protocol_pack_data(&protocol.txBuffer[1], &protocol.txBuffer[2]);

    switch (protocol.parm.version)
    {
    case PROTOCOL_BASIC_V1:
        protocol.txBuffer[18] = CRC8_calculate(&protocolCrc, protocol.txBuffer, 18);
        break;
    case PROTOCOL_LONG_V1:
        protocol.txBuffer[13] = CRC8_calculate(&protocolCrc, protocol.txBuffer, 13);
        break;
    case PROTOCOL_MULTIPLE_V1:
        protocol.txBuffer[15] = CRC8_calculate(&protocolCrc, protocol.txBuffer, 15);
        break;
    case PROTOCOL_DUPLEX_V1:
        // protocol.txBuffer[17] = CRC8_calculate(&protocolCrc, protocol.txBuffer, 17);
        break;
    default:
        break;
    }

    protocol_send_data(protocol.txBuffer, protocol.packLen);
    PROTOCOL_DEBUG("cnt:%d point:%d list:%d", protocol.frameCnt, protocol.channelPoint, protocol.channelList[protocol.channelPoint]);
}

static void protocol_rx_timer_callback(void)
{
    if (protocol.loseCnt < 30)
    {
        protocol_set_tim(PROTOCOL_HOP_FREQ_PERIOD_US - 1);
        protocol.loseCnt++;
    }
    else
    {
        protocol_set_tim(PROTOCOL_SLOW_HOP_FREQ_PERIOD_US - 1);
        if (protocol.workStatus == PROTOCOL_CONNECTED)
        {
            user_led_loss_signal();
        }
        protocol.workStatus = PROTOCOL_DISCONNECTED;
    }
    protocol.channelPoint = (protocol.channelPoint + 1) % 127;
    PROTOCOL_DEBUG("rx ch %d,\r\n", protocol.channelPoint);
    protocol_set_channel(protocol.channelList[protocol.channelPoint]);
}

static void protocol_tx_interrupt(NRF24L01TypeDef *dev)
{
    if (dev == &wirelessModule)
    {
        PROTOCOL_DEBUG("tx fin\r\n");
    }
}

static void protocol_rx_interrupt(NRF24L01TypeDef *dev)
{
    if (dev == &wirelessModule)
    {
        protocol_receive_data();
    }
}

void protocol_init(protocolRole role)
{
    protocol_module_init();

    CRC8_init(&protocolCrc, 0xD5); // 初始化CRC

    protocol.role = role;

    protocol.workStatus = PROTOCOL_INIT;

    if (protocol.role == PROTOCOL_MASTER)
    {
        protocol.parm.seed = protocol_seed_generate(); // 生成跳频种子
        protocol_channel_generate(protocol.parm.seed); // 生成跳频表
        protocol_set_tx_freq(PROTOCOL_TX_400HZ);
    }
    else if (protocol.role == PROTOCOL_SLAVER)
    {
        protocol_addr_generate();
    }
    else
    {
        return;
    }

    protocol_set_channel(protocol.bindCh);
    protocol_set_tx_addr(protocol.bindAddr);
    protocol_set_rx_addr(protocol.bindAddr);

    if (protocol_get_parm(&protocol.parm) == PROTOCOL_PARM_NONE) // 读取参数
    {
        protocol_bind_req();
    }
    else
    {
        // 根据读取的参数设置
        protocol_set_power(protocol.parm.power);
        protocol_set_tx_addr(protocol.parm.addr);
        protocol_set_rx_addr(protocol.parm.addr);

        // 生成跳频表
        protocol_channel_generate(protocol.parm.seed);

        // 输出参数
        PROTOCOL_DEBUG("parm read success\r\n");
        PROTOCOL_DEBUG("Seed: 0x%hX\r\n", protocol.parm.seed);
        PROTOCOL_DEBUG("Power: %d\r\n", protocol.parm.power);
        PROTOCOL_DEBUG("version: %d\r\n", protocol.parm.version);
        PROTOCOL_DEBUG("Addr: %d %d %d %d %d\r\n", protocol.parm.addr[0], protocol.parm.addr[1], protocol.parm.addr[2], protocol.parm.addr[3], protocol.parm.addr[4]);
        PROTOCOL_DEBUG("Channel: ");
        uint8_t i;
        for (i = 0; i < 127; i++)
        {
            PROTOCOL_DEBUG("%d ", protocol.channelList[i]);
        }
        PROTOCOL_DEBUG("\r\n");
    }

    PROTOCOL_DEBUG("Init seccess!\r\n");
}

void protocol_tick(void)
{
    switch (protocol.workStatus)
    {
    case PROTOCOL_INIT:
        protocol.workStatus = PROTOCOL_DISCONNECTED;
        break;
    case PROTOCOL_BINDING:
        if (protocol_get_bind_req() == PROTOCOL_BIND_REQ_NONE)
        {
            user_led_loss_signal();
            protocol_bind_exit();
            protocol.workStatus = PROTOCOL_DISCONNECTED;
        }
        protocol_bind();
        break;
    case PROTOCOL_CONNECTED:
        if (protocol_get_bind_req() == PROTOCOL_BIND_REQ)
        {
            user_led_binding();
            protocol_bind_enter();
            protocol.workStatus = PROTOCOL_BINDING;
        }
        break;
    case PROTOCOL_DISCONNECTED:
        if (protocol_get_bind_req() == PROTOCOL_BIND_REQ)
        {
            user_led_binding();
            protocol_bind_enter();
            protocol.workStatus = PROTOCOL_BINDING;
        }
        break;
    default:
        break;
    }
    while ((HAL_GPIO_ReadPin(IRQ_GPIO_Port, IRQ_Pin) == 0))
    {
        NRF24L01_IRQ(&wirelessModule);
    }
}

void protocol_set_ch(uint32_t *analogCh, uint8_t *digitalCh)
{
    uint32_t i = 0;
    for (i = 0; i < PROTOCOL_ANALOG_CH_NUM; i++)
    {
        protocol.analogChBuffer[i] = analogCh[i];
    }
    for (i = 0; i < PROTOCOL_DIGITAL_CH_NUM; i++)
    {
        protocol.digitalChBuffer[i] = digitalCh[i];
    }
}

void protocol_get_ch(uint32_t *analogCh, uint8_t *digitalCh)
{
    uint32_t i = 0;
    for (i = 0; i < PROTOCOL_ANALOG_CH_NUM; i++)
    {
        analogCh[i] = protocol.analogChBuffer[i];
    }
    for (i = 0; i < PROTOCOL_DIGITAL_CH_NUM; i++)
    {
        digitalCh[i] = protocol.digitalChBuffer[i];
    }
}

void protocol_bind_req(void)
{
    protocol.bindReq = PROTOCOL_BIND_REQ;
}

void protocol_clean_bind_req(void)
{
    protocol.bindReq = PROTOCOL_BIND_REQ_NONE;
}

void protocol_timer_callback(void)
{
    if (protocol.role == PROTOCOL_MASTER)
    {
        protocol_tx_timer_callback();
    }
    else if (protocol.role == PROTOCOL_SLAVER)
    {
        protocol_rx_timer_callback();
    }
}

void protocol_set_tx_freq(protocolTxFreq freq)
{
    protocol.parm.txFreq = freq;
    switch (freq)
    {
    case PROTOCOL_TX_50HZ:
        protocol.parm.txPeriod = 20000; // us
        break;

    case PROTOCOL_TX_100HZ:
        protocol.parm.txPeriod = 10000; // us
        break;
    case PROTOCOL_TX_200HZ:
        protocol.parm.txPeriod = 5000; // us
        break;
    case PROTOCOL_TX_400HZ:
        protocol.parm.txPeriod = 2500; // us
        break;
    default:
        break;
    }
}

void protocol_set_protocol_version(protocolVersion ver)
{
    protocol.parm.version = ver;
}

protocolVersion protocol_get_protocol_version(void)
{
    return protocol.parm.version;
}

protocolParmNum protocol_get_parm(parmTypeDef *parm)
{
    return PROTOCOL_PARM_NONE;
}