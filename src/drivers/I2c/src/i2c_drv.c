/**
 * @file    i2c_drv.c
 * @brief   STM32F103C8T6 I2C 驱动层实现 (V4.0)
 */

#include "i2c_api.h"
#include "i2c_ll.h"
#include "dma_api.h"
#include <stddef.h>

#define I2C_INSTANCE    I2C1
#define I2C_TIMEOUT     (100000UL)

/* DMA Channels for I2C1 (RM0008 Page 272) */
#define I2C1_DMA_TX_CH  DMA_CH6
#define I2C1_DMA_RX_CH  DMA_CH7

typedef enum { I2C_STATE_READY, I2C_STATE_BUSY_TX, I2C_STATE_BUSY_RX } I2c_State_e;
static volatile I2c_State_e g_i2c_state = I2C_STATE_READY;

/*===========================================================================*/
/* Internal Callbacks (R8#16 Service Linkage)                                */
/*===========================================================================*/
static void I2c_DmaCallback(void *pContext)
{
    (void)pContext;
    if (g_i2c_state == I2C_STATE_BUSY_TX)
    {
        I2C_LL_GenerateStop(I2C_INSTANCE);
    }
    g_i2c_state = I2C_STATE_READY;
}

/*===========================================================================*/
/* Polling Helpers                                                           */
/*===========================================================================*/
static I2c_ReturnType I2C_WaitFlag(uint32_t flag)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (!(I2C_LL_GetSR1(I2C_INSTANCE) & flag))
    {
        if (--timeout == 0U) return I2C_ERR_TIMEOUT;
        if (I2C_LL_GetSR1(I2C_INSTANCE) & I2C_SR1_AF_Msk) return I2C_AF;
    }
    return I2C_OK;
}

/*===========================================================================*/
/* Public API Implementation                                                 */
/*===========================================================================*/

I2c_ReturnType I2c_Init(const I2c_ConfigType *pConfig)
{
    if (pConfig == NULL) return I2C_ERR_PARAM;
    I2C_LL_Disable(I2C_INSTANCE);

    I2C_LL_ConfigureTiming(I2C_INSTANCE, pConfig);

    I2C_LL_Enable(I2C_INSTANCE);
    g_i2c_state = I2C_STATE_READY;
    return I2C_OK;
}

void I2c_DeInit(void)
{
    I2C_LL_Disable(I2C_INSTANCE);
    g_i2c_state = I2C_STATE_READY;
}

I2c_ReturnType I2c_MasterTransmit(uint16_t slaveAddr, const uint8_t *pData, uint16_t size)
{
    if (g_i2c_state != I2C_STATE_READY) return I2C_BUSY;
    
    I2C_LL_GenerateStart(I2C_INSTANCE);
    if (I2C_WaitFlag(I2C_SR1_SB_Msk) != I2C_OK) return I2C_ERR_TIMEOUT;

    I2C_LL_WriteDR(I2C_INSTANCE, (uint16_t)(slaveAddr << 1U));
    if (I2C_WaitFlag(I2C_SR1_ADDR_Msk) != I2C_OK) return I2C_AF;
    I2C_LL_ClearAddrFlag(I2C_INSTANCE);

    for (uint16_t i = 0U; i < size; i++)
    {
        I2C_LL_WriteDR(I2C_INSTANCE, pData[i]);
        if (I2C_WaitFlag(I2C_SR1_TxE_Msk) != I2C_OK) return I2C_ERR_TIMEOUT;
    }

    if (I2C_WaitFlag(I2C_SR1_BTF_Msk) != I2C_OK) return I2C_ERR_TIMEOUT;
    I2C_LL_GenerateStop(I2C_INSTANCE);
    return I2C_OK;
}

I2c_ReturnType I2c_MasterReceive(uint16_t slaveAddr, uint8_t *pData, uint16_t size)
{
    if (g_i2c_state != I2C_STATE_READY) return I2C_BUSY;
    
    I2C_LL_EnableAck(I2C_INSTANCE);
    I2C_LL_GenerateStart(I2C_INSTANCE);
    if (I2C_WaitFlag(I2C_SR1_SB_Msk) != I2C_OK) return I2C_ERR_TIMEOUT;

    I2C_LL_WriteDR(I2C_INSTANCE, (uint16_t)((slaveAddr << 1U) | 0x01U));
    if (I2C_WaitFlag(I2C_SR1_ADDR_Msk) != I2C_OK) return I2C_AF;

    if (size == 1U)
    {
        I2C_LL_DisableAck(I2C_INSTANCE);
        I2C_LL_ClearAddrFlag(I2C_INSTANCE);
        I2C_LL_GenerateStop(I2C_INSTANCE);
    }
    else
    {
        I2C_LL_ClearAddrFlag(I2C_INSTANCE);
    }

    for (uint16_t i = 0U; i < size; i++)
    {
        if (size > 1U && i == (size - 2U))
        {
            if (I2C_WaitFlag(I2C_SR1_RxNE_Msk) != I2C_OK) return I2C_ERR_TIMEOUT;
            I2C_LL_DisableAck(I2C_INSTANCE);
            I2C_LL_GenerateStop(I2C_INSTANCE);
        }
        if (I2C_WaitFlag(I2C_SR1_RxNE_Msk) != I2C_OK) return I2C_ERR_TIMEOUT;
        pData[i] = (uint8_t)I2C_LL_ReadDR(I2C_INSTANCE);
    }
    return I2C_OK;
}

/* Asynchronous DMA Transmit (R8#15 & R8#16) */
I2c_ReturnType I2c_MasterTransmitDMA(uint16_t slaveAddr, const uint8_t *pData, uint16_t size)
{
    if (g_i2c_state != I2C_STATE_READY) return I2C_BUSY;

    Dma_ConfigType dmaCfg = {
        .channel = I2C1_DMA_TX_CH,
        .direction = DMA_DIR_M2P,
        .srcSize = DMA_SIZE_8BIT, .dstSize = DMA_SIZE_8BIT,
        .srcInc = true, .dstInc = false,
        .pCallback = I2c_DmaCallback, .pContext = NULL
    };
    Dma_InitChannel(&dmaCfg);
    Dma_StartTransfer(I2C1_DMA_TX_CH, (uint32_t)pData, I2C_LL_GetDRAddress(I2C_INSTANCE), size);

    g_i2c_state = I2C_STATE_BUSY_TX;
    I2C_LL_GenerateStart(I2C_INSTANCE);
    if (I2C_WaitFlag(I2C_SR1_SB_Msk) != I2C_OK) return I2C_ERR_TIMEOUT;

    I2C_LL_WriteDR(I2C_INSTANCE, (uint16_t)(slaveAddr << 1U));
    if (I2C_WaitFlag(I2C_SR1_ADDR_Msk) != I2C_OK) return I2C_AF;
    I2C_LL_ClearAddrFlag(I2C_INSTANCE);

    I2C_LL_EnableDMA(I2C_INSTANCE);
    return I2C_OK;
}

/* Asynchronous DMA Receive (R8#15 & R8#16) */
I2c_ReturnType I2c_MasterReceiveDMA(uint16_t slaveAddr, uint8_t *pData, uint16_t size)
{
    if (g_i2c_state != I2C_STATE_READY) return I2C_BUSY;

    Dma_ConfigType dmaCfg = {
        .channel = I2C1_DMA_RX_CH,
        .direction = DMA_DIR_P2M,
        .srcSize = DMA_SIZE_8BIT, .dstSize = DMA_SIZE_8BIT,
        .srcInc = false, .dstInc = true,
        .pCallback = I2c_DmaCallback, .pContext = NULL
    };
    Dma_InitChannel(&dmaCfg);
    Dma_StartTransfer(I2C1_DMA_RX_CH, I2C_LL_GetDRAddress(I2C_INSTANCE), (uint32_t)pData, size);

    g_i2c_state = I2C_STATE_BUSY_RX;
    I2C_LL_EnableAck(I2C_INSTANCE);
    I2C_LL_GenerateStart(I2C_INSTANCE);
    if (I2C_WaitFlag(I2C_SR1_SB_Msk) != I2C_OK) return I2C_ERR_TIMEOUT;

    I2C_LL_WriteDR(I2C_INSTANCE, (uint16_t)((slaveAddr << 1U) | 0x01U));
    if (I2C_WaitFlag(I2C_SR1_ADDR_Msk) != I2C_OK) return I2C_AF;
    I2C_LL_ClearAddrFlag(I2C_INSTANCE);

    I2C_LL_EnableDMA(I2C_INSTANCE);
    I2C_LL_EnableLastDMA(I2C_INSTANCE);
    return I2C_OK;
}
