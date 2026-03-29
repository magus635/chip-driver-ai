#ifndef CAN_API_H
#define CAN_API_H

#include "can_types.h"
#include "can_cfg.h"

/* Layer 4: Public CAN API (V2.3 Hardware Exhaustion Edition) 
 * STRICT CONFORMITY: No _ll.h headers.
 */

Can_ReturnType Can_Init(const Can_ConfigType *ConfigPtr);
Can_ReturnType Can_Write(const Can_PduType *PduInfo);
Can_ReturnType Can_Receive(Can_PduType *PduInfo);
void           Can_GetControllerErrorState(uint8_t *Txc, uint8_t *Rxc, uint8_t *Lec);

#endif 
