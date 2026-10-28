/**
 * @file    button_panel.c
 * @brief   Steering wheel button panel implementation.
 *
 * Signal sources (all active-low with internal pull-up):
 *   PB12  KEY1  – Lap-timer start/stop toggle (TX on release)
 *   PB13  KEY2  – Error-clear trigger (TX on release)
 *   KEY2 held, then KEY1 pressed, both held CHORD_HOLD_MS – exit-enable request
 *   PA6   S1    – Rotary pos.0 → drive mode S
 *   PA7   S2    – Rotary pos.1 → drive mode Q
 *   PB0   S3    – Rotary pos.2 → drive mode C
 *   PB1   S4    – Rotary pos.3 → drive mode E
 *   PA1   ADC1  – Potentiometer → 8-level slip rate
 *
 * CAN output (event-driven):
 *   ID 0x703 — SteeringCmd, DLC 2, every frame carries the current slip level:
 *              sent when the slip level changes;
 *              LapTimerToggle = 1 once when KEY1 is released on its own;
 *              ErrorClearActive = 1 once when KEY2 is released on its own;
 *              DisableRequest = 1 every DISABLE_TX_PERIOD_MS while the chord is
 *              held (at least DISABLE_MIN_FRAMES frames), then one frame with 0.
 *              Once both keys have been down together, neither key sends its
 *              single action on release.
 *   ID 0x310 — drive mode request (vehicle protocol), sent on change
 */
#include "button_panel.h"
#include "can.h"

/* ========================================================================= */
/* Module-local types                                                        */
/* ========================================================================= */
typedef struct {
    uint8_t  stable;            /* 1 = debounced state, 0 = still bouncing    */
    uint8_t  level;             /* current stable level (0=pressed,1=idle)    */
    uint32_t cnt;               /* consecutive consistent-sample counter      */
} btn_debounce_t;

/* ========================================================================= */
/* Module-local variables                                                    */
/* ========================================================================= */
static uint8_t  drive_mode;          /* 0..3 or 0xFF                          */
static uint8_t  slip_level;          /* 0..7 from ADC                         */

/* Last-sent snapshot for change detection */
static uint8_t  last_sent_drive_mode;
static uint8_t  last_sent_slip_level;

/* ADC moving-average filter ring buffer */
static uint16_t adc_ring[ADC_FILTER_DEPTH];
static uint8_t  adc_ring_idx;
static uint8_t  adc_ring_full;
static uint32_t adc_sum;

/* Button debounce state machine (per button) */
static btn_debounce_t db_pb12;
static btn_debounce_t db_pb13;
static btn_debounce_t db_s1;
static btn_debounce_t db_s2;
static btn_debounce_t db_s3;
static btn_debounce_t db_s4;

/* Two-key handling. A key that was down together with the other one is
 * "consumed": its release sends nothing, so the chord never also toggles the
 * lap timer or clears alerts. */
static uint8_t  key1_consumed;
static uint8_t  key2_consumed;
static uint8_t  chord_armed;          /* KEY2 was down before KEY1 went down  */
static uint32_t chord_start_ms;
static uint8_t  disable_active;       /* DisableRequest frames being sent     */
static uint8_t  disable_frames;       /* frames sent in the current request   */
static uint32_t last_disable_tx_ms;

/* Timing book-keeping */
static uint32_t last_button_scan_ms;
static uint32_t last_led2_toggle_ms;
static uint32_t led1_off_until_ms;    /* LED1 restore timestamp (active-low)  */

/* External references (can.c globals) */
extern CAN_HandleTypeDef hcan;
extern CAN_TxHeaderTypeDef  TxHeader;
extern ADC_HandleTypeDef    hadc1;

/* ========================================================================= */
/* Static helper prototypes                                                  */
/* ========================================================================= */
static void      ADC_Read_And_Filter(void);
static void      Scan_Buttons(uint32_t now);
static void      Update_Disable_Request(uint32_t now);
static void      Update_Drive_Mode(void);
static void      Send_CAN_SteeringCmd(uint8_t event_bits);
static void      Send_CAN_DriveMode(void);
static void      Flash_LED1(void);
static uint8_t   Debounce_Sample(GPIO_TypeDef *port, uint16_t pin,
                                 btn_debounce_t *db);

/* ========================================================================= */
/* Public API                                                                */
/* ========================================================================= */

/**
 * @brief  Initialise the button-panel module.
 * @note   Call once after MX_ADC_Init() and CAN filter setup.
 */
void ButtonPanel_Init(void)
{
    /* State variables */
    drive_mode           = DRIVE_MODE_INVALID;
    slip_level           = 0U;

    /* Last-sent snapshot — force first TX by setting to impossible values */
    last_sent_drive_mode = 0xFFU;
    last_sent_slip_level = 0xFFU;

    /* ADC filter ring */
    for (uint8_t i = 0U; i < ADC_FILTER_DEPTH; i++) {
        adc_ring[i] = 0U;
    }
    adc_ring_idx  = 0U;
    adc_ring_full = 0U;
    adc_sum       = 0U;

    /* Debounce state — assume idle (=1 because pull-up, button not pressed) */
    db_pb12.stable = 1U;  db_pb12.level = 1U;  db_pb12.cnt = 0U;
    db_pb13.stable = 1U;  db_pb13.level = 1U;  db_pb13.cnt = 0U;
    db_s1.stable   = 1U;  db_s1.level   = 1U;  db_s1.cnt   = 0U;
    db_s2.stable   = 1U;  db_s2.level   = 1U;  db_s2.cnt   = 0U;
    db_s3.stable   = 1U;  db_s3.level   = 1U;  db_s3.cnt   = 0U;
    db_s4.stable   = 1U;  db_s4.level   = 1U;  db_s4.cnt   = 0U;

    /* Two-key state */
    key1_consumed      = 0U;
    key2_consumed      = 0U;
    chord_armed        = 0U;
    chord_start_ms     = 0U;
    disable_active     = 0U;
    disable_frames     = 0U;
    last_disable_tx_ms = 0U;

    /* Timing */
    last_button_scan_ms = HAL_GetTick();
    last_led2_toggle_ms = HAL_GetTick();
    led1_off_until_ms   = 0U;

    /* LEDs off (active-low: high = off) */
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

    /* Prime the ADC filter with a few initial readings */
    for (uint8_t i = 0U; i < ADC_FILTER_DEPTH; i++) {
        ADC_Read_And_Filter();
    }

    /* Immediately read initial drive mode */
    Update_Drive_Mode();
}

/**
 * @brief  Periodic process — call from main loop as fast as possible.
 *         Sub-tasks are rate-limited internally.
 *         CAN messages are sent only when data has changed.
 */
void ButtonPanel_Process(void)
{
    uint32_t now = HAL_GetTick();

    /* ---- LED1 flash restore: turn off after 100 ms low pulse ---- */
    if ((led1_off_until_ms > 0U) && (now >= led1_off_until_ms)) {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        led1_off_until_ms = 0U;
    }

    /* ---- LED2 1 Hz blink (toggle every 500 ms) ---- */
    if ((now - last_led2_toggle_ms) >= 500U) {
        last_led2_toggle_ms = now;
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
    }

    /* ---- Button scan – every BUTTON_SCAN_PERIOD_MS ---- */
    if ((now - last_button_scan_ms) >= BUTTON_SCAN_PERIOD_MS) {
        last_button_scan_ms = now;
        Scan_Buttons(now);     /* release edge → lap-timer / error-clear TX */
        Update_Disable_Request(now);
        Update_Drive_Mode();
        ADC_Read_And_Filter();

        /* ---- Slip level change detection → 0x703 ---- */
        if (slip_level != last_sent_slip_level) {
            last_sent_slip_level = slip_level;
            Send_CAN_SteeringCmd(0U);
            Flash_LED1();
        }

        /* ---- Drive mode change detection → 0x310 ---- */
        if (drive_mode != last_sent_drive_mode) {
            last_sent_drive_mode = drive_mode;
            Send_CAN_DriveMode();
            Flash_LED1();
        }
    }
}

/* ========================================================================= */
/* Static helpers                                                            */
/* ========================================================================= */

/**
 * @brief  Software-trigger one ADC conversion, read the value, and feed the
 *         moving-average ring buffer.  The averaged result is stored in
 *         slip_level (0..7).
 */
static void ADC_Read_And_Filter(void)
{
    uint32_t filtered;
    uint16_t raw;

    HAL_ADC_Start(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 1U) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return;
    }

    raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    /* ---- Moving-average ring buffer ---- */
    adc_sum -= adc_ring[adc_ring_idx];
    adc_ring[adc_ring_idx] = raw;
    adc_sum += raw;
    adc_ring_idx++;
    if (adc_ring_idx >= ADC_FILTER_DEPTH) {
        adc_ring_idx  = 0U;
        adc_ring_full = 1U;
    }

    uint8_t count = adc_ring_full ? ADC_FILTER_DEPTH : adc_ring_idx;
    if (count == 0U) { count = 1U; }
    filtered = adc_sum / count;

    /* Quantise: 12-bit ADC (0..4095) → 8 levels (0..7) */
    slip_level = (uint8_t)((filtered * ADC_SLIP_LEVELS) / 4096U);
}

/**
 * @brief  Scan the two push-buttons with software debounce.
 *
 *   Release of a key that was pressed on its own (level 0→1):
 *     PB12 KEY1 → 0x703 LapTimerToggle = 1
 *     PB13 KEY2 → 0x703 ErrorClearActive = 1
 *   Each press sends exactly one frame; no repeat while held, no 0 frame.
 *
 *   KEY1 going down while KEY2 is already down arms the exit-enable chord
 *   (see Update_Disable_Request). KEY2 going down while KEY1 is down, or both
 *   in the same scan, is the wrong order: nothing fires. Either way both keys
 *   are consumed until they are released.
 */
static void Scan_Buttons(uint32_t now)
{
    uint8_t pb12_old = db_pb12.level;
    uint8_t pb13_old = db_pb13.level;

    uint8_t pb12_new = Debounce_Sample(KEY1_GPIO_Port, KEY1_Pin, &db_pb12);
    uint8_t pb13_new = Debounce_Sample(KEY2_GPIO_Port, KEY2_Pin, &db_pb13);

    /* Press edge = level 1 (idle, pull-up) → 0 (pressed) */
    if (pb12_old == 1U && pb12_new == 0U && pb13_new == 0U) {
        key1_consumed = 1U;
        key2_consumed = 1U;
        if (pb13_old == 0U) {
            chord_armed    = 1U;
            chord_start_ms = now;
        }
    }
    if (pb13_old == 1U && pb13_new == 0U && pb12_new == 0U) {
        key1_consumed = 1U;
        key2_consumed = 1U;
    }
    if (pb12_new != 0U || pb13_new != 0U) {
        chord_armed = 0U;   /* either key up: the chord is broken */
    }

    /* Release edge = level 0 (pressed) → 1 (idle) */
    if (pb12_old == 0U && pb12_new == 1U) {
        if (key1_consumed != 0U) {
            key1_consumed = 0U;
        } else {
            Send_CAN_SteeringCmd(STEERING_CMD_LAP_TOGGLE);
            Flash_LED1();
        }
    }
    if (pb13_old == 0U && pb13_new == 1U) {
        if (key2_consumed != 0U) {
            key2_consumed = 0U;
        } else {
            Send_CAN_SteeringCmd(STEERING_CMD_ERROR_CLEAR);
            Flash_LED1();
        }
    }
}

/**
 * @brief  Drive the exit-enable request (0x703 DisableRequest).
 *
 *   Fires once the armed chord has been held CHORD_HOLD_MS. While active, a
 *   frame with DisableRequest = 1 goes out every DISABLE_TX_PERIOD_MS until
 *   either key is released and at least DISABLE_MIN_FRAMES were sent, so a
 *   single lost frame does not lose the request. A final frame with 0 marks
 *   the end. Re-firing needs KEY1 to be released and pressed again.
 */
static void Update_Disable_Request(uint32_t now)
{
    uint8_t both_down = (uint8_t)((db_pb12.level == 0U) && (db_pb13.level == 0U));

    if ((chord_armed != 0U) && ((now - chord_start_ms) >= CHORD_HOLD_MS)) {
        chord_armed        = 0U;
        disable_active     = 1U;
        disable_frames     = 0U;
        last_disable_tx_ms = now - DISABLE_TX_PERIOD_MS;  /* send right away */
    }

    if (disable_active == 0U) {
        return;
    }

    if ((both_down == 0U) && (disable_frames >= DISABLE_MIN_FRAMES)) {
        disable_active = 0U;
        Send_CAN_SteeringCmd(0U);   /* DisableRequest back to 0 */
        return;
    }

    if ((now - last_disable_tx_ms) >= DISABLE_TX_PERIOD_MS) {
        last_disable_tx_ms = now;
        Send_CAN_SteeringCmd(0U);   /* DisableRequest added while active */
        if (disable_frames < 0xFFU) {
            disable_frames++;
        }
        Flash_LED1();
    }
}

/**
 * @brief  Debounce a single button input.
 * @return Current stable level (0 or 1).
 */
static uint8_t Debounce_Sample(GPIO_TypeDef *port, uint16_t pin,
                               btn_debounce_t *db)
{
    uint8_t raw = (uint8_t)((port->IDR & pin) ? 1U : 0U);

    if (raw == db->level) {
        db->cnt = 0U;
    } else {
        db->cnt++;
        if (db->cnt >= BUTTON_DEBOUNCE_CNT) {
            db->level  = raw;
            db->cnt    = 0U;
            db->stable = 1U;
        }
    }
    return db->level;
}

/**
 * @brief  Read the 4-position rotary switch and update drive_mode.
 *
 *         S1=PA6, S2=PA7, S3=PB0, S4=PB1  (all active-low with pull-up)
 */
static void Update_Drive_Mode(void)
{
    Debounce_Sample(S1_GPIO_Port, S1_Pin, &db_s1);
    Debounce_Sample(S2_GPIO_Port, S2_Pin, &db_s2);
    Debounce_Sample(S3_GPIO_Port, S3_Pin, &db_s3);
    Debounce_Sample(S4_GPIO_Port, S4_Pin, &db_s4);

    uint8_t s1 = db_s1.level;
    uint8_t s2 = db_s2.level;
    uint8_t s3 = db_s3.level;
    uint8_t s4 = db_s4.level;

    uint8_t active_cnt = (uint8_t)((s1 == 0U) + (s2 == 0U)
                                 + (s3 == 0U) + (s4 == 0U));

    if (active_cnt == 1U) {
        if      (s1 == 0U) { drive_mode = DRIVE_MODE_S1; }
        else if (s2 == 0U) { drive_mode = DRIVE_MODE_S2; }
        else if (s3 == 0U) { drive_mode = DRIVE_MODE_S3; }
        else               { drive_mode = DRIVE_MODE_S4; }
    } else {
        drive_mode = DRIVE_MODE_INVALID;
    }
}

/**
 * @brief  Send 0x703 SteeringCmd with the current slip level.
 * @param  event_bits  STEERING_CMD_LAP_TOGGLE / STEERING_CMD_ERROR_CLEAR for
 *                     a one-shot key frame, 0 otherwise. DisableRequest is
 *                     added here whenever the request is active, so a slip or
 *                     key frame sent in between never shows it as 0.
 */
static void Send_CAN_SteeringCmd(uint8_t event_bits)
{
    uint8_t data[STEERING_CMD_DLC];

    data[0] = (uint8_t)(slip_level & 0x07U);
    data[1] = event_bits;
    if (disable_active != 0U) {
        data[1] |= STEERING_CMD_DISABLE_REQ;
    }

    TxHeader.RTR                = CAN_RTR_DATA;
    TxHeader.IDE                = CAN_ID_STD;
    TxHeader.StdId              = CAN_ID_STEERING_CMD;
    TxHeader.TransmitGlobalTime = DISABLE;
    TxHeader.DLC                = STEERING_CMD_DLC;
    uint32_t mail;
    HAL_CAN_AddTxMessage(&hcan, &TxHeader, data, &mail);
}

/**
 * @brief  Flash LED1 with a brief low pulse (100 ms).
 */
static void Flash_LED1(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    led1_off_until_ms = HAL_GetTick() + 100U;
}

/**
 * @brief  Send drive mode on CAN ID 0x310 (vehicle DriveMode_Request).
 *         Encodes internal 0..3 as ASCII '0'..'3' (48..51).
 */
static void Send_CAN_DriveMode(void)
{
    uint8_t data[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    data[0] = (drive_mode <= DRIVE_MODE_S4)
                ? (uint8_t)(drive_mode + DRIVE_MODE_ASCII_OFFSET)
                : DRIVE_MODE_INVALID;

    TxHeader.RTR                = CAN_RTR_DATA;
    TxHeader.IDE                = CAN_ID_STD;
    TxHeader.StdId              = CAN_ID_DRIVE_MODE;
    TxHeader.TransmitGlobalTime = DISABLE;
    TxHeader.DLC                = 8U;
    uint32_t mail;
    HAL_CAN_AddTxMessage(&hcan, &TxHeader, data, &mail);
}
