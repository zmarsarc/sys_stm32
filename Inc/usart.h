#ifndef USART_H_
#define USART_H_

#include "Driver_USART.h"
#include "cmsis_os2.h"

#define cErrorBadAlloc 0x80000001U  // error during create new object

// USART 1
#define USART_1_Enable_RX()  Driver_USART1.Control(ARM_USART_CONTROL_RX, 1)
#define USART_1_Enable_TX()  Driver_USART1.Control(ARM_USART_CONTROL_TX, 1)
#define USART_1_Disable_RX() Driver_USART1.Control(ARM_USART_CONTROL_RX, 0)
#define USART_1_Disable_TX() Driver_USART1.Control(ARM_USART_CONTROL_TX, 0)
#define USART_1_Enable_Transfer()  USART_1_Enable_RX();USART_1_Enable_TX()
#define USART_1_Disable_Transfer() USART_1_Disable_RX();USART_1_Disable_TX()
extern ARM_DRIVER_USART Driver_USART1;
extern osEventFlagsId_t USART_1_Events;
int32_t USART_1_init(void);

#endif
