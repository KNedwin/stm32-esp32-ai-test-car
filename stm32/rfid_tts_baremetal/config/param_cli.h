#ifndef __PARAM_CLI_H
#define __PARAM_CLI_H

#include <stdint.h>

/* USART3 参数配置行协议（115200，行以 \r\n 结尾），命令集同 RTOS 版说明。 */

typedef enum {
    PARAM_CLI_ACT_NONE = 0,
    PARAM_CLI_ACT_ISP,
    PARAM_CLI_ACT_REBOOT,
} param_cli_action_t;

void ParamCli_Init(void);
void ParamCli_Poll(void);
uint8_t ParamCli_ShouldEnterIsp(void);

#endif /* __PARAM_CLI_H */
