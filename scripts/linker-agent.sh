#!/usr/bin/env bash
# scripts/linker-agent.sh — 链接修复循环驱动
# 自动循环：链接 → 分析错误 → 调用 AI 修复 → 重新链接

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

source config/project.env
source scripts/state-manager.sh
source scripts/error-classifier.sh
source scripts/repair-decision-engine.sh

# 参数处理
MAX_ITERATIONS=${1:-10}
MODULE=${2:-unknown}

# ═══════════════════════════════════════════════════════════════════════════
# 链接循环
# ═══════════════════════════════════════════════════════════════════════════

iteration=0
last_error_hash=""
consecutive_same_errors=0

while [ $iteration -lt $MAX_ITERATIONS ]; do
    iteration=$((iteration + 1))

    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║ 链接轮次 [$iteration/$MAX_ITERATIONS]                         ║"
    echo "╚════════════════════════════════════════════════════════════════╝"

    # ───────────────────────────────────────────────────────────────────
    # 阶段 1：执行链接
    # ───────────────────────────────────────────────────────────────────

    LINK_ITERATION=$iteration bash scripts/link.sh

    link_result=$?

    if [ $link_result -eq 0 ]; then
        # 链接成功，完成循环
        echo ""
        echo "╔════════════════════════════════════════════════════════════════╗"
        echo "║ ✅ 链接成功！已生成固件，可进行烧录调试                       ║"
        echo "╚════════════════════════════════════════════════════════════════╝"
        exit 0
    fi

    # ───────────────────────────────────────────────────────────────────
    # 阶段 2：分析链接错误（使用分类器）
    # ───────────────────────────────────────────────────────────────────

    echo ""
    echo "[阶段 2] 使用错误分类器分析链接错误..."

    link_log="build/last-link.log"
    if [ ! -f "$link_log" ]; then
        echo "⚠️  链接日志未找到"
        continue
    fi

    # 提取原始错误消息
    error_msg=$(grep -oE "error:.*" "$link_log" | head -1 || echo "")

    if [ -z "$error_msg" ]; then
        echo "⚠️  未能识别具体错误，跳过分类"
        continue
    fi

    echo "[分类前] 原始错误: $error_msg"

    # 使用分类器分析错误
    classification=$(classify_link_error "$error_msg")

    echo "[分类结果]"
    echo "$classification" | jq .

    # 获取分类信息
    error_type=$(echo "$classification" | jq -r '.type')
    error_subtype=$(echo "$classification" | jq -r '.subtype')
    confidence=$(echo "$classification" | jq -r '.confidence')
    symbol=$(echo "$classification" | jq -r '.symbol')

    echo "  • 错误类型: $error_type"
    echo "  • 错误具体: $error_subtype"
    echo "  • 置信度: $confidence"
    [ -n "$symbol" ] && echo "  • 符号: $symbol"

    # 使用决策引擎生成修复建议
    echo ""
    echo "[决策引擎] 生成修复建议..."
    decision=$(decide_repair_action "$classification")

    echo "$decision" | jq .

    # 提取决策信息
    action=$(echo "$decision" | jq -r '.action')
    priority=$(echo "$decision" | jq -r '.priority')
    ai_instructions=$(echo "$decision" | jq -r '.ai_instructions')

    echo "  • 建议行动: $action (优先级 $priority)"

    # 生成错误哈希
    error_hash=$(echo "$error_msg" | md5sum | cut -d' ' -f1)

    # ───────────────────────────────────────────────────────────────────
    # 阶段 3：错误去重检查
    # ───────────────────────────────────────────────────────────────────

    if [ -n "$error_hash" ]; then
        if [ "$error_hash" == "$last_error_hash" ]; then
            consecutive_same_errors=$((consecutive_same_errors + 1))
            echo ""
            echo "⚠️  连续 $consecutive_same_errors 次相同错误（hash: $error_hash）"

            if [ $consecutive_same_errors -ge 3 ]; then
                echo ""
                echo "╔════════════════════════════════════════════════════════════════╗"
                echo "║ ❌ 连续 3 次相同链接错误，可能需要架构性改变。                 ║"
                echo "║ 建议人工介入进行深度诊断。                                     ║"
                echo "╚════════════════════════════════════════════════════════════════╝"
                exit 2
            fi
        else
            consecutive_same_errors=0
            last_error_hash="$error_hash"
        fi
    fi

    # ───────────────────────────────────────────────────────────────────
    # 阶段 4：基于决策引擎提供修复建议
    # ───────────────────────────────────────────────────────────────────

    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║ [阶段 4] 修复建议（基于错误分类）                             ║"
    echo "║ ────────────────────────────────────────────────────────────── ║"
    echo "║ 修复步骤："

    # 显示决策引擎的修复步骤
    echo "$decision" | jq -r '.steps[]' | while read -r step; do
        echo "║   → $step"
    done

    echo "║                                                               ║"
    echo "║ AI 指令："
    echo "║   $ai_instructions"
    echo "║                                                               ║"
    echo "║ 错误详情："
    echo "║   • 错误类型: $error_type (具体: $error_subtype)"
    [ -n "$symbol" ] && echo "║   • 符号: $symbol"
    echo "║   • 置信度: $confidence"
    echo "║   • 链接日志: $link_log"
    echo "║                                                               ║"
    echo "║ 修改完成后，脚本将自动重新链接。                             ║"
    echo "╚════════════════════════════════════════════════════════════════╝"

    if [ $iteration -lt $MAX_ITERATIONS ]; then
        echo ""
        echo "⏳ 等待修复... 将在下一轮重新链接"
        echo ""
    fi

done

# ═══════════════════════════════════════════════════════════════════════════
# 超出最大迭代次数
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║ ❌ 已超出最大链接轮次 ($MAX_ITERATIONS)                        ║"
echo "║ 链接仍未成功，需要人工深度诊断。                               ║"
echo "╚════════════════════════════════════════════════════════════════╝"

exit 1
