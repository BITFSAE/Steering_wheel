# SteeringWheel_Button 方向盘按钮板固件

BIT FSAE 方向盘按钮板固件，与车手屏（HMI_Display）配套使用。板上读取旋钮、电位器和两个按键，把操作以 CAN 报文发给车手屏。方向盘与车手屏如何分工、各控件最终起什么作用，见车手屏仓库的 [方向盘与车手屏](../HMI_Display/docs/steering_wheel.md)，本文只写方向盘板自身的硬件、帧和代码行为。

- 旋钮（4 位）选择驾驶模式，档位变化时发送 0x310 DriveMode_Request。
- 电位器设定滑移率等级，共 8 级（0～7）。滑级和两个按键共用一帧 0x703 SteeringCmd，车手屏原样转发给 ECU。
- KEY1 是圈速计时开关，单独按下松开时 0x703 的 `LapTimerToggle` 置 1，车手屏据此开始或停止圈速/冲刺计时；KEY2 是消错，单独按下松开时 `ErrorClearActive` 置 1。
- 退使组合键：先按住 KEY2，再按下 KEY1，两键一起按住 300 ms 后，0x703 的 `DisableRequest` 置 1，向 ECU 请求退出使能。

微控制器为 STM32F103C8T6，裸机轮询，无 RTOS。CAN 位率 500 kbit/s。

## 硬件

| 项目 | 参数 |
| --- | --- |
| MCU | STM32F103C8T6（Cortex-M3，64 KB Flash，20 KB RAM） |
| 系统时钟 | HSE 8 MHz × 9 = 72 MHz，APB1 = 36 MHz |
| CAN | bxCAN1，500 kbit/s（36 MHz ÷ 4 分频 ÷ 18 tq） |
| 调试口 | SWD（PA13/PA14） |

### 引脚分配

引脚定义在 `Core/Inc/main.h`，源头是 CubeMX 工程 `SteeringWheel_Button.ioc`。输入信号均为低有效、内部上拉。

| 引脚 | 标号 | 方向 | 功能 |
| --- | --- | --- | --- |
| PA1 | ADC1_IN1 | 输入 | 电位器，0～3.3 V，量化为滑移等级 0..7 |
| PA4 | LED1 | 输出 | CAN 事件指示，事件发送时拉低 100 ms 点亮 |
| PA5 | LED2 | 输出 | 心跳灯，每 500 ms 翻转（1 Hz） |
| PA6、PA7、PB0、PB1 | S1、S2、S3、S4 | 输入 | 4 位旋钮 |
| PA11、PA12 | CAN_RX、CAN_TX | — | CAN1 收发器接口 |
| PB12 | KEY1 | 输入 | 圈速计时开关（开始/停止）；退使组合键的第二个键 |
| PB13 | KEY2 | 输入 | 消错（清除车手屏上的全部告警）；退使组合键的第一个键 |
| PB2、PB11、PB14、PB15 | S5、KEY3、KEY4 | 输入 | 预留，固件未使用 |

## CAN 接口

### 发送

CAN ID 和 0x703 的位定义在 `Core/Inc/button_panel.h`，与中央库 `vehicle-interfaces` 的 `Vehicle_CanB.dbc`（BO_ 1795、BO_ 784）一致。两帧都是事件驱动，没有周期发送。

| ID | 帧名 | DLC | 触发条件 | 载荷 |
| --- | --- | --- | --- | --- |
| 0x703 | SteeringCmd | 2 | 滑级变化、单键松开、退使组合键 | byte0 bit0..2 = 滑级 0..7；byte1 bit0 `DisableRequest`、bit1 `ErrorClearActive`、bit2 `LapTimerToggle`，其余位为 0 |
| 0x310 | DriveMode_Request | 8 | 旋钮档位变化 | byte0 = '0'..'3'（48..51，对应旋钮 S1..S4），档位无效时 0xFF；byte1..7 为 0 |

0x703 的发送规则：

- 每帧都带当前滑级。上电后第一次扫描必发一帧，之后滑级变化才发。
- 单按 KEY1 或 KEY2：在**松开**时发一帧，对应位为 1；按住期间不发，也不补发 0 帧。
- 退使组合键：KEY2 已按下后再按下 KEY1，两键一起按住 300 ms（从按下 KEY1 算起约 340 ms，含 40 ms 去抖）后开始发送，每 20 ms 一帧 `DisableRequest`=1，直到任一键松开，且至少发 3 帧；结束时补一帧 `DisableRequest`=0。请求期间因滑级变化发出的帧同样带 1。
- 两键只要同时按下过，松开时都不发计时和清错。先按 KEY1 再按 KEY2，或在同一个 10 ms 扫描周期内同时按下，不触发退使。
- 想再次请求退使：保持 KEY2，松开 KEY1 后再按一次。

0x310 在去抖确认出有效档位（上电后约 30 ms）后发第一帧，旋钮没有有效档位时不发首帧；换档途中出现的无效读数会发一帧 0xFF，车手屏丢弃。

`steering_wheel.dbc` 是本节点两帧的 DBC。整车 CAN 协议的权威来源是 `vehicle-interfaces` 仓库的 `can/Vehicle_CanB.dbc`，本仓库不保留整车 DBC 副本。

车手屏收到 0x703 后原样转发到 CANB，计时锁期间也转发；收到 0x310 后，计时进行中不转发。0x310 在中央库中的发送节点标为 Display：本板在方向盘专用总线上发出，车手屏再转发到 CANB。

### 接收

本板的功能不需要接收任何帧。`Core/Src/can.c` 里保留了早前 interface 工程的接收解析，硬件过滤器放行 0x401、0x501、0x502、0x503、0x504，解析结果写入 `ECU_INFO`，不外发也不影响按键逻辑。车手屏的 CAN2 只收不发，这些帧不会出现在方向盘总线上，这段代码实际不会被触发。

IMU 原始帧 0x50 的接收和向 0x60～0x66 的转发已于 2026-10-11 删除（IMU 设备已不存在，见 [更新记录](CHANGELOG.md)）。

`Core/Inc/fsae_telemetry.pb.h` 是为编译通过保留的类型 stub，本工程未接入 nanopb。

## 运行行为

核心逻辑在 `Core/Src/button_panel.c`：

- 主循环无阻塞，反复调用 `ButtonPanel_Process()`，内部按 10 ms 周期执行扫描任务。
- 按键和旋钮采用软件去抖，连续 4 次采样一致才更新状态，响应窗口约 40 ms。
- 电位器每次扫描转换一次，做 16 点滑动平均，再按 `平均值 × 8 ÷ 4096` 量化为 0～7。
- 旋钮只认“恰好一档接地”的读数，零档或多档接地判为无效（0xFF）。
- 邮箱满时发送失败静默丢帧，不重试。

## 目录结构

```
SteeringWheel_Button/
├── Core/
│   ├── Inc/                  # button_panel.h：面板参数、CAN ID 与 0x703 位定义
│   └── Src/                  # button_panel.c：面板逻辑
│                              # can.c：CAN 收发、过滤与解析
│                              # ecu_info.c：整车信息汇总
│                              # 其余为 CubeMX 生成代码和正点原子模板
├── Drivers/                   # STM32F1 HAL 与 CMSIS
├── MDK-ARM/                   # Keil MDK 工程
├── cmake/                     # GCC 工具链文件（STM32CubeCLT）
├── CMakeLists.txt             # GCC/Ninja 构建入口
├── CMakePresets.json          # Debug/Release 预设
├── STM32F103XX_FLASH.ld       # 链接脚本
├── SteeringWheel_Button.ioc   # CubeMX 工程，引脚与时钟的源头
├── steering_wheel.dbc         # 本节点 CAN 定义（0x703、0x310）
├── tests/                     # button_panel.c 的主机单元测试（HAL 桩）
└── CHANGELOG.md               # 变更记录与待办
```

`main()` 只初始化 GPIO、ADC1、CAN1、TIM1。TIM1 与 usart.c、dma.c、delay.c、sys.c 为模板遗留，当前固件没有使用其功能。`usart.c` 里残留一条 LoRa 串口命令路径（收到 `M`+一个字节后发送 0x310），因 `main()` 没有初始化 USART，不会运行；启用 USART1 前须先决定是否保留它。

## 构建与烧录

### CMake + GCC（STM32CubeCLT）

需要安装 STM32CubeCLT（提供 arm-none-eabi-gcc、CMake、Ninja）。VS Code 装 STM32Cube 扩展后，工程已按 `.vscode/settings.json` 配置好，可直接构建和烧录调试。

命令行方式：

```bash
cmake --preset Debug
cmake --build build/Debug
```

产物为 `build/Debug/SteeringWheel_Button.elf`。当前 Debug 构建占用 FLASH 22.4%、RAM 9.3%。

### 主机单元测试

`tests/run_host_tests.sh` 用 HAL 桩在电脑上编译运行 `button_panel.c`，覆盖单键、退使组合键（时序、最少帧数、顺序、重复触发、请求期间滑级变化）和上电首帧。只需要系统自带的 `cc`：

```bash
tests/run_host_tests.sh
```

### Keil MDK

打开 `MDK-ARM/SteeringWheel_Button.uvprojx`，直接构建烧录。

### 修改外设配置

引脚、时钟、外设参数以 `SteeringWheel_Button.ioc` 为准，修改后在 CubeMX 中重新生成代码。自定义代码写在 `/* USER CODE BEGIN */` 与 `/* USER CODE END */` 区间内，重新生成不会覆盖。
