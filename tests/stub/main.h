#pragma once
/* Host stub: only the HAL types, pins and calls used by button_panel.c. */
#include <stdint.h>
typedef struct { uint32_t IDR; } GPIO_TypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinState;
typedef enum { HAL_OK = 0, HAL_ERROR } HAL_StatusTypeDef;
typedef struct { int dummy; } CAN_HandleTypeDef;
typedef struct { int dummy; } ADC_HandleTypeDef;
typedef struct { uint32_t StdId, ExtId, IDE, RTR, DLC, TransmitGlobalTime; } CAN_TxHeaderTypeDef;
#define CAN_RTR_DATA 0U
#define CAN_ID_STD 0U
#define DISABLE 0U
extern GPIO_TypeDef GA, GB;
#define KEY1_GPIO_Port (&GB)
#define KEY1_Pin 0x1000U
#define KEY2_GPIO_Port (&GB)
#define KEY2_Pin 0x2000U
#define S1_GPIO_Port (&GA)
#define S1_Pin 0x40U
#define S2_GPIO_Port (&GA)
#define S2_Pin 0x80U
#define S3_GPIO_Port (&GB)
#define S3_Pin 0x1U
#define S4_GPIO_Port (&GB)
#define S4_Pin 0x2U
#define LED1_GPIO_Port (&GA)
#define LED1_Pin 0x10U
#define LED2_GPIO_Port (&GA)
#define LED2_Pin 0x20U
uint32_t HAL_GetTick(void);
void HAL_GPIO_WritePin(GPIO_TypeDef*, uint16_t, GPIO_PinState);
void HAL_GPIO_TogglePin(GPIO_TypeDef*, uint16_t);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef*, uint32_t);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef*);
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef*, CAN_TxHeaderTypeDef*, uint8_t*, uint32_t*);
