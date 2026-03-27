#!/usr/bin/env bash
# scripts/test-error-classification.sh — 错误分类和决策引擎测试

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

source scripts/error-classifier.sh
source scripts/repair-decision-engine.sh

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

TESTS_PASSED=0
TESTS_FAILED=0

# ═══════════════════════════════════════════════════════════════════════════
# 测试工具函数
# ═══════════════════════════════════════════════════════════════════════════

test_classification() {
    local name=$1
    local error_type=$2
    local expected_type=$3

    echo ""
    echo "测试：$name"
    echo "  错误：$error_type"

    # 获取分类结果
    local result=$(classify_compile_error "$error_type" "test.c" "42")
    local actual_type=$(echo "$result" | jq -r '.type')

    if [ "$actual_type" = "$expected_type" ]; then
        echo -e "  ${GREEN}✓ PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo "$result" | jq '.subtype, .confidence'
    else
        echo -e "  ${RED}✗ FAIL${NC}"
        echo "    期望：$expected_type，实际：$actual_type"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

test_decision() {
    local name=$1
    local classification=$2
    local expected_action=$3

    echo ""
    echo "测试决策：$name"

    # 获取决策结果
    local result=$(decide_repair_action "$classification")
    local actual_action=$(echo "$result" | jq -r '.action')

    if [ "$actual_action" = "$expected_action" ]; then
        echo -e "  ${GREEN}✓ PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo "$result" | jq '.ai_instructions'
    else
        echo -e "  ${RED}✗ FAIL${NC}"
        echo "    期望 action：$expected_action，实际：$actual_action"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# ═══════════════════════════════════════════════════════════════════════════
# 编译错误分类测试
# ═══════════════════════════════════════════════════════════════════════════

echo "═══════════════════════════════════════════════════════════"
echo "错误分类测试"
echo "═══════════════════════════════════════════════════════════"

# 语法错误测试
test_classification "缺少分号" "expected ';' before" "syntax_error"
test_classification "大括号不匹配" "expected '}'" "syntax_error"

# 类型错误测试
test_classification "类型不匹配" "assignment to 'uint32_t' from 'uint16_t' with loss of precision" "type_error"
test_classification "隐式函数声明" "implicit declaration of function 'printf'" "declaration_error"

# 声明错误测试
test_classification "未定义标识符" "undeclared identifier 'CAN_Init'" "declaration_error"

# 预处理错误测试
test_classification "头文件不存在" "fatal error: can.h: No such file or directory" "preprocessor_error"

# ═══════════════════════════════════════════════════════════════════════════
# 链接错误分类测试
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "链接错误分类测试"
echo "═══════════════════════════════════════════════════════════"

# 符号错误
link_classification=$(classify_link_error "undefined reference to \`can_init'")
echo ""
echo "链接错误分类：未定义符号"
echo "$link_classification" | jq '{type, subtype, symbol}'

# ═══════════════════════════════════════════════════════════════════════════
# 修复决策测试
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "修复决策测试"
echo "═══════════════════════════════════════════════════════════"

# 测试用例 1：缺少分号
semicolon_class=$(classify_compile_error "expected ';'" "main.c" "10")
test_decision "缺少分号的修复" "$semicolon_class" "add_semicolon"

# 测试用例 2：类型不匹配
type_class=$(classify_compile_error "assignment to 'uint32_t' from 'uint16_t'" "driver.c" "25")
test_decision "类型不匹配的修复" "$type_class" "fix_type_mismatch"

# 测试用例 3：链接 - 未定义符号
link_decision=$(echo '{"type":"symbol_error","subtype":"undefined_symbol","symbol":"can_init","stage":"link"}' | jq .)
result=$(decide_repair_action "$link_decision")
echo ""
echo "测试决策：未定义符号的修复"
echo "  Action: $(echo "$result" | jq -r '.action')"
TESTS_PASSED=$((TESTS_PASSED + 1))

# ═══════════════════════════════════════════════════════════════════════════
# 测试总结
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "测试结果"
echo "═══════════════════════════════════════════════════════════"
echo -e "通过：${GREEN}$TESTS_PASSED${NC}"
echo -e "失败：${RED}$TESTS_FAILED${NC}"
echo "═══════════════════════════════════════════════════════════"

[ $TESTS_FAILED -eq 0 ] && exit 0 || exit 1
