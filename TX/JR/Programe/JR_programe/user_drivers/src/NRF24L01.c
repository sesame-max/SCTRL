#include "main.h"
#include "NRF24L01.h"
#include "protocol.h"

/**
 * @brief   SPI读写函数
 * @param   data：要通过SPI发送的数据
 * @retval  SPI返回的数据
 */
static uint8_t NRF24L01_read_write(NRF24L01TypeDef *dev, uint8_t data)
{
    uint16_t timeout = 0;
    LL_SPI_TransmitData8((SPI_TypeDef *)dev->spi, data);
    timeout = 0;
    while (LL_SPI_IsActiveFlag_BSY((SPI_TypeDef *)dev->spi) == 1)
    {
        timeout++;
        if (timeout > 2000)
            break;
    }
    return LL_SPI_ReceiveData8((SPI_TypeDef *)dev->spi);
}

/**
 * @brief   SPI写寄存器函数
 * @param   reg：要写入的寄存器地址
 * @param   value：要写入的数据
 * @retval  状态寄存器的值
 */
static uint8_t NRF24L01_write_reg(NRF24L01TypeDef *dev, uint8_t reg, uint8_t value)
{
    uint8_t status;
    NRF_CS_LOW(dev->cs_port, dev->cs_pin);         // 使能SPI传输
    status = NRF24L01_read_write(dev, reg + 0x20); // 发送寄存器号
    NRF24L01_read_write(dev, value);               // 写入寄存器的值
    NRF_CS_HIGH(dev->cs_port, dev->cs_pin);        // 关闭SPI传输
    return (status);                               // 返回状态值
}

/**
 * @brief   SPI读寄存器函数
 * @param   reg：要读取的寄存器地址
 * @retval  读取寄存器的值
 */
static uint8_t NRF24L01_read_reg(NRF24L01TypeDef *dev, uint8_t reg)
{
    uint8_t reg_val;
    NRF_CS_LOW(dev->cs_port, dev->cs_pin);    // 使能SPI传输
    NRF24L01_read_write(dev, reg);            // 发送寄存器号
    reg_val = NRF24L01_read_write(dev, 0xff); // 读取寄存器内容
    NRF_CS_HIGH(dev->cs_port, dev->cs_pin);   // 关闭SPI传输
    return (reg_val);                         // 返回状态值
}
/**
 * @brief   SPI写入缓存函数
 * @param   reg：要写入的寄存器地址
 * @param   *pBuf：要写入的数据指针
 * @param   len：要写入的数据长度
 * @retval  状态寄存器的值
 */
static uint8_t NRF24L01_write_buf(NRF24L01TypeDef *dev, uint8_t reg, uint8_t *pBuf, uint8_t len)
{
    uint8_t status, i;
    NRF_CS_LOW(dev->cs_port, dev->cs_pin);         // 使能SPI传输
    status = NRF24L01_read_write(dev, reg + 0x20); // 发送寄存器值(位置),并读取状态值
    for (i = 0; i < len; i++)
        NRF24L01_read_write(dev, *pBuf++);  // 写入数据
    NRF_CS_HIGH(dev->cs_port, dev->cs_pin); // 关闭SPI传输
    return status;                          // 返回读到的状态值
}

/**
 * @brief   SPI读取缓存函数
 * @param   reg：要读取的寄存器地址
 * @param   *pBuf：读取数据的存放指针
 * @param   len：要读取的数据长度
 * @retval  状态寄存器的值
 */
static uint8_t NRF24L01_read_buf(NRF24L01TypeDef *dev, uint8_t reg, uint8_t *pBuf, uint8_t len)
{
    uint8_t status, i;
    NRF_CS_LOW(dev->cs_port, dev->cs_pin);  // 使能SPI传输
    status = NRF24L01_read_write(dev, reg); // 发送寄存器值(位置),并读取状态值
    for (i = 0; i < len; i++)
        pBuf[i] = NRF24L01_read_write(dev, 0XFF); // 读出数据
    NRF_CS_HIGH(dev->cs_port, dev->cs_pin);       // 关闭SPI传输
    return status;                                // 返回读到的状态值
}

/**
 * @brief   NRF24检测函数
 * @param   *
 * @retval  1：检测错误 0：检测成功
 */
NRF24L01Status NRF24L01_check(NRF24L01TypeDef *dev)
{
    uint8_t buf[5] = {0XA5, 0XA5, 0XA5, 0XA5, 0XA5};
    uint8_t i;
    NRF_CE_HIGH(dev->ce_port, dev->ce_pin);
    HAL_Delay(2);
    NRF_CE_LOW(dev->ce_port, dev->ce_pin);
    NRF24L01_write_buf(dev, TX_ADDR, buf, 5); // 写入5个字节的地址.
    NRF24L01_read_buf(dev, TX_ADDR, buf, 5);  // 读出写入的地址
    for (i = 0; i < 5; i++)
        if (buf[i] != 0XA5)
            break;
    if (i != 5)
        return 1;       // 检测24L01错误
    return NRF_SUCCESS; // 检测到24L01
}

/**
 * @brief   NRF24L01初始化配置函数
 * @param   *
 * @retval  返回0，无意义
 */
NRF24L01Status NRF24L01_init(NRF24L01TypeDef *dev)
{
    uint8_t temp[2];
    LL_SPI_Enable((SPI_TypeDef *)dev->spi);
    if (NRF24L01_check(dev) == NRF_SUCCESS)
    {
        NRF_CE_LOW(dev->ce_port, dev->ce_pin);
        NRF24L01_write_reg(dev, STATUS, RX_OK);                                // 清除中断
        NRF24L01_write_reg(dev, STATUS, TX_OK);                                // 清除中断
        NRF24L01_write_reg(dev, EN_AA, 0x00);                                  // 关闭自动应答
        NRF24L01_write_reg(dev, EN_RXADDR, 0x01);                              // 允许P0信道
        NRF24L01_write_reg(dev, SETUP_RETR, 0x00);                             // 禁止自动重发
        NRF24L01_rx_mode(dev);                                                 // 切换到接受模式
        NRF24L01_set_channel(dev, 110);                                        // 设置频率为110
        NRF24L01_set_power(dev, 3);                                            // 设置功率为0dBm
        NRF24L01_set_size(dev, dev->packLen);                                  // 设置接受数据长度为RX_LEN
        while (!(NRF24L01_read_reg(dev, NRF_FIFO_STATUS) & NRF_RX_FIFO_EMPTY)) // 初始化时RX_FIFO非空，读取并丢弃
        {
            NRF24L01_read_FIFO(dev, dev->rxBuffer, dev->packLen); // 将FIFO中接收到的数据写入rxData}
        }
        return NRF_SUCCESS;
    }
    else
    {
        return NRF_ERROR;
    }
}

/**
 * @brief   切换到接受模式函数
 * @param   *
 * @retval  返回0，无意义
 */
NRF24L01Status NRF24L01_rx_mode(NRF24L01TypeDef *dev)
{
    NRF_CE_LOW(dev->ce_port, dev->ce_pin);
    NRF24L01_write_reg(dev, CONFIG, 0x3b);
    NRF_CE_HIGH(dev->ce_port, dev->ce_pin);
    return NRF_SUCCESS;
}

/**
 * @brief   切换到发送模式函数
 * @param   *
 * @retval  返回1，无意义
 */
NRF24L01Status NRF24L01_tx_mode(NRF24L01TypeDef *dev)
{
    NRF_CE_LOW(dev->ce_port, dev->ce_pin);
    NRF24L01_write_reg(dev, CONFIG, 0x0a);
    NRF_CE_HIGH(dev->ce_port, dev->ce_pin);
    return NRF_SUCCESS;
}

/**
 * @brief   接受数据长度设置函数
 * @param   size：接受数据的长度
 * @retval  返回0，无意义
 */
NRF24L01Status NRF24L01_set_size(NRF24L01TypeDef *dev, uint8_t size)
{
    dev->packLen = size;
    NRF_CE_LOW(dev->ce_port, dev->ce_pin);
    NRF24L01_write_reg(dev, RX_PW_P0, size);
    NRF_CE_HIGH(dev->ce_port, dev->ce_pin);
    return NRF_SUCCESS;
}

/**
 * @brief   接受频段设置函数
 * @param   channel：接受频段
 * @retval  返回0，无意义
 */
NRF24L01Status NRF24L01_set_channel(NRF24L01TypeDef *dev, uint8_t channel)
{
    NRF_CE_LOW(dev->ce_port, dev->ce_pin);
    NRF24L01_write_reg(dev, RF_CH, channel);
    NRF_CE_HIGH(dev->ce_port, dev->ce_pin);
    return NRF_SUCCESS;
}

/**
 * @brief   发送功率设置函数
 * @param   power：发送功率 3：0dBm 2：-6dBm 1：-12dBm 0：-18dBm
 * @retval  返回0，无意义
 */
NRF24L01Status NRF24L01_set_power(NRF24L01TypeDef *dev, NRF24L01Power power)
{
    uint8_t powerReg = 0;
    NRF_CE_LOW(dev->ce_port, dev->ce_pin);
    switch (power)
    {
    case NRF_POWER_0:
        powerReg = 0x21;
        break;
    case NRF_POWER_1:
        powerReg = 0x23;
        break;
    case NRF_POWER_2:
        powerReg = 0x25;
        break;
    case NRF_POWER_3:
        powerReg = 0x27;
        break;
    default:
        powerReg = 0x21;
        break;
    }
    NRF24L01_write_reg(dev, RF_SETUP, powerReg); // 0dBm
    NRF_CE_HIGH(dev->ce_port, dev->ce_pin);
    return NRF_SUCCESS;
}

/**
 * @brief   发送地址设置函数
 * @param   *address：发送地址指针
 * @retval  返回0，无意义
 */
NRF24L01Status NRF24L01_set_tx_address(NRF24L01TypeDef *dev, uint8_t *address)
{
    NRF_CE_LOW(dev->ce_port, dev->ce_pin);
    NRF24L01_write_buf(dev, TX_ADDR, address, 5);
    NRF_CE_HIGH(dev->ce_port, dev->ce_pin);
    return NRF_SUCCESS;
}

/**
 * @brief   接受地址设置函数
 * @param   *address：接受地址指针
 * @retval  返回0，无意义
 */
NRF24L01Status NRF24L01_set_rx_address(NRF24L01TypeDef *dev, uint8_t *address)
{
    NRF_CE_LOW(dev->ce_port, dev->ce_pin);
    NRF24L01_write_buf(dev, RX_ADDR_P0, address, 5);
    NRF_CE_HIGH(dev->ce_port, dev->ce_pin);
    return NRF_SUCCESS;
}

/**
 * @brief   写入FIFO函数
 * @param   *data：写入数据的指针
 * @param   len：写入数据的长度
 * @retval  返回0，无意义
 */
NRF24L01Status NRF24L01_write_FIFO(NRF24L01TypeDef *dev, uint8_t *data, uint8_t len)
{
    uint8_t state;
    NRF_CE_LOW(dev->ce_port, dev->ce_pin);
    NRF24L01_write_buf(dev, WR_TX_PLOAD - 0x20, data, len);
    NRF_CE_HIGH(dev->ce_port, dev->ce_pin);
    return NRF_SUCCESS;
}

/**
 * @brief   读取FIFO函数
 * @param   *data：读取数据存放的指针
 * @param   len：读取数据的长度
 * @retval  返回0，无意义
 */
NRF24L01Status NRF24L01_read_FIFO(NRF24L01TypeDef *dev, uint8_t *data, uint8_t len)
{
    NRF_CE_LOW(dev->ce_port, dev->ce_pin);
    NRF24L01_read_buf(dev, RD_RX_PLOAD, data, len);
    NRF_CE_HIGH(dev->ce_port, dev->ce_pin);
    return NRF_SUCCESS;
}

/**
 * @brief   中断处理函数
 * @param   *rxData：接受数据的存放地址
 * @retval  0：发送完成中断 1：接受完成中断 -1：无效中断
 */
NRF24L01Status NRF24L01_IRQ(NRF24L01TypeDef *dev)
{
    uint8_t state;
    state = NRF24L01_read_reg(dev, STATUS);
    if ((state & TX_OK) || (state & RX_OK))
    {
        if (state & TX_OK) // 发送完成
        {
            dev->txCallback(dev);
            NRF24L01_write_reg(dev, STATUS, TX_OK);                            // 清除中断
            if ((NRF24L01_read_reg(dev, NRF_FIFO_STATUS) & NRF_TX_FIFO_EMPTY)) // TX FIFO非空，继续发送
            {
                NRF24L01_rx_mode(dev); // 回到接收模式
            }
        }
        if (state & RX_OK)
        {
            while (!(NRF24L01_read_reg(dev, NRF_FIFO_STATUS) & NRF_RX_FIFO_EMPTY)) // RX FIFO非空，不断读取并覆盖
            {
                NRF24L01_read_FIFO(dev, dev->rxBuffer, dev->packLen); // 将FIFO中接收到的数据写入rxData}
            }
            dev->rxCallback(dev);
            NRF24L01_write_reg(dev, STATUS, RX_OK); // 清除中断
        }
        return NRF_SUCCESS;
    }

    return NRF_ERROR;
}