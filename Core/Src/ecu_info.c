#include "ecu_info.h"

/*
 * 业务全局变量统一在此定义/初始化。
 * 其他 .c 仅通过 ecu_info.h 访问，避免散落的全局变量定义。
 * 遥测字段使用 nanopb _init_default 宏确保类型安全。
 */
ECU_INFO_t ECU_INFO =
{
    .bms     = fsae_BmsSummary_init_default,
    .vehicle = fsae_VehicleState_init_default,

    .ERRO    = {0, 0, 0, 0},

    .flag    = 0,

    .g_usart_rx_sta = 0,
    .g_usart_rx_buf = {0},
    .g_rx_buffer    = {0},

    .lora_rx_byte   = {0},
    .lora_rx_sta    = 0,
    .lora_rx_buf    = {0},

    .mode_sent      = {0},
};
