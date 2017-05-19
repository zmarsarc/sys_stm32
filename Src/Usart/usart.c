#include "usart.h"
#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2

#define IFNC(EXP) if (ARM_DRIVER_OK != (hr = (EXP))) goto cleanup

/*
* Helper methods def
*/

static int32_t USART_default_init(ARM_DRIVER_USART* usart, ARM_USART_SignalEvent_t cb) { /*
    *
    * Initialize USART by default parameters
    *
    * mode:           asynchronus
    * band rate:      9600
    * data bits:      8
    * stop bits:      1
    * parity:         None
    * flow control:   None
    * clock polarity: rising edge
    * clock phase     leading edge
    *
    * return: Status Error Code
*/
    int32_t hr = ARM_DRIVER_OK;
    uint32_t mode = ARM_USART_MODE_ASYNCHRONOUS;
    uint32_t bandrate = 9600;
    
    IFNC(usart->Initialize(cb));
    IFNC(usart->PowerControl(ARM_POWER_FULL));
    IFNC(usart->Control(mode, bandrate));
    
    cleanup:
    return hr;
}
/*
* End of Helper methods def
*/


/*
* USART 1 
*/

osEventFlagsId_t USART_1_Events;

static void USART_1_IRS_callback(uint32_t event) {
    switch (event) {
        case ARM_USART_EVENT_SEND_COMPLETE:
            osEventFlagsSet(USART_1_Events, 0x01);
            break;
        case ARM_USART_EVENT_RECEIVE_COMPLETE:
            osEventFlagsSet(USART_1_Events, 0x02);
            break;
        default:
            break;
    }
}

int32_t USART_1_init(void) {
    extern ARM_DRIVER_USART Driver_USART1;
    
    USART_1_Events = osEventFlagsNew(NULL);
    if (USART_1_Events == NULL) {
        return cErrorBadAlloc;
    }
    return USART_default_init(&Driver_USART1, USART_1_IRS_callback);
}

/*
* End of USART 1
*/
