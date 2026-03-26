#!/usr/bin/env bash
# scripts/error-classifier.sh — 编译/链接错误分类器
# 根据错误消息，自动分类并提供修复建议
#
# 使用：
#   classify_compile_error "error_message" "file" "line"
#   classify_link_error "error_message"

set -euo pipefail

# ═══════════════════════════════════════════════════════════════════════════
# 编译错误分类
# ═══════════════════════════════════════════════════════════════════════════

classify_compile_error() {
    local error_msg=$1
    local file=${2:-"unknown"}
    local line=${3:-"0"}

    local error_type="unknown"
    local subtype="unknown"
    local confidence="low"
    local fix_actions=""

    # ─────────────────────────────────────────────────────────────────
    # 1. 语法错误
    # ─────────────────────────────────────────────────────────────────

    if echo "$error_msg" | grep -qi "expected ';'"; then
        error_type="syntax_error"
        subtype="missing_semicolon"
        confidence="high"
        fix_actions='[{"priority":1,"action":"add_semicolon","description":"在行末添加分号"}]'

    elif echo "$error_msg" | grep -qi "expected declaration\|expected statement"; then
        error_type="syntax_error"
        subtype="syntax_error"
        confidence="medium"
        fix_actions='[{"priority":1,"action":"check_braces","description":"检查大括号配对"}]'

    elif echo "$error_msg" | grep -qi "expected '}'"; then
        error_type="syntax_error"
        subtype="mismatched_braces"
        confidence="high"
        fix_actions='[{"priority":1,"action":"fix_braces","description":"修复大括号不匹配"}]'

    # ─────────────────────────────────────────────────────────────────
    # 2. 类型错误
    # ─────────────────────────────────────────────────────────────────

    elif echo "$error_msg" | grep -qi "incompatible.*type\|assignment.*loss of precision"; then
        error_type="type_error"
        subtype="type_mismatch"
        confidence="high"
        fix_actions='[
            {"priority":1,"action":"add_cast","description":"添加显式类型转换"},
            {"priority":2,"action":"change_variable_type","description":"修改变量声明的类型"}
        ]'

    elif echo "$error_msg" | grep -qi "conflicting types for"; then
        error_type="type_error"
        subtype="conflicting_types"
        confidence="high"
        fix_actions='[
            {"priority":1,"action":"unify_declaration","description":"统一函数声明"}
        ]'

    # ─────────────────────────────────────────────────────────────────
    # 3. 声明错误
    # ─────────────────────────────────────────────────────────────────

    elif echo "$error_msg" | grep -qi "undeclared identifier\|'.*' undeclared"; then
        error_type="declaration_error"
        subtype="undefined_identifier"
        confidence="high"
        fix_actions='[
            {"priority":1,"action":"add_include","description":"添加相应头文件"},
            {"priority":2,"action":"declare_variable","description":"声明变量或函数"}
        ]'

    elif echo "$error_msg" | grep -qi "implicit declaration of function"; then
        error_type="declaration_error"
        subtype="implicit_function"
        confidence="high"
        fix_actions='[
            {"priority":1,"action":"add_include","description":"添加头文件包含"},
            {"priority":2,"action":"forward_declaration","description":"添加前向声明"}
        ]'

    elif echo "$error_msg" | grep -qi "redeclared"; then
        error_type="declaration_error"
        subtype="redeclared_identifier"
        confidence="high"
        fix_actions='[
            {"priority":1,"action":"remove_duplicate","description":"移除重复声明"},
            {"priority":2,"action":"add_include_guard","description":"添加 include guard"}
        ]'

    # ─────────────────────────────────────────────────────────────────
    # 4. 预处理错误
    # ─────────────────────────────────────────────────────────────────

    elif echo "$error_msg" | grep -qi "No such file or directory\|include.*not found"; then
        error_type="preprocessor_error"
        subtype="include_not_found"
        confidence="high"
        fix_actions='[
            {"priority":1,"action":"fix_include_path","description":"修正 #include 路径"},
            {"priority":2,"action":"check_file_exists","description":"验证文件是否存在"}
        ]'

    elif echo "$error_msg" | grep -qi "#error"; then
        error_type="preprocessor_error"
        subtype="macro_error"
        confidence="high"
        fix_actions='[
            {"priority":1,"action":"define_macro","description":"定义所需的宏"},
            {"priority":2,"action":"check_config","description":"检查配置是否正确"}
        ]'

    fi

    # ─────────────────────────────────────────────────────────────────
    # 输出结果（JSON 格式）
    # ─────────────────────────────────────────────────────────────────

    jq -n \
        --arg type "$error_type" \
        --arg subtype "$subtype" \
        --arg conf "$confidence" \
        --arg msg "$error_msg" \
        --arg f "$file" \
        --arg l "$line" \
        --argjson actions "$fix_actions" \
        '{
            stage: "compile",
            type: $type,
            subtype: $subtype,
            confidence: $conf,
            message: $msg,
            file: $f,
            line: ($l | tonumber),
            suggested_fixes: $actions
        }'
}

# ═══════════════════════════════════════════════════════════════════════════
# 链接错误分类
# ═══════════════════════════════════════════════════════════════════════════

classify_link_error() {
    local error_msg=$1

    local error_type="unknown"
    local subtype="unknown"
    local symbol=""
    local confidence="low"
    local fix_actions=""

    # ─────────────────────────────────────────────────────────────────
    # 1. 符号错误
    # ─────────────────────────────────────────────────────────────────

    if echo "$error_msg" | grep -qi "undefined reference"; then
        error_type="symbol_error"
        subtype="undefined_symbol"
        confidence="high"

        # 提取符号名
        symbol=$(echo "$error_msg" | sed -n "s/.*undefined reference to \`\(.*\)'.*/\1/p" || echo "")

        fix_actions='[
            {"priority":1,"action":"check_implementation","description":"检查函数是否已实现"},
            {"priority":2,"action":"add_source_file","description":"确认源文件已被编译"},
            {"priority":3,"action":"add_library","description":"检查是否需要链接库"}
        ]'

    elif echo "$error_msg" | grep -qi "multiple definition"; then
        error_type="symbol_error"
        subtype="duplicate_symbol"
        confidence="high"

        symbol=$(echo "$error_msg" | sed -n "s/.*multiple definition of \`\(.*\)'.*/\1/p" || echo "")

        fix_actions='[
            {"priority":1,"action":"move_to_source","description":"将头文件的实现移至 .c 文件"},
            {"priority":2,"action":"add_include_guard","description":"检查 include guard"}
        ]'

    # ─────────────────────────────────────────────────────────────────
    # 2. 内存错误
    # ─────────────────────────────────────────────────────────────────

    elif echo "$error_msg" | grep -qi "FLASH.*overflowed\|Flash.*overflow"; then
        error_type="memory_error"
        subtype="flash_overflow"
        confidence="high"

        # 提取溢出大小
        overflow_size=$(echo "$error_msg" | sed -n 's/.*overflow.*\([0-9]\+\).*/\1/p' || echo "unknown")

        fix_actions='[
            {"priority":1,"action":"enable_optimization","description":"启用编译优化 (-Os)"},
            {"priority":2,"action":"enable_gc_sections","description":"启用 --gc-sections"},
            {"priority":3,"action":"verify_flash_size","description":"验证 Flash 大小配置"}
        ]'

    elif echo "$error_msg" | grep -qi "RAM.*overflowed\|RAM.*overflow"; then
        error_type="memory_error"
        subtype="ram_overflow"
        confidence="high"

        fix_actions='[
            {"priority":1,"action":"move_to_flash","description":"将数据移至 Flash"},
            {"priority":2,"action":"reduce_global_data","description":"减少全局变量"}
        ]'

    # ─────────────────────────────────────────────────────────────────
    # 3. 库错误
    # ─────────────────────────────────────────────────────────────────

    elif echo "$error_msg" | grep -qi "cannot find.*-l"; then
        error_type="library_error"
        subtype="missing_library"
        confidence="high"

        fix_actions='[
            {"priority":1,"action":"add_library_path","description":"检查库路径配置"},
            {"priority":2,"action":"verify_toolchain","description":"验证工具链是否完整"}
        ]'

    fi

    # ─────────────────────────────────────────────────────────────────
    # 输出结果
    # ─────────────────────────────────────────────────────────────────

    jq -n \
        --arg type "$error_type" \
        --arg subtype "$subtype" \
        --arg sym "$symbol" \
        --arg conf "$confidence" \
        --arg msg "$error_msg" \
        --argjson actions "$fix_actions" \
        '{
            stage: "link",
            type: $type,
            subtype: $subtype,
            symbol: $sym,
            confidence: $conf,
            message: $msg,
            suggested_fixes: $actions
        }'
}

# ═══════════════════════════════════════════════════════════════════════════
# 调试问题分类
# ═══════════════════════════════════════════════════════════════════════════

classify_debug_issue() {
    local serial_output=$1
    local registers_json=${2:-'{}'}
    local variables_json=${3:-'{}'}

    local issue_type="unknown"
    local severity="low"
    local suggested_action=""

    # ─────────────────────────────────────────────────────────────────
    # 检查初始化问题
    # ─────────────────────────────────────────────────────────────────

    if echo "$serial_output" | grep -qi "error\|fail\|panic"; then
        issue_type="runtime_error"
        severity="high"
        suggested_action="检查串口输出中的具体错误信息，回编译阶段添加调试日志"

    elif echo "$serial_output" | grep -qi "timeout\|hang\|stuck"; then
        issue_type="timeout_error"
        severity="high"
        suggested_action="检查初始化超时，可能需要调整时序参数"

    elif echo "$serial_output" | grep -qi "ok\|success\|ready\|initialized"; then
        issue_type="normal_operation"
        severity="low"
        suggested_action="模块初始化成功，继续验证功能"

    else
        issue_type="unknown_state"
        severity="medium"
        suggested_action="输出不清晰，需要人工检查或添加更多调试信息"
    fi

    # ─────────────────────────────────────────────────────────────────
    # 输出结果
    # ─────────────────────────────────────────────────────────────────

    jq -n \
        --arg type "$issue_type" \
        --arg sev "$severity" \
        --arg action "$suggested_action" \
        --arg output "$serial_output" \
        '{
            stage: "debug",
            type: $type,
            severity: $sev,
            suggested_action: $action,
            serial_output: $output
        }'
}

# ═══════════════════════════════════════════════════════════════════════════
# 命令行接口
# ═══════════════════════════════════════════════════════════════════════════

if [ "${BASH_SOURCE[0]##*/}" = "error-classifier.sh" ] && [ ${#BASH_SOURCE[@]} -eq 1 ]; then
    case "${1:-help}" in
        classify-compile)
            classify_compile_error "${2:-}" "${3:-}" "${4:-}"
            ;;
        classify-link)
            classify_link_error "${2:-}"
            ;;
        classify-debug)
            classify_debug_issue "${2:-}" "${3:-}" "${4:-}"
            ;;
        *)
            echo "Usage: $0 <command> [args]"
            echo ""
            echo "Commands:"
            echo "  classify-compile <msg> [file] [line]  - 分类编译错误"
            echo "  classify-link <msg>                   - 分类链接错误"
            echo "  classify-debug <output> [regs] [vars] - 分类调试问题"
            exit 1
            ;;
    esac
fi
