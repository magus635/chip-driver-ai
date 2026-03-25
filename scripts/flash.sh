#!/usr/bin/env bash
# scripts/flash.sh — 烧录脚本
# 被 debugger-agent 调用，返回码 0 = 成功

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

source config/project.env

echo "═══════════════════════════════════════"
echo " 烧录开始 · $(date '+%Y-%m-%d %H:%M:%S')"
echo " 目标芯片: $CHIP_MODEL"
echo " 探针类型: $DEBUG_PROBE"
echo " 固件路径: $TARGET_BIN"
echo "═══════════════════════════════════════"

# 检查固件文件
if [ ! -f "$TARGET_BIN" ]; then
    echo "[ERROR] 固件文件 $TARGET_BIN 不存在，请先完成链接" >&2
    exit 1
fi

FIRMWARE_SIZE=$(wc -c < "$TARGET_BIN")
echo "  固件大小: $FIRMWARE_SIZE bytes"

# 检查调试探针
case "$DEBUG_PROBE" in
    stlink)
        if ! command -v openocd &> /dev/null; then
            echo "[ERROR] openocd 未安装，请执行: sudo apt-get install openocd" >&2
            exit 1
        fi
        echo "  使用 OpenOCD + ST-Link 烧录..."
        openocd \
            -f "$OPENOCD_CFG" \
            -c "program $TARGET_BIN verify reset exit $FLASH_ADDR" \
            2>&1
        ;;
    jlink)
        if ! command -v JLinkExe &> /dev/null; then
            echo "[ERROR] JLinkExe 未安装，请安装 J-Link Software" >&2
            exit 1
        fi
        echo "  使用 JLink 烧录..."
        cat > /tmp/jlink-flash.jlink << EOF
device $CHIP_MODEL
si 1
speed 4000
connect
loadbin $TARGET_BIN, $FLASH_ADDR
r
g
exit
EOF
        JLinkExe -nogui 1 -commandfile /tmp/jlink-flash.jlink 2>&1
        ;;
    cmsis-dap)
        if ! command -v pyocd &> /dev/null; then
            echo "[ERROR] pyocd 未安装，请执行: pip install pyocd" >&2
            exit 1
        fi
        echo "  使用 pyOCD + CMSIS-DAP 烧录..."
        pyocd flash --target="$(echo $CHIP_MODEL | tr '[:upper:]' '[:lower:]')" \
            "$TARGET_BIN" 2>&1
        ;;
    *)
        echo "[ERROR] 未知探针类型: $DEBUG_PROBE" >&2
        echo "[INFO]  支持的类型: stlink, jlink, cmsis-dap" >&2
        exit 1
        ;;
esac

echo ""
echo "═══════════════════════════════════════"
echo " 烧录成功 · 目标板已复位并开始运行"
echo "═══════════════════════════════════════"
exit 0
