/* card_parse（U13T 帧解析）主机单元测试
 * 编译：gcc -std=c11 -I <esp32项目>/main \
 *          test_card_parse.c <esp32项目>/main/card_parse.c
 * 使用真实 card_parse.c 代码，验证帧解析与状态标志更新。
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "card_parse.h"

static int g_pass = 0, g_fail = 0;
#define CHECK(cond) do { if(cond) g_pass++; else { g_fail++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while(0)

static void test_read_card_uid(void)
{
	/* 读卡号响应 7F 0A 00 90 00 04 00 E0 45 AF AB 3F */
	const uint8_t frame[] = {0x7F,0x0A,0x00,0x90,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
	uint8_t r = 0;
	for( uint16_t i = 0; i < sizeof(frame); i++ ) r = UartReceiveCommand(frame[i]);
	CHECK(r == 2);   /* 读到卡号 */
}

static void test_read_block_data(void)
{
	/* 读块响应：7F 1A 00 91 00 04 00 E0 45 AF AB <16字节> <校验>，共 28 字节 */
	uint8_t frame[28] = {0x7F,0x1A,0x00,0x91,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB};
	uint8_t data[16] = {0xD3,0xC5,0xC1,0xE9,0xBF,0xC6,0xBC,0xBC,0,0,0,0,0,0,0,0};
	for( int i = 0; i < 16; i++ ) frame[11+i] = data[i];
	frame[27] = 0x69;
	uint8_t r = 0;
	for( uint16_t i = 0; i < sizeof(frame); i++ ) r = UartReceiveCommand(frame[i]);
	CHECK(r == 1);
	CHECK(memcmp(Cmd.block_data, data, 16) == 0);
}

static void test_no_card(void)
{
	const uint8_t frame[] = {0x7F,0x0A,0x00,0x90,0xFF,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
	uint8_t r = 0;
	for( uint16_t i = 0; i < sizeof(frame); i++ ) r = UartReceiveCommand(frame[i]);
	CHECK(r == 3);
}

static void test_length_overrun(void)
{
	uint8_t frame[40];
	frame[0] = 0x7F;
	frame[1] = 32;   /* 长度超上限 → 丢弃并复位 */
	CHECK(UartReceiveCommand(frame[0]) == 0);
	CHECK(UartReceiveCommand(frame[1]) == 0);
	/* 复位后可正常解析新帧 */
	const uint8_t ok[] = {0x7F,0x0A,0x00,0x90,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
	uint8_t r = 0;
	for( uint16_t i = 0; i < sizeof(ok); i++ ) r = UartReceiveCommand(ok[i]);
	CHECK(r == 2);
}

static void test_baud_response_no_event(void)
{
	/* 0xAC 响应不产生"读块数据"事件 */
	const uint8_t frame[] = {0x7F,0x04,0x00,0xAC,0x00,0xAA};
	uint8_t r = 0;
	for( uint16_t i = 0; i < sizeof(frame); i++ ) r = UartReceiveCommand(frame[i]);
	CHECK(r == 0);
}

static void test_feed_exist(void)
{
	/* Feed：非 LEDLIGHT 态读到卡号 → EXIST */
	card_res_flag = CARD_FLAG_NONE;
	const uint8_t frame[] = {0x7F,0x0A,0x00,0x90,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
	for( uint16_t i = 0; i < sizeof(frame); i++ ) Card_Parse_Feed(frame[i], 6000);
	CHECK(card_res_flag == CARD_FLAG_EXIST);
}

static void test_feed_resdata(void)
{
	card_res_flag = CARD_FLAG_WAIT;
	uint8_t frame[28] = {0x7F,0x1A,0x00,0x91,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB};
	memset(&frame[11], 0, 16);
	frame[11] = 0xCC; frame[12] = 0xAB; frame[13] = 0xD1; frame[14] = 0xF4;
	frame[27] = 0x69;
	for( uint16_t i = 0; i < sizeof(frame); i++ ) Card_Parse_Feed(frame[i], 8000);
	CHECK(card_res_flag == CARD_FLAG_RESDATA);
	CHECK(Cmd.block_data[0] == 0xCC && Cmd.block_data[3] == 0xF4);
}

static void test_feed_ledlight_refresh(void)
{
	/* LEDLIGHT 态读到卡号 → 刷新卡在场时间戳，flag 保持 */
	card_res_flag = CARD_FLAG_LEDLIGHT;
	rfid_last_card_tick = 1000;
	const uint8_t frame[] = {0x7F,0x0A,0x00,0x90,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
	for( uint16_t i = 0; i < sizeof(frame); i++ ) Card_Parse_Feed(frame[i], 5000);
	CHECK(rfid_last_card_tick == 5000);
	CHECK(card_res_flag == CARD_FLAG_LEDLIGHT);
}

static void test_feed_no_card_ledlight_kept(void)
{
	/* LEDLIGHT 态无卡响应 → 不主动熄灯 */
	card_res_flag = CARD_FLAG_LEDLIGHT;
	const uint8_t frame[] = {0x7F,0x0A,0x00,0x90,0xFF,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
	for( uint16_t i = 0; i < sizeof(frame); i++ ) Card_Parse_Feed(frame[i], 7000);
	CHECK(card_res_flag == CARD_FLAG_LEDLIGHT);
}

int main(void)
{
	printf("=== card_parse 帧解析单元测试 ===\n");
	test_read_card_uid();
	test_read_block_data();
	test_no_card();
	test_length_overrun();
	test_baud_response_no_event();
	test_feed_exist();
	test_feed_resdata();
	test_feed_ledlight_refresh();
	test_feed_no_card_ledlight_kept();
	printf("通过 %d，失败 %d\n", g_pass, g_fail);
	return g_fail ? 1 : 0;
}
