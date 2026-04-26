/**
 * @file    spi_ll.h
 * @brief   STM32F103C8T6 SPI 硬件抽象底端层 (Low-Level)
 *
 * @note    原子级硬件操作封装。W1C/RC_SEQ 等副作用在此层完全封装。
 *          LL 层不含 while 等待循环（等待归 driver 层）。
 *          non-atomic RMW 操作已标注"调用方须保证临界区"。
 *
 * Source:  RM0008 Rev 21, §25.5
 *          ir/spi_ir_summary.json v3.0
 */

#ifndef SPI_LL_H
#define SPI_LL_H

#include "spi_reg.h"
#include "spi_types.h"
#include <stddef.h>

/*===========================================================================*/
/* Instance mapping helper                                                    */
/*===========================================================================*/

/**
 * @brief  Get SPI_TypeDef pointer from instance enum.
 * @param  inst  SPI_INSTANCE_1 or SPI_INSTANCE_2
 * @return Peripheral pointer (NULL if invalid — caller must check).
 */
static inline SPI_TypeDef* SPI_LL_GetInstance(Spi_Instance_e inst)
{
    SPI_TypeDef *p = NULL;
    if (inst == SPI_INSTANCE_1)
    {
        p = SPI1;
    }
    else if (inst == SPI_INSTANCE_2)
    {
        p = SPI2;
    }
    else
    {
        /* invalid */
    }
    return p;
}

/*===========================================================================*/
/* CR1 — Enable / Disable / Configure                                        */
/*===========================================================================*/

/**
 * @brief  Enable SPI (set SPE=1). Must be last step in init sequence.
 *         Guard: INV_SPI_002 — after this, BR/CPOL/CPHA are locked.
 *         (non-atomic RMW — caller must ensure critical section)
 * Source: ir/spi_ir_summary.json registers[0] CR1.SPE
 */
static inline void SPI_LL_Enable(SPI_TypeDef *SPIx)
{
    /* Guard: INV_SPI_003 — SPE cannot be changed if BSY=1 */
    if ((SPIx->SR & SPI_SR_BSY_Msk) != 0U) return;
    SPIx->CR1 |= SPI_CR1_SPE_Msk;   /* IR: init_sequence step 5 — RM0008 §25.5.1 */
}

/**
 * @brief  Disable SPI (clear SPE).
 *         Guard: INV_SPI_003 — caller (driver) must ensure BSY=0 first.
 *         (non-atomic RMW — caller must ensure critical section)
 * Source: ir/spi_ir_summary.json invariants[2] INV_SPI_003
 */
static inline void SPI_LL_Disable(SPI_TypeDef *SPIx)
{
    /* Guard: INV_SPI_003 — SPE cannot be changed if BSY=1 */
    if ((SPIx->SR & SPI_SR_BSY_Msk) != 0U) return;
    SPIx->CR1 &= (uint16_t)~SPI_CR1_SPE_Msk;   /* IR: invariants[2] — RM0008 §25.3.8 */
}

/**
 * @brief  Check if SPI is enabled (SPE=1).
 */
static inline bool SPI_LL_IsEnabled(const SPI_TypeDef *SPIx)
{
    return ((SPIx->CR1 & SPI_CR1_SPE_Msk) != 0U);
}

/**
 * @brief  Read CR2 register.
 */
static inline uint16_t SPI_LL_ReadCR2(const SPI_TypeDef *SPIx)
{
    return SPIx->CR2;
}

/**
 * @brief  Configure CR1 register (SPE cleared first per INV_SPI_002).
 *         This function clears SPE before writing to ensure BR/CPOL/CPHA
 *         are writable. Caller should re-enable SPE after.
 *         (non-atomic RMW — caller must ensure critical section)
 *
 * @param  cr1_val  Full 16-bit CR1 value (SPE bit will be masked off).
 * Source: ir/spi_ir_summary.json invariants[1] INV_SPI_002
 */
static inline void SPI_LL_WriteCR1(SPI_TypeDef *SPIx, uint16_t cr1_val)
{
    /* Guard: INV_SPI_002, INV_SPI_004 — SPE must be 0 to write BR/CPOL/CPHA/DFF/CRCEN */
    if ((SPIx->CR1 & SPI_CR1_SPE_Msk) != 0U) return;
    /* Guard: INV_SPI_003 — SPE cannot be changed if BSY=1 */
    if ((SPIx->SR & SPI_SR_BSY_Msk) != 0U) return;

    SPIx->CR1 = cr1_val & (uint16_t)~SPI_CR1_SPE_Msk;   /* RM0008 §25.5.1 */
}

/**
 * @brief  Compose CR1 value from configuration.
 *         (Implemented in spi_ll.c per Guidelines §2)
 */
uint16_t SPI_LL_ComposeCR1(const Spi_ConfigType *pConfig);

/**
 * @brief  Compose CR2 value from configuration.
 *         (Implemented in spi_ll.c per Guidelines §2)
 */
uint16_t SPI_LL_ComposeCR2(const Spi_ConfigType *pConfig);

/**
 * @brief  Configure CR2 register.
 *         (non-atomic write — caller must ensure critical section)
 * Source: ir/spi_ir_summary.json registers[1]
 */
static inline void SPI_LL_WriteCR2(SPI_TypeDef *SPIx, uint16_t cr2_val)
{
    SPIx->CR2 = cr2_val;   /* RM0008 §25.5.2 */
}

/**
 * @brief  Set master mode (MSTR=1).
 */
static inline void SPI_LL_SetMaster(SPI_TypeDef *SPIx)
{
    SPIx->CR1 |= SPI_CR1_MSTR_Msk;
}

/*===========================================================================*/
/* SR — Status queries (all read-only, naturally atomic)                      */
/*===========================================================================*/

static inline bool SPI_LL_IsBusy(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_BSY_Msk) != 0U);   /* RM0008 §25.5.3 */
}

static inline bool SPI_LL_IsTxEmpty(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_TXE_Msk) != 0U);   /* RM0008 §25.5.3 */
}

static inline bool SPI_LL_IsRxNotEmpty(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_RXNE_Msk) != 0U);  /* RM0008 §25.5.3 */
}

static inline bool SPI_LL_IsOverrun(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_OVR_Msk) != 0U);   /* RM0008 §25.5.3 */
}

static inline bool SPI_LL_IsModeFault(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_MODF_Msk) != 0U);  /* RM0008 §25.5.3 */
}

static inline bool SPI_LL_IsCrcError(const SPI_TypeDef *SPIx)
{
    return ((SPIx->SR & SPI_SR_CRCERR_Msk) != 0U);
}

static inline uint16_t SPI_LL_ReadSR(const SPI_TypeDef *SPIx)
{
    return SPIx->SR;
}

/*===========================================================================*/
/* DR — Data register                                                         */
/* Source: ir/spi_ir_summary.json registers[3]                               */
/*===========================================================================*/

static inline void SPI_LL_WriteData(SPI_TypeDef *SPIx, uint16_t data)
{
    SPIx->DR = data;   /* RM0008 §25.5.4 */
}

static inline uint16_t SPI_LL_ReadData(const SPI_TypeDef *SPIx)
{
    return SPIx->DR;   /* RM0008 §25.5.4 — also clears RXNE */
}

/*===========================================================================*/
/* CRC                                                                        */
/* Source: ir/spi_ir_summary.json registers[4..6]                            */
/*===========================================================================*/

static inline void SPI_LL_SetCrcPolynomial(SPI_TypeDef *SPIx, uint16_t poly)
{
    SPIx->CRCPR = poly;   /* RM0008 §25.5.5 */
}

static inline uint16_t SPI_LL_GetRxCrc(const SPI_TypeDef *SPIx)
{
    return SPIx->RXCRCR;   /* RM0008 §25.5.6 */
}

static inline uint16_t SPI_LL_GetTxCrc(const SPI_TypeDef *SPIx)
{
    return SPIx->TXCRCR;   /* RM0008 §25.5.7 */
}

/*===========================================================================*/
/* Error flag clear sequences                                                 */
/* Source: ir/spi_ir_summary.json atomic_sequences[]                         */
/*===========================================================================*/

/**
 * @brief  Clear OVR flag (RC_SEQ: read DR then read SR).
 * Source: ir/spi_ir_summary.json atomic_sequences[0] SEQ_OVR_CLEAR
 *         RM0008 §25.3.10 p.693
 */
static inline void SPI_LL_ClearOVR(SPI_TypeDef *SPIx)
{
    (void)SPIx->DR;    /* Step 1: read DR — SEQ_OVR_CLEAR */
    (void)SPIx->SR;    /* Step 2: read SR — SEQ_OVR_CLEAR */
}

/**
 * @brief  Clear MODF flag (RC_SEQ: read SR then write CR1).
 * Source: ir/spi_ir_summary.json atomic_sequences[1] SEQ_MODF_CLEAR
 *         RM0008 §25.3.10 p.693
 */
static inline void SPI_LL_ClearMODF(SPI_TypeDef *SPIx)
{
    (void)SPIx->SR;                  /* Step 1: read SR — SEQ_MODF_CLEAR */
    SPIx->CR1 = SPIx->CR1;          /* Step 2: write CR1 unchanged — SEQ_MODF_CLEAR */
}

/**
 * @brief  Clear CRCERR flag (W0C: write 0 to CRCERR bit).
 *         W0C — direct assignment, NOT |= (R8#1).
 * Source: ir/spi_ir_summary.json registers[2] SR.CRCERR
 *         RM0008 §25.5.3 p.717
 */
static inline void SPI_LL_ClearCRCERR(SPI_TypeDef *SPIx)
{
    /* W0C: write 0 to CRCERR, preserve other bits by writing 1s */
    /* SR is mostly RO so writing 1 to RO bits has no effect      */
    SPIx->SR = (uint16_t)~SPI_SR_CRCERR_Msk;   /* RM0008 §25.5.3 */
}

/*===========================================================================*/
/* CR2 — Interrupt enable/disable                                             */
/* Source: ir/spi_ir_summary.json registers[1]                               */
/*===========================================================================*/

static inline void SPI_LL_EnableIT_TXE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= SPI_CR2_TXEIE_Msk;
}

static inline void SPI_LL_DisableIT_TXE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= (uint16_t)~SPI_CR2_TXEIE_Msk;
}

static inline void SPI_LL_EnableIT_RXNE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= SPI_CR2_RXNEIE_Msk;
}

static inline void SPI_LL_DisableIT_RXNE(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= (uint16_t)~SPI_CR2_RXNEIE_Msk;
}

static inline void SPI_LL_EnableIT_ERR(SPI_TypeDef *SPIx)
{
    SPIx->CR2 |= SPI_CR2_ERRIE_Msk;
}

static inline void SPI_LL_DisableIT_ERR(SPI_TypeDef *SPIx)
{
    SPIx->CR2 &= (uint16_t)~SPI_CR2_ERRIE_Msk;
}

/*===========================================================================*/
/* I2SCFGR — Ensure SPI mode (not I2S)                                       */
/* Source: ir/spi_ir_summary.json registers[7]                               */
/*===========================================================================*/

/**
 * @brief  Force SPI mode by clearing I2SMOD bit.
 */
static inline void SPI_LL_ForceSpiMode(SPI_TypeDef *SPIx)
{
    SPIx->I2SCFGR &= (uint16_t)~SPI_I2SCFGR_I2SMOD_Msk;   /* RM0008 §25.5.8 */
}

#endif /* SPI_LL_H */
