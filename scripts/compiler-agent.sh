#!/usr/bin/env bash
# scripts/compiler-agent.sh — 编译修复循环驱动
# 自动循环：编译 → 分析错误 → 调用 AI 修复 → 重新编译
#
# 使用：
#   bash scripts/compiler-agent.sh [--max-iterations 15] [--module can_bus]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# 源文件
source config/project.env
source scripts/state-manager.sh
source scripts/error-classifier.sh
source scripts/repair-decision-engine.sh

# 参数处理
MAX_ITERATIONS=${1:-15}
MODULE=${2:-unknown}

# ═══════════════════════════════════════════════════════════════════════════
# 编译循环
# ═══════════════════════════════════════════════════════════════════════════

iteration=0
last_error_hash=""
consecutive_same_errors=0

while [ $iteration -lt $MAX_ITERATIONS ]; do
    iteration=$((iteration + 1))

    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║ 编译轮次 [$iteration/$MAX_ITERATIONS]                         ║"
    echo "╚════════════════════════════════════════════════════════════════╝"

    # ───────────────────────────────────────────────────────────────────
    # 阶段 1：执行编译
    # ───────────────────────────────────────────────────────────────────

    COMPILE_ITERATION=$iteration bash scripts/compile.sh

    compile_result=$?

    if [ $compile_result -eq 0 ]; then
        # 编译成功，完成循环
        echo ""
        echo "╔════════════════════════════════════════════════════════════════╗"
        echo "║ ✅ 编译成功！已进入链接阶段                                    ║"
        echo "╚════════════════════════════════════════════════════════════════╝"
        exit 0
    fi

    # ───────────────────────────────────────────────────────────────────
    # 阶段 2：分析错误（使用分类器）
    # ───────────────────────────────────────────────────────────────────

    echo ""
    echo "[阶段 2] 使用错误分类器分析编译错误..."

    # 从编译日志中提取错误信息
    compile_log="build/last-compile.log"
    if [ ! -f "$compile_log" ]; then
        echo "⚠️  编译日志未找到，跳过详细分析"
        continue
    fi

    # 提取原始错误消息、文件和行号
    error_msg=$(grep -oE "error:.*" "$compile_log" | head -1 || echo "")
    error_file=$(grep -oE "^[^:]+:" "$compile_log" | head -1 | tr -d ':' || echo "unknown")
    error_line=$(grep -oE ":[0-9]+:" "$compile_log" | head -1 | tr -d ':' || echo "0")

    if [ -z "$error_msg" ]; then
        echo "⚠️  未能识别具体错误，跳过分类"
        continue
    fi

    echo "[分类前] 原始错误: $error_msg"

    # 使用分类器分析错误
    classification=$(classify_compile_error "$error_msg" "$error_file" "$error_line")

    echo "[分类结果]"
    echo "$classification" | jq .

    # 获取分类的错误类型和具体分类
    error_type=$(echo "$classification" | jq -r '.type')
    error_subtype=$(echo "$classification" | jq -r '.subtype')
    confidence=$(echo "$classification" | jq -r '.confidence')

    echo "  • 错误类型: $error_type"
    echo "  • 错误具体: $error_subtype"
    echo "  • 置信度: $confidence"

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

    # 提取错误哈希用于去重
    error_hash=$(echo "$error_msg" | md5sum | cut -d' ' -f1)

    # ───────────────────────────────────────────────────────────────────
    # 阶段 3：错误去重检查
    # ───────────────────────────────────────────────────────────────────

    if [ -n "$error_hash" ]; then
        if [ "$error_hash" == "$last_error_hash" ]; then
            consecutive_same_errors=$((consecutive_same_errors + 1))
            echo ""
            echo "⚠️  连续 $consecutive_same_errors 次相同错误（hash: $error_hash）"

            # 连续 3 次相同错误则停止
            if [ $consecutive_same_errors -ge 3 ]; then
                echo ""
                echo "╔════════════════════════════════════════════════════════════════╗"
                echo "║ ❌ 连续 3 次相同错误，可能陷入死循环。                          ║"
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
    # 阶段 4：AI 修复提示（基于决策引擎结果）
    # ───────────────────────────────────────────────────────────────────

    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║ [阶段 4] 需要 AI 修复                                         ║"
    echo "║ ────────────────────────────────────────────────────────────── ║"
    echo "║ 根据错误分类和决策引擎分析，请执行以下修复：                   ║"
    echo "║                                                               ║"

    # 显示决策引擎的建议步骤
    echo "║ 修复步骤："
    echo "$decision" | jq -r '.steps[]' | while read -r step; do
        echo "║   → $step"
    done

    echo "║                                                               ║"
    echo "║ AI 指令："
    echo "║   $ai_instructions"
    echo "║                                                               ║"
    echo "║ 错误详情："
    echo "║   • 错误类型: $error_type (具体: $error_subtype)"
    echo "║   • 置信度: $confidence"
    echo "║   • 文件: $error_file"
    echo "║   • 行号: $error_line"
    echo "║   • 编译日志: $compile_log"
    echo "╚════════════════════════════════════════════════════════════════╝"

    # 这里，AI 会暂停并等待用户修改代码
    # 在实际使用中，dev-driver 命令会捕获此输出并调用 AI 修复

    if [ $iteration -lt $MAX_ITERATIONS ]; then
        echo ""
        echo "⏳ 等待代码修改... 将在下一轮重新编译"
        echo ""
    fi

done

# ═══════════════════════════════════════════════════════════════════════════
# 超出最大迭代次数
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║ ❌ 已超出最大编译轮次 ($MAX_ITERATIONS)                        ║"
echo "║ 编译仍未成功，需要人工深度诊断。                               ║"
echo "╚════════════════════════════════════════════════════════════════╝"

exit 1
