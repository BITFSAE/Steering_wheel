#ifndef __ECU_INFO_H__
#define __ECU_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "usart.h" /* for USART_REC_LEN / RXBUFFERSIZE + stdint via stm32f1xx_hal.h */
#include "fsae_telemetry.pb.h"

/**
 * @brief  interface 子工程的业务全局变量汇总
 * @note   仅收拢业务层全局变量；HAL 外设句柄(huart/hcan/htim 等)仍保留在各自 xxx.c
 * @note   遥测字段已对齐 nanopb proto 类型 (fsae_BmsSummary / fsae_VehicleState)
 */
typedef struct
{
    /* === nanopb 遥测结构体 (proto 映射) === */
    fsae_BmsSummary   bms;     /* SOC, 总电压, 温度, 电流, IMD, 工作状态 */
    fsae_VehicleState vehicle; /* 车速, 驾驶模式, 四电机状态(rpm/torque/power/error) */

    /* === 非 proto 字段 (CAN 裸字节 / 通信层) === */
    int     ERRO[4]; /* BMS 原始错误字节 (CAN 0x401), proto 无对应字段 */

    /* main loop / app flags */
    int     flag;

    /* USART2 Bluetooth rx state machine (\r\n framing) */
    uint16_t g_usart_rx_sta;
    uint8_t  g_usart_rx_buf[USART_REC_LEN];
    uint8_t  g_rx_buffer[RXBUFFERSIZE];

    /* LORA USART1 rx state machine (\n framing, "M" command) */
    uint8_t  lora_rx_byte[RXBUFFERSIZE];
    uint16_t lora_rx_sta;
    uint8_t  lora_rx_buf[LORA_REC_LEN];

    /* CAN mode command (8 bytes for CAN DLC=8) */
    uint8_t  mode_sent[8];

} ECU_INFO_t;

/**
 * @brief 全局 ECU 信息实例
 */
extern ECU_INFO_t ECU_INFO;

#ifdef __cplusplus
}
#endif

#endif /* __ECU_INFO_H__ */
