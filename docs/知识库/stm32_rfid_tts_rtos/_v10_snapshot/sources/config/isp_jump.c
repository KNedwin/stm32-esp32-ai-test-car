#include "isp_jump.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"

void param_cli_do_isp(void)    { ISP_Enter(); }
void param_cli_do_reboot(void) { HAL_Delay(100); NVIC_SystemReset(); }

#define SYSTEM_BOOTLOADER_ADDR 0x1FFFF000UL

/* 软跳转系统 Bootloader：换向量表+主栈后跳转 */
void ISP_Enter(void)
{
    uint32_t msp = *(volatile uint32_t *)SYSTEM_BOOTLOADER_ADDR;
    uint32_t pc  = *(volatile uint32_t *)(SYSTEM_BOOTLOADER_ADDR + 4);

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    HAL_RCC_DeInit();

    SCB->VTOR = SYSTEM_BOOTLOADER_ADDR;
    __set_MSP(msp);
    __DSB();
    __ISB();
    ((void (*)(void))pc)();
}
