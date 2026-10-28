/**
 * @file    button_panel.h
 * @brief   Steering wheel button panel module header.
 *          Handles:
 *            - PB12 KEY1: lap-timer start/stop toggle (0x703 LapTimerToggle at release)
 *            - PB13 KEY2: error clear (0x703 ErrorClearActive at release)
 *            - KEY2 held, then KEY1 pressed, both held 300 ms: exit-enable
 *              request to the ECU (0x703 DisableRequest, repeated every 20 ms)
 *            - PA6/PA7/PB0/PB1: 4-position rotary switch → drive mode
 *            - PA1 ADC: 0-3.3V → 8-level slip rate with moving-average filter
 *          Slip level and the three button bits share CAN ID 0x703, which the
 *          Display relays verbatim to the ECU.
 *          Drive mode is sent on CAN ID 0x310 on change.
 */
#ifndef __BUTTON_PANEL_H__
#define __BUTTON_PANEL_H__

#include "main.h"

/* ------------------------------------------------------------------------- */
/* CAN identifiers                                                           */
/* ------------------------------------------------------------------------- */
#define CAN_ID_STEERING_CMD     0x703U   /* SteeringCmd, relayed to ECU by Display */
#define CAN_ID_DRIVE_MODE       0x310U   /* DriveMode_Request (vehicle protocol) */

/* ------------------------------------------------------------------------- */
/* 0x703 SteeringCmd layout (vehicle-interfaces Vehicle_CanB.dbc BO_ 1795)   */
/*   byte0 bit0..2 SlipLevel 0..7                                            */
/*   byte1 bit0 DisableRequest, bit1 ErrorClearActive, bit2 LapTimerToggle    */
/* Byte2 is reserved for a later DriveModeCode merge; DLC stays 2 for now.   */
/* ------------------------------------------------------------------------- */
#define STEERING_CMD_DLC            2U
#define STEERING_CMD_DISABLE_REQ    0x01U
#define STEERING_CMD_ERROR_CLEAR    0x02U
#define STEERING_CMD_LAP_TOGGLE     0x04U

/* Exit-enable chord: KEY2 held first, then KEY1; both held this long fires
 * the request. While it is active, 0x703 goes out every DISABLE_TX_PERIOD_MS
 * with DisableRequest=1, at least DISABLE_MIN_FRAMES times, then once with 0. */
#define CHORD_HOLD_MS               300U
#define DISABLE_TX_PERIOD_MS         20U
#define DISABLE_MIN_FRAMES            3U

/* ------------------------------------------------------------------------- */
/* Drive mode codes (rotary-switch positions)                                */
/*   Internal 0..3 is offset by DRIVE_MODE_ASCII_OFFSET when sent on 0x310   */
/*   to yield ASCII '0'..'3' (48..51).                                       */
/* ------------------------------------------------------------------------- */
#define DRIVE_MODE_S1           0x00U
#define DRIVE_MODE_S2           0x01U
#define DRIVE_MODE_S3           0x02U
#define DRIVE_MODE_S4           0x03U
#define DRIVE_MODE_INVALID      0xFFU

/* Offset applied when sending drive mode on 0x310 (ASCII '0'..'3' = 48..51) */
#define DRIVE_MODE_ASCII_OFFSET 0x30U

/* ------------------------------------------------------------------------- */
/* ADC filter parameters                                                     */
/* ------------------------------------------------------------------------- */
#define ADC_FILTER_DEPTH        16U     /* Moving-average window size        */
#define ADC_SLIP_LEVELS         8U      /* Number of slip-rate steps         */

/* ------------------------------------------------------------------------- */
/* Button scan / debounce parameters                                         */
/* ------------------------------------------------------------------------- */
#define BUTTON_SCAN_PERIOD_MS   10U     /* Scan interval in ms               */
#define BUTTON_DEBOUNCE_CNT      4U     /* Consecutive stable reads needed   */

/* ------------------------------------------------------------------------- */
/* Public API                                                                */
/* ------------------------------------------------------------------------- */
void ButtonPanel_Init(void);
void ButtonPanel_Process(void);

#endif /* __BUTTON_PANEL_H__ */
