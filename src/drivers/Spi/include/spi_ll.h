/**
 * @file    spi_ll.h
 * @brief   STM32F103C8T6 SPI 硬件抽象底端层 (Low-Level)
 *
 * @note    提供原子级硬件操作的 static inline 函数。
 *          所有寄存器副作用（W1C、W0C、RC_SEQ）完全封装在此层。
 *          禁止 include spi_drv.h 或 spi_api.h（规则 R8-8）。
 *
 * Source:  RM0008 Rev 14, §25.5 SPI and I2S registers (p.714-720)
 */

#ifndef SPI_LL_H
#define SPI_LL_H

#include "spi_reg.h"
#include "spi_types.h"
#include "spi_cfg.h"

#include <stddef.h>   /* NULL */
#include <stdbool.h>

/*===========================================================================*/
/* Non-inline composite init helpers (implemented in spi_ll.c)               */
/*===========================================================================*/

/**
 * @brief Compose and write CR1 from a Spi_ConfigType (SPE=0, Guard: INV_SPI_002).
 * @param[in] SPIx     Pointer to SPI peripheral.
 * @param[in] pConfig  Configuration structure. Must not be NULL.
 */
void SPI_LL_ComposeCR1(SPI_TypeDef *SPIx, const Spi_ConfigType *pConfig);

/**
 * @brief Configure CR2 from a Spi_ConfigType (NSS output, interrupts/DMA off).
 * @param[in] SPIx     Pointer to SPI peripheral.
 * @param[in] pConfig  Configuration structure. Must not be NULL.
 */
void SPI_LL_ComposeCR2(SPI_TypeDef *SPIx, const Spi_ConfigType *pConfig);

/*===========================================================================*/
/* RCC clock enable / disable                                                 */
/* Source: RM0008 §7.3.7 p.109, §7.3.8 p.111                               */
/*===========================================================================*/

/**
 * @brief Enable clock for the given SPI instance via RCC.
 * @param[in] SPIx  Pointer to SPI peripheral (SPI1 or SPI2).
 */
static inline void SPI_LL_EnableClock(SPI_TypeDef *SPIx)
{
    if (SPIx == SPI1)
    {
        /* SPI1 on APB2, bit 12 — RM0008 §7.3.7 p.109 */
        RCC_APB2ENR |= RCC_APB2ENR_SPI1EN_Msk;
    }
    else if (SPIx == SPI2)
    {
        /* SPI2 on APB1, bit 14 — RM0008 §7.3.8 p.111 */
        RCC_APB1ENR |= RCC_APB1ENR_SPI2EN_Msk;
    }
    else
    {
        /* Invalid instance — do nothing */
    }
}

/**
 * @brief Disable clock for the given SPI instance via RCC.
 * @param[in] SPIx  Pointer to SPI peripheral (SPI1 or SPI2).
 */
static inline void SPI_LL_DisableClock(SPI_TypeDef *SPIx)
{
    if (SPIx == SPI1)
    {
        RCC_APB2ENR &= ~RCC_APB2ENR_SPI1EN_Msk; /* RM0008 §7.3.7 p.109 */
    }
    else if (SPIx == SPI2)
    {
        RCC_APB1ENR &= ~RCC_APB1ENR_SPI2EN_Msk; /* RM0008 §7.3.8 p.111 */
    }
    else
    {
        /* Invalid instance — do nothing */
    }
}

/*===========================================================================*/
/* CR1 — SPI enable / disable                                                 */
/* Source: RM0008 §25.5.1 p.715 CR1.SPE                                     */
/*===========================================================================*/

/**
 * @brief Enable the SPI peripheral (SPE=1).
 *        After this call BR/CPOL/CPHA/DFF/MSTR/CRCEN are LOCKED (INV_SPI_002).
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_Enable(SPI_TypeDef *SPIx)
{
    SPIx->CR1 |= SPI_CR1_SPE_Msk; /* RM0008 §25.5.1 p.715 */
}

/**
 * @brief Disable the SPI peripheral (SPE=0).
 *        Caller MUST ensure TXE=1, BSY=0 before calling (INV_SPI_003).
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_Disable(SPI_TypeDef *SPIx)
{
    SPIx->CR1 &= ~SPI_CR1_SPE_Msk; /* RM0008 §25.5.1 p.715 */
}

/**
 * @brief Check whether SPE is set.
 * @param[in] SPIx  Pointer to SPI peripheral.
 * @return true if SPE=1.
 */
static inline bool SPI_LL_IsEnabled(const SPI_TypeDef *SPIx)
{
    return ((SPIx->CR1 & SPI_CR1_SPE_Msk) != 0U);
}

/*===========================================================================*/
/* CR1 — configuration (all must be set while SPE=0, Guard: INV_SPI_002)    */
/* Source: RM0008 §25.5.1 p.714-715                                          */
/*===========================================================================*/

/**
 * @brief Write full CR1 configuration word.
 *        Guard: INV_SPI_002 — SPE must be 0 before calling.
 * @param[in] SPIx   Pointer to SPI peripheral.
 * @param[in] cr1Val Composed CR1 value (without SPE bit set).
 */
static inline void SPI_LL_WriteCR1Config(SPI_TypeDef *SPIx, uint32_t cr1Val)
{
    /* Guard: INV_SPI_002 — caller must ensure SPE=0 */
    SPIx->CR1 = cr1Val; /* RM0008 §25.5.1 p.714 */
}

/**
 * @brief Set baud rate prescaler BR[2:0].
 *        Guard: INV_SPI_002 — SPE must be 0.
 * @param[in] SPIx    Pointer to SPI peripheral.
 * @param[in] baudDiv Baud rate divider (0..7).
 */
static inline void SPI_LL_SetBaudRate(SPI_TypeDef *SPIx, uint32_t baudDiv)
{
    uint32_t cr1 = SPIx->CR1;
    cr1 &= ~SPI_CR1_BR_Msk;
    cr1 |= ((baudDiv << SPI_CR1_BR_Pos) & SPI_CR1_BR_Msk); /* RM0008 §25.5.1 p.715 */
    SPIx->CR1 = cr1;
}

/**
 * @brief Set clock polarity (CPOL) and phase (CPHA).
 *        Guard: INV_SPI_002 — SPE must be 0.
 * @param[in] SPIx  Pointer to SPI peripheral.
 * @param[in] cpol  Clock polarity (0 or 1).
 * @param[in] cpha  Clock phase (0 or 1).
 */
static inline void SPI_LL_SetClockMode(SPI_TypeDef *SPIx, uint32_t cpol, uint32_t cpha)
{
    uint32_t cr1 = SPIx->CR1;
    cr1 &= ~(SPI_CR1_CPOL_Msk | SPI_CR1_CPHA_Msk);
    if (cpol != 0U)
    {
        cr1 |= SPI_CR1_CPOL_Msk; /* RM0008 §25.5.1 p.715 */
    }
    if (cpha != 0U)
    {
        cr1 |= SPI_CR1_CPHA_Msk; /* RM0008 §25.5.1 p.715 */
    }
    SPIx->CR1 = cr1;
}

/**
 * @brief Set data frame format DFF.
 *        Guard: INV_SPI_002 — SPE must be 0.
 * @param[in] SPIx  Pointer to SPI peripheral.
 * @param[in] dff   0=8-bit, 1=16-bit.
 */
static inline void SPI_LL_SetDataFrameFormat(SPI_TypeDef *SPIx, uint32_t dff)
{
    uint32_t cr1 = SPIx->CR1;
    cr1 &= ~SPI_CR1_DFF_Msk;
    if (dff != 0U)
    {
        cr1 |= SPI_CR1_DFF_Msk; /* RM0008 §25.5.1 p.714 */
    }
    SPIx->CR1 = cr1;
}

/**
 * @brief Set frame bit order (LSBFIRST).
 *        Guard: INV_SPI_002 — SPE must be 0.
 * @param[in] SPIx      Pointer to SPI peripheral.
 * @param[in] lsbFirst  0=MSB first, 1=LSB first.
 */
static inline void SPI_LL_SetFirstBit(SPI_TypeDef *SPIx, uint32_t lsbFirst)
{
    uint32_t cr1 = SPIx->CR1;
    cr1 &= ~SPI_CR1_LSBFIRST_Msk;
    if (lsbFirst != 0U)
    {
        cr1 |= SPI_CR1_LSBFIRST_Msk; /* RM0008 §25.5.1 p.715 */
    }
    SPIx->CR1 = cr1;
}

/**
 * @brief Configure master mode and software NSS (SSM=1, SSI=1, MSTR=1).
 *        INV_SPI_001: when MSTR=1 and SSM=1, SSI must be 1 to prevent MODF.
 *        Guard: INV_SPI_002 — SPE must be 0.
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_SetMasterModeSoftNSS(SPI_TypeDef *SPIx)
{
    uint32_t cr1 = SPIx->CR1;
    /* Guard: INV_SPI_002 */
    /* INV_SPI_001: SSM=1, SSI=1 required when MSTR=1 */
    cr1 |= (SPI_CR1_MSTR_Msk | SPI_CR1_SSM_Msk | SPI_CR1_SSI_Msk); /* RM0008 §25.3.1 p.676 */
    SPIx->CR1 = cr1;
}

/**
 * @brief Configure slave mode.
 *        Guard: INV_SPI_002 — SPE must be 0.
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_SetSlaveMode(SPI_TypeDef *SPIx)
{
    uint32_t cr1 = SPIx->CR1;
    cr1 &= ~(SPI_CR1_MSTR_Msk | SPI_CR1_SSM_Msk | SPI_CR1_SSI_Msk); /* RM0008 §25.5.1 p.715 */
    SPIx->CR1 = cr1;
}

/**
 * @brief Enable hardware CRC calculation.
 *        Guard: INV_SPI_002 — SPE must be 0.
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_EnableCRC(SPI_TypeDef *SPIx)
{
    SPIx->CR1 |= SPI_CR1_CRCEN_Msk; /* RM0008 §25.5.1 p.714 */
}

/**
 * @brief Disable hardware CRC calculation.
 *        Guard: INV_SPI_002 — SPE must be 0.
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_DisableCRC(SPI_TypeDef *SPIx)
{
    SPIx->CR1 &= ~SPI_CR1_CRCEN_Msk; /* RM0008 §25.5.1 p.714 */
}

/**
 * @brief Set CRC polynomial register.
 *        Guard: INV_SPI_002 — SPE must be 0.
 * @param[in] SPIx  Pointer to SPI peripheral.
 * @param[in] poly  CRC polynomial value (default 0x0007).
 */
static inline void SPI_LL_SetCRCPolynomial(SPI_TypeDef *SPIx, uint16_t poly)
{
    SPIx->CRCPR = (uint32_t)poly; /* RM0008 §25.5.5 p.718 */
}

/*===========================================================================*/
/* CR1 — communication mode (BIDIMODE, BIDIOE, RXONLY)                      */
/* Source: RM0008 §25.5.1 p.714-715, IR operation_modes[]                   */
/*===========================================================================*/

/**
 * @brief Set communication mode (BIDIMODE, BIDIOE, RXONLY).
 *        Guard: INV_SPI_002 — SPE must be 0.
 * @param[in] SPIx     Pointer to SPI peripheral.
 * @param[in] commMode Communication mode enum value.
 */
static inline void SPI_LL_SetCommMode(SPI_TypeDef *SPIx, Spi_CommMode_e commMode)
{
    uint32_t cr1 = SPIx->CR1;
    cr1 &= ~(SPI_CR1_BIDIMODE_Msk | SPI_CR1_BIDIOE_Msk | SPI_CR1_RXONLY_Msk);

    switch (commMode)
    {
        case SPI_COMM_FULL_DUPLEX:
            /* BIDIMODE=0, RXONLY=0 — default, no bits to set */
            break;
        case SPI_COMM_RECEIVE_ONLY:
            cr1 |= SPI_CR1_RXONLY_Msk;   /* RM0008 §25.5.1 p.715 */
            break;
        case SPI_COMM_BIDI_TX:
            cr1 |= (SPI_CR1_BIDIMODE_Msk | SPI_CR1_BIDIOE_Msk); /* RM0008 §25.5.1 p.714 */
            break;
        case SPI_COMM_BIDI_RX:
            cr1 |= SPI_CR1_BIDIMODE_Msk; /* BIDIOE=0 for RX — RM0008 §25.5.1 p.714 */
            break;
        default:
            /* Invalid — leave as full duplex */
            break;
    }

    SPIx->CR1 = cr1;
}

/*===========================================================================*/
/* AFIO — SPI1 pin remap                                                     */
/* Source: RM0008 §9.3.10 p.181; CONSTRAINT_GPIO_SPI_ORDER                  */
/*===========================================================================*/

/**
 * @brief Enable or disable SPI1 pin remapping via AFIO_MAPR.
 *        CONSTRAINT_GPIO_SPI_ORDER: must be called BEFORE GPIO config.
 *        Only applicable to SPI1; SPI2 has no remap option.
 * @param[in] enableRemap  true = remap (PA15,PB3-5), false = no remap (PA4-7).
 */
static inline void SPI_LL_SetSPI1Remap(bool enableRemap)
{
    if (enableRemap)
    {
        /* Enable AFIO clock first — RM0008 §7.3.7 p.111 */
        RCC_APB2ENR |= RCC_APB2ENR_AFIOEN_Msk;
        AFIO_MAPR |= AFIO_MAPR_SPI1_REMAP_Msk;  /* RM0008 §9.3.10 p.181 */
    }
    else
    {
        AFIO_MAPR &= ~AFIO_MAPR_SPI1_REMAP_Msk; /* RM0008 §9.3.10 p.181 */
    }
}

/*===========================================================================*/
/* CR2 — interrupt and DMA enables                                           */
/* Source: RM0008 §25.5.2 p.716                                              */
/*===========================================================================*/

/**
 * @brief Enable TXE interrupt (TXEIE=1).
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_EnableTXEIE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= SPI_CR2_TXEIE_Msk; /* RM0008 §25.5.2 p.716 */
}

/**
 * @brief Disable TXE interrupt (TXEIE=0).
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_DisableTXEIE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= ~SPI_CR2_TXEIE_Msk; /* RM0008 §25.5.2 p.716 */
}

/**
 * @brief Enable RXNE interrupt (RXNEIE=1).
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_EnableRXNEIE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= SPI_CR2_RXNEIE_Msk; /* RM0008 §25.5.2 p.716 */
}

/**
 * @brief Disable RXNE interrupt (RXNEIE=0).
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_DisableRXNEIE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= ~SPI_CR2_RXNEIE_Msk; /* RM0008 §25.5.2 p.716 */
}

/**
 * @brief Enable error interrupt (ERRIE=1) — covers MODF, OVR, CRCERR.
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_EnableERRIE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= SPI_CR2_ERRIE_Msk; /* RM0008 §25.5.2 p.716 */
}

/**
 * @brief Disable error interrupt (ERRIE=0).
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_DisableERRIE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= ~SPI_CR2_ERRIE_Msk; /* RM0008 §25.5.2 p.716 */
}

/**
 * @brief Enable hardware NSS output (SSOE=1, master mode only).
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_EnableSSOE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= SPI_CR2_SSOE_Msk; /* RM0008 §25.5.2 p.716 */
}

/**
 * @brief Disable hardware NSS output (SSOE=0).
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_DisableSSOE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= ~SPI_CR2_SSOE_Msk; /* RM0008 §25.5.2 p.716 */
}

/*===========================================================================*/
/* SR — status flag reads (read-only, no side effects)                       */
/* Source: RM0008 §25.5.3 p.717                                              */
/*===========================================================================*/

/** @brief Return true if TXE=1 (transmit buffer empty). */
static inline bool SPI_LL_IsTXE(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_TXE_Msk) != 0U); /* RM0008 §25.5.3 p.717 */
}

/** @brief Return true if RXNE=1 (receive buffer not empty). */
static inline bool SPI_LL_IsRXNE(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_RXNE_Msk) != 0U); /* RM0008 §25.5.3 p.717 */
}

/** @brief Return true if BSY=1 (SPI busy). */
static inline bool SPI_LL_IsBusy(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_BSY_Msk) != 0U); /* RM0008 §25.5.3 p.717 */
}

/** @brief Return true if OVR=1 (overrun detected). */
static inline bool SPI_LL_IsOVR(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_OVR_Msk) != 0U); /* RM0008 §25.5.3 p.717 */
}

/** @brief Return true if MODF=1 (mode fault detected). */
static inline bool SPI_LL_IsMODF(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_MODF_Msk) != 0U); /* RM0008 §25.5.3 p.717 */
}

/** @brief Return true if CRCERR=1 (CRC mismatch). */
static inline bool SPI_LL_IsCRCERR(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_CRCERR_Msk) != 0U); /* RM0008 §25.5.3 p.717 */
}

/** @brief Read raw SR value (used in atomic clear sequences). */
static inline uint32_t SPI_LL_ReadSR(const SPI_TypeDef *SPIx)
{
    return SPIx->SR; /* RM0008 §25.5.3 p.717 */
}

/** @brief Read raw CR1 value (used by driver layer for MODF clear sequence). */
static inline uint32_t SPI_LL_ReadCR1(const SPI_TypeDef *SPIx)
{
    return SPIx->CR1; /* RM0008 §25.5.1 p.714 */
}

/*===========================================================================*/
/* SR — flag clear sequences                                                  */
/*===========================================================================*/

/**
 * @brief Clear CRCERR flag (W0C: write 0 to bit 4).
 *        Safe to use &= because CRCERR is the only writable-to-clear bit;
 *        other SR bits are RO or RC_SEQ and not affected by a zero write.
 *        Source: RM0008 §25.5.3 p.717; error_handling note in IR
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_ClearCRCERR(SPI_TypeDef *SPIx)
{
    SPIx->SR &= ~SPI_SR_CRCERR_Msk; /* W0C — RM0008 §25.5.3 p.717 */
}

/**
 * @brief Clear OVR flag via hardware-mandated read sequence (RC_SEQ).
 *        Step 1: Read DR (discards overrun data).
 *        Step 2: Read SR (completes clear sequence).
 *        Steps must not be swapped. No other SPI operations between steps.
 *        Source: RM0008 §25.3.10 p.694 (SEQ_OVR_CLEAR)
 * @param[in] SPIx  Pointer to SPI peripheral.
 */
static inline void SPI_LL_ClearOVR(SPI_TypeDef *SPIx)
{
    volatile uint32_t dummy;
    dummy = SPIx->DR;  /* Step 1: read DR — RM0008 §25.3.10 p.694 */
    dummy = SPIx->SR;  /* Step 2: read SR — RM0008 §25.3.10 p.694 */
    (void)dummy;
}

/**
 * @brief Clear MODF flag via hardware-mandated read/write sequence (RC_SEQ).
 *        Step 1: Read SR while MODF=1.
 *        Step 2: Write CR1 (restore SPE=1, MSTR=1 as appropriate).
 *        Source: RM0008 §25.3.10 p.693 (SEQ_MODF_CLEAR)
 * @param[in] SPIx    Pointer to SPI peripheral.
 * @param[in] cr1Val  CR1 value to write in step 2 (restore MSTR/SPE).
 */
static inline void SPI_LL_ClearMODF(SPI_TypeDef *SPIx, uint32_t cr1Val)
{
    volatile uint32_t dummy;
    dummy = SPIx->SR;     /* Step 1: read SR — RM0008 §25.3.10 p.693 */
    (void)dummy;
    SPIx->CR1 = cr1Val;   /* Step 2: write CR1 — RM0008 §25.3.10 p.693 */
}

/*===========================================================================*/
/* DR — data read / write                                                     */
/* Source: RM0008 §25.5.4 p.718                                              */
/*===========================================================================*/

/**
 * @brief Write one byte (8-bit frame) to the TX buffer.
 *        Caller must verify TXE=1 before calling.
 * @param[in] SPIx  Pointer to SPI peripheral.
 * @param[in] data  Byte to transmit.
 */
static inline void SPI_LL_WriteDR8(SPI_TypeDef *SPIx, uint8_t data)
{
    SPIx->DR = (uint32_t)data; /* RM0008 §25.5.4 p.718 */
}

/**
 * @brief Write one 16-bit word to the TX buffer.
 *        Caller must verify TXE=1 and DFF=1 before calling.
 * @param[in] SPIx  Pointer to SPI peripheral.
 * @param[in] data  16-bit word to transmit.
 */
static inline void SPI_LL_WriteDR16(SPI_TypeDef *SPIx, uint16_t data)
{
    SPIx->DR = (uint32_t)data; /* RM0008 §25.5.4 p.718 */
}

/**
 * @brief Read one byte (8-bit frame) from the RX buffer.
 *        Caller must verify RXNE=1 before calling.
 * @param[in] SPIx  Pointer to SPI peripheral.
 * @return  Received byte.
 */
static inline uint8_t SPI_LL_ReadDR8(const SPI_TypeDef *SPIx)
{
    return (uint8_t)(SPIx->DR & 0xFFU); /* RM0008 §25.5.4 p.718 */
}

/**
 * @brief Read one 16-bit word from the RX buffer.
 *        Caller must verify RXNE=1 and DFF=1 before calling.
 * @param[in] SPIx  Pointer to SPI peripheral.
 * @return  Received 16-bit word.
 */
static inline uint16_t SPI_LL_ReadDR16(const SPI_TypeDef *SPIx)
{
    return (uint16_t)(SPIx->DR & 0xFFFFU); /* RM0008 §25.5.4 p.718 */
}

/*===========================================================================*/
/* Utility: resolve instance index to TypeDef pointer                        */
/*===========================================================================*/

/**
 * @brief Map a Spi_InstanceIndex_e to its SPI_TypeDef pointer.
 * @param[in] idx  Instance index (SPI_INSTANCE_1 or SPI_INSTANCE_2).
 * @return Pointer to SPI_TypeDef, or NULL for invalid index.
 */
static inline SPI_TypeDef *SPI_LL_GetInstance(Spi_InstanceIndex_e idx)
{
    SPI_TypeDef *pResult = NULL;

    if (idx == SPI_INSTANCE_1)
    {
        pResult = SPI1;
    }
    else if (idx == SPI_INSTANCE_2)
    {
        pResult = SPI2;
    }
    else
    {
        pResult = NULL;
    }

    return pResult;
}

#endif /* SPI_LL_H */
