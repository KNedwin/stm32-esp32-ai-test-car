#include "tts.h"
#include "pins.h"
#include "config.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"

#if DEMO_MODE
#include <stdio.h>
#endif

#define TTS_BUF     256

void TTS_Init(void)
{
	uart_config_t conf = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};

	ESP_ERROR_CHECK(uart_param_config(TTS_UART, &conf));
	ESP_ERROR_CHECK(uart_set_pin(TTS_UART, PIN_TTS_TX, PIN_TTS_RX,
								 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ESP_ERROR_CHECK(uart_driver_install(TTS_UART, TTS_BUF, TTS_BUF, 0, NULL, 0));
}

/* 开机默认设置（模块指令集在此集中，应用层只调用本函数） */
void TTS_SetupDefaults(void)
{
#if RFID_SETTING_SPEAK_SPEED
	TTS_Send((const uint8_t *)"<S>3");   /* 语速最快 */
	vTaskDelay(pdMS_TO_TICKS(80));
	TTS_Send((const uint8_t *)"<V>6");   /* 音量 */
	vTaskDelay(pdMS_TO_TICKS(80));
	TTS_Send((const uint8_t *)"<I>7");   /* 上电提示音选7号(试听选定)；<I>指令启用断电保存 */
	vTaskDelay(pdMS_TO_TICKS(200));
#endif
}

void TTS_Send(const uint8_t *str)
{
	uint16_t len = 0;

	while( str[len] ) len++;
	if( len > 0 )
	{
#if DEMO_MODE
		/* 无硬件演示：打印即将发送的帧（hex），便于对照协议文档核对 */
		printf("[TTS] send %2uB: ", len);
		for( uint16_t i = 0; i < len; i++ )
		{
			printf("%02X ", str[i]);
		}
		printf("\r\n");
#else
		uart_write_bytes(TTS_UART, str, len);
		uart_wait_tx_done(TTS_UART, pdMS_TO_TICKS(200));
#endif
	}
}
