#!/usr/bin/env bash
# scripts/compile.sh — 增强编译脚本（集成状态管理）
# 被 compiler-agent 调用，返回码 0 = 成功, 1 = 编译失败
#
# 功能：
# - 执行编译
# - 捕获错误并分类
# - 记录到状态系统
# - 支持错误去重

set -uo pipefail

# 跨平台 MD5 封装（macOS 用 md5，Linux 用 md5sum）
_md5() {
    if command -v md5sum &>/dev/null; then
        md5sum "$@" | cut -c1-12
    elif command -v md5 &>/dev/null; then
        md5 -q "$@" | cut -c1-12
    else
        # fallback: 用 cksum
        cksum "$@" | awk '{print $1}'
    fi
}

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# 源文件：加载配置和状态管理
source config/project.env

# 如果未指定，使用默认值
ITERATION=${COMPILE_ITERATION:-1}
COMPILE_LOG="${BUILD_DIR:-build}/.compile-$(date +%s).log"
LAST_COMPILE_LOG="${BUILD_DIR:-build}/last-compile.log"

# ═══════════════════════════════════════════════════════════════════════════
# 编译执行
# ═══════════════════════════════════════════════════════════════════════════

mkdir -p "$BUILD_DIR"

# ═══════════════════════════════════════════════════════════════════════════
# Reviewer Pass Token Gate (V3.0 强制关卡)
# ═══════════════════════════════════════════════════════════════════════════

VERIFY_TOKEN_SCRIPT="$SCRIPT_DIR/verify-token.py"

# 选择可用的 Python3（优先系统 Python，避免 conda 环境问题）
PYTHON3=""
if /usr/bin/python3 -c "pass" &>/dev/null; then
    PYTHON3="/usr/bin/python3"
elif python3 -c "pass" &>/dev/null; then
    PYTHON3="python3"
else
    echo "  ⚠ python3 不可用，跳过 Token 校验"
fi

if [[ -f "$VERIFY_TOKEN_SCRIPT" ]] && [[ -n "$PYTHON3" ]]; then
    echo "═══════════════════════════════════════"
    echo " [校验] Reviewer Pass Token"
    echo "═══════════════════════════════════════"

    # 执行校验并消耗 token
    if ! $PYTHON3 "$VERIFY_TOKEN_SCRIPT" --consume; then
        echo ""
        echo "═══════════════════════════════════════"
        echo " [编译中止] Token 校验失败 (Exit 3)"
        echo " 请确保已运行 reviewer-agent 且代码无改动"
        echo "═══════════════════════════════════════"
        exit 3
    fi
    echo ""
fi

# ═══════════════════════════════════════════════════════════════════════════

echo "═══════════════════════════════════════"
echo " [编译轮次 $ITERATION] 开始 · $(date '+%Y-%m-%d %H:%M:%S')"
echo " 目标芯片: ${CHIP_MODEL:-unknown}"
echo " 工具链:   ${CC:-arm-none-eabi-gcc}"
echo "═══════════════════════════════════════"

# 收集源文件
SRC_FILES=$(find "$SRC_DIR" -name "*.c" 2>/dev/null | sort)
INC_FLAGS=$(for d in ${INC_DIRS:-"include"}; do echo -n "-I$d "; done)

if [ -z "$SRC_FILES" ]; then
    echo "[ERROR] $SRC_DIR 目录下未找到 .c 文件" >&2
    exit 1
fi

# 编译每个源文件，捕获所有输出
COMPILE_ERRORS=0
OBJ_FILES=""
COMPILE_OUTPUT="/tmp/compile-output-$$.log"

: > "$COMPILE_OUTPUT"  # 清空日志

for src in $SRC_FILES; do
    obj="${BUILD_DIR}/$(basename ${src%.c}.o)"
    echo "  CC  $src"

    # 捕获编译输出（同时显示和保存）
    # 注意：用 PIPESTATUS 获取编译器的真实返回码，而非 tee 的返回码
    $CC $CFLAGS ${INC_FLAGS} -c "$src" -o "$obj" 2>&1 | tee -a "$COMPILE_OUTPUT"
    if [ ${PIPESTATUS[0]} -eq 0 ]; then
        OBJ_FILES="$OBJ_FILES $obj"
    else
        COMPILE_ERRORS=$((COMPILE_ERRORS + 1))
    fi
done

# 保存编译日志
cp "$COMPILE_OUTPUT" "$LAST_COMPILE_LOG"

# ═══════════════════════════════════════════════════════════════════════════
# 编译结果处理
# ═══════════════════════════════════════════════════════════════════════════

if [ $COMPILE_ERRORS -eq 0 ]; then
    # 编译成功
    echo "$OBJ_FILES" > "$BUILD_DIR/.obj-list"

    echo ""
    echo "═══════════════════════════════════════"
    echo " [编译轮次 $ITERATION] 成功"
    echo " 目标文件: $(echo $OBJ_FILES | wc -w) 个"
    echo "═══════════════════════════════════════"

    rm -f "$COMPILE_OUTPUT"
    exit 0
else
    # 编译失败：分析错误
    echo ""
    echo "═══════════════════════════════════════"
    echo " [编译轮次 $ITERATION] 失败"
    echo " 错误数: $COMPILE_ERRORS"
    echo "═══════════════════════════════════════"

    # 提取第一个错误（通常是根源）
    first_error=$(grep -m1 "error:" "$COMPILE_OUTPUT" || echo "Unknown error")
    error_file=$(echo "$first_error" | cut -d: -f1)
    error_line=$(echo "$first_error" | cut -d: -f2)
    error_msg=$(echo "$first_error" | cut -d: -f4-)

    # 简化错误消息
    error_type="compilation_error"
    if echo "$error_msg" | grep -q "undeclared"; then
        error_type="undeclared_identifier"
    elif echo "$error_msg" | grep -q "expected"; then
        error_type="syntax_error"
    elif echo "$error_msg" | grep -q "undefined reference"; then
        error_type="undefined_reference"
    elif echo "$error_msg" | grep -q "conflicting"; then
        error_type="conflicting_types"
    elif echo "$error_msg" | grep -q "incompatible"; then
        error_type="type_mismatch"
    fi

    # 计算错误指纹 (V3.0 规范)
    fingerprint="unknown"
    if [[ -x "$SCRIPT_DIR/fingerprint.py" ]]; then
        fingerprint=$(python3 "$SCRIPT_DIR/fingerprint.py" \
            "$(echo "$error_msg" | grep -oE "E[0-9]{4}" || echo "error")" \
            "$(echo "$error_msg" | grep -oE "'[^']+'" | head -1 | tr -d "'" || echo "none")" \
            "compile" \
            "$error_type")
    fi

    # 输出错误详情（供 AI 分析）
    echo ""
    echo "[ERROR_ANALYSIS]"
    echo "Type:    $error_type"
    echo "Fingerprint: $fingerprint"
    echo "File:    $error_file:$error_line"
    echo "Message: $error_msg"
    echo "[/ERROR_ANALYSIS]"



    # 输出完整日志供 AI 分析
    echo ""
    echo "[FULL_COMPILE_LOG]"
    cat "$COMPILE_OUTPUT"
    echo "[/FULL_COMPILE_LOG]"

    rm -f "$COMPILE_OUTPUT"
    exit 1
fi
