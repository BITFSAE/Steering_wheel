/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include "stdio.h"
#include "string.h"
#include "ecu_info.h"
#include "delay.h"
#include "can.h"

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = RS485_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_RX_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RS485_TX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(RS485_TX_GPIO_Port, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_RX Init */
    hdma_usart1_rx.Instance = DMA1_Channel5;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_NORMAL;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, RS485_RX_Pin|RS485_TX_Pin);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void send(char sign, int number)
{
	char buff[30];
	sprintf(buff, "%d\n", number);

	/* UART2 (Bluetooth): send sign + value */
	HAL_UART_Transmit(&huart2, (uint8_t*)&sign, 1, HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)buff, strlen(buff), HAL_MAX_DELAY);

	/* UART1 (RS485): set TX mode, send label + value */
	HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_SET);
	delay_us(20); /* RS485 TX enable settling */

	switch(sign){
	case'A':{HAL_UART_Transmit(&huart1,(uint8_t*)"speed:",6, HAL_MAX_DELAY);break;}
	case'B':{HAL_UART_Transmit(&huart1,(uint8_t*)"SOC:",4, HAL_MAX_DELAY);break;}
	case'C':{HAL_UART_Transmit(&huart1,(uint8_t*)"Sum_Voltage:",12, HAL_MAX_DELAY);break;}
	case'D':{HAL_UART_Transmit(&huart1,(uint8_t*)"Top_Temperature:",16, HAL_MAX_DELAY);break;}
	case'E':{HAL_UART_Transmit(&huart1,(uint8_t*)"Sum_I:",6, HAL_MAX_DELAY);break;}
	case'F':{HAL_UART_Transmit(&huart1,(uint8_t*)"Mode_Index:",11, HAL_MAX_DELAY);break;}
	case'G':{HAL_UART_Transmit(&huart1,(uint8_t*)"Sum_Voltage:",12, HAL_MAX_DELAY);break;}
	case'H':{HAL_UART_Transmit(&huart1,(uint8_t*)"Top_Temperature:",16, HAL_MAX_DELAY);break;}
	case'I':{HAL_UART_Transmit(&huart1,(uint8_t*)"Sum_I:",6, HAL_MAX_DELAY);break;}
	case'R':{HAL_UART_Transmit(&huart1,(uint8_t*)"RPM[]:",6, HAL_MAX_DELAY);break;}
	case'T':{HAL_UART_Transmit(&huart1,(uint8_t*)"Torque[]:",8, HAL_MAX_DELAY);break;}

	}
	HAL_UART_Transmit(&huart1, (uint8_t*)buff, strlen(buff), HAL_MAX_DELAY);
	delay_us(20); /* wait for last byte to shift out */

	HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_RESET);
}
/**
 * @brief  发送数组数据 (UART2+UART1), 用于 proto 中 repeated 字段
 */
void send_array(char sign, const int32_t *arr, int count)
{
	char buff[30];
	HAL_UART_Transmit(&huart2, (uint8_t*)&sign, 1, HAL_MAX_DELAY);
	for (int i = 0; i < count; i++) {
		sprintf(buff, "%ld", (long)arr[i]);
		HAL_UART_Transmit(&huart2, (uint8_t*)buff, strlen(buff), HAL_MAX_DELAY);
		if (i < count - 1) {
			HAL_UART_Transmit(&huart2, (uint8_t*)",", 1, HAL_MAX_DELAY);
		}
	}
	HAL_UART_Transmit(&huart2, (uint8_t*)"\n", 1, HAL_MAX_DELAY);
	/* RS485 端发送: 标签 + 所有值 */
	HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_SET);
	delay_us(20); /* RS485 TX enable settling */
	sprintf(buff, "%c[%d]:", sign, count);
	HAL_UART_Transmit(&huart1, (uint8_t*)buff, strlen(buff), HAL_MAX_DELAY);
	for (int i = 0; i < count; i++) {
		sprintf(buff, i < count - 1 ? "%ld," : "%ld\n", (long)arr[i]);
		HAL_UART_Transmit(&huart1, (uint8_t*)buff, strlen(buff), HAL_MAX_DELAY);
	}
	delay_us(20); /* wait for last byte to shift out */
	HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_RESET);
}
/**
 * @brief   UART receive complete callback function
 * @param   huart: UART handle
 * @retval  None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

    // HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);
    /* === USART1: LORA mode command ("M" + 1 data byte, \\n delimiter) === */
    if (huart->Instance == USART1)
    {
        uint8_t ch = ECU_INFO.lora_rx_byte[0];

        // HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_4);

        if (ch == 0x0A)
        {
            if (ECU_INFO.lora_rx_sta >= 2U && ECU_INFO.lora_rx_buf[0] == 'M')
            {
                ECU_INFO.mode_sent[0] = ECU_INFO.lora_rx_buf[1];
                for (int i = 1; i < 8; i++) ECU_INFO.mode_sent[i] = 0;
                HAL_UART_Transmit(&huart1, (uint8_t *)"receive_M!\r\n", 12, 100);
                User_CAN_Send_sq(0x310, ECU_INFO.mode_sent);
            }

            ECU_INFO.lora_rx_sta = 0;
        }
        else
        {
            if (ECU_INFO.lora_rx_sta >= LORA_REC_LEN)
            {
                ECU_INFO.lora_rx_sta = 0;
            }
            else
            {
                ECU_INFO.lora_rx_buf[ECU_INFO.lora_rx_sta++] = ch;
            }
        }

        ECU_INFO.lora_rx_byte[0] = 0;
        ch  = 0;
        HAL_UART_Receive_IT(&huart1, (uint8_t *)ECU_INFO.lora_rx_byte, RXBUFFERSIZE);
        return;
    }

    /* === USART2: Bluetooth AT command (\\r\\n framing) === */
    if (huart->Instance == USART2)
    {
        if ((ECU_INFO.g_usart_rx_sta & 0x8000) == 0)
        {
            if (ECU_INFO.g_usart_rx_sta & 0x4000)
            {
                if (ECU_INFO.g_rx_buffer[0] != 0x0a)
                {
                    ECU_INFO.g_usart_rx_sta = 0;
                }
                else
                {
                    ECU_INFO.g_usart_rx_sta |= 0x8000;
                }
            }
            else
            {
                if (ECU_INFO.g_rx_buffer[0] == 0x0d)
                {
                    ECU_INFO.g_usart_rx_sta |= 0x4000;
                }
                else
                {
                    ECU_INFO.g_usart_rx_buf[ECU_INFO.g_usart_rx_sta & 0x3FFF] = ECU_INFO.g_rx_buffer[0];
                    ECU_INFO.g_usart_rx_sta++;
                    if (ECU_INFO.g_usart_rx_sta > (USART_REC_LEN - 1))
                    {
                        ECU_INFO.g_usart_rx_sta = 0;
                    }
                }
            }
        }
        HAL_UART_Receive_IT(&huart2, (uint8_t *)ECU_INFO.g_rx_buffer, RXBUFFERSIZE);
    }
}

/**
 * @brief   USART interrupt handler
 * @param   None
 * @retval  None
 */
void USART_UX_IRQHandler(void)
{
#if SYS_SUPPORT_OS // Use OS
    OSIntEnter();
#endif

    HAL_UART_IRQHandler(&huart1); // Call HAL UART interrupt handler

#if SYS_SUPPORT_OS // Use OS
    OSIntExit();
#endif

}

/* Redirect printf to UART1, requires MicroLIB */

#if 1

#if (__ARMCC_VERSION >= 6010050) // Use AC6
__asm(".global __use_no_semihosting\n\t"); // Disable semihosting
__asm(".global __ARM_use_no_argv \n\t"); // AC6 requires main to be in C style, not bare metal

#else
/* Use AC5: need to define __FILE to disable semihosting mode */

struct __FILE
{
    int handle;
    // File handle structure for semihosting redirection
};

#endif

/* Disable semihosting: need to implement _ttywrch, _sys_exit, _sys_command_string for both AC6 and AC5 */
int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

/* Disable _sys_exit() for semihosting mode */
void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

// FILE struct defined in stdio.h
FILE __stdout;

// MDK requires fputc function; printf redirects through fputc
int fputc(int ch, FILE *f)
{
    while ((USART1->SR & 0X40) == 0); // Wait for TX buffer empty
    USART1->DR = (uint8_t)ch; // Send character to DR register
    return ch;
}
#endif
/* USER CODE END 1 */

