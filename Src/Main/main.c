#include "GPIO_STM32F10x.h"             // Keil::Device:GPIO
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#include "stdio.h"
#include "dependent_io.h"


int main(void) {
    SystemInit();
    SystemCoreClockUpdate();

    osKernelInitialize();
    
    osKernelStart();
}
