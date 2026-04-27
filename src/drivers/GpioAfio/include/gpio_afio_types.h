/**
 * @file    gpio_afio_types.h
 * @brief   GPIO + AFIO Layer 0 — type & enum definitions
 * Source:  ir/gpio_afio_ir_summary.json v3.0
 */
#ifndef GPIO_AFIO_TYPES_H
#define GPIO_AFIO_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    GPIO_OK          = 0,
    GPIO_PARAM       = 1,  /* RM0008 §9 — invalid port/pin */
    GPIO_LOCKED      = 2,  /* INV_GPIO_001 — pin locked, reset required */
    GPIO_LOCK_ABORT  = 3   /* SEQ_LCKR_LOCK aborted by HW */
} GpioAfio_ReturnType;

typedef enum {
    GPIO_PORT_A = 0, GPIO_PORT_B, GPIO_PORT_C, GPIO_PORT_D,
    GPIO_PORT_E, GPIO_PORT_F, GPIO_PORT_G,
    GPIO_PORT_COUNT
} GpioAfio_Port_e;

/* MODE[1:0] x CNF[1:0] composed (low 4 bits per pin in CRL/CRH) */
typedef enum {
    GPIO_MODE_INPUT_ANALOG       = 0x0U,  /* MODE=00 CNF=00 */
    GPIO_MODE_INPUT_FLOATING     = 0x4U,  /* MODE=00 CNF=01 */
    GPIO_MODE_INPUT_PUPD         = 0x8U,  /* MODE=00 CNF=10 (ODR bit selects pull) */
    GPIO_MODE_OUTPUT_PP_10MHZ    = 0x1U,  /* MODE=01 CNF=00 */
    GPIO_MODE_OUTPUT_PP_2MHZ     = 0x2U,  /* MODE=10 CNF=00 */
    GPIO_MODE_OUTPUT_PP_50MHZ    = 0x3U,  /* MODE=11 CNF=00 */
    GPIO_MODE_OUTPUT_OD_10MHZ    = 0x5U,  /* MODE=01 CNF=01 */
    GPIO_MODE_OUTPUT_OD_2MHZ     = 0x6U,
    GPIO_MODE_OUTPUT_OD_50MHZ    = 0x7U,
    GPIO_MODE_OUTPUT_AFPP_10MHZ  = 0x9U,
    GPIO_MODE_OUTPUT_AFPP_2MHZ   = 0xAU,
    GPIO_MODE_OUTPUT_AFPP_50MHZ  = 0xBU,
    GPIO_MODE_OUTPUT_AFOD_10MHZ  = 0xDU,
    GPIO_MODE_OUTPUT_AFOD_2MHZ   = 0xEU,
    GPIO_MODE_OUTPUT_AFOD_50MHZ  = 0xFU
} GpioAfio_Mode_e;

typedef enum {
    GPIO_PULL_NONE = 0,
    GPIO_PULL_UP   = 1,
    GPIO_PULL_DOWN = 2
} GpioAfio_Pull_e;

typedef struct {
    GpioAfio_Port_e port;
    uint16_t        pin_mask;     /* bitmap of pins (bit n -> pin n) */
    GpioAfio_Mode_e mode;
    GpioAfio_Pull_e pull;         /* only honored when mode == INPUT_PUPD */
} GpioAfio_PinConfigType;

#endif /* GPIO_AFIO_TYPES_H */
