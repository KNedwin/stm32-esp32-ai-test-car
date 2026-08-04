#include "tts.h"
#include "pins.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TTS_UART    UART_NUM_2
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

	uart_param_config(TTS_UART, &conf);
	uart_set_pin(TTS_UART, PIN_TTS_TX, PIN_TTS_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_driver_install(TTS_UART, TTS_BUF, TTS_BUF, 0, NULL, 0);
}

void TTS_Send(const uint8_t *str)
{
	uint16_t len = 0;

	while( str[len] ) len++;
	if( len > 0 )
	{
		uart_write_bytes(TTS_UART, str, len);
		uart_wait_tx_done(TTS_UART, pdMS_TO_TICKS(200));
	}
}
