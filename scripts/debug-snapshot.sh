#!/usr/bin/env bash
# scripts/debug-snapshot.sh — 调试快照脚本
# 采集目标板运行时状态，输出结构化文本供 debugger-agent 分析
# 被 debugger-agent 调用

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

source config/project.env

echo "═══════════════════════════════════════"
echo " 调试快照采集 · $(date '+%Y-%m-%d %H:%M:%S')"
echo "═══════════════════════════════════════"
echo ""

# ── 1. 串口输出采集 ──────────────────────────────────────
echo "=== SERIAL OUTPUT (${SERIAL_CAPTURE_SECONDS}s) ==="

# 兼容 timeout 命令
TIMEOUT_CMD=""
if command -v timeout &> /dev/null; then
    TIMEOUT_CMD="timeout"
elif command -v gtimeout &> /dev/null; then
    TIMEOUT_CMD="gtimeout"
else
    # Perl fallback 如果没有 timeout/gtimeout
    TIMEOUT_CMD="perl -e 'alarm shift; exec @ARGV' "
fi

if [ -n "$TIMEOUT_CMD" ] && [ -e "$SERIAL_PORT" ]; then
    # stty 兼容性
    if uname | grep -q 'Darwin'; then
        eval "$TIMEOUT_CMD $SERIAL_CAPTURE_SECONDS stty -f \"$SERIAL_PORT\" \"$SERIAL_BAUD\" cs8 -cstopb -parenb 2>/dev/null" || true
    else
        eval "$TIMEOUT_CMD $SERIAL_CAPTURE_SECONDS stty -F \"$SERIAL_PORT\" \"$SERIAL_BAUD\" cs8 -cstopb -parenb 2>/dev/null" || true
    fi
    eval "$TIMEOUT_CMD $SERIAL_CAPTURE_SECONDS cat \"$SERIAL_PORT\" 2>/dev/null" || echo "[串口无输出或端口不可用: $SERIAL_PORT]"
else
    echo "[串口端口 $SERIAL_PORT 未连接，跳过串口采集]"
    echo "[提示] 请在 config/project.env 中设置正确的 SERIAL_PORT"
fi
echo ""

# ── 2. 寄存器读取（通过 OpenOCD telnet 接口）──────────────
echo "=== REGISTER DUMP ==="

# 从 .claude/debug-registers.txt 读取需要监控的寄存器列表
# 格式: <寄存器名> <地址>
# 例如: CAN1_MCR 0x40006400
if [ -f ".claude/debug-registers.txt" ] && command -v nc &> /dev/null; then
    # 启动 OpenOCD（如未运行）
    OPENOCD_PID=""
    if ! nc -z localhost "$DEBUG_PORT" 2>/dev/null; then
        openocd -f "$OPENOCD_CFG" &
        OPENOCD_PID=$!
        sleep 2
    fi

    # netcat 参数兼容性 (macOS 是 BSD nc 无 -q)
    NC_ARGS=""
    if nc -h 2>&1 | grep -q '\-q'; then
        NC_ARGS="-q1"
    else
        NC_ARGS=""
        # 或者考虑用 expect/python 替代 nc，如果是 BSD nc
    fi

    while IFS=' ' read -r reg_name reg_addr; do
        [[ "$reg_name" =~ ^#.*$ ]] && continue  # 跳过注释
        [ -z "$reg_name" ] && continue
        # 通过 OpenOCD telnet 读取内存 (注意 BSD nc 可能需要在发送命令后关闭 stdin 或用 -G 控制)
        # 兼容写法：用 echo 发送并通过 tee 发给 nc，然后通过 head 获取返回
        VALUE=$( (echo "mdw $reg_addr"; sleep 0.1; echo "exit") | \
            nc localhost 4444 2>/dev/null | \
            grep "$reg_addr" | awk '{print $2}' || echo "读取失败")
        echo "$reg_name @ $reg_addr: $VALUE"
    done < ".claude/debug-registers.txt"

    # 如果是我们启动的 OpenOCD，关闭它
    if [ -n "$OPENOCD_PID" ]; then
        kill "$OPENOCD_PID" 2>/dev/null || true
    fi
else
    echo "[未配置寄存器监控列表或 nc 不可用]"
    echo "[提示] 创建 .claude/debug-registers.txt，每行格式: <名称> <十六进制地址>"
fi
echo ""

# ── 3. 关键变量读取 ──────────────────────────────────────
echo "=== VARIABLE DUMP ==="

# 从 .map 文件提取全局变量地址
if [ -f "$MAP_FILE" ] && [ -f ".claude/debug-variables.txt" ]; then
    while IFS=' ' read -r var_name; do
        [[ "$var_name" =~ ^#.*$ ]] && continue
        [ -z "$var_name" ] && continue
        # 从 .map 文件查找符号地址
        ADDR=$(grep -m1 " $var_name$" "$MAP_FILE" | awk '{print $1}' || echo "未找到")
        echo "$var_name @ $ADDR"
    done < ".claude/debug-variables.txt"
else
    echo "[未配置变量监控列表]"
    echo "[提示] 创建 .claude/debug-variables.txt，每行一个全局变量名"
fi
echo ""

# ── 4. 错误标志读取 ──────────────────────────────────────
echo "=== ERROR FLAGS ==="
echo "[此部分由 .claude/debug-error-flags.sh 扩展，用户可自定义]"

if [ -f ".claude/debug-error-flags.sh" ]; then
    bash ".claude/debug-error-flags.sh"
fi
echo ""

# ── 5. 时间戳 ──────────────────────────────────────────
echo "=== SNAPSHOT METADATA ==="
echo "采集时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "固件文件: $TARGET_BIN"
echo "固件大小: $(wc -c < $TARGET_BIN 2>/dev/null || echo N/A) bytes"
if command -v git &> /dev/null; then
    echo "Git Hash:  $(git rev-parse --short HEAD 2>/dev/null || echo N/A)"
fi
echo ""
echo "═══════════════════════════════════════"
echo " 快照采集完成"
echo "═══════════════════════════════════════"
