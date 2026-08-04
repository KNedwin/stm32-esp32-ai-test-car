/* U13T 读卡模块：ESP-IDF UART1 驱动（帧解析在 card_parse.c 纯逻辑层）
 *  - ESP-IDF UART 驱动内部环形缓冲，读取慢不丢字节，无 ORE 锁死问题
 *  - 波特率切换：9600 发 0x2C 命令 → uart_set_baudrate 115200
 *  - TX 组帧用局部缓冲（card_parse 仅持有接收缓冲）
 */
#include "card_uart.h"
#include "card_parse.h"
#include "pins.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_check.h"

#define RX_BUF_SIZE  256
#define TX_FRAME_MAX 24    /* 最大命令帧：帧头1 + 长度1 + 参数≤20 + 0x7F 转义冗余 */

static uint8_t CheckSum(uint8_t *dat, uint8_t num);
static void UartSendCommand(const uint8_t *buff, uint8_t cnt);

/* 设置读卡模块波特率为 115200（命令 0x2C，确认码 98 24 31） */
static void SetBound115200(void)
{
	uint8_t len = 0x0A;
	uint8_t buf[16];

	buf[0] = len;
	buf[1] = 0x00;
	buf[2] = 0x2C;
	buf[3] = 0x00;
	buf[4] = 0x01;
	buf[5] = 0xC2;
	buf[6] = 0x00;
	buf[7] = 0x98;
	buf[8] = 0x24;
	buf[9] = 0x31;
	buf[10] = CheckSum(buf, len);
	UartSendCommand(buf, len);
}

void Card_ReadCard(void)
{
	uint8_t len = 3;
	uint8_t buf[8];

	buf[0] = len;
	buf[1] = 0;
	buf[2] = 0x10;
	buf[3] = CheckSum(buf, len);
	UartSendCommand(buf, len);
}

void Card_ReadBlock(uint8_t block)
{
	uint8_t len = 4;
	uint8_t buf[8];

	buf[0] = len;
	buf[1] = 0;
	buf[2] = 0x11;
	buf[3] = block;
	buf[4] = CheckSum(buf, len);
	UartSendCommand(buf, len);
}

static uint8_t CheckSum(uint8_t *dat, uint8_t num)
{
	uint8_t bTemp = 0, i;

	for(i = 0; i < num; i ++){bTemp ^= dat[i];}
	return bTemp;
}

/* 发送命令帧：加 0x7F 帧头，参数中的 0x7F 双写转义
 * （输入/输出索引分离；依赖命令数据不含 0x7F——0x2C/0x10/0x11 命令字节固定） */
static void UartSendCommand(const uint8_t *buff, uint8_t cnt)
{
	uint8_t out[TX_FRAME_MAX];
	uint8_t in_idx, out_idx;

	out[0] = 0x7F;
	out_idx = 1;
	for( in_idx = 0; in_idx <= cnt && out_idx < TX_FRAME_MAX; in_idx++ )
	{
		out[out_idx++] = buff[in_idx];
		if( buff[in_idx] == 0x7F && out_idx < TX_FRAME_MAX )
		{
			out[out_idx++] = 0x7F;   /* 双写转义 */
		}
	}

	uart_write_bytes(RFID_UART, out, out_idx);
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

	ESP_ERROR_CHECK(uart_param_config(RFID_UART, &conf));
	ESP_ERROR_CHECK(uart_set_pin(RFID_UART, PIN_RFID_TX, PIN_RFID_RX,
								 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ESP_ERROR_CHECK(uart_driver_install(RFID_UART, RX_BUF_SIZE, 0, 0, NULL, 0));

	/* 波特率切换：9600 发命令 → 模块切 115200（可能记忆也可能不记忆，本机无条件跟随） */
	SetBound115200();
	vTaskDelay(pdMS_TO_TICKS(50));   /* 等待模块完成切换 */
	ESP_ERROR_CHECK(uart_set_baudrate(RFID_UART, 115200));
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
