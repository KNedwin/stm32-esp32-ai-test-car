# 时钟
- CPU 主频 **160MHz**（sdkconfig:CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_160=y —— 非 240MHz 默认值，旧文档 240MHz 写法作废）
- 晶振: XTAL 40MHz（sdkconfig:CONFIG_XTAL_FREQ=40）
- FreeRTOS tick: **1000Hz**（sdkconfig.defaults:CONFIG_FREERTOS_HZ=1000；修复 pdMS_TO_TICKS(1)=0 导致的忙等与 IDLE 饿死 WDT 刷屏；vTaskDelay(1)=1ms）
- 电机 PWM: LEDC 20kHz 10-bit（LEDC_AUTO_CLK，sources/components/common/motor_drv.c）
- WS2812: RMT TX 时基 10MHz（sources/components/common/ws2812.c RES_HZ=10000000）
- 时间基准: esp_timer_get_time()（64 位 us，应用层截断为 32 位 ms，差值比较回绕安全）
（2026-08 更新：主频/节拍/RMT 时基按当前 sdkconfig 与源码核实）