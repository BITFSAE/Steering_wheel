#!/bin/sh
# 在主机上编译运行 button_panel.c 的单元测试（不需要板子和 ARM 工具链）。
# button_panel.h 以 "main.h" 引用 HAL，这里把它复制到临时目录，与桩 main.h 放在一起。
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
cp "$ROOT/tests/stub/main.h" "$ROOT/tests/stub/can.h" "$ROOT/Core/Inc/button_panel.h" "$OUT/"
cc -std=c99 -Wall -Wextra -Werror -I"$OUT" -o "$OUT/test_button_panel" \
    "$ROOT/tests/test_button_panel.c" "$ROOT/Core/Src/button_panel.c"
"$OUT/test_button_panel"
