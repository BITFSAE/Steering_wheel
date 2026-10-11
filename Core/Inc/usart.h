/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* STM32F1 HAL does not ship stm32f1xx_hal_uart.h – forward-declare the
   UART handle type so extern declarations compile. */
typedef struct __UART_HandleTypeDef UART_HandleTypeDef;

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */
#define USART_REC_LEN 256 /* 最大接收字节数 256 */
#define RXBUFFERSIZE 1 /* 单字节缓冲 */
#define LORA_REC_LEN 200 /* LORA USART1 最大接收字节数 */
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void USART_UX_IRQHandler(void);
void send(char sign, int number);
void send_array(char sign, const int32_t *arr, int count);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

