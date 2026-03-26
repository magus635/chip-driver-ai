#!/usr/bin/env bash
# scripts/test-phase2-integration.sh — Phase 2 集成测试
# 测试错误分类器和决策引擎的集成

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# 导入库
source scripts/error-classifier.sh
source scripts/repair-decision-engine.sh

# ═══════════════════════════════════════════════════════════════════════════
# 测试函数
# ═══════════════════════════════════════════════════════════════════════════

test_compile_error_classification() {
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║ 测试 1: 编译错误分类                                          ║"
    echo "╚════════════════════════════════════════════════════════════════╝"

    local test_count=0
    local passed=0

    # 测试 1.1: 缺少分号
    echo ""
    echo "  [1.1] 缺少分号错误"
    local result=$(classify_compile_error "expected ';' before '}'" "src/main.c" "42")
    local type=$(echo "$result" | jq -r '.type')
    local subtype=$(echo "$result" | jq -r '.subtype')
    test_count=$((test_count + 1))
    if [ "$type" = "syntax_error" ] && [ "$subtype" = "missing_semicolon" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 syntax_error/missing_semicolon，得到 $type/$subtype"
    fi

    # 测试 1.2: 类型不匹配
    echo ""
    echo "  [1.2] 类型不匹配错误"
    result=$(classify_compile_error "incompatible pointer type" "src/driver.c" "100")
    type=$(echo "$result" | jq -r '.type')
    subtype=$(echo "$result" | jq -r '.subtype')
    test_count=$((test_count + 1))
    if [ "$type" = "type_error" ] && [ "$subtype" = "type_mismatch" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 type_error/type_mismatch，得到 $type/$subtype"
    fi

    # 测试 1.3: 未声明标识符
    echo ""
    echo "  [1.3] 未声明标识符"
    result=$(classify_compile_error "undeclared identifier 'can_init'" "src/main.c" "50")
    type=$(echo "$result" | jq -r '.type')
    subtype=$(echo "$result" | jq -r '.subtype')
    test_count=$((test_count + 1))
    if [ "$type" = "declaration_error" ] && [ "$subtype" = "undefined_identifier" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 declaration_error/undefined_identifier，得到 $type/$subtype"
    fi

    # 测试 1.4: 头文件未找到
    echo ""
    echo "  [1.4] 头文件未找到"
    result=$(classify_compile_error "No such file or directory" "src/hal.c" "5")
    type=$(echo "$result" | jq -r '.type')
    subtype=$(echo "$result" | jq -r '.subtype')
    test_count=$((test_count + 1))
    if [ "$type" = "preprocessor_error" ] && [ "$subtype" = "include_not_found" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 preprocessor_error/include_not_found，得到 $type/$subtype"
    fi

    echo ""
    echo "编译错误分类: $passed/$test_count 通过"
    return $([ $passed -eq $test_count ] && echo 0 || echo 1)
}

test_link_error_classification() {
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║ 测试 2: 链接错误分类                                          ║"
    echo "╚════════════════════════════════════════════════════════════════╝"

    local test_count=0
    local passed=0

    # 测试 2.1: 未定义符号
    echo ""
    echo "  [2.1] 未定义符号"
    local result=$(classify_link_error "undefined reference to \`can_init'")
    local type=$(echo "$result" | jq -r '.type')
    local subtype=$(echo "$result" | jq -r '.subtype')
    local symbol=$(echo "$result" | jq -r '.symbol')
    test_count=$((test_count + 1))
    if [ "$type" = "symbol_error" ] && [ "$subtype" = "undefined_symbol" ] && [ "$symbol" = "can_init" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 symbol_error/undefined_symbol，得到 $type/$subtype (symbol: $symbol)"
    fi

    # 测试 2.2: 重复定义符号
    echo ""
    echo "  [2.2] 重复定义符号"
    result=$(classify_link_error "multiple definition of \`init_gpio'")
    type=$(echo "$result" | jq -r '.type')
    subtype=$(echo "$result" | jq -r '.subtype')
    test_count=$((test_count + 1))
    if [ "$type" = "symbol_error" ] && [ "$subtype" = "duplicate_symbol" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 symbol_error/duplicate_symbol，得到 $type/$subtype"
    fi

    # 测试 2.3: Flash 溢出
    echo ""
    echo "  [2.3] Flash 内存溢出"
    result=$(classify_link_error "region FLASH overflowed by 1024")
    type=$(echo "$result" | jq -r '.type')
    subtype=$(echo "$result" | jq -r '.subtype')
    test_count=$((test_count + 1))
    if [ "$type" = "memory_error" ] && [ "$subtype" = "flash_overflow" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 memory_error/flash_overflow，得到 $type/$subtype"
    fi

    echo ""
    echo "链接错误分类: $passed/$test_count 通过"
    return $([ $passed -eq $test_count ] && echo 0 || echo 1)
}

test_compile_decision() {
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║ 测试 3: 编译错误决策引擎                                      ║"
    echo "╚════════════════════════════════════════════════════════════════╝"

    local test_count=0
    local passed=0

    # 测试 3.1: 缺少分号的决策
    echo ""
    echo "  [3.1] 缺少分号的修复决策"
    local classification=$(classify_compile_error "expected ';' before '}'" "src/main.c" "42")
    local decision=$(decide_repair_action "$classification")
    local action=$(echo "$decision" | jq -r '.action')
    local priority=$(echo "$decision" | jq -r '.priority')
    test_count=$((test_count + 1))
    if [ "$action" = "add_semicolon" ] && [ "$priority" = "1" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 add_semicolon/priority 1，得到 $action/$priority"
    fi

    # 测试 3.2: 类型不匹配的决策
    echo ""
    echo "  [3.2] 类型不匹配的修复决策"
    classification=$(classify_compile_error "incompatible pointer type" "src/driver.c" "100")
    decision=$(decide_repair_action "$classification")
    action=$(echo "$decision" | jq -r '.action')
    test_count=$((test_count + 1))
    if [ "$action" = "fix_type_mismatch" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 fix_type_mismatch，得到 $action"
    fi

    # 测试 3.3: 检查步骤和AI指令
    echo ""
    echo "  [3.3] 决策包含步骤和AI指令"
    classification=$(classify_compile_error "undeclared identifier 'can_init'" "src/main.c" "50")
    decision=$(decide_repair_action "$classification")
    local steps_count=$(echo "$decision" | jq '.steps | length')
    local ai_instr=$(echo "$decision" | jq -r '.ai_instructions')
    test_count=$((test_count + 1))
    if [ "$steps_count" -gt 0 ] && [ -n "$ai_instr" ] && [ "$ai_instr" != "null" ]; then
        echo "  ✅ PASS (步骤数: $steps_count)"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 决策缺少步骤或AI指令"
    fi

    echo ""
    echo "编译决策: $passed/$test_count 通过"
    return $([ $passed -eq $test_count ] && echo 0 || echo 1)
}

test_link_decision() {
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║ 测试 4: 链接错误决策引擎                                      ║"
    echo "╚════════════════════════════════════════════════════════════════╝"

    local test_count=0
    local passed=0

    # 测试 4.1: 未定义符号的决策
    echo ""
    echo "  [4.1] 未定义符号的修复决策"
    local classification=$(classify_link_error "undefined reference to \`can_init'")
    local decision=$(decide_repair_action "$classification")
    local action=$(echo "$decision" | jq -r '.action')
    test_count=$((test_count + 1))
    if [ "$action" = "resolve_undefined_symbol" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 resolve_undefined_symbol，得到 $action"
    fi

    # 测试 4.2: Flash溢出的决策
    echo ""
    echo "  [4.2] Flash 溢出的修复决策"
    classification=$(classify_link_error "region FLASH overflowed by 1024")
    decision=$(decide_repair_action "$classification")
    action=$(echo "$decision" | jq -r '.action')
    test_count=$((test_count + 1))
    if [ "$action" = "optimize_flash_usage" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 optimize_flash_usage，得到 $action"
    fi

    echo ""
    echo "链接决策: $passed/$test_count 通过"
    return $([ $passed -eq $test_count ] && echo 0 || echo 1)
}

test_debug_classification() {
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo "║ 测试 5: 调试问题分类                                          ║"
    echo "╚════════════════════════════════════════════════════════════════╝"

    local test_count=0
    local passed=0

    # 测试 5.1: 正常运行
    echo ""
    echo "  [5.1] 正常运行检测"
    local result=$(classify_debug_issue "CAN init: OK")
    local type=$(echo "$result" | jq -r '.type')
    test_count=$((test_count + 1))
    if [ "$type" = "normal_operation" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 normal_operation，得到 $type"
    fi

    # 测试 5.2: 运行时错误
    echo ""
    echo "  [5.2] 运行时错误检测"
    result=$(classify_debug_issue "ERROR: CAN bus fault")
    type=$(echo "$result" | jq -r '.type')
    test_count=$((test_count + 1))
    if [ "$type" = "runtime_error" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 runtime_error，得到 $type"
    fi

    # 测试 5.3: 超时错误
    echo ""
    echo "  [5.3] 超时错误检测"
    result=$(classify_debug_issue "timeout waiting for response")
    type=$(echo "$result" | jq -r '.type')
    test_count=$((test_count + 1))
    if [ "$type" = "timeout_error" ]; then
        echo "  ✅ PASS"
        passed=$((passed + 1))
    else
        echo "  ❌ FAIL: 期望 timeout_error，得到 $type"
    fi

    echo ""
    echo "调试分类: $passed/$test_count 通过"
    return $([ $passed -eq $test_count ] && echo 0 || echo 1)
}

# ═══════════════════════════════════════════════════════════════════════════
# 主测试流程
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║ Phase 2 集成测试套件                                          ║"
echo "║ 测试错误分类器和决策引擎的完整功能                             ║"
echo "╚════════════════════════════════════════════════════════════════╝"

total_tests=0
total_passed=0

# 运行所有测试
test_compile_error_classification && ((total_passed++)) || true
((total_tests++))

test_link_error_classification && ((total_passed++)) || true
((total_tests++))

test_compile_decision && ((total_passed++)) || true
((total_tests++))

test_link_decision && ((total_passed++)) || true
((total_tests++))

test_debug_classification && ((total_passed++)) || true
((total_tests++))

# 输出总结
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║ 测试总结                                                      ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "总测试组数: $total_tests"
echo "通过: $total_passed"
echo "失败: $((total_tests - total_passed))"

if [ $total_passed -eq $total_tests ]; then
    echo ""
    echo "✅ 所有测试组通过！Phase 2 集成完成"
    exit 0
else
    echo ""
    echo "❌ 部分测试失败，请检查上述输出"
    exit 1
fi
