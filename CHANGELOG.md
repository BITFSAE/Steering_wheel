# CHANGELOG

记录固件行为、接口和构建的变更。新条目放在最上面。本仓库没有 git 历史，早期变更无从追溯，以 2026-10-11 的代码状态作为基线记录。

## 未发布

固件当前能力基线（未打过版本标签）：

- 按钮面板模块（`Core/Src/button_panel.c`）：
  - 4 位旋钮选择驾驶模式，档位变化时发送 0x310 DriveMode_Request，byte0 为 ASCII '0'..'3'，对应模式 S/Q/C/E；档位无效时发送 0xFF。
  - 电位器输入经 16 点滑动平均后量化为 8 级；滑级与按键共用 0x703 SteeringCmd（DLC 2），滑级变化时发送。
  - KEY1 单独松开时 0x703 `LapTimerToggle`=1，KEY2 单独松开时 `ErrorClearActive`=1；先按住 KEY2 再按 KEY1、两键按住 300 ms 后每 20 ms 发 `DisableRequest`=1（退使请求），至少 3 帧，松开后补一帧 0。
  - 扫描周期 10 ms，去抖窗口约 40 ms，全部为事件驱动发送，上电各强制首发一帧。
  - 不发送 0x784：中央库已于 2026-10-11 删除该冗余驾驶模式帧，无需处理。
- CAN 接收（`Core/Src/can.c`）：硬件过滤只收 0x401、0x501、0x502、0x503、0x504，解析结果写入 ECU_INFO，方向盘总线上不会出现这些帧。
- 遗留代码：can.c 的接收解析、ecu_info 汇总结构、usart/dma/delay/sys 模板来自早前的 interface 工程，编译参与但 main 未初始化 USART/DMA。
- 构建双路径：STM32CubeCLT + CMake/Ninja（Debug/Release 预设）与 Keil MDK 工程。Debug 构建占用 FLASH 21.9%、RAM 9.2%（2026-10-11 实测）。

## 未发布/待办

- 上板与抓包确认 0x703：单键松开只发一帧、不补 0 帧；退使组合键的 20 ms 周期、最少 3 帧和结束 0 帧。目前只有主机单元测试，未上板、未抓包。车手屏 300 ms 去重间隔仍会把间隔小于 300 ms 的两次单键连按当作一次。
- 退使需与 ECU 联调：ECU 是否在收到 `DisableRequest`=1 后退出使能、是否按“连续 2 帧”判据执行；车手屏须同批升级，旧版车手屏不认 0x703 输入，并且会在每次计时键时置 bit0。
- `AutoBusOff` 在 `.ioc` 中关闭，固件也没有处理 bus-off（只开了接收中断）：一旦进入 bus-off，面板停发直到复位。是否改为自动恢复须在 CubeMX 中修改。
- 驾驶模式：旋钮在车手屏计时锁期间被拒绝时，本板不会在锁解除后重发，旋钮位置与实际模式可能不一致，须重新拨动旋钮。
- `usart.c` 的 LoRa `M` 命令会发送 0x310，当前不运行；启用 USART1 前须先评审。
- `can.c` 中 GBK 编码的中文注释在 UTF-8 环境显示为乱码，需重存为 UTF-8。
- KEY3、KEY4、S5 引脚已在 CubeMX 中配置为输入，固件未使用，接入功能或从 .ioc 移除。
- `Core/Inc/fsae_telemetry.pb.h` 是编译 stub，接入 nanopb 遥测时需替换为真实生成文件。
- 固件无版本号宏，CAN 上也不发版本帧；建议增加版本标识便于赛场排查。

## 2026-10-28

- 不兼容修改（须与车手屏同批烧录）：方向盘不再发送 0x700/0x701/0x702，改为直接组好整帧 0x703 SteeringCmd，车手屏原样转发给 ECU，与 0x310 的处理方式一致。位布局按中央库 `vehicle-interfaces` 同日修改：byte0 bit0..2 滑级，byte1 bit0 `DisableRequest`（ECU 侧含义为退出使能）、bit1 `ErrorClearActive`、bit2 `LapTimerToggle`。
- 新增退使组合键（`Scan_Buttons()` 与 `Update_Disable_Request()`）：KEY2 先按下、再按 KEY1，两键按住 300 ms 后每 20 ms 发一帧请求，至少 3 帧，松开后补一帧 0；请求期间滑级变化帧也带 1。两键同时按下过就不再触发单键动作；顺序反了或同一扫描周期内同时按下不触发。
- `steering_wheel.dbc` 用 cantools 重新生成：只含 0x703 和 0x310，接收方改为 Display。原文件的 `BS_: 500000` 不符合 DBC 语法，cantools strict 无法解析。
- 新增 `tests/`：用 HAL 桩在主机上运行 `button_panel.c` 的单元测试，脚本为 `tests/run_host_tests.sh`。
- 清理：删除根目录的整车 DBC 早期快照 `Vehicle_CanB.dbc`（仍含 0x700..0x702 和旧版 SOP 帧），整车协议只以 `vehicle-interfaces` 为准；关闭对应的两条待办（快照处理、0x310 发送节点说明，后者已写入 README）。
- README 的 CAN 接口、引脚表、目录与构建占用同步更新。
- 验证：`cmake --build build/Debug` 通过、无警告，FLASH 22.4%（14700 B）、RAM 9.3%；`tests/run_host_tests.sh` 通过（按住 1 s 发出 34 帧请求，第一帧在按下 KEY1 后 340 ms）。未上板、未抓包，Keil 工程未构建。

## 2026-10-11

- 更正按键含义：KEY1/0x701 是圈速计时开关，不是录像/相机切换；`RecorderToggle` 仅为 DBC 沿用的旧信号名。更新 README、`button_panel.c/.h` 注释和 `steering_wheel.dbc` 注释，标识符与帧格式未变。
- 删除 IMU 原始帧 0x50 的接收和 0x60～0x66 转发（含注释掉的 FIFO1 回调），硬件过滤列表中的 0x50 改为重复 0x502 占位。IMU 设备已不存在。该转发在接收中断里使用与主循环共用的 `TxHeader`，删除后，运行中的代码只在主循环里使用 `TxHeader`（`usart.c` 的残留路径不运行）。
- README 修正上电首帧的描述：驾驶模式在去抖确认后才发，无有效档位时不发（`last_sent_drive_mode` 初值与 `DRIVE_MODE_INVALID` 同为 0xFF）。
- 验证：`cmake --build build/Debug` 构建通过无警告，FLASH 21.9%、RAM 9.2%。未上板，未抓包。

- 新增 `README.md`：硬件参数、引脚表、CAN 收发接口、运行行为、目录结构和两条构建路径，事实均核对自代码与 DBC。
- 扩充 `.gitignore`：原 3 行基础上补充 Keil 布局文件、编译中间产物、调试器日志和系统杂项；保留 `.settings` 跟踪。
- 用 ninja 增量构建验证 Debug 配置通过，产物 `build/Debug/SteeringWheel_Button.elf`。
- 初始化 git 仓库，首次导入 127 个文件并推送到 `origin/main`（https://github.com/niulitong/Steering_wheel）。
