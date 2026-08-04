# 时钟
- 芯片主频 240MHz（ESP32-S3 默认，IDF 自动配置）
- 电机 PWM: LEDC 20kHz 10-bit（LEDC_AUTO_CLK，sources/components/common/motor_drv.c）
- 时间基准: esp_timer_get_time()（64 位 us，应用层截断为 ms）
