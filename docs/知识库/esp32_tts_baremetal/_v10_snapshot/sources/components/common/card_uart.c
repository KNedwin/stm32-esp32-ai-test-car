/* U13T 读卡模块：ESP-IDF UART1 驱动（帧解析在 card_parse.c 纯逻辑层）
 *  - ESP-IDF UART 驱动内部环形缓冲，读取慢不丢字节，无 ORE 锁死问题
 *  - 波特率切换：9600 发 0x2C 命令 → uart_set_baudrate 115200
 *  - TX 组帧用局部缓冲（card_parse 仅持有接收缓冲）
 */
#include "card_uart.h"
#include "card_parse.h"
#include "pins.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_check.h"

#if DEMO_MODE
#include <stdio.h>
#include <string.h>

/* ============ 无硬件演示模式：模拟 U13T 模块响应帧 ============
 * 帧格式与 card_parse 解析一致：0x7F 帧头 + 长度 + 数据（长度字节 <0x7F）。
 *  - 卡号帧（0x90 响应）：触发 EXIST → 状态机发读块命令
 *  - 读块帧（0x91 响应）：ReceiveBuffer[9..24] = 16 字节块数据（GBK 触发词）
 * 每 ~11s 一轮：卡号帧 → 150ms 后块帧；触发词按 太阳→地球→地球 轮换，
 * 演示 一次性词(太阳) 与 计数型词(地球, 2 次间隔≥10s) 两种触发语义。
 */
static uint8_t demo_word_idx = 0;      /* 0=太阳 1=地球 */
static uint8_t demo_earth_count = 0;   /* 地球已刷卡次数 */
static uint8_t demo_phase = 0;         /* 0=待发卡号帧 1=待发块帧 */
static uint32_t demo_next_ms = 0;      /* 下次注入时间(ms) */

/* 模拟卡号帧：0x7F | 0x04 | 04 90 00 00 */
static const uint8_t demo_cardid_frame[] = {0x7F, 0x04, 0x04, 0x90, 0x00, 0x00};

static void Card_Demo_Feed(void)
{
	uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
	uint8_t frame[32];
	uint8_t n, i;

	if( now < demo_next_ms ) return;

	if( demo_phase == 0 )
	{
		/* 卡号帧 → card_res_flag = EXIST，状态机开始读块 */
		for( i = 0; i < sizeof(demo_cardid_frame); i++ )
		{
			Card_Parse_Feed(demo_cardid_frame[i], now);
		}
		demo_phase = 1;
		demo_next_ms = now + 150;    /* 模拟模块处理延时后应答 */
		return;
	}

	/* 读块帧：构造 0x7F | 0x1A | 26 字节数据
	 * 数据字节定位：frame[k] = D(k-1)，块数据 D10..D25 = frame[11..26] */
	memset(frame, 0, sizeof(frame));
	frame[0] = 0x7F;
	frame[1] = 0x1A;
	frame[2] = 0x1A;      /* D1 占位 */
	frame[3] = 0x91;      /* D2 命令=读块响应 */
	frame[4] = 0x00;      /* D3 状态=成功 */
	/* D4..D9 占位（frame[5..10]）已由 memset 清零 */
	/* D10..D25 = 块数据 16 字节：GBK 触发词 + 0 填充 */
	{
		static const uint8_t words[2][4] = {
			{0xCC, 0xAB, 0xD1, 0xF4},   /* 太阳 */
			{0xB5, 0xD8, 0xC7, 0xF2},   /* 地球 */
		};
		const uint8_t *w = words[demo_word_idx];
		for( n = 0; n < 4; n++ ) frame[11+n] = w[n];
		/* frame[15..26] 保持 0 */
	}
	frame[26] = 0x00;   /* D26 触发判断字节 */

	printf("[DEMO] simulate card: ");
	for( n = 0; n < 4; n++ ) printf("%02X ", frame[11+n]);
	printf("\r\n");
	for( i = 0; i < 27; i++ )
	{
		Card_Parse_Feed(frame[i], now);
	}

	/* 轮换触发词语义：太阳 → 地球(第1次) → 地球(第2次触发) → 太阳… */
	if( demo_word_idx == 0 )
	{
		demo_word_idx = 1;
	}
	else
	{
		demo_earth_count++;
		if( demo_earth_count >= 2 )
		{
			demo_word_idx = 0;
			demo_earth_count = 0;
		}
	}
	demo_phase = 0;
	demo_next_ms = now + 11000;   /* 每 11s 一轮：LED 3s 熄灭 + 间隔 + 地球计数间隔≥10s */
}
#endif /* DEMO_MODE */

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

/* 非阻塞消费接收缓冲，逐字节喂解析器
 * DEMO_MODE：绕过真实 UART，由模拟器周期注入模块响应帧 */
void Card_Uart_Poll(void)
{
#if DEMO_MODE
	Card_Demo_Feed();
#else
	uint8_t buf[32];
	int len = uart_read_bytes(RFID_UART, buf, sizeof(buf), 0);
	uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

	for( int i = 0; i < len; i++ )
	{
		Card_Parse_Feed(buf[i], now);
	}
#endif
}
