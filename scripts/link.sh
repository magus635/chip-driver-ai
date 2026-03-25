#!/usr/bin/env bash
# scripts/link.sh — 链接脚本
# 被 linker-agent 调用，返回码 0 = 成功

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

source config/project.env

echo "═══════════════════════════════════════"
echo " 链接开始 · $(date '+%Y-%m-%d %H:%M:%S')"
echo "═══════════════════════════════════════"

# 读取目标文件列表
if [ ! -f "$BUILD_DIR/.obj-list" ]; then
    echo "[ERROR] 未找到目标文件列表，请先运行 compile.sh" >&2
    exit 1
fi
OBJ_FILES=$(cat "$BUILD_DIR/.obj-list")

# 检查链接脚本
if [ ! -f "$LINKER_SCRIPT" ]; then
    echo "[ERROR] 链接脚本 $LINKER_SCRIPT 不存在" >&2
    echo "[INFO]  请从芯片厂商 SDK 中复制，或根据芯片手册创建" >&2
    exit 1
fi

echo "  链接脚本: $LINKER_SCRIPT"
echo "  输出文件: $TARGET_ELF"
echo ""

# 执行链接
$CC $LDFLAGS \
    -T "$LINKER_SCRIPT" \
    $OBJ_FILES \
    -o "$TARGET_ELF" \
    2>&1

# 生成 BIN 和 HEX
echo "  OBJCOPY → $TARGET_BIN"
$OBJCOPY -O binary "$TARGET_ELF" "$TARGET_BIN"

echo "  OBJCOPY → $TARGET_HEX"
$OBJCOPY -O ihex "$TARGET_ELF" "$TARGET_HEX"

# 输出内存使用情况
echo ""
echo "── 内存使用 ─────────────────────────"
$SIZE "$TARGET_ELF"
echo ""

# 检查是否超过 Flash/RAM 限制
TEXT_SIZE=$($SIZE "$TARGET_ELF" | awk 'NR==2{print $1}')
DATA_SIZE=$($SIZE "$TARGET_ELF" | awk 'NR==2{print $2}')
BSS_SIZE=$($SIZE "$TARGET_ELF"  | awk 'NR==2{print $3}')
FLASH_USED=$((TEXT_SIZE + DATA_SIZE))
RAM_USED=$((DATA_SIZE + BSS_SIZE))
FLASH_KB=$((FLASH_SIZE * 1024))
RAM_KB=$((RAM_SIZE * 1024))

echo "  Flash: $FLASH_USED / $FLASH_KB bytes ($(( FLASH_USED * 100 / FLASH_KB ))%)"
echo "  RAM:   $RAM_USED / $RAM_KB bytes ($(( RAM_USED * 100 / RAM_KB ))%)"

if [ $FLASH_USED -gt $FLASH_KB ]; then
    echo "[ERROR] Flash 溢出！超出 $((FLASH_USED - FLASH_KB)) bytes" >&2
    exit 1
fi

if [ $RAM_USED -gt $RAM_KB ]; then
    echo "[ERROR] RAM 溢出！超出 $((RAM_USED - RAM_KB)) bytes" >&2
    exit 1
fi

echo ""
echo "═══════════════════════════════════════"
echo " 链接成功 · 固件已生成: $TARGET_BIN"
echo "═══════════════════════════════════════"
exit 0
