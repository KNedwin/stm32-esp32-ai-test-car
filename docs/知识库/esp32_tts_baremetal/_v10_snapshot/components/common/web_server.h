#ifndef __WEB_SERVER_H
#define __WEB_SERVER_H

/* 启动 HTTP 配置服务（端口 80）：
 * GET  /            配置页面（内嵌 webpage.html）
 * GET  /api/params  当前参数 JSON（触发词转 UTF-8 文本）
 * POST /api/params  JSON 提交（触发词转 GBK），sanitize 后存 NVS
 * POST /api/restart 保存并重启
 *
 * 空闲超时：web_server_idle_seconds() 供配置模式监控自动退出 */
void web_server_start(void);
int  web_server_idle_seconds(void);

#endif /* __WEB_SERVER_H */
