# 时钟
- 芯片主频 240MHz（ESP32-S3 默认，IDF 自动配置）
- FreeRTOS tick = **1000Hz**（CONFIG_FREERTOS_HZ=1000，sdkconfig.defaults；修复 pdMS_TO_TICKS(1)=0 忙等/WDT 刷屏）
- 电机 PWM: LEDC 20kHz 10-bit（LEDC_AUTO_CLK，components/common/motor_drv.c）
- WS2812 RMT 时基 10MHz（RES_HZ，components/common/ws2812.c）
- 时间基准: esp_timer_get_time()（64 位 us，应用层截断为 ms，差值比较回绕安全）