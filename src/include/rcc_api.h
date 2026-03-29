#ifndef RCC_API_H
#define RCC_API_H

#include <stdint.h>

/* Mock Rcc_Api layer to respect Cross-Module Dependency Rules */
static inline void Rcc_Api_EnableCan1(void) {
    volatile uint32_t *RCC_AHB1ENR = (volatile uint32_t *)0x40023830U;
    volatile uint32_t *RCC_APB1ENR = (volatile uint32_t *)0x40023840U;
    *RCC_AHB1ENR |= (1U << 0);
    *RCC_APB1ENR |= (1U << 25);
}

#endif /* RCC_API_H */
