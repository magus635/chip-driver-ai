#ifndef PORT_API_H
#define PORT_API_H

#include <stdint.h>

/* Mock Port_Api layer to respect Cross-Module Dependency Rules */
static inline void Port_Api_InitCanPins(void) {
    volatile uint32_t *GPIOA_MODER = (volatile uint32_t *)(0x40020000U + 0x00U);
    volatile uint32_t *GPIOA_OTYPER = (volatile uint32_t *)(0x40020000U + 0x04U);
    volatile uint32_t *GPIOA_OSPEEDR = (volatile uint32_t *)(0x40020000U + 0x08U);
    volatile uint32_t *GPIOA_PUPDR = (volatile uint32_t *)(0x40020000U + 0x0CU);
    volatile uint32_t *GPIOA_AFRH = (volatile uint32_t *)(0x40020000U + 0x24U);
    
    *GPIOA_MODER &= ~((3U << 22) | (3U << 24));
    *GPIOA_MODER |= ((2U << 22) | (2U << 24));
    *GPIOA_OTYPER &= ~((1U << 11) | (1U << 12));
    *GPIOA_OSPEEDR &= ~((3U << 22) | (3U << 24));
    *GPIOA_OSPEEDR |= ((2U << 22) | (2U << 24));
    *GPIOA_PUPDR &= ~((3U << 22) | (3U << 24));
    *GPIOA_PUPDR |= ((1U << 22) | (1U << 24));
    *GPIOA_AFRH &= ~((0xFU << 12) | (0xFU << 16));
    *GPIOA_AFRH |= ((0x9U << 12) | (0x9U << 16));
}

#endif /* PORT_API_H */
