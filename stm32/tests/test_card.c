/* Card.c（U13T 读卡协议）主机单元测试
 * 编译：gcc -std=c11 -I hal_stub -I <工程>/hardware/rfid_card \
 *          test_card.c <工程>/hardware/rfid_card/Card.c
 * 使用真实 Card.c 代码 + HAL 桩，验证协议解析与回调状态迁移。
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "Card.h"

/* ---- HAL 桩实现 ---- */
static uint32_t g_tick = 0;
static uint8_t  g_tx_buf[64];
static uint16_t g_tx_len = 0;

USART_TypeDef usart1_dev = {0};
UART_HandleTypeDef huart1 = { .Instance = &usart1_dev };

void test_hal_set_tick(uint32_t t) { g_tick = t; }
uint32_t test_hal_get_tx_len(void) { return g_tx_len; }
const uint8_t *test_hal_get_tx(void) { return g_tx_buf; }

uint32_t HAL_GetTick(void) { return g_tick; }

int HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    (void)huart; (void)Timeout;
    g_tx_len = 0;
    for( uint16_t i = 0; i < Size && i < sizeof(g_tx_buf); i++ ) g_tx_buf[i] = pData[i];
    g_tx_len = Size;
    return HAL_OK;
}

int HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    (void)huart; (void)pData; (void)Size;
    huart->RxState = HAL_UART_STATE_READY;
    return HAL_OK;
}

/* ---- 测试框架 ---- */
static int g_pass = 0, g_fail = 0;
#define CHECK(cond) do { if(cond) g_pass++; else { g_fail++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while(0)

/* 回调函数（Card.c 定义，测试侧声明） */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);

/* ---- 用例 ---- */
static void test_read_card_uid(void)
{
    /* 手册示例：读卡号响应 7F 0A 00 90 00 04 00 E0 45 AF AB 3F */
    const uint8_t frame[] = {0x7F,0x0A,0x00,0x90,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
    uint8_t r = UartReceiveCommand(frame[0]);
    CHECK(r == 0);
    for( uint16_t i = 1; i < sizeof(frame)-1; i++ ) r = UartReceiveCommand(frame[i]);
    CHECK(r == 0);
    r = UartReceiveCommand(frame[sizeof(frame)-1]);
    CHECK(r == 2);   /* 读到卡号 */
}

static void test_read_block_data(void)
{
    /* 手册示例：读块响应（帧 7F 1A 00 91 00 04 00 E0 45 AF AB <16字节> <校验>，共 28 字节） */
    uint8_t frame[28] = {0x7F,0x1A,0x00,0x91,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB};
    uint8_t data[16] = {0xD3,0xC5,0xC1,0xE9,0xBF,0xC6,0xBC,0xBC,0,0,0,0,0,0,0,0};
    for( int i = 0; i < 16; i++ ) frame[11+i] = data[i];
    frame[27] = 0x69;   /* 校验字节（解析器不校验，仅补全帧长） */
    uint8_t r = 0;
    for( uint16_t i = 0; i < sizeof(frame); i++ ) r = UartReceiveCommand(frame[i]);
    CHECK(r == 1);   /* 读到块数据 */
    CHECK(memcmp(Cmd.block_data, data, 16) == 0);
}

static void test_no_card_response(void)
{
    /* 读卡号响应，状态 0xFF（无卡） */
    const uint8_t frame[] = {0x7F,0x0A,0x00,0x90,0xFF,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
    uint8_t r = 0;
    for( uint16_t i = 0; i < sizeof(frame); i++ ) r = UartReceiveCommand(frame[i]);
    CHECK(r == 3);   /* 无卡/错误 */
}

static void test_garbage_before_frame(void)
{
    /* 前置垃圾字节不应破坏解析 */
    const uint8_t garbage[] = {0x00,0xAA,0x55,0x12};
    const uint8_t frame[] = {0x7F,0x04,0x00,0x11,0x01,0x14};  /* 读块命令（发往模块的，这里仅作结构测试） */
    uint8_t r = 0;
    for( uint16_t i = 0; i < sizeof(garbage); i++ ) r = UartReceiveCommand(garbage[i]);
    CHECK(r == 0);
    for( uint16_t i = 0; i < sizeof(frame); i++ ) r = UartReceiveCommand(frame[i]);
    /* 0x11 命令响应不在解析范围 → 0 */
    CHECK(r == 0);
}

static void test_length_overrun_protection(void)
{
    /* 长度 32（>31）应被丢弃并重置状态机，不得越界写 ReceiveBuffer */
    uint8_t frame[40];
    frame[0] = 0x7F;
    frame[1] = 32;   /* 长度 32，超过 ReceiveBuffer[32] 上限 */
    uint8_t r = UartReceiveCommand(frame[0]);
    CHECK(r == 0);
    r = UartReceiveCommand(frame[1]);
    CHECK(r == 0);   /* 直接丢弃，状态机复位 */
    /* 复位后应能正常解析新帧 */
    const uint8_t ok_frame[] = {0x7F,0x0A,0x00,0x90,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
    for( uint16_t i = 0; i < sizeof(ok_frame); i++ ) r = UartReceiveCommand(ok_frame[i]);
    CHECK(r == 2);
}

static void test_baud_response_not_block_data(void)
{
    /* 0xAC（设波特率）响应不应产生"读块数据"事件（修复 M1） */
    const uint8_t frame[] = {0x7F,0x04,0x00,0xAC,0x00,0xAA};
    uint8_t r = 0;
    for( uint16_t i = 0; i < sizeof(frame); i++ ) r = UartReceiveCommand(frame[i]);
    CHECK(r == 0);   /* 之前错误返回 1，现应返回 0 */
}

static void test_set_bound_frame(void)
{
    /* SetBound115200 发送帧校验（帧头/长度/命令/确认码/校验） */
    card_res_flag = CARD_FLAG_NONE;
    SetBound115200();
    CHECK(test_hal_get_tx_len() == 12);
    const uint8_t *tx = test_hal_get_tx();
    CHECK(tx[0] == 0x7F);                       /* 帧头 */
    CHECK(tx[1] == 0x0A);                       /* 长度 */
    CHECK(tx[2] == 0x00);                       /* 地址 */
    CHECK(tx[3] == 0x2C);                       /* 设波特率命令 */
    CHECK(tx[8] == 0x98 && tx[9] == 0x24 && tx[10] == 0x31);  /* 确认码 */
    /* 校验 = 长度^地址^命令^参数 异或 */
    uint8_t cs = tx[1];
    for( int i = 2; i <= 10; i++ ) cs ^= tx[i];
    CHECK(tx[11] == cs);
}

static void test_callback_ledlight_refresh(void)
{
    /* LEDLIGHT 态收到"读到卡号" → 刷新 rfid_last_card_tick */
    test_hal_set_tick(5000);
    card_res_flag = CARD_FLAG_LEDLIGHT;
    rfid_last_card_tick = 1000;
    const uint8_t frame[] = {0x7F,0x0A,0x00,0x90,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
    for( uint16_t i = 0; i < sizeof(frame); i++ )
    {
        card_res = frame[i];
        HAL_UART_RxCpltCallback(&huart1);
    }
    CHECK(rfid_last_card_tick == 5000);   /* 卡在场时间戳被刷新 */
    CHECK(card_res_flag == CARD_FLAG_LEDLIGHT);  /* 保持 LEDLIGHT */
}

static void test_callback_exist_transition(void)
{
    /* 非 LEDLIGHT 态读到卡号 → 置 EXIST */
    test_hal_set_tick(6000);
    card_res_flag = CARD_FLAG_NONE;
    const uint8_t frame[] = {0x7F,0x0A,0x00,0x90,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
    for( uint16_t i = 0; i < sizeof(frame); i++ )
    {
        card_res = frame[i];
        HAL_UART_RxCpltCallback(&huart1);
    }
    CHECK(card_res_flag == CARD_FLAG_EXIST);
}

static void test_callback_no_card_ledlight(void)
{
    /* LEDLIGHT 态收到无卡响应 → 不主动熄灯（flag 保持），修复点② */
    test_hal_set_tick(7000);
    card_res_flag = CARD_FLAG_LEDLIGHT;
    rfid_last_card_tick = 7000;
    const uint8_t frame[] = {0x7F,0x0A,0x00,0x90,0xFF,0x04,0x00,0xE0,0x45,0xAF,0xAB,0x3F};
    for( uint16_t i = 0; i < sizeof(frame); i++ )
    {
        card_res = frame[i];
        HAL_UART_RxCpltCallback(&huart1);
    }
    CHECK(card_res_flag == CARD_FLAG_LEDLIGHT);   /* 保持亮灯，熄灭由状态机 tick 判断 */
}

static void test_callback_block_data_flag(void)
{
    /* 读块响应 → RESDATA */
    test_hal_set_tick(8000);
    card_res_flag = CARD_FLAG_WAIT;
    uint8_t frame[28] = {0x7F,0x1A,0x00,0x91,0x00,0x04,0x00,0xE0,0x45,0xAF,0xAB};
    memset(&frame[11], 0, 16);
    frame[11] = 0xCC; frame[12] = 0xAB; frame[13] = 0xD1; frame[14] = 0xF4;  /* "太阳" */
    frame[27] = 0x69;
    for( uint16_t i = 0; i < sizeof(frame); i++ )
    {
        card_res = frame[i];
        HAL_UART_RxCpltCallback(&huart1);
    }
    CHECK(card_res_flag == CARD_FLAG_RESDATA);
    CHECK(Cmd.block_data[0] == 0xCC && Cmd.block_data[3] == 0xF4);
}

int main(void)
{
    printf("=== Card.c 单元测试 ===\n");
    test_read_card_uid();
    test_read_block_data();
    test_no_card_response();
    test_garbage_before_frame();
    test_length_overrun_protection();
    test_baud_response_not_block_data();
    test_set_bound_frame();
    test_callback_ledlight_refresh();
    test_callback_exist_transition();
    test_callback_no_card_ledlight();
    test_callback_block_data_flag();
    printf("通过 %d，失败 %d\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}
