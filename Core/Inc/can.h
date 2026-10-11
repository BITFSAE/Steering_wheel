/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.h
  * @brief   This file contains all the function prototypes for
  *          the can.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#ifndef __CAN_H__
#define __CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern CAN_HandleTypeDef hcan;

/* USER CODE BEGIN Private defines */

// CAN������ر���
extern CAN_TxHeaderTypeDef TxHeader;   //�洢����֡�ṹ��Ϣ
extern uint8_t CAN_TxData[8];           //�洢���ݶ�
extern uint32_t CAN_TxMail;             //���ķŵ�����id
extern uint32_t CAN_ID;                 //���ı�ʶ���ֶΣ�id��

extern CAN_RxHeaderTypeDef RxHeader;   //����֡�ṹ
extern uint8_t CAN_RxData[8];           //���ݴ洢

/* USER CODE END Private defines */

void MX_CAN_Init(void);

/* USER CODE BEGIN Prototypes */

void CAN_Filter_Config(void);
void User_CAN_Send(void);
void User_CAN_Send_sq(uint32_t CAN_ID_NEW, uint8_t* CAN_TxData_NEW);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __CAN_H__ */

