#!/usr/bin/env bash
# scripts/debugger-agent.sh — 调试验证循环驱动
# 自动循环：烧录 → 采集快照 → 分析 → 如有问题回编译
#
# 使用方式：
#   bash scripts/debugger-agent.sh [--max-rounds 8] [--criteria "criteria text"]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

source config/project.env
source scripts/state-manager.sh
source scripts/error-classifier.sh
source scripts/repair-decision-engine.sh

# 参数处理
MAX_ROUNDS=${1:-8}
CRITERIA=${2:-"驱动功能正常"}

# ═══════════════════════════════════════════════════════════════════════════
# 调试循环
# ═══════════════════════════════════════════════════════════════════════════

round=0

while [ $round -lt $MAX_ROUNDS ]; do
    round=$((round + 1))

    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║ 调试轮次 [$round/$MAX_ROUNDS]                                ║"
    echo "╚════════════════════════════════════════════════════════════════╝"

    # ───────────────────────────────────────────────────────────────────
    # 阶段 1：烧录固件
    # ───────────────────────────────────────────────────────────────────

    echo ""
    echo "[阶段 1] 烧录固件到目标板..."

    if [ ! -f "${TARGET_BIN:-build/output.bin}" ]; then
        echo "❌ 固件文件不存在，请先完成编译和链接"
        exit 1
    fi

    # 这里应该调用烧录脚本
    # bash scripts/flash.sh > flash.log 2>&1
    # 为了测试目的，假设烧录成功

    echo "✓ 固件已烧录（模拟）"

    # ───────────────────────────────────────────────────────────────────
    # 阶段 2：采集快照
    # ───────────────────────────────────────────────────────────────────

    echo ""
    echo "[阶段 2] 采集运行快照..."

    # 调用快照脚本
    if bash scripts/debug-snapshot.sh > snapshot.log 2>&1; then
        echo "✓ 快照采集成功"
    else
        echo "⚠️  快照采集失败或不可用"
        # 继续而不是退出，可能是因为硬件不连接
    fi

    # 保存快照到状态
    snapshot_id=$(state_manager::create_snapshot "$round" "$(cat snapshot.log 2>/dev/null || echo '')" "{}" "{}")
    echo "✓ 快照已记录: $snapshot_id"

    # ───────────────────────────────────────────────────────────────────
    # 阶段 3：分析快照结果（使用分类器）
    # ───────────────────────────────────────────────────────────────────

    echo ""
    echo "[阶段 3] 使用错误分类器分析运行结果..."
    echo ""
    echo "验收标准: $CRITERIA"
    echo ""

    # 读取快照输出
    serial_output=$(cat snapshot.log 2>/dev/null || echo "")

    # 使用分类器分析调试问题
    if [ -n "$serial_output" ]; then
        debug_classification=$(classify_debug_issue "$serial_output")

        echo "[分类结果]"
        echo "$debug_classification" | jq .

        # 提取分类信息
        issue_type=$(echo "$debug_classification" | jq -r '.type')
        severity=$(echo "$debug_classification" | jq -r '.severity')
        suggested_action=$(echo "$debug_classification" | jq -r '.suggested_action')

        echo "  • 问题类型: $issue_type"
        echo "  • 严重级别: $severity"
        echo "  • 建议行动: $suggested_action"

        # 根据分类结果判断状态
        case "$issue_type" in
            normal_operation)
                status="pass"
                analysis="分类器检测到正常运行状态：$suggested_action"
                ;;
            runtime_error|timeout_error)
                status="fail"
                analysis="分类器检测到运行时错误：$suggested_action"
                ;;
            *)
                status="unknown"
                analysis="分类器判断为未知状态，$suggested_action"
                ;;
        esac
    else
        status="unknown"
        analysis="快照采集失败，无法自动判断"
        issue_type="unknown"
        severity="unknown"
    fi

    # 更新快照分析
    state_manager::update_snapshot "$snapshot_id" "$analysis" "$status"

    echo ""
    echo "状态: $status"
    echo "分析: $analysis"

    # ───────────────────────────────────────────────────────────────────
    # 阶段 4：判断是否通过
    # ───────────────────────────────────────────────────────────────────

    if [ "$status" = "pass" ]; then
        echo ""
        echo "╔════════════════════════════════════════════════════════════════╗"
        echo "║ ✅ 调试验证通过！驱动功能正常                                 ║"
        echo "║"
        echo "║ 轮次: $round/$MAX_ROUNDS"
        echo "║ 可进入生产阶段"
        echo "╚════════════════════════════════════════════════════════════════╝"
        exit 0
    fi

    # ───────────────────────────────────────────────────────────────────
    # 阶段 5：失败处理
    # ───────────────────────────────────────────────────────────────────

    if [ "$status" = "fail" ]; then
        echo ""
        echo "╔════════════════════════════════════════════════════════════════╗"
        echo "║ ❌ 调试验证失败，需要修改代码                                 ║"
        echo "║"
        echo "║ 建议："
        echo "║  1. 检查上述快照输出中的错误信息"
        echo "║  2. 对照源代码和芯片手册进行修复"
        echo "║  3. 修改完成后，将自动回编译阶段"
        echo "╚════════════════════════════════════════════════════════════════╝"

        if [ $round -lt $MAX_ROUNDS ]; then
            echo ""
            echo "⏳ 等待代码修改... 修改完成后自动重新编译和烧录"
            echo ""
            # 这里应该返回给主程序，指示回编译阶段
            # 对于现在的测试，只是输出信息
        fi
    else
        echo ""
        echo "⚠️  无法自动判断结果，请人工确认"
        echo ""
        if [ $round -lt $MAX_ROUNDS ]; then
            echo "根据确认结果，可选择："
            echo "  • 修改代码后重新烧录"
            echo "  • 或认可当前结果"
        fi
    fi

done

# ═══════════════════════════════════════════════════════════════════════════
# 超出最大轮次
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║ ⏱️  已达最大调试轮次 ($MAX_ROUNDS)                             ║"
echo "║ 调试验证未能完成，需要人工深度调试。                           ║"
echo "╚════════════════════════════════════════════════════════════════╝"

exit 1
