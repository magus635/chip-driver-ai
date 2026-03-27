#!/usr/bin/env bash
# scripts/link.sh — 增强链接脚本（集成状态管理和 .map 分析）
# 被 linker-agent 调用，返回码 0 = 成功, 1 = 链接失败
#
# 功能：
# - 执行链接
# - 捕获链接错误
# - 分析 .map 文件
# - 记录到状态系统

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# 源文件
source config/project.env
source scripts/state-manager.sh

# 参数处理
ITERATION=${LINK_ITERATION:-1}
LINK_LOG="${BUILD_DIR}/.link-$(date +%s).log"
LAST_LINK_LOG="${BUILD_DIR}/last-link.log"
MAP_FILE="${BUILD_DIR}/output.map"

# ═══════════════════════════════════════════════════════════════════════════
# 链接执行
# ═══════════════════════════════════════════════════════════════════════════

mkdir -p "$BUILD_DIR"

echo "═══════════════════════════════════════"
echo " [链接轮次 $ITERATION] 开始 · $(date '+%Y-%m-%d %H:%M:%S')"
echo "═══════════════════════════════════════"

# 检查目标文件列表
if [ ! -f "$BUILD_DIR/.obj-list" ]; then
    echo "[ERROR] 未找到目标文件列表，请先运行编译" >&2
    exit 1
fi
OBJ_FILES=$(cat "$BUILD_DIR/.obj-list")

# 检查链接脚本
if [ ! -f "${LINKER_SCRIPT:-}" ]; then
    echo "[ERROR] 链接脚本未配置或不存在" >&2
    exit 1
fi

echo "  链接脚本: $LINKER_SCRIPT"
echo "  输出文件: ${TARGET_ELF:-build/output.elf}"
echo ""

# 执行链接，捕获输出
LINK_OUTPUT="/tmp/link-output-$$.log"
: > "$LINK_OUTPUT"

# 执行链接，生成 .map 文件
if $CC ${LDFLAGS:-} \
    -T "$LINKER_SCRIPT" \
    -Wl,-Map="$MAP_FILE" \
    $OBJ_FILES \
    -o "${TARGET_ELF:-build/output.elf}" \
    2>&1 | tee -a "$LINK_OUTPUT"; then

    # 链接成功，生成其他格式
    echo ""
    echo "  OBJCOPY → ${TARGET_BIN:-build/output.bin}"
    ${OBJCOPY:-arm-none-eabi-objcopy} -O binary "${TARGET_ELF:-build/output.elf}" "${TARGET_BIN:-build/output.bin}" || true

    echo "  OBJCOPY → ${TARGET_HEX:-build/output.hex}"
    ${OBJCOPY:-arm-none-eabi-objcopy} -O ihex "${TARGET_ELF:-build/output.elf}" "${TARGET_HEX:-build/output.hex}" || true

    # 内存使用情况
    echo ""
    echo "── 内存使用 ─────────────────────────"
    ${SIZE:-arm-none-eabi-size} "${TARGET_ELF:-build/output.elf}" || true
    echo ""

    cp "$LINK_OUTPUT" "$LAST_LINK_LOG"

    echo "═══════════════════════════════════════"
    echo " [链接轮次 $ITERATION] 成功"
    echo "═══════════════════════════════════════"

    rm -f "$LINK_OUTPUT"
    exit 0

else
    # 链接失败
    echo ""
    echo "═══════════════════════════════════════"
    echo " [链接轮次 $ITERATION] 失败"
    echo "═══════════════════════════════════════"

    # 保存日志
    cp "$LINK_OUTPUT" "$LAST_LINK_LOG"

    # ───────────────────────────────────────────────────────────────────
    # 阶段：链接错误分析
    # ───────────────────────────────────────────────────────────────────

    echo ""
    echo "[LINK_ERROR_ANALYSIS]"

    # 检测错误类型
    error_type="unknown_link_error"
    error_symbol=""

    if grep -q "undefined reference to" "$LINK_OUTPUT"; then
        error_type="undefined_symbol"
        # 提取符号名称
        error_symbol=$(grep "undefined reference to" "$LINK_OUTPUT" | head -1 | sed "s/.*undefined reference to \`\(.*\)'.*/\1/")
        echo "Type:    $error_type"
        echo "Symbol:  $error_symbol"

    elif grep -q "multiple definition of" "$LINK_OUTPUT"; then
        error_type="duplicate_symbol"
        error_symbol=$(grep "multiple definition of" "$LINK_OUTPUT" | head -1 | sed "s/.*multiple definition of \`\(.*\)'.*/\1/")
        echo "Type:    $error_type"
        echo "Symbol:  $error_symbol"

    elif grep -q "region.*overflowed" "$LINK_OUTPUT"; then
        error_type="memory_overflow"
        region=$(grep "region.*overflowed" "$LINK_OUTPUT" | head -1 | cut -d' ' -f2)
        echo "Type:    $error_type"
        echo "Region:  $region"

    elif grep -q "cannot find" "$LINK_OUTPUT"; then
        error_type="missing_library"
        library=$(grep "cannot find" "$LINK_OUTPUT" | head -1 | sed "s/.*cannot find -l\(.*\)/\1/")
        echo "Type:    $error_type"
        echo "Library: $library"

    else
        error_msg=$(grep -i "error:" "$LINK_OUTPUT" | head -1 || echo "Unknown error")
        echo "Type:    $error_type"
        echo "Message: $error_msg"
    fi

    # 计算错误哈希
    error_hash=$(echo "${error_type}:${error_symbol}" | md5sum | cut -c1-12)
    echo "Hash:    $error_hash"

    echo "[/LINK_ERROR_ANALYSIS]"

    # ───────────────────────────────────────────────────────────────────
    # 阶段：.map 文件分析（如果存在）
    # ───────────────────────────────────────────────────────────────────

    if [ -f "$MAP_FILE" ] && [ "$error_type" = "undefined_symbol" ]; then
        echo ""
        echo "[MAP_ANALYSIS]"
        echo "Searching for symbol: $error_symbol"

        # 从 .map 文件中查找符号
        if grep -q "$error_symbol" "$MAP_FILE"; then
            echo "Found in .map: $error_symbol is referenced"
        else
            echo "Not found in .map: $error_symbol may not be linked"
        fi

        echo "[/MAP_ANALYSIS]"
    fi

    # ───────────────────────────────────────────────────────────────────
    # 阶段：错误去重检查
    # ───────────────────────────────────────────────────────────────────

    if [ -f ".claude/state/session.json" ]; then
        echo ""
        echo "[DEDUP_CHECK_LINK]"

        if prev_repair=$(bash scripts/state-manager.sh query-error "$error_hash" 2>/dev/null || echo ""); then
            echo "Previous: $prev_repair (此错误之前在此次修复中出现过)"
        else
            echo "Previous: (新错误，首次出现)"
        fi

        echo "[/DEDUP_CHECK_LINK]"
    fi

    # 输出完整日志
    echo ""
    echo "[FULL_LINK_LOG]"
    cat "$LINK_OUTPUT"
    echo "[/FULL_LINK_LOG]"

    rm -f "$LINK_OUTPUT"
    exit 1
fi
