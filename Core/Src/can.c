/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
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
/* Includes ------------------------------------------------------------------*/
#include "can.h"

/* USER CODE BEGIN 0 */
#include "ecu_info.h"
#include <stdio.h>

CAN_TxHeaderTypeDef TxHeader;
uint8_t CAN_TxData[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };
uint32_t CAN_TxMail;
uint32_t CAN_ID = 0x102;

CAN_RxHeaderTypeDef RxHeader;
uint8_t CAN_RxData[8] = { 0 };

/* USER CODE END 0 */

CAN_HandleTypeDef hcan;

/* CAN init function */
void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 4;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_14TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = ENABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */

}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN GPIO Configuration
    PA11     ------> CAN_RX
    PA12     ------> CAN_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN GPIO Configuration
    PA11     ------> CAN_RX
    PA12     ------> CAN_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
// ������յı���ID
//shaoqi_change
/* ID 列表模式需要 4 项：0x50(已停用的 IMU 原始帧)不再接收，第 4 项以 0x502 重复占位 */
uint16_t CAN_RX_MSG_ID[4] = {0x401, 0x501, 0x502, 0x502};

/*
 * @func: ����CAN���˲�
 *
 */
void CAN_Filter_Config(void)
{
	CAN_FilterTypeDef CAN_FilterInitStructure = {
			.FilterActivation = ENABLE,
			.FilterBank = 0x00,
			.FilterFIFOAssignment = CAN_FILTER_FIFO0,
			.FilterIdHigh = CAN_RX_MSG_ID[0]<<5,
			.FilterIdLow = CAN_RX_MSG_ID[1]<<5,
			.FilterMaskIdHigh = CAN_RX_MSG_ID[2]<<5,
			.FilterMaskIdLow = CAN_RX_MSG_ID[3]<<5,
			.FilterMode = CAN_FILTERMODE_IDLIST,
			.FilterScale = CAN_FILTERSCALE_16BIT,
			.SlaveStartFilterBank = 0
	};
	HAL_CAN_ConfigFilter(&hcan, &CAN_FilterInitStructure);

	/* Second filter bank for 0x503 and 0x504 motor error frames */
	uint16_t motor_err_ids[4] = {0x503, 0x504, 0x503, 0x504};
	CAN_FilterInitStructure.FilterBank = 0x01;
	CAN_FilterInitStructure.FilterIdHigh = motor_err_ids[0]<<5;
	CAN_FilterInitStructure.FilterIdLow = motor_err_ids[1]<<5;
	CAN_FilterInitStructure.FilterMaskIdHigh = motor_err_ids[2]<<5;
	CAN_FilterInitStructure.FilterMaskIdLow = motor_err_ids[3]<<5;
	HAL_CAN_ConfigFilter(&hcan, &CAN_FilterInitStructure);
}

/*
 * @func: CAN���ķ���[��׼��ʽ������֡]
 */
void User_CAN_Send()
{
	TxHeader.RTR = CAN_RTR_DATA;
	TxHeader.IDE = CAN_ID_STD;
	TxHeader.StdId = CAN_ID;
	TxHeader.TransmitGlobalTime = DISABLE;
	TxHeader.DLC = 8;
	if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, CAN_TxData, &CAN_TxMail) != HAL_OK)
	{
		/* All 3 mailboxes full — message dropped */
	}
}

/*
 * @func: CAN���ķ���[��׼��ʽ������֡]
 */
//shaoqi_add
//����ָ��ID������
void User_CAN_Send_sq(uint32_t CAN_ID_NEW,uint8_t* CAN_TxData_NEW)
{
	TxHeader.RTR = CAN_RTR_DATA;
	TxHeader.IDE = CAN_ID_STD;
	TxHeader.StdId = CAN_ID_NEW;
	TxHeader.TransmitGlobalTime = DISABLE;
	TxHeader.DLC = 8;
	if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, CAN_TxData_NEW, &CAN_TxMail) != HAL_OK)
	{
		/* All 3 mailboxes full — message dropped */
	}
}

/*
 * @func: CAN���Ľ����ж�[FIFO0]
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	if(HAL_CAN_GetRxMessage(hcan, CAN_FILTER_FIFO0, &RxHeader, CAN_RxData)!= HAL_OK)
	{
	    // ������
	    Error_Handler();
	}
	(void)RxHeader.StdId; /* printf removed from ISR to avoid blocking */

	//���ݲ�ͬ��ID����ò�ͬ����Ϣ�������뿴���ӵ�CANЭ��
	if(RxHeader.StdId==0x401)
	{
		ECU_INFO.bms.soc_pct        = (uint32_t)CAN_RxData[6];
		ECU_INFO.bms.total_voltage  = CAN_RxData[0] + (uint32_t)(CAN_RxData[1]) * 256;
		ECU_INFO.bms.max_temp_c     = (int32_t)CAN_RxData[7];
		ECU_INFO.bms.total_current  = CAN_RxData[4] + (int32_t)(CAN_RxData[5]) * 256;

		/* error bytes: CAN_RxData[4..7] */
		ECU_INFO.ERRO[0] = CAN_RxData[4];
		ECU_INFO.ERRO[1] = CAN_RxData[5];
		ECU_INFO.ERRO[2] = CAN_RxData[6];
		ECU_INFO.ERRO[3] = CAN_RxData[7];
	}
	if(RxHeader.StdId==0x501)
	{
		ECU_INFO.vehicle.speed_kmh = (uint32_t)CAN_RxData[0];
	}
	if(RxHeader.StdId==0x502)
	{
		/* CAN raw = RPM; power = RPM / 9549 (approx W per unit torque) */
		ECU_INFO.vehicle.motors[0].rpm     = (int32_t)(256 * CAN_RxData[3] + CAN_RxData[2]); /* FL */
		ECU_INFO.vehicle.motors[1].rpm     = (int32_t)(256 * CAN_RxData[7] + CAN_RxData[6]); /* FR */
		ECU_INFO.vehicle.motors[2].rpm     = (int32_t)(256 * CAN_RxData[1] + CAN_RxData[0]); /* RL */
		ECU_INFO.vehicle.motors[3].rpm     = (int32_t)(256 * CAN_RxData[5] + CAN_RxData[4]); /* RR */
		ECU_INFO.vehicle.motors[0].power_w = ECU_INFO.vehicle.motors[0].rpm / 9549;
		ECU_INFO.vehicle.motors[1].power_w = ECU_INFO.vehicle.motors[1].rpm / 9549;
		ECU_INFO.vehicle.motors[2].power_w = ECU_INFO.vehicle.motors[2].rpm / 9549;
		ECU_INFO.vehicle.motors[3].power_w = ECU_INFO.vehicle.motors[3].rpm / 9549;
	}
	if(RxHeader.StdId==0x503)
	{
		ECU_INFO.vehicle.motors[2].motor_error = (int32_t)CAN_RxData[0]; /* RL */
		ECU_INFO.vehicle.motors[3].motor_error = (int32_t)CAN_RxData[4]; /* RR */
	}
	if(RxHeader.StdId==0x504)
	{
		ECU_INFO.vehicle.motors[0].motor_error = (int32_t)CAN_RxData[0]; /* FL */
		ECU_INFO.vehicle.motors[1].motor_error = (int32_t)CAN_RxData[4]; /* FR */
	}
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	/* Called from CAN ISR when bus errors, stuff/form/CRC/ACK errors,
	   error-passive, bus-off, or overrun occur.
	   Keep minimal — just record flags for main-loop diagnostics. */
	uint32_t esr = hcan->Instance->ESR;
	(void)esr;   /* can inspect with debugger when needed */
}

/* USER CODE END 1 */

