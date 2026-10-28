/* 主机单元测试：用 HAL 桩运行 button_panel.c，核对 0x703 的单键与退使组合键行为。
 * 每次 run(ms) 推进 1 ms 节拍并调用 ButtonPanel_Process()。 */
#include <assert.h>
#include <stdio.h>
#include "button_panel.h"

GPIO_TypeDef GA = {0xFFFFU}, GB = {0xFFFFU};
CAN_HandleTypeDef hcan;
CAN_TxHeaderTypeDef TxHeader;
ADC_HandleTypeDef hadc1;

static uint32_t tick;
static uint32_t adc = 2600U;   /* 2600 * 8 / 4096 = 滑级 5 */
typedef struct { uint32_t t, id; uint8_t d0, d1, dlc; } tx_t;
static tx_t txs[1024];
static int ntx;

uint32_t HAL_GetTick(void) { return tick; }
void HAL_GPIO_WritePin(GPIO_TypeDef *p, uint16_t pin, GPIO_PinState s) { (void)p; (void)pin; (void)s; }
void HAL_GPIO_TogglePin(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; }
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *h, uint32_t t) { (void)h; (void)t; return HAL_OK; }
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h) { (void)h; return adc; }
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *h, CAN_TxHeaderTypeDef *hd, uint8_t *d, uint32_t *m)
{
    (void)h; (void)m;
    assert(ntx < (int)(sizeof(txs) / sizeof(txs[0])));
    txs[ntx++] = (tx_t){ tick, hd->StdId, d[0], (hd->DLC > 1U) ? d[1] : 0U, (uint8_t)hd->DLC };
    return HAL_OK;
}

static void key(int k, int down)
{
    uint32_t pin = (k == 1) ? KEY1_Pin : KEY2_Pin;
    if (down) { GB.IDR &= ~pin; } else { GB.IDR |= pin; }
}
static void run(uint32_t ms) { for (uint32_t i = 0U; i < ms; i++) { tick++; ButtonPanel_Process(); } }
static int count_bits(uint8_t mask, uint8_t val, int from)
{
    int c = 0;
    for (int i = from; i < ntx; i++) { if (txs[i].id == CAN_ID_STEERING_CMD && (txs[i].d1 & mask) == val) c++; }
    return c;
}
static int request_edges(int from)
{
    int e = 0;
    for (int i = from; i < ntx; i++) {
        if ((txs[i].d1 & STEERING_CMD_DISABLE_REQ) && (i == from || !(txs[i - 1].d1 & STEERING_CMD_DISABLE_REQ))) e++;
    }
    return e;
}

int main(void)
{
    const uint8_t singles = STEERING_CMD_LAP_TOGGLE | STEERING_CMD_ERROR_CLEAR;
    int n;

    /* 上电：一帧 0x703 带当前滑级，一帧 0x310；不再出现 0x700..0x702。 */
    GA.IDR &= ~S1_Pin;
    ButtonPanel_Init();
    run(200);
    assert(ntx == 2 && txs[0].id == 0x703U && txs[0].d0 == 5U && txs[0].d1 == 0U && txs[0].dlc == 2U);
    assert(txs[1].id == 0x310U);

    /* 单按 KEY1：按住不发，松开发一帧 LapTimerToggle。 */
    n = ntx; key(1, 1); run(100); assert(ntx == n);
    key(1, 0); run(100);
    assert(ntx == n + 1 && txs[n].d1 == STEERING_CMD_LAP_TOGGLE && txs[n].d0 == 5U);

    /* 单按 KEY2：松开发一帧 ErrorClearActive。 */
    n = ntx; key(2, 1); run(100); key(2, 0); run(100);
    assert(ntx == n + 1 && txs[n].d1 == STEERING_CMD_ERROR_CLEAR);

    /* 组合键：先 KEY2 后 KEY1，按住 1 s。去抖 40 ms + 保持 300 ms 后开始，每 20 ms 一帧。 */
    n = ntx; key(2, 1); run(100);
    uint32_t t_k1 = tick;
    key(1, 1); run(1000);
    int ones = count_bits(STEERING_CMD_DISABLE_REQ, STEERING_CMD_DISABLE_REQ, n);
    uint32_t first = 0U;
    for (int i = n; i < ntx; i++) { if (txs[i].d1 & STEERING_CMD_DISABLE_REQ) { first = txs[i].t; break; } }
    assert(first - t_k1 >= CHORD_HOLD_MS && first - t_k1 <= CHORD_HOLD_MS + 60U);
    assert(ones >= 30 && ones <= 36);
    for (int i = n + 1; i < ntx; i++) { assert(txs[i].t - txs[i - 1].t >= DISABLE_TX_PERIOD_MS); }
    key(1, 0); key(2, 0); run(200);
    assert(count_bits(singles, 0U, n) == ntx - n);   /* 组合键不触发计时和清错 */
    assert(txs[ntx - 1].d1 == 0U);                    /* 结束时补一帧 0 */
    printf("chord held 1 s: %d request frames, first %u ms after KEY1 press\n", ones, (unsigned)(first - t_k1));

    /* 保持不足 300 ms 就松开：什么都不发。 */
    n = ntx; key(2, 1); run(100); key(1, 1); run(200); key(1, 0); key(2, 0); run(200);
    assert(ntx == n);

    /* 顺序反了（先 KEY1 后 KEY2）：什么都不发。 */
    n = ntx; key(1, 1); run(100); key(2, 1); run(1000); key(2, 0); key(1, 0); run(200);
    assert(ntx == n);

    /* 同一扫描周期内同时按下：不算先 KEY2，什么都不发。 */
    n = ntx; key(1, 1); key(2, 1); run(1000); key(1, 0); key(2, 0); run(200);
    assert(ntx == n);

    /* 刚过门限就松开：仍至少发 3 帧请求，再补一帧 0。 */
    n = ntx; key(2, 1); run(100); key(1, 1); run(345); key(1, 0); run(200); key(2, 0); run(200);
    assert(count_bits(STEERING_CMD_DISABLE_REQ, STEERING_CMD_DISABLE_REQ, n) >= (int)DISABLE_MIN_FRAMES);
    assert(txs[ntx - 1].d1 == 0U && count_bits(singles, 0U, n) == ntx - n);

    /* KEY2 一直按住，KEY1 松开再按：第二次请求重新触发。 */
    n = ntx; key(2, 1); run(100);
    key(1, 1); run(400); key(1, 0); run(200);
    key(1, 1); run(400); key(1, 0); key(2, 0); run(200);
    assert(request_edges(n) == 2 && count_bits(singles, 0U, n) == ntx - n);

    /* 请求期间滑级变化：变化帧也带 DisableRequest=1，不会打断连续的 1。 */
    n = ntx; key(2, 1); run(100); key(1, 1); run(400);
    adc = 600U;   /* 滑级 1 */
    run(300);
    for (int i = n; i < ntx; i++) {
        if (txs[i].d0 == 1U) { assert(txs[i].d1 & STEERING_CMD_DISABLE_REQ); break; }
    }
    assert(request_edges(n) == 1);
    key(1, 0); key(2, 0); run(200);
    assert(txs[ntx - 1].d1 == 0U && txs[ntx - 1].d0 == 1U);

    /* 之后单键仍然正常。 */
    n = ntx; key(1, 1); run(100); key(1, 0); run(100);
    assert(ntx == n + 1 && txs[n].d1 == STEERING_CMD_LAP_TOGGLE && txs[n].d0 == 1U);

    puts("button_panel host tests passed");
    return 0;
}
