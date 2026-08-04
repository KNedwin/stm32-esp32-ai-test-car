#ifndef STM32F1XX_HAL_STUB_H
#define STM32F1XX_HAL_STUB_H

/* 最小 HAL 桩：仅覆盖 Card.c 单元测试所需类型/宏/函数 */

#include <stdint.h>

typedef struct { volatile uint32_t SR; volatile uint32_t DR; } USART_TypeDef;

/* 测试用 USART1 实例（test_card.c 定义） */
extern USART_TypeDef usart1_dev;
#define USART1 (&usart1_dev)

typedef enum {
    HAL_UART_STATE_RESET = 0x00,
    HAL_UART_STATE_READY,
    HAL_UART_STATE_BUSY_RX,
    HAL_UART_STATE_ERROR
} HAL_UART_StateTypeDef;

typedef struct {
    USART_TypeDef *Instance;
    uint32_t ErrorCode;
    HAL_UART_StateTypeDef gState;
    HAL_UART_StateTypeDef RxState;
    uint32_t BaudRate;
} UART_HandleTypeDef;

#define HAL_UART_ERROR_NONE   0x00000000
#define HAL_UART_ERROR_ORE    0x00000002
#define HAL_OK                0
#define HAL_ERROR             1

#define UART_FLAG_TC  ((uint32_t)0x00000040)
#define __HAL_UART_GET_FLAG(__HANDLE__, __FLAG__) (((__HANDLE__)->Instance->SR & (__FLAG__)) == (__FLAG__))
#define __HAL_UART_CLEAR_OREFLAG(__HANDLE__) ((__HANDLE__)->Instance->SR = 0)

int  HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
int  HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
uint32_t HAL_GetTick(void);

/* 测试注入接口（test_card.c 实现） */
void test_hal_set_tick(uint32_t t);
uint32_t test_hal_get_tx_len(void);
const uint8_t *test_hal_get_tx(void);

#endif
