/* U13T 读卡模块：ESP-IDF UART1 驱动（帧解析在 card_parse.c 纯逻辑层）
 *  - ESP-IDF UART 驱动内部环形缓冲，读取慢不丢字节，无 ORE 锁死问题
 *  - 波特率切换：9600 发 0x2C 命令 → uart_set_baudrate 115200
 */
#include "card_uart.h"
#include "card_parse.h"
#include "pins.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_timer.h"

#define RFID_UART    UART_NUM_1
#define RX_BUF_SIZE  256

static uint8_t CheckSum(uint8_t *dat, uint8_t num);
static void UartSendCommand(uint8_t *buff, uint8_t cnt);

/* 设置读卡模块波特率为 115200（命令 0x2C，确认码 98 24 31） */
static void SetBound115200(void)
{
	unsigned char len = 0x0A;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0x00;
	Cmd.SendBuffer[2] = 0x2C;
	Cmd.SendBuffer[3] = 0x00;
	Cmd.SendBuffer[4] = 0x01;
	Cmd.SendBuffer[5] = 0xC2;
	Cmd.SendBuffer[6] = 0x00;
	Cmd.SendBuffer[7] = 0x98;
	Cmd.SendBuffer[8] = 0x24;
	Cmd.SendBuffer[9] = 0x31;
	Cmd.SendBuffer[10] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

void Card_ReadCard(void)
{
	unsigned char len = 3;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0;
	Cmd.SendBuffer[2] = 0x10;
	Cmd.SendBuffer[3] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

void Card_ReadBlock(uint8_t block)
{
	unsigned char len = 4;

	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0;
	Cmd.SendBuffer[2] = 0x11;
	Cmd.SendBuffer[3] = block;
	Cmd.SendBuffer[4] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len);
}

static uint8_t CheckSum(uint8_t *dat, uint8_t num)
{
	uint8_t bTemp = 0, i;

	for(i = 0; i < num; i ++){bTemp ^= dat[i];}
	return bTemp;
}

/* 发送命令帧：加 0x7F 帧头，参数中的 0x7F 双写转义（依赖命令数据不含 0x7F） */
static void UartSendCommand(uint8_t *buff, uint8_t cnt)
{
	uint8_t i;
	uint8_t data[24];
	uint8_t num = 0;

	data[0] = 0x7F;
	for( i = 0; num < cnt+1; i++ )
	{
		data[i+1] = buff[i];
		num++;
		if( buff[i] == 0x7F )
		{
			i += 1;
			data[i+1] = 0x7F;
		}
	}
	num = i+1;

	uart_write_bytes(RFID_UART, data, num);
	uart_wait_tx_done(RFID_UART, pdMS_TO_TICKS(100));
}

void Card_Uart_Init(void)
{
	uart_config_t conf = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};

	uart_param_config(RFID_UART, &conf);
	uart_set_pin(RFID_UART, PIN_RFID_TX, PIN_RFID_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_driver_install(RFID_UART, RX_BUF_SIZE, 0, 0, NULL, 0);

	/* 波特率切换：9600 发命令 → 模块切 115200（可能记忆也可能不记忆，本机无条件跟随） */
	SetBound115200();
	vTaskDelay(pdMS_TO_TICKS(50));
	uart_set_baudrate(RFID_UART, 115200);
}

/* 非阻塞消费接收缓冲，逐字节喂解析器 */
void Card_Uart_Poll(void)
{
	uint8_t buf[32];
	int len = uart_read_bytes(RFID_UART, buf, sizeof(buf), 0);
	uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

	for( int i = 0; i < len; i++ )
	{
		Card_Parse_Feed(buf[i], now);
	}
}
