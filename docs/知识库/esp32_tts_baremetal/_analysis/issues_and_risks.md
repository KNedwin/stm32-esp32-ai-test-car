# 已知问题与风险（导航，2026-08 复核）
- 已修复（历史）：真实时间差超时（原 100Hz 漂移）；NONE 态 200ms 轮询防失联；TX 0x7F 转义分支重写；初始化 ESP_ERROR_CHECK
- 已修复（2026-08）：FreeRTOS tick 100→1000Hz（pdMS_TO_TICKS(1)=0 忙等/WDT 刷屏）；RGB 完整流转（绿确认窗+RAMPUP 占用保护）；停车序列期 RFID 不覆盖 LED 色
## 注释 vs 代码冲突（低危文档债）
| 位置 | 注释说 | 代码实际 |
|---|---|---|
| app_main.c:17 / rfid_process.c:48 | FreeRTOS 默认 100Hz，vTaskDelay(1)=10ms | sdkconfig FREERTOS_HZ=1000 实为 1ms（超时全走 esp_timer，行为无碍） |
| wifi_ap.h 头注释 | 开放网络 | WIFI_AUTH_WPA_WPA2_PSK + 密码 12345678 |
| config_mode.h 头注释 | 计数达 4（上电+3次）进入 | CFG_ENTER_THRESHOLD=3，cnt>=3 即进 |
## 遗留低危
- adc.c/h 无调用方仍参与编译；app_main.c 与 motor_process.c 的 #include adc.h 残留
- motor_drv.h Motor_SetDirection 声明重复两次；config_mode.c 重复 #include esp_system.h
- ESP32 版 NVS blob 无 magic/CRC（靠 len==sizeof(params_t) 匹配；结构变更回落默认一次，网页需重配；STM32 版才有 magic+CRC）
- speak_en=0 语义、IDLE/RAMPUP 期触发延迟消费、SLOW 窗口重启后二次进入、绝对计时重启即停（设计既定）
- 中文路径陷阱: picolibc specs 不支持中文路径 → sdkconfig.defaults CONFIG_LIBC_NEWLIB=y（勿删）
- run_tests.sh 步骤编号 [1/3][2/3][3/4][4/4] 不一致（纯打印瑕疵）