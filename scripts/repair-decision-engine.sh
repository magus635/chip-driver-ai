#!/usr/bin/env bash
# scripts/repair-decision-engine.sh — 修复决策引擎
# 根据错误分类结果，生成智能修复建议和 AI 指令
#
# 使用：
#   decide_repair_action "error_classification_json"

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# ═══════════════════════════════════════════════════════════════════════════
# 编译错误决策树
# ═══════════════════════════════════════════════════════════════════════════

decide_compile_repair() {
    local classification=$1

    local error_type=$(echo "$classification" | jq -r '.type')
    local subtype=$(echo "$classification" | jq -r '.subtype')
    local confidence=$(echo "$classification" | jq -r '.confidence')
    local error_msg=$(echo "$classification" | jq -r '.message')
    local file=$(echo "$classification" | jq -r '.file')
    local line=$(echo "$classification" | jq -r '.line')

    local action=""
    local priority=""
    local steps='[]'
    local ai_instructions=""

    # ─────────────────────────────────────────────────────────────────
    # 语法错误的决策
    # ─────────────────────────────────────────────────────────────────

    if [ "$error_type" = "syntax_error" ]; then
        case "$subtype" in
            missing_semicolon)
                action="add_semicolon"
                priority="1"
                steps=$(jq -n '[
                    "定位错误文件和行号：" + $file + ":" + ($line | tostring),
                    "检查该行末尾是否缺少分号",
                    "添加分号"
                ]' --arg file "$file" --arg line "$line")
                ai_instructions="请在文件 $file 第 $line 行末尾添加分号"
                ;;

            mismatched_braces)
                action="fix_braces"
                priority="2"
                steps=$(jq -n '[
                    "检查 { } 或 ( ) 是否配对",
                    "使用编辑器的匹配查找功能",
                    "修复所有不匹配的大括号"
                ]')
                ai_instructions="请检查并修复文件 $file 中不匹配的大括号"
                ;;
        esac

    # ─────────────────────────────────────────────────────────────────
    # 类型错误的决策
    # ─────────────────────────────────────────────────────────────────

    elif [ "$error_type" = "type_error" ]; then
        case "$subtype" in
            type_mismatch)
                action="fix_type_mismatch"
                priority="1"

                # 提取类型信息
                local type_a=$(echo "$error_msg" | sed -n "s/.*assignment to '\(.*\)'.*/\1/p" | head -1)
                local type_b=$(echo "$error_msg" | sed -n "s/.*from '\(.*\)'.*/\1/p" | head -1)

                steps=$(jq -n '[
                    "分析错误：类型 \"" + $ta + "\" 和 \"" + $tb + "\" 不兼容",
                    "选择方案：",
                    "  方案 1（推荐）：添加类型转换 (cast)",
                    "  方案 2：修改变量声明的类型",
                    "  方案 3：修改函数签名"
                ]' --arg ta "$type_a" --arg tb "$type_b")

                ai_instructions="请在文件 $file 第 $line 行修复类型不匹配问题。建议优先尝试添加类型转换。"
                ;;

            implicit_function)
                action="add_declaration"
                priority="1"
                steps=$(jq -n '[
                    "识别未声明的函数",
                    "查找函数所在的头文件（通常是标准库或本地头文件）",
                    "添加 #include 或前向声明"
                ]')
                ai_instructions="请在文件顶部添加必要的 #include 语句来声明该函数"
                ;;
        esac

    # ─────────────────────────────────────────────────────────────────
    # 声明错误的决策
    # ─────────────────────────────────────────────────────────────────

    elif [ "$error_type" = "declaration_error" ]; then
        case "$subtype" in
            undefined_identifier)
                action="declare_identifier"
                priority="1"
                steps=$(jq -n '[
                    "确定标识符的来源：",
                    "  来源 1：标准库函数 → 添加 #include",
                    "  来源 2：本地头文件 → 添加 #include",
                    "  来源 3：本地变量 → 添加声明",
                    "检查拼写是否正确"
                ]')
                ai_instructions="请在文件中添加必要的声明或 #include。如果是自定义标识符，请检查拼写。"
                ;;
        esac

    # ─────────────────────────────────────────────────────────────────
    # 预处理错误的决策
    # ─────────────────────────────────────────────────────────────────

    elif [ "$error_type" = "preprocessor_error" ]; then
        case "$subtype" in
            include_not_found)
                action="fix_include_path"
                priority="1"
                steps=$(jq -n '[
                    "检查头文件路径是否正确",
                    "验证 INC_DIRS 配置中是否包含该文件所在目录",
                    "确认文件是否存在"
                ]')
                ai_instructions="请验证 #include 路径，或检查 config/project.env 中的 INC_DIRS 配置"
                ;;
        esac
    fi

    # ─────────────────────────────────────────────────────────────────
    # 输出决策结果
    # ─────────────────────────────────────────────────────────────────

    jq -n \
        --arg act "$action" \
        --arg pri "$priority" \
        --argjson stp "$steps" \
        --arg instr "$ai_instructions" \
        '{
            action: $act,
            priority: ($pri | tonumber),
            confidence: "high",
            steps: $stp,
            ai_instructions: $instr
        }'
}

# ═══════════════════════════════════════════════════════════════════════════
# 链接错误决策树
# ═══════════════════════════════════════════════════════════════════════════

decide_link_repair() {
    local classification=$1

    local error_type=$(echo "$classification" | jq -r '.type')
    local subtype=$(echo "$classification" | jq -r '.subtype')
    local symbol=$(echo "$classification" | jq -r '.symbol')

    local action=""
    local priority=""
    local steps='[]'
    local ai_instructions=""

    # ─────────────────────────────────────────────────────────────────
    # 符号错误的决策
    # ─────────────────────────────────────────────────────────────────

    if [ "$error_type" = "symbol_error" ]; then
        case "$subtype" in
            undefined_symbol)
                action="resolve_undefined_symbol"
                priority="1"

                steps=$(jq -n '[
                    "分析符号：\"" + $sym + "\"",
                    "检查 build/.map 文件，查看该符号是否存在",
                    "如果存在：检查对象文件是否被正确链接",
                    "如果不存在：需要回编译阶段实现该符号",
                    "可能的原因：",
                    "  1. 函数未实现（需回编译）",
                    "  2. 源文件未被编译（修改 Makefile）",
                    "  3. 库文件未链接（检查 LIBS 变量）"
                ]' --arg sym "$symbol")

                ai_instructions="请检查 $symbol 是否已在编译阶段正确实现。如未实现，需回编译阶段添加实现。"
                ;;

            duplicate_symbol)
                action="resolve_duplicate_symbol"
                priority="1"

                steps=$(jq -n '[
                    "分析重复符号：\"" + $sym + "\"",
                    "检查该符号在哪些源文件中定义",
                    "可能的原因：",
                    "  1. 头文件中有实现（应为声明）",
                    "  2. 多个源文件定义同一符号",
                    "  3. 头文件缺少 include guard",
                    "修复步骤：",
                    "  1. 将头文件中的实现移至 .c 文件",
                    "  2. 或添加 #pragma once"
                ]' --arg sym "$symbol")

                ai_instructions="请将符号 $symbol 的实现从头文件移至对应的源文件，或添加 include guard"
                ;;
        esac

    # ─────────────────────────────────────────────────────────────────
    # 内存错误的决策
    # ─────────────────────────────────────────────────────────────────

    elif [ "$error_type" = "memory_error" ]; then
        case "$subtype" in
            flash_overflow)
                action="optimize_flash_usage"
                priority="1"

                steps=$(jq -n '[
                    "Flash 内存溢出",
                    "优化方案（按优先级）：",
                    "  1. 启用编译优化：修改 CFLAGS 添加 -Os",
                    "  2. 启用 dead code elimination：",
                    "     • 添加 CFLAGS 中的 -fdata-sections -ffunction-sections",
                    "     • 添加 LDFLAGS 中的 --gc-sections",
                    "  3. 验证 FLASH_SIZE 配置是否正确",
                    "  4. 移除不必要的代码"
                ]')

                ai_instructions="请修改 config/project.env，在 CFLAGS 中添加 -Os，在 LDFLAGS 中添加优化选项"
                ;;

            ram_overflow)
                action="optimize_ram_usage"
                priority="1"

                steps=$(jq -n '[
                    "RAM 内存溢出",
                    "优化方案：",
                    "  1. 将可初始化数据改为 const（移至 Flash）",
                    "  2. 减少全局数组大小",
                    "  3. 使用 __attribute__((section)) 将数据移至其他区域",
                    "  4. 考虑动态分配而非全局分配"
                ]')

                ai_instructions="请减少全局变量的使用，或将大型数据结构改为 const 存储在 Flash 中"
                ;;
        esac
    fi

    # ─────────────────────────────────────────────────────────────────
    # 输出决策结果
    # ─────────────────────────────────────────────────────────────────

    jq -n \
        --arg act "$action" \
        --arg pri "$priority" \
        --argjson stp "$steps" \
        --arg instr "$ai_instructions" \
        '{
            action: $act,
            priority: ($pri | tonumber),
            confidence: "high",
            steps: $stp,
            ai_instructions: $instr
        }'
}

# ═══════════════════════════════════════════════════════════════════════════
# 主决策函数
# ═══════════════════════════════════════════════════════════════════════════

decide_repair_action() {
    local classification=$1

    # 获取阶段信息（首先尝试 stage 字段，然后从类型推断）
    local stage=$(echo "$classification" | jq -r '.stage // "unknown"')

    # 如果 stage 是 unknown，尝试从 type 字段推断
    if [ "$stage" = "unknown" ]; then
        local error_type=$(echo "$classification" | jq -r '.type // "unknown"')

        # 根据错误类型推断阶段
        case "$error_type" in
            syntax_error|type_error|declaration_error|preprocessor_error)
                stage="compile"
                ;;
            symbol_error|memory_error|library_error|script_error)
                stage="link"
                ;;
            runtime_error|timeout_error|normal_operation|unknown_state)
                stage="debug"
                ;;
        esac
    fi

    case "$stage" in
        compile)
            decide_compile_repair "$classification"
            ;;
        link)
            decide_link_repair "$classification"
            ;;
        debug)
            echo "$classification" | jq '. + {action: "investigate_debug", priority: 1}'
            ;;
        *)
            echo "$classification" | jq '. + {action: "unknown", priority: 0}'
            ;;
    esac
}

# ═══════════════════════════════════════════════════════════════════════════
# 命令行接口
# ═══════════════════════════════════════════════════════════════════════════

if [ "${BASH_SOURCE[0]##*/}" = "repair-decision-engine.sh" ] && [ ${#BASH_SOURCE[@]} -eq 1 ]; then
    case "${1:-help}" in
        decide)
            if [ -z "${2:-}" ]; then
                echo "Error: provide classification JSON as argument" >&2
                exit 1
            fi
            decide_repair_action "$2"
            ;;
        *)
            echo "Usage: $0 decide <classification_json>"
            exit 1
            ;;
    esac
fi
