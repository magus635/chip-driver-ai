#include "i2c_api.h"
#include "i2c_ll.h"
#include <stddef.h>

/**
 * @file i2c_api.c
 * @brief I2C High-Level Driver Implementation (V2.0 Extended)
 * @note RM0090 §27 / RM0008 §26 compliant
 */

/*============================================================================
 * Private Macros
 *===========================================================================*/

#define I2C_WAIT_FLAG(I2Cx, FLAG_CHECK, STATUS, TIMEOUT) \
    do { \
        uint32_t wait_ticks = (TIMEOUT); \
        while (!FLAG_CHECK(I2Cx)) { \
            if (wait_ticks-- == 0U) return I2C_STATUS_TIMEOUT; \
            if (I2C_LL_GetErrorFlags(I2Cx) != 0U) return I2C_STATUS_ERROR; \
        } \
    } while (0)

#define I2C_WAIT_FLAG_HANDLE(hi2c, FLAG_CHECK, TIMEOUT) \
    do { \
        uint32_t wait_ticks = (TIMEOUT); \
        I2C_TypeDef *inst = (I2C_TypeDef *)(hi2c)->Instance; \
        while (!FLAG_CHECK(inst)) { \
            if (wait_ticks-- == 0U) { \
                (hi2c)->ErrorCode |= I2C_ERROR_TIMEOUT; \
                return I2C_STATUS_TIMEOUT; \
            } \
            if (I2C_LL_GetErrorFlags(inst) != 0U) { \
                (hi2c)->ErrorCode |= I2C_ERROR_BERR; \
                return I2C_STATUS_ERROR; \
            } \
        } \
    } while (0)

I2C_StatusType I2C_Init(I2C_TypeDef *I2Cx, const I2C_ConfigType *Config)
{
    if ((I2Cx == NULL) || (Config == NULL)) return I2C_STATUS_ERROR;

    /* Disable to configure */
    I2C_LL_Disable(I2Cx);

    /* Atomic Sequence: Software Reset if busy */
    if (I2C_LL_IsBusy(I2Cx)) {
        I2Cx->CR1 |= I2C_CR1_SWRST_Msk;
        /* Wait small delay and clear reset bit */
        for(volatile int i=0; i<100; i++);
        I2Cx->CR1 &= ~I2C_CR1_SWRST_Msk;
    }

    /* Configure timing and frequency */
    I2C_LL_Configure(I2Cx, Config);

    /* Set Own Address (Slave Mode related) */
    I2Cx->OAR1 = (uint32_t)(Config->own_address1 << 1UL); 
    if (Config->address_10bit) {
        I2Cx->OAR1 |= (1UL << 15UL); /* Set ADDMODE */
    }

    /* Set ACK Mode */
    I2C_LL_AcknowledgeNext(I2Cx, Config->ack_enable);

    /* Enable peripheral */
    I2C_LL_Enable(I2Cx);

    return I2C_STATUS_OK;
}

I2C_StatusType I2C_MasterTransmit(I2C_TypeDef *I2Cx, uint16_t SlaveAddr, const uint8_t *Data, uint16_t Size, uint32_t Timeout)
{
    /* 1. Generate START */
    I2C_LL_GenerateStart(I2Cx);
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsStartSent, SB, Timeout);

    /* 2. Send Slave Address (Write Mode: LSB=0) */
    I2C_LL_WriteData(I2Cx, (uint8_t)(SlaveAddr << 1U));
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsAddressSent, ADDR, Timeout);

    /* 3. Atomic Sequence: Clear ADDR Flag (RM §26.6.7) */
    I2C_LL_ClearAddressFlag(I2Cx);

    /* 4. Transmit Data */
    for (uint16_t i = 0U; i < Size; i++) {
        I2C_LL_WriteData(I2Cx, Data[i]);
        I2C_WAIT_FLAG(I2Cx, I2C_LL_IsTxEmpty, TXE, Timeout);
    }

    /* 5. Wait for last byte to be transmitted from shift register (BTF) */
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsByteFinished, BTF, Timeout);

    /* 6. Generate STOP */
    I2C_LL_GenerateStop(I2Cx);

    return I2C_STATUS_OK;
}

I2C_StatusType I2C_MasterReceive(I2C_TypeDef *I2Cx, uint16_t SlaveAddr, uint8_t *Data, uint16_t Size, uint32_t Timeout)
{
    if (Size == 0U) return I2C_STATUS_OK;

    /* 1. Generate START */
    I2C_LL_GenerateStart(I2Cx);
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsStartSent, SB, Timeout);

    /* 2. Send Slave Address (Read Mode: LSB=1) */
    I2C_LL_WriteData(I2Cx, (uint8_t)((SlaveAddr << 1U) | 1U));
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsAddressSent, ADDR, Timeout);

    /* 3. Handling Special N=1 and N=2 cases (RM §26.3.3 / ES0182) */
    if (Size == 1U) {
        /* N=1: Clear ACK before clearing ADDR */
        I2C_LL_AcknowledgeNext(I2Cx, false);
        I2C_LL_ClearAddressFlag(I2Cx);
        I2C_LL_GenerateStop(I2Cx);
        
        I2C_WAIT_FLAG(I2Cx, I2C_LL_IsRxNotEmpty, RXNE, Timeout);
        Data[0] = I2C_LL_ReadData(I2Cx);
    } 
    else if (Size == 2U) {
        /* N=2: Set POS bit, clear ACK */
        I2Cx->CR1 |= (1UL << 11UL); /* Set POS */
        I2C_LL_AcknowledgeNext(I2Cx, false);
        I2C_LL_ClearAddressFlag(I2Cx);
        
        /* Wait for BTF (Second byte in shift reg, first byte in DR) */
        I2C_WAIT_FLAG(I2Cx, I2C_LL_IsByteFinished, BTF, Timeout);
        
        I2C_LL_GenerateStop(I2Cx);
        Data[0] = I2C_LL_ReadData(I2Cx);
        Data[1] = I2C_LL_ReadData(I2Cx);
    } 
    else {
        /* N > 2: Standard sequence */
        I2C_LL_ClearAddressFlag(I2Cx);
        for (uint16_t i = 0U; i < Size; i++) {
            if (i == (Size - 3U)) {
                /* N-2 byte reached: wait for BTF to stretch SCL */
                I2C_WAIT_FLAG(I2Cx, I2C_LL_IsByteFinished, BTF, Timeout);
                /* Clear ACK */
                I2C_LL_AcknowledgeNext(I2Cx, false);
                /* Read Data N-2 */
                Data[i++] = I2C_LL_ReadData(I2Cx);
                /* Wait for BTF again */
                I2C_WAIT_FLAG(I2Cx, I2C_LL_IsByteFinished, BTF, Timeout);
                /* Generate STOP */
                I2C_LL_GenerateStop(I2Cx);
                /* Read Data N-1 */
                Data[i++] = I2C_LL_ReadData(I2Cx);
                /* Read Data N after RXNE */
                I2C_WAIT_FLAG(I2Cx, I2C_LL_IsRxNotEmpty, RXNE, Timeout);
                Data[i] = I2C_LL_ReadData(I2Cx);
                break;
            }
            I2C_WAIT_FLAG(I2Cx, I2C_LL_IsRxNotEmpty, RXNE, Timeout);
            Data[i] = I2C_LL_ReadData(I2Cx);
        }
    }

    return I2C_STATUS_OK;
}

/*============================================================================
 * Handle Initialization
 *===========================================================================*/

I2C_StatusType I2C_Handle_Init(I2C_HandleType *hi2c)
{
    if (hi2c == NULL) return I2C_STATUS_ERROR;
    if (hi2c->Instance == NULL) return I2C_STATUS_ERROR;

    I2C_TypeDef *I2Cx = (I2C_TypeDef *)hi2c->Instance;

    /* Initialize state */
    hi2c->State = I2C_STATE_BUSY;
    hi2c->ErrorCode = I2C_ERROR_NONE;

    /* Initialize peripheral */
    I2C_StatusType status = I2C_Init(I2Cx, &hi2c->Config);
    if (status != I2C_STATUS_OK) {
        hi2c->State = I2C_STATE_RESET;
        return status;
    }

    hi2c->State = I2C_STATE_READY;
    return I2C_STATUS_OK;
}

I2C_StatusType I2C_DeInit(I2C_TypeDef *I2Cx)
{
    if (I2Cx == NULL) return I2C_STATUS_ERROR;

    /* Disable peripheral */
    I2C_LL_Disable(I2Cx);

    /* Software reset */
    I2Cx->CR1 |= I2C_CR1_SWRST_Msk;
    for (volatile uint32_t i = 0U; i < 100U; i++) { }
    I2Cx->CR1 &= ~I2C_CR1_SWRST_Msk;

    return I2C_STATUS_OK;
}

/*============================================================================
 * Memory Access (Polling) - RM0090 §27.3.3
 *===========================================================================*/

I2C_StatusType I2C_MemWrite(I2C_TypeDef *I2Cx, uint16_t DevAddr, uint16_t MemAddr,
                             I2C_MemAddrSizeType MemAddrSize, const uint8_t *Data,
                             uint16_t Size, uint32_t Timeout)
{
    if ((I2Cx == NULL) || (Data == NULL)) return I2C_STATUS_ERROR;

    /* 1. Generate START */
    I2C_LL_GenerateStart(I2Cx);
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsStartSent, SB, Timeout);

    /* 2. Send Device Address (Write Mode) */
    I2C_LL_WriteData(I2Cx, (uint8_t)(DevAddr << 1U));
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsAddressSent, ADDR, Timeout);
    I2C_LL_ClearAddressFlag(I2Cx);

    /* 3. Send Memory Address */
    if (MemAddrSize == I2C_MEMADDR_SIZE_16BIT) {
        /* High byte first for 16-bit address */
        I2C_LL_WriteData(I2Cx, (uint8_t)(MemAddr >> 8U));
        I2C_WAIT_FLAG(I2Cx, I2C_LL_IsTxEmpty, TXE, Timeout);
    }
    I2C_LL_WriteData(I2Cx, (uint8_t)(MemAddr & 0xFFU));
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsTxEmpty, TXE, Timeout);

    /* 4. Transmit Data */
    for (uint16_t i = 0U; i < Size; i++) {
        I2C_LL_WriteData(I2Cx, Data[i]);
        I2C_WAIT_FLAG(I2Cx, I2C_LL_IsTxEmpty, TXE, Timeout);
    }

    /* 5. Wait BTF and Generate STOP */
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsByteFinished, BTF, Timeout);
    I2C_LL_GenerateStop(I2Cx);

    return I2C_STATUS_OK;
}

I2C_StatusType I2C_MemRead(I2C_TypeDef *I2Cx, uint16_t DevAddr, uint16_t MemAddr,
                            I2C_MemAddrSizeType MemAddrSize, uint8_t *Data,
                            uint16_t Size, uint32_t Timeout)
{
    if ((I2Cx == NULL) || (Data == NULL) || (Size == 0U)) return I2C_STATUS_ERROR;

    /* 1. Generate START */
    I2C_LL_GenerateStart(I2Cx);
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsStartSent, SB, Timeout);

    /* 2. Send Device Address (Write Mode) for memory address */
    I2C_LL_WriteData(I2Cx, (uint8_t)(DevAddr << 1U));
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsAddressSent, ADDR, Timeout);
    I2C_LL_ClearAddressFlag(I2Cx);

    /* 3. Send Memory Address */
    if (MemAddrSize == I2C_MEMADDR_SIZE_16BIT) {
        I2C_LL_WriteData(I2Cx, (uint8_t)(MemAddr >> 8U));
        I2C_WAIT_FLAG(I2Cx, I2C_LL_IsTxEmpty, TXE, Timeout);
    }
    I2C_LL_WriteData(I2Cx, (uint8_t)(MemAddr & 0xFFU));
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsTxEmpty, TXE, Timeout);

    /* 4. Generate Repeated START for read phase */
    I2C_LL_GenerateStart(I2Cx);
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsStartSent, SB, Timeout);

    /* 5. Send Device Address (Read Mode) */
    I2C_LL_WriteData(I2Cx, (uint8_t)((DevAddr << 1U) | 1U));
    I2C_WAIT_FLAG(I2Cx, I2C_LL_IsAddressSent, ADDR, Timeout);

    /* 6. Receive Data with N=1/N=2/N>2 handling */
    if (Size == 1U) {
        I2C_LL_AcknowledgeNext(I2Cx, false);
        I2C_LL_ClearAddressFlag(I2Cx);
        I2C_LL_GenerateStop(I2Cx);
        I2C_WAIT_FLAG(I2Cx, I2C_LL_IsRxNotEmpty, RXNE, Timeout);
        Data[0] = I2C_LL_ReadData(I2Cx);
    }
    else if (Size == 2U) {
        I2Cx->CR1 |= I2C_CR1_POS_Msk;
        I2C_LL_AcknowledgeNext(I2Cx, false);
        I2C_LL_ClearAddressFlag(I2Cx);
        I2C_WAIT_FLAG(I2Cx, I2C_LL_IsByteFinished, BTF, Timeout);
        I2C_LL_GenerateStop(I2Cx);
        Data[0] = I2C_LL_ReadData(I2Cx);
        Data[1] = I2C_LL_ReadData(I2Cx);
        I2Cx->CR1 &= ~I2C_CR1_POS_Msk;
    }
    else {
        I2C_LL_AcknowledgeNext(I2Cx, true);
        I2C_LL_ClearAddressFlag(I2Cx);
        for (uint16_t i = 0U; i < Size; i++) {
            if (i == (Size - 3U)) {
                I2C_WAIT_FLAG(I2Cx, I2C_LL_IsByteFinished, BTF, Timeout);
                I2C_LL_AcknowledgeNext(I2Cx, false);
                Data[i++] = I2C_LL_ReadData(I2Cx);
                I2C_WAIT_FLAG(I2Cx, I2C_LL_IsByteFinished, BTF, Timeout);
                I2C_LL_GenerateStop(I2Cx);
                Data[i++] = I2C_LL_ReadData(I2Cx);
                I2C_WAIT_FLAG(I2Cx, I2C_LL_IsRxNotEmpty, RXNE, Timeout);
                Data[i] = I2C_LL_ReadData(I2Cx);
                break;
            }
            I2C_WAIT_FLAG(I2Cx, I2C_LL_IsRxNotEmpty, RXNE, Timeout);
            Data[i] = I2C_LL_ReadData(I2Cx);
        }
    }

    return I2C_STATUS_OK;
}

/*============================================================================
 * Device Detection
 *===========================================================================*/

I2C_StatusType I2C_IsDeviceReady(I2C_TypeDef *I2Cx, uint16_t DevAddr,
                                  uint32_t Trials, uint32_t Timeout)
{
    if (I2Cx == NULL) return I2C_STATUS_ERROR;

    for (uint32_t trial = 0U; trial < Trials; trial++) {
        /* Generate START */
        I2C_LL_GenerateStart(I2Cx);

        uint32_t wait = Timeout;
        while (!I2C_LL_IsStartSent(I2Cx)) {
            if (wait-- == 0U) goto next_trial;
        }

        /* Send address */
        I2C_LL_WriteData(I2Cx, (uint8_t)(DevAddr << 1U));

        wait = Timeout;
        while (!I2C_LL_IsAddressSent(I2Cx)) {
            if (I2C_LL_IsAckFailure(I2Cx)) {
                I2C_LL_ClearAckFailure(I2Cx);
                I2C_LL_GenerateStop(I2Cx);
                goto next_trial;
            }
            if (wait-- == 0U) goto next_trial;
        }

        /* Device responded with ACK */
        I2C_LL_ClearAddressFlag(I2Cx);
        I2C_LL_GenerateStop(I2Cx);

        /* Wait for bus to be free */
        wait = Timeout;
        while (I2C_LL_IsBusy(I2Cx)) {
            if (wait-- == 0U) break;
        }

        return I2C_STATUS_OK;

    next_trial:
        /* Small delay before retry */
        for (volatile uint32_t d = 0U; d < 1000U; d++) { }
    }

    return I2C_STATUS_ERROR;
}

/*============================================================================
 * Interrupt Mode - State Machine
 *===========================================================================*/

I2C_StatusType I2C_MasterTransmit_IT(I2C_HandleType *hi2c, uint16_t DevAddr,
                                      uint8_t *Data, uint16_t Size)
{
    if (hi2c == NULL) return I2C_STATUS_ERROR;
    if (hi2c->State != I2C_STATE_READY) return I2C_STATUS_BUSY;

    I2C_TypeDef *I2Cx = (I2C_TypeDef *)hi2c->Instance;

    /* Setup transfer */
    hi2c->State = I2C_STATE_BUSY_TX;
    hi2c->Mode = I2C_MODE_MASTER;
    hi2c->ErrorCode = I2C_ERROR_NONE;
    hi2c->pBuffPtr = Data;
    hi2c->XferSize = Size;
    hi2c->XferCount = Size;
    hi2c->DevAddress = DevAddr;

    /* Enable ACK */
    I2C_LL_AcknowledgeNext(I2Cx, true);

    /* Generate START */
    I2C_LL_GenerateStart(I2Cx);

    /* Enable interrupts: EVT + BUF + ERR */
    I2Cx->CR2 |= (I2C_CR2_ITEVTEN_Msk | I2C_CR2_ITBUFEN_Msk | I2C_CR2_ITERREN_Msk);

    return I2C_STATUS_OK;
}

I2C_StatusType I2C_MasterReceive_IT(I2C_HandleType *hi2c, uint16_t DevAddr,
                                     uint8_t *Data, uint16_t Size)
{
    if (hi2c == NULL) return I2C_STATUS_ERROR;
    if (hi2c->State != I2C_STATE_READY) return I2C_STATUS_BUSY;

    I2C_TypeDef *I2Cx = (I2C_TypeDef *)hi2c->Instance;

    /* Setup transfer */
    hi2c->State = I2C_STATE_BUSY_RX;
    hi2c->Mode = I2C_MODE_MASTER;
    hi2c->ErrorCode = I2C_ERROR_NONE;
    hi2c->pBuffPtr = Data;
    hi2c->XferSize = Size;
    hi2c->XferCount = Size;
    hi2c->DevAddress = DevAddr;

    /* Enable ACK for N > 1 */
    I2C_LL_AcknowledgeNext(I2Cx, (Size > 1U));

    /* Generate START */
    I2C_LL_GenerateStart(I2Cx);

    /* Enable interrupts */
    I2Cx->CR2 |= (I2C_CR2_ITEVTEN_Msk | I2C_CR2_ITBUFEN_Msk | I2C_CR2_ITERREN_Msk);

    return I2C_STATUS_OK;
}

I2C_StatusType I2C_MemWrite_IT(I2C_HandleType *hi2c, uint16_t DevAddr,
                                uint16_t MemAddr, I2C_MemAddrSizeType MemAddrSize,
                                uint8_t *Data, uint16_t Size)
{
    if (hi2c == NULL) return I2C_STATUS_ERROR;
    if (hi2c->State != I2C_STATE_READY) return I2C_STATUS_BUSY;

    I2C_TypeDef *I2Cx = (I2C_TypeDef *)hi2c->Instance;

    /* Setup transfer */
    hi2c->State = I2C_STATE_BUSY_TX;
    hi2c->Mode = I2C_MODE_MEM;
    hi2c->ErrorCode = I2C_ERROR_NONE;
    hi2c->pBuffPtr = Data;
    hi2c->XferSize = Size;
    hi2c->XferCount = Size;
    hi2c->DevAddress = DevAddr;
    hi2c->MemAddress = MemAddr;
    hi2c->MemAddrSize = (uint8_t)MemAddrSize;

    /* Generate START */
    I2C_LL_GenerateStart(I2Cx);

    /* Enable interrupts */
    I2Cx->CR2 |= (I2C_CR2_ITEVTEN_Msk | I2C_CR2_ITBUFEN_Msk | I2C_CR2_ITERREN_Msk);

    return I2C_STATUS_OK;
}

I2C_StatusType I2C_MemRead_IT(I2C_HandleType *hi2c, uint16_t DevAddr,
                               uint16_t MemAddr, I2C_MemAddrSizeType MemAddrSize,
                               uint8_t *Data, uint16_t Size)
{
    if (hi2c == NULL) return I2C_STATUS_ERROR;
    if (hi2c->State != I2C_STATE_READY) return I2C_STATUS_BUSY;

    I2C_TypeDef *I2Cx = (I2C_TypeDef *)hi2c->Instance;

    /* Setup transfer */
    hi2c->State = I2C_STATE_BUSY_RX;
    hi2c->Mode = I2C_MODE_MEM;
    hi2c->ErrorCode = I2C_ERROR_NONE;
    hi2c->pBuffPtr = Data;
    hi2c->XferSize = Size;
    hi2c->XferCount = Size;
    hi2c->DevAddress = DevAddr;
    hi2c->MemAddress = MemAddr;
    hi2c->MemAddrSize = (uint8_t)MemAddrSize;

    /* Generate START */
    I2C_LL_GenerateStart(I2Cx);

    /* Enable interrupts */
    I2Cx->CR2 |= (I2C_CR2_ITEVTEN_Msk | I2C_CR2_ITBUFEN_Msk | I2C_CR2_ITERREN_Msk);

    return I2C_STATUS_OK;
}

/*============================================================================
 * IRQ Handlers
 *===========================================================================*/

/**
 * @brief Event IRQ Handler - handles SB, ADDR, TXE, RXNE, BTF
 */
void I2C_EV_IRQHandler(I2C_HandleType *hi2c)
{
    if (hi2c == NULL) return;

    I2C_TypeDef *I2Cx = (I2C_TypeDef *)hi2c->Instance;
    uint32_t sr1 = I2Cx->SR1;
    uint32_t sr2 = I2Cx->SR2; /* Read SR2 for some flag clearing */
    (void)sr2;

    /* SB: Start Bit sent */
    if ((sr1 & I2C_SR1_SB_Msk) != 0U) {
        if (hi2c->State == I2C_STATE_BUSY_TX) {
            /* Send address with write bit */
            I2C_LL_WriteData(I2Cx, (uint8_t)(hi2c->DevAddress << 1U));
        }
        else if (hi2c->State == I2C_STATE_BUSY_RX) {
            if ((hi2c->Mode == I2C_MODE_MEM) && (hi2c->MemAddrSize > 0U)) {
                /* Memory read: first send write address for mem addr */
                I2C_LL_WriteData(I2Cx, (uint8_t)(hi2c->DevAddress << 1U));
            }
            else {
                /* Normal read: send read address */
                I2C_LL_WriteData(I2Cx, (uint8_t)((hi2c->DevAddress << 1U) | 1U));
            }
        }
        return;
    }

    /* ADDR: Address sent and ACKed */
    if ((sr1 & I2C_SR1_ADDR_Msk) != 0U) {
        if (hi2c->State == I2C_STATE_BUSY_RX) {
            if (hi2c->XferSize == 1U) {
                /* N=1: Disable ACK before clearing ADDR */
                I2C_LL_AcknowledgeNext(I2Cx, false);
            }
            else if (hi2c->XferSize == 2U) {
                /* N=2: Set POS, disable ACK */
                I2Cx->CR1 |= I2C_CR1_POS_Msk;
                I2C_LL_AcknowledgeNext(I2Cx, false);
            }
        }
        /* Clear ADDR by reading SR1+SR2 (already done above) */
        I2C_LL_ClearAddressFlag(I2Cx);

        if ((hi2c->State == I2C_STATE_BUSY_RX) && (hi2c->XferSize == 1U)) {
            /* N=1: Generate STOP immediately after clearing ADDR */
            I2C_LL_GenerateStop(I2Cx);
        }
        return;
    }

    /* TXE: Transmit buffer empty */
    if ((sr1 & I2C_SR1_TXE_Msk) != 0U) {
        if (hi2c->State == I2C_STATE_BUSY_TX) {
            /* Memory mode: send memory address first */
            if ((hi2c->Mode == I2C_MODE_MEM) && (hi2c->MemAddrSize > 0U)) {
                if (hi2c->MemAddrSize == 2U) {
                    I2C_LL_WriteData(I2Cx, (uint8_t)(hi2c->MemAddress >> 8U));
                    hi2c->MemAddrSize = 1U;
                }
                else {
                    I2C_LL_WriteData(I2Cx, (uint8_t)(hi2c->MemAddress & 0xFFU));
                    hi2c->MemAddrSize = 0U;
                }
                return;
            }

            if (hi2c->XferCount > 0U) {
                I2C_LL_WriteData(I2Cx, *hi2c->pBuffPtr);
                hi2c->pBuffPtr++;
                hi2c->XferCount--;
            }
        }
        return;
    }

    /* BTF: Byte Transfer Finished */
    if ((sr1 & I2C_SR1_BTF_Msk) != 0U) {
        if (hi2c->State == I2C_STATE_BUSY_TX) {
            if (hi2c->XferCount == 0U) {
                /* All data transmitted, generate STOP */
                I2C_LL_GenerateStop(I2Cx);
                /* Disable interrupts */
                I2Cx->CR2 &= ~(I2C_CR2_ITEVTEN_Msk | I2C_CR2_ITBUFEN_Msk | I2C_CR2_ITERREN_Msk);
                hi2c->State = I2C_STATE_READY;
                if (hi2c->TxCpltCallback != NULL) {
                    hi2c->TxCpltCallback(hi2c);
                }
            }
        }
        else if (hi2c->State == I2C_STATE_BUSY_RX) {
            if (hi2c->XferSize == 2U) {
                /* N=2: BTF means both bytes ready */
                I2C_LL_GenerateStop(I2Cx);
                *hi2c->pBuffPtr++ = I2C_LL_ReadData(I2Cx);
                *hi2c->pBuffPtr = I2C_LL_ReadData(I2Cx);
                hi2c->XferCount = 0U;
                I2Cx->CR1 &= ~I2C_CR1_POS_Msk;
                I2Cx->CR2 &= ~(I2C_CR2_ITEVTEN_Msk | I2C_CR2_ITBUFEN_Msk | I2C_CR2_ITERREN_Msk);
                hi2c->State = I2C_STATE_READY;
                if (hi2c->RxCpltCallback != NULL) {
                    hi2c->RxCpltCallback(hi2c);
                }
            }
            else if (hi2c->XferCount == 3U) {
                /* N-2 position for N>2 */
                I2C_LL_AcknowledgeNext(I2Cx, false);
                *hi2c->pBuffPtr++ = I2C_LL_ReadData(I2Cx);
                hi2c->XferCount--;
            }
        }
        return;
    }

    /* RXNE: Receive buffer not empty */
    if ((sr1 & I2C_SR1_RXNE_Msk) != 0U) {
        if (hi2c->State == I2C_STATE_BUSY_RX) {
            *hi2c->pBuffPtr++ = I2C_LL_ReadData(I2Cx);
            hi2c->XferCount--;

            if (hi2c->XferCount == 0U) {
                /* Disable interrupts */
                I2Cx->CR2 &= ~(I2C_CR2_ITEVTEN_Msk | I2C_CR2_ITBUFEN_Msk | I2C_CR2_ITERREN_Msk);
                hi2c->State = I2C_STATE_READY;
                if (hi2c->RxCpltCallback != NULL) {
                    hi2c->RxCpltCallback(hi2c);
                }
            }
            else if (hi2c->XferCount == 1U) {
                /* Prepare for last byte */
                I2C_LL_AcknowledgeNext(I2Cx, false);
                I2C_LL_GenerateStop(I2Cx);
            }
        }
        return;
    }
}

/**
 * @brief Error IRQ Handler - handles BERR, ARLO, AF, OVR
 */
void I2C_ER_IRQHandler(I2C_HandleType *hi2c)
{
    if (hi2c == NULL) return;

    I2C_TypeDef *I2Cx = (I2C_TypeDef *)hi2c->Instance;
    uint32_t sr1 = I2Cx->SR1;

    /* Bus Error */
    if ((sr1 & I2C_SR1_BERR_Msk) != 0U) {
        hi2c->ErrorCode |= I2C_ERROR_BERR;
        I2Cx->SR1 &= ~I2C_SR1_BERR_Msk;
    }

    /* Arbitration Lost */
    if ((sr1 & I2C_SR1_ARLO_Msk) != 0U) {
        hi2c->ErrorCode |= I2C_ERROR_ARLO;
        I2Cx->SR1 &= ~I2C_SR1_ARLO_Msk;
    }

    /* ACK Failure */
    if ((sr1 & I2C_SR1_AF_Msk) != 0U) {
        hi2c->ErrorCode |= I2C_ERROR_AF;
        I2Cx->SR1 &= ~I2C_SR1_AF_Msk;
        /* Generate STOP to release bus */
        I2C_LL_GenerateStop(I2Cx);
    }

    /* Overrun/Underrun */
    if ((sr1 & I2C_SR1_OVR_Msk) != 0U) {
        hi2c->ErrorCode |= I2C_ERROR_OVR;
        I2Cx->SR1 &= ~I2C_SR1_OVR_Msk;
    }

    /* Disable interrupts and reset state */
    I2Cx->CR2 &= ~(I2C_CR2_ITEVTEN_Msk | I2C_CR2_ITBUFEN_Msk | I2C_CR2_ITERREN_Msk);
    hi2c->State = I2C_STATE_READY;

    /* Call error callback */
    if (hi2c->ErrorCallback != NULL) {
        hi2c->ErrorCallback(hi2c);
    }
}

/*============================================================================
 * Bus Recovery - 9-Clock Pulse Method (Zephyr-style)
 *===========================================================================*/

/**
 * @brief GPIO register structure for bitbang operations
 */
typedef struct {
    __IO uint32_t MODER;
    __IO uint32_t OTYPER;
    __IO uint32_t OSPEEDR;
    __IO uint32_t PUPDR;
    __IO uint32_t IDR;
    __IO uint32_t ODR;
    __IO uint32_t BSRR;
} GPIO_TypeDef_Simple;

I2C_StatusType I2C_BusRecovery(I2C_TypeDef *I2Cx, const I2C_GpioConfigType *GpioConfig)
{
    if ((I2Cx == NULL) || (GpioConfig == NULL)) return I2C_STATUS_ERROR;

    GPIO_TypeDef_Simple *gpio_scl = (GPIO_TypeDef_Simple *)GpioConfig->gpio_port_scl;
    GPIO_TypeDef_Simple *gpio_sda = (GPIO_TypeDef_Simple *)GpioConfig->gpio_port_sda;
    uint16_t pin_scl = GpioConfig->gpio_pin_scl;
    uint16_t pin_sda = GpioConfig->gpio_pin_sda;

    /* 1. Disable I2C peripheral */
    I2C_LL_Disable(I2Cx);

    /* 2. Save original GPIO modes */
    uint32_t scl_mode_backup = gpio_scl->MODER;
    uint32_t sda_mode_backup = gpio_sda->MODER;

    /* 3. Configure SCL as output, SDA as input */
    /* SCL: Output mode (01) */
    gpio_scl->MODER &= ~(3UL << (pin_scl * 2U));
    gpio_scl->MODER |= (1UL << (pin_scl * 2U));
    /* SDA: Input mode (00) - just clear the bits */
    gpio_sda->MODER &= ~(3UL << (pin_sda * 2U));

    /* 4. Check if SDA is stuck low */
    bool sda_stuck = ((gpio_sda->IDR & (1UL << pin_sda)) == 0U);

    if (sda_stuck) {
        /* 5. Generate up to 9 clock pulses to release SDA */
        for (uint32_t i = 0U; i < 9U; i++) {
            /* SCL low */
            gpio_scl->BSRR = (1UL << (pin_scl + 16U));
            for (volatile uint32_t d = 0U; d < 500U; d++) { }

            /* SCL high */
            gpio_scl->BSRR = (1UL << pin_scl);
            for (volatile uint32_t d = 0U; d < 500U; d++) { }

            /* Check if SDA is released */
            if ((gpio_sda->IDR & (1UL << pin_sda)) != 0U) {
                break;
            }
        }

        /* 6. Generate STOP condition (SDA low->high while SCL high) */
        /* Set SDA as output */
        gpio_sda->MODER |= (1UL << (pin_sda * 2U));

        /* SDA low */
        gpio_sda->BSRR = (1UL << (pin_sda + 16U));
        for (volatile uint32_t d = 0U; d < 500U; d++) { }

        /* SCL high (should already be high) */
        gpio_scl->BSRR = (1UL << pin_scl);
        for (volatile uint32_t d = 0U; d < 500U; d++) { }

        /* SDA high (STOP condition) */
        gpio_sda->BSRR = (1UL << pin_sda);
        for (volatile uint32_t d = 0U; d < 500U; d++) { }
    }

    /* 7. Restore GPIO modes (back to alternate function) */
    gpio_scl->MODER = scl_mode_backup;
    gpio_sda->MODER = sda_mode_backup;

    /* 8. Software reset I2C peripheral */
    I2Cx->CR1 |= I2C_CR1_SWRST_Msk;
    for (volatile uint32_t d = 0U; d < 100U; d++) { }
    I2Cx->CR1 &= ~I2C_CR1_SWRST_Msk;

    /* 9. Re-enable I2C */
    I2C_LL_Enable(I2Cx);

    /* 10. Check if bus is now free */
    if (I2C_LL_IsBusy(I2Cx)) {
        return I2C_STATUS_BUSY;
    }

    return I2C_STATUS_OK;
}

/*============================================================================
 * Utility Functions
 *===========================================================================*/

uint32_t I2C_GetError(const I2C_HandleType *hi2c)
{
    if (hi2c == NULL) return I2C_ERROR_NONE;
    return hi2c->ErrorCode;
}

I2C_StatusType I2C_Abort(I2C_HandleType *hi2c)
{
    if (hi2c == NULL) return I2C_STATUS_ERROR;

    I2C_TypeDef *I2Cx = (I2C_TypeDef *)hi2c->Instance;

    /* Disable all interrupts */
    I2Cx->CR2 &= ~(I2C_CR2_ITEVTEN_Msk | I2C_CR2_ITBUFEN_Msk | I2C_CR2_ITERREN_Msk);

    /* Generate STOP */
    I2C_LL_GenerateStop(I2Cx);

    /* Reset state */
    hi2c->State = I2C_STATE_ABORT;

    /* Wait for bus to be free */
    uint32_t timeout = 10000U;
    while (I2C_LL_IsBusy(I2Cx) && (timeout-- > 0U)) { }

    hi2c->State = I2C_STATE_READY;

    if (hi2c->AbortCpltCallback != NULL) {
        hi2c->AbortCpltCallback(hi2c);
    }

    return I2C_STATUS_OK;
}
