# CHANGELOG

记录固件行为、接口和构建的变更。新条目放在最上面。本仓库没有 git 历史，早期变更无从追溯，以 2026-10-11 的代码状态作为基线记录。

## 未发布

固件当前能力基线（未打过版本标签）：

- 按钮面板模块（`Core/Src/button_panel.c`）：
  - 4 位旋钮选择驾驶模式，档位变化时发送 0x310 DriveMode_Request，byte0 为 ASCII '0'..'3'，对应模式 S/Q/C/E；档位无效时发送 0xFF。
  - 电位器输入经 16 点滑动平均后量化为 8 级，数值变化时发送 0x700 SlipLevel。
  - KEY1 释放时发送 0x701（圈速计时开关，DBC 名 RecorderToggle，bit0=1），KEY2 释放时发送 0x702 ErrorClear（bit0=1）。
  - 扫描周期 10 ms，去抖窗口约 40 ms，全部为事件驱动发送，上电各强制首发一帧。
  - 不发送 0x784：中央库已于 2026-10-11 删除该冗余驾驶模式帧，无需处理。
- CAN 接收（`Core/Src/can.c`）：硬件过滤只收 0x401、0x501、0x502、0x503、0x504，解析结果写入 ECU_INFO，方向盘总线上不会出现这些帧。
- 遗留代码：can.c 的接收解析、ecu_info 汇总结构、usart/dma/delay/sys 模板来自早前的 interface 工程，编译参与但 main 未初始化 USART/DMA。
- 构建双路径：STM32CubeCLT + CMake/Ninja（Debug/Release 预设）与 Keil MDK 工程。Debug 构建占用 FLASH 21.9%、RAM 9.2%（2026-10-11 实测）。

## 未发布/待办

- 上板与抓包确认：0x701/0x702 每次按下只发一帧、不发 0 帧、释放时发（代码如此，未在总线上抓包验证）；车手屏端 300 ms 去重间隔因此会吞掉间隔小于 300 ms 的两次连按。
- `AutoBusOff` 在 `.ioc` 中关闭，固件也没有处理 bus-off（只开了接收中断）：一旦进入 bus-off，面板停发直到复位。是否改为自动恢复须在 CubeMX 中修改。
- 驾驶模式：旋钮在车手屏计时锁期间被拒绝时，本板不会在锁解除后重发，旋钮位置与实际模式可能不一致，须重新拨动旋钮。
- 根目录 `Vehicle_CanB.dbc` 是早期快照，已与中央库不一致，应删除或改为引用 `vehicle-interfaces`。
- `usart.c` 的 LoRa `M` 命令会发送 0x310，当前不运行；启用 USART1 前须先评审。
- 与显示屏协议侧对齐 0x310 的发送节点：`Vehicle_CanB.dbc` 中标为 Display，本板直接发送同一格式。
- `can.c` 中 GBK 编码的中文注释在 UTF-8 环境显示为乱码，需重存为 UTF-8。
- KEY3、KEY4、S5 引脚已在 CubeMX 中配置为输入，固件未使用，接入功能或从 .ioc 移除。
- `Core/Inc/fsae_telemetry.pb.h` 是编译 stub，接入 nanopb 遥测时需替换为真实生成文件。
- 固件无版本号宏，CAN 上也不发版本帧；建议增加版本标识便于赛场排查。

## 2026-10-11

- 更正按键含义：KEY1/0x701 是圈速计时开关，不是录像/相机切换；`RecorderToggle` 仅为 DBC 沿用的旧信号名。更新 README、`button_panel.c/.h` 注释和 `steering_wheel.dbc` 注释，标识符与帧格式未变。
- 删除 IMU 原始帧 0x50 的接收和 0x60～0x66 转发（含注释掉的 FIFO1 回调），硬件过滤列表中的 0x50 改为重复 0x502 占位。IMU 设备已不存在。该转发在接收中断里使用与主循环共用的 `TxHeader`，删除后，运行中的代码只在主循环里使用 `TxHeader`（`usart.c` 的残留路径不运行）。
- README 修正上电首帧的描述：驾驶模式在去抖确认后才发，无有效档位时不发（`last_sent_drive_mode` 初值与 `DRIVE_MODE_INVALID` 同为 0xFF）。
- 验证：`cmake --build build/Debug` 构建通过无警告，FLASH 21.9%、RAM 9.2%。未上板，未抓包。

- 新增 `README.md`：硬件参数、引脚表、CAN 收发接口、运行行为、目录结构和两条构建路径，事实均核对自代码与 DBC。
- 扩充 `.gitignore`：原 3 行基础上补充 Keil 布局文件、编译中间产物、调试器日志和系统杂项；保留 `.settings` 跟踪。
- 用 ninja 增量构建验证 Debug 配置通过，产物 `build/Debug/SteeringWheel_Button.elf`。
- 初始化 git 仓库，首次导入 127 个文件并推送到 `origin/main`（https://github.com/niulitong/Steering_wheel）。
