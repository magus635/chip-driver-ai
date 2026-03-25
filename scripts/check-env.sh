#!/usr/bin/env bash
# scripts/check-env.sh — 环境检查脚本
# 验证工具链、文档、目标板连接等前置条件

set -uo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

ERRORS=0
WARNINGS=0

check_pass() { echo "  ✓ $1"; }
check_warn() { echo "  ⚠ $1"; WARNINGS=$((WARNINGS+1)); }
check_fail() { echo "  ✗ $1"; ERRORS=$((ERRORS+1)); }

echo "══════════════════════════════════════════"
echo "  环境检查 · chip-driver-ai"
echo "══════════════════════════════════════════"
echo ""

# ── 工具链 ────────────────────────────────────────────────
echo "[ 工具链 ]"
source config/project.env 2>/dev/null || { check_fail "config/project.env 加载失败"; exit 1; }

command -v $CC &>/dev/null  && check_pass "$CC 已找到" || check_fail "$CC 未找到"
command -v $LD &>/dev/null  && check_pass "$LD 已找到" || check_fail "链接器未找到"
command -v $NM &>/dev/null  && check_pass "$NM 已找到" || check_warn "nm 未找到（可选）"
command -v make &>/dev/null && check_pass "make 已找到" || check_warn "make 未找到"
echo ""

# ── 调试工具 ──────────────────────────────────────────────
echo "[ 调试工具 ]"
case "$DEBUG_PROBE" in
    stlink)
        command -v openocd &>/dev/null && check_pass "openocd 已安装" \
            || check_fail "openocd 未安装 (sudo apt-get install openocd)"
        ;;
    jlink)
        command -v JLinkExe &>/dev/null && check_pass "JLinkExe 已安装" \
            || check_fail "JLink Software 未安装"
        ;;
    cmsis-dap)
        command -v pyocd &>/dev/null && check_pass "pyocd 已安装" \
            || check_fail "pyocd 未安装 (pip install pyocd)"
        ;;
esac
echo ""

# ── 文档 ──────────────────────────────────────────────────
echo "[ 文档 ]"
DOC_COUNT=$(find docs/ -type f \( -name "*.pdf" -o -name "*.md" -o -name "*.txt" \) 2>/dev/null | wc -l)
if [ "$DOC_COUNT" -gt 0 ]; then
    check_pass "docs/ 目录包含 $DOC_COUNT 个文档文件"
    find docs/ -type f | while read f; do echo "       $f"; done
else
    check_fail "docs/ 目录为空，请将芯片手册放入 docs/ 目录"
fi
echo ""

# ── 项目源码 ──────────────────────────────────────────────
echo "[ 源码框架 ]"
SRC_COUNT=$(find src/ -name "*.c" 2>/dev/null | wc -l)
if [ "$SRC_COUNT" -gt 0 ]; then
    check_pass "src/ 目录包含 $SRC_COUNT 个 .c 文件"
else
    check_warn "src/ 目录为空，AI 将从头生成驱动代码"
fi

[ -f "$LINKER_SCRIPT" ] && check_pass "链接脚本: $LINKER_SCRIPT" \
    || check_fail "链接脚本 $LINKER_SCRIPT 不存在"
echo ""

# ── 串口 ─────────────────────────────────────────────────
echo "[ 串口连接 ]"
[ -e "$SERIAL_PORT" ] && check_pass "串口 $SERIAL_PORT 存在" \
    || check_warn "串口 $SERIAL_PORT 未连接（调试时需要）"
echo ""

# ── 汇总 ──────────────────────────────────────────────────
echo "══════════════════════════════════════════"
if [ $ERRORS -gt 0 ]; then
    echo "  检查结果: ✗ 失败（$ERRORS 个错误，$WARNINGS 个警告）"
    echo "  请解决上述错误后重新运行"
    exit 1
elif [ $WARNINGS -gt 0 ]; then
    echo "  检查结果: ⚠ 通过（0 个错误，$WARNINGS 个警告）"
    echo "  警告项不影响主流程，但部分功能可能受限"
    exit 0
else
    echo "  检查结果: ✓ 全部通过"
    exit 0
fi
