#ifndef I2C_TYPES_H
#define I2C_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file i2c_types.h
 * @brief I2C Driver Type Definitions (V2.0 Extended)
 * @note Supports polling, interrupt, and DMA modes
 */

/* Forward declaration */
struct I2C_Handle;

/*============================================================================
 * Basic Enumerations
 *===========================================================================*/

typedef enum {
    I2C_SPEED_STANDARD = 100000U, /**< 100 kHz Standard Mode */
    I2C_SPEED_FAST     = 400000U  /**< 400 kHz Fast Mode */
} I2C_SpeedType;

typedef enum {
    I2C_DUTYCYCLE_2    = 0U,      /**< Fast mode duty cycle Tlow/Thigh = 2 */
    I2C_DUTYCYCLE_16_9 = 1U       /**< Fast mode duty cycle Tlow/Thigh = 16/9 */
} I2C_DutyCycleType;

typedef enum {
    I2C_STATUS_OK      = 0U,
    I2C_STATUS_BUSY    = 1U,
    I2C_STATUS_ERROR   = 2U,
    I2C_STATUS_TIMEOUT = 3U,
    I2C_STATUS_AF      = 4U,      /**< Acknowledge Failure */
    I2C_STATUS_BERR    = 5U,      /**< Bus Error */
    I2C_STATUS_ARLO    = 6U,      /**< Arbitration Lost */
    I2C_STATUS_OVR     = 7U,      /**< Overrun/Underrun */
    I2C_STATUS_DMA_ERR = 8U       /**< DMA Transfer Error */
} I2C_StatusType;

/*============================================================================
 * State Machine (for interrupt/DMA modes)
 *===========================================================================*/

typedef enum {
    I2C_STATE_RESET     = 0x00U,  /**< Not initialized */
    I2C_STATE_READY     = 0x01U,  /**< Ready for new transfer */
    I2C_STATE_BUSY      = 0x02U,  /**< General busy state */
    I2C_STATE_BUSY_TX   = 0x12U,  /**< Transmitting */
    I2C_STATE_BUSY_RX   = 0x22U,  /**< Receiving */
    I2C_STATE_LISTEN    = 0x04U,  /**< Slave listening */
    I2C_STATE_ABORT     = 0x08U   /**< Abort in progress */
} I2C_StateType;

typedef enum {
    I2C_MODE_NONE   = 0x00U,
    I2C_MODE_MASTER = 0x10U,
    I2C_MODE_SLAVE  = 0x20U,
    I2C_MODE_MEM    = 0x40U       /**< Memory access mode */
} I2C_ModeType;

typedef enum {
    I2C_DIR_WRITE = 0U,
    I2C_DIR_READ  = 1U
} I2C_DirectionType;

typedef enum {
    I2C_MEMADDR_SIZE_8BIT  = 1U,
    I2C_MEMADDR_SIZE_16BIT = 2U
} I2C_MemAddrSizeType;

/*============================================================================
 * Configuration Structure
 *===========================================================================*/

typedef struct {
    uint32_t pclk1_freq;          /**< APB1 Clock Frequency (Hz) */
    I2C_SpeedType clock_speed;    /**< Target I2C SCL Frequency */
    I2C_DutyCycleType duty_cycle; /**< Fast mode duty cycle */
    uint16_t own_address1;        /**< 7-bit or 10-bit address if in Slave mode */
    bool address_10bit;           /**< Is address 10-bit? */
    bool ack_enable;              /**< Enable ACK by default */
} I2C_ConfigType;

/*============================================================================
 * Callback Types
 *===========================================================================*/

typedef void (*I2C_CallbackType)(struct I2C_Handle *hi2c);

/*============================================================================
 * Handle Structure (for interrupt/DMA modes)
 *===========================================================================*/

typedef struct I2C_Handle {
    void              *Instance;      /**< I2C peripheral base address */
    I2C_ConfigType     Config;        /**< Configuration parameters */
    volatile I2C_StateType State;     /**< Current state */
    volatile I2C_ModeType  Mode;      /**< Current mode */
    volatile uint32_t  ErrorCode;     /**< Error code bitmap */

    /* Transfer context */
    uint8_t           *pBuffPtr;      /**< Data buffer pointer */
    volatile uint16_t  XferSize;      /**< Transfer size */
    volatile uint16_t  XferCount;     /**< Remaining bytes */
    uint16_t           DevAddress;    /**< Target device address */
    uint32_t           MemAddress;    /**< Memory address (for Mem_Read/Write) */
    uint8_t            MemAddrSize;   /**< Memory address size (1 or 2 bytes) */

    /* Callbacks */
    I2C_CallbackType   TxCpltCallback;     /**< TX complete callback */
    I2C_CallbackType   RxCpltCallback;     /**< RX complete callback */
    I2C_CallbackType   MemTxCpltCallback;  /**< Memory TX complete callback */
    I2C_CallbackType   MemRxCpltCallback;  /**< Memory RX complete callback */
    I2C_CallbackType   ErrorCallback;      /**< Error callback */
    I2C_CallbackType   AbortCpltCallback;  /**< Abort complete callback */

    /* DMA handles (optional, void* for flexibility) */
    void              *hdmatx;        /**< DMA TX handle */
    void              *hdmarx;        /**< DMA RX handle */
} I2C_HandleType;

/*============================================================================
 * Error Code Bitmap
 *===========================================================================*/

#define I2C_ERROR_NONE      (0x00000000U)
#define I2C_ERROR_BERR      (0x00000001U)  /**< Bus error */
#define I2C_ERROR_ARLO      (0x00000002U)  /**< Arbitration lost */
#define I2C_ERROR_AF        (0x00000004U)  /**< ACK failure */
#define I2C_ERROR_OVR       (0x00000008U)  /**< Overrun/Underrun */
#define I2C_ERROR_DMA       (0x00000010U)  /**< DMA transfer error */
#define I2C_ERROR_TIMEOUT   (0x00000020U)  /**< Timeout */

/*============================================================================
 * GPIO Config for Bus Recovery
 *===========================================================================*/

typedef struct {
    void    *gpio_port_scl;   /**< GPIO port for SCL (e.g., GPIOB) */
    uint16_t gpio_pin_scl;    /**< GPIO pin number for SCL */
    void    *gpio_port_sda;   /**< GPIO port for SDA */
    uint16_t gpio_pin_sda;    /**< GPIO pin number for SDA */
} I2C_GpioConfigType;

#endif /* I2C_TYPES_H */
