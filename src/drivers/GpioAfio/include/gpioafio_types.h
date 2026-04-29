/**
 * @file    gpioafio_types.h
 * @brief   GPIO + AFIO Layer 0 — type & enum definitions
 * Source:  ir/gpioafio_ir_summary.json v3.0
 */
#ifndef GPIO_AFIO_TYPES_H
#define GPIO_AFIO_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    GPIO_OK          = 0,  /* success */
    GPIO_PARAM       = 1,  /* invalid port/pin/NULL config (RM0008 §9) */
    GPIO_LOCKED      = 2,  /* bank locked by LCKR; reset required (INV_GPIO_001) */
    GPIO_LOCK_ABORT  = 3   /* SEQ_LCKR_LOCK sequence aborted by HW */
} GpioAfio_ReturnType;

typedef enum {
    GPIO_PORT_A = 0,    /* GPIOA @ 0x40010800 */
    GPIO_PORT_B,        /* GPIOB @ 0x40010C00 */
    GPIO_PORT_C,        /* GPIOC @ 0x40011000 (LQFP48: PC13..15 only) */
    GPIO_PORT_D,        /* GPIOD @ 0x40011400 (LQFP48: PD0,PD1 only) */
    GPIO_PORT_E,        /* GPIOE @ 0x40011800 (LQFP48: 不引出) */
    GPIO_PORT_F,        /* GPIOF @ 0x40011C00 (LQFP48: 不引出) */
    GPIO_PORT_G,        /* GPIOG @ 0x40012000 (LQFP48: 不引出) */
    GPIO_PORT_COUNT     /* sentinel — must be last */
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
    GPIO_MODE_OUTPUT_OD_2MHZ     = 0x6U,  /* MODE=10 CNF=01 */
    GPIO_MODE_OUTPUT_OD_50MHZ    = 0x7U,  /* MODE=11 CNF=01 */
    GPIO_MODE_OUTPUT_AFPP_10MHZ  = 0x9U,  /* MODE=01 CNF=10 (alternate function PP) */
    GPIO_MODE_OUTPUT_AFPP_2MHZ   = 0xAU,  /* MODE=10 CNF=10 */
    GPIO_MODE_OUTPUT_AFPP_50MHZ  = 0xBU,  /* MODE=11 CNF=10 */
    GPIO_MODE_OUTPUT_AFOD_10MHZ  = 0xDU,  /* MODE=01 CNF=11 (alternate function OD) */
    GPIO_MODE_OUTPUT_AFOD_2MHZ   = 0xEU,  /* MODE=10 CNF=11 */
    GPIO_MODE_OUTPUT_AFOD_50MHZ  = 0xFU   /* MODE=11 CNF=11 */
} GpioAfio_Mode_e;

typedef enum {
    GPIO_PULL_NONE = 0,  /* floating (only valid for output / non-PUPD input modes) */
    GPIO_PULL_UP   = 1,  /* internal pull-up via ODR=1 (INPUT_PUPD only) */
    GPIO_PULL_DOWN = 2   /* internal pull-down via ODR=0 (INPUT_PUPD only) */
} GpioAfio_Pull_e;

typedef struct {
    GpioAfio_Port_e port;         /* target bank: < GPIO_PORT_COUNT */
    uint16_t        pin_mask;     /* pin bitmap (bit n -> pin n); at least one bit set */
    GpioAfio_Mode_e mode;         /* one of 16 mode values (MODE+CNF composed) */
    GpioAfio_Pull_e pull;         /* honored only when mode == INPUT_PUPD */
} GpioAfio_PinConfigType;

#endif /* GPIO_AFIO_TYPES_H */
