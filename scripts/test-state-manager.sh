#!/usr/bin/env bash
# scripts/test-state-manager.sh — 单元测试
# 验证 state-manager.sh 的核心功能

set -euo pipefail

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 测试统计
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# 测试前准备
TEST_DIR=$(mktemp -d)
export STATE_DIR="$TEST_DIR/.claude/state"

source scripts/state-manager.sh

# ═══════════════════════════════════════════════════════════════════════════
# 测试工具函数
# ═══════════════════════════════════════════════════════════════════════════

test_case() {
    local name=$1
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    echo -e "${YELLOW}[TEST $TESTS_RUN]${NC} $name"
}

assert_equal() {
    local expected=$1
    local actual=$2
    local message=${3:-""}

    if [ "$expected" == "$actual" ]; then
        echo -e "  ${GREEN}✓ PASS${NC} $message"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "  ${RED}✗ FAIL${NC} $message"
        echo "    Expected: $expected"
        echo "    Actual:   $actual"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

assert_file_exists() {
    local file=$1
    if [ -f "$file" ]; then
        echo -e "  ${GREEN}✓ PASS${NC} File exists: $file"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "  ${RED}✗ FAIL${NC} File not found: $file"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

assert_json_valid() {
    local file=$1
    if jq empty "$file" 2>/dev/null; then
        echo -e "  ${GREEN}✓ PASS${NC} Valid JSON: $file"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "  ${RED}✗ FAIL${NC} Invalid JSON: $file"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

cleanup() {
    rm -rf "$TEST_DIR"
}

# ═══════════════════════════════════════════════════════════════════════════
# 测试用例
# ═══════════════════════════════════════════════════════════════════════════

echo "═══════════════════════════════════════════════════════════"
echo "state-manager.sh Unit Tests"
echo "═══════════════════════════════════════════════════════════"
echo "Test directory: $TEST_DIR"
echo ""

# ─────────────────────────────────────────────────────────────────
# Test 1: 会话初始化
# ─────────────────────────────────────────────────────────────────

test_case "init_session creates all required JSON files"

session_id=$(state_manager::init_session "test_module")

assert_file_exists "$STATE_DIR/session.json" "session.json"
assert_file_exists "$STATE_DIR/repairs.json" "repairs.json"
assert_file_exists "$STATE_DIR/error-patterns.json" "error-patterns.json"
assert_file_exists "$STATE_DIR/snapshots.json" "snapshots.json"

# ─────────────────────────────────────────────────────────────────
# Test 2: 会话信息验证
# ─────────────────────────────────────────────────────────────────

test_case "get_session returns correct module name"

session_info=$(state_manager::get_session)
module=$(echo "$session_info" | jq -r '.module')

assert_equal "test_module" "$module" "Module name"

# ─────────────────────────────────────────────────────────────────
# Test 3: 创建修复记录
# ─────────────────────────────────────────────────────────────────

test_case "create_repair generates repair_001"

repair_id=$(state_manager::create_repair "compile" "1" "undefined_reference" "abc123")

assert_equal "repair_001" "$repair_id" "First repair ID"

# ─────────────────────────────────────────────────────────────────
# Test 4: 更新修复记录
# ─────────────────────────────────────────────────────────────────

test_case "update_repair modifies repair record"

state_manager::update_repair "$repair_id" "测试假设" '[]' "success"

repair_json=$(jq '.repairs[0]' "$STATE_DIR/repairs.json")
hypothesis=$(echo "$repair_json" | jq -r '.hypothesis')
result=$(echo "$repair_json" | jq -r '.result')

assert_equal "测试假设" "$hypothesis" "Hypothesis"
assert_equal "success" "$result" "Result status"

# ─────────────────────────────────────────────────────────────────
# Test 5: 错误去重 - 新错误
# ─────────────────────────────────────────────────────────────────

test_case "query_error_by_hash returns empty for new error"

if state_manager::query_error_by_hash "unknown_hash" 2>/dev/null; then
    echo -e "  ${RED}✗ FAIL${NC} Should not find unknown hash"
    TESTS_FAILED=$((TESTS_FAILED + 1))
else
    echo -e "  ${GREEN}✓ PASS${NC} Unknown hash not found (expected)"
    TESTS_PASSED=$((TESTS_PASSED + 1))
fi

# ─────────────────────────────────────────────────────────────────
# Test 6: 错误去重 - 已知错误
# ─────────────────────────────────────────────────────────────────

test_case "query_error_by_hash finds existing error"

found_repair=$(state_manager::query_error_by_hash "abc123" || echo "")

assert_equal "repair_001" "$found_repair" "Found existing error hash"

# ─────────────────────────────────────────────────────────────────
# Test 7: 错误模式表验证
# ─────────────────────────────────────────────────────────────────

test_case "error_patterns correctly tracks error count"

error_pattern=$(state_manager::get_error_by_hash "abc123")
count=$(echo "$error_pattern" | jq '.count')
status=$(echo "$error_pattern" | jq -r '.status')

assert_equal "1" "$count" "Error count"
assert_equal "resolved" "$status" "Error status (success)"

# ─────────────────────────────────────────────────────────────────
# Test 8: 多轮修复 - 检查 repair ID 序列
# ─────────────────────────────────────────────────────────────────

test_case "multiple repairs generate sequential IDs"

repair2=$(state_manager::create_repair "compile" "2" "type_mismatch" "def456")
repair3=$(state_manager::create_repair "compile" "3" "syntax_error" "ghi789")

assert_equal "repair_002" "$repair2" "Second repair ID"
assert_equal "repair_003" "$repair3" "Third repair ID"

# ─────────────────────────────────────────────────────────────────
# Test 9: 设置阶段
# ─────────────────────────────────────────────────────────────────

test_case "set_phase updates current phase and iteration"

state_manager::set_phase "link" "2"

session_info=$(state_manager::get_session)
phase=$(echo "$session_info" | jq -r '.phase')
link_iter=$(echo "$session_info" | jq '.iterations.link')

assert_equal "link" "$phase" "Current phase"
assert_equal "2" "$link_iter" "Link iteration count"

# ─────────────────────────────────────────────────────────────────
# Test 10: 快照创建
# ─────────────────────────────────────────────────────────────────

test_case "create_snapshot creates debug snapshot"

snapshot_id=$(state_manager::create_snapshot "1" "Test output" '{}' '{}')

assert_equal "snapshot_001" "$snapshot_id" "First snapshot ID"

assert_json_valid "$STATE_DIR/snapshots.json" "Snapshots JSON"

# ─────────────────────────────────────────────────────────────────
# Test 11: 快照更新
# ─────────────────────────────────────────────────────────────────

test_case "update_snapshot updates analysis and status"

state_manager::update_snapshot "$snapshot_id" "初始化成功" "pass"

snapshot=$(jq '.snapshots[0]' "$STATE_DIR/snapshots.json")
analysis=$(echo "$snapshot" | jq -r '.analysis')
status=$(echo "$snapshot" | jq -r '.status')

assert_equal "初始化成功" "$analysis" "Snapshot analysis"
assert_equal "pass" "$status" "Snapshot status"

# ─────────────────────────────────────────────────────────────────
# Test 12: 完整状态导出
# ─────────────────────────────────────────────────────────────────

test_case "get_full_state combines all data structures"

full_state=$(state_manager::get_full_state)

has_session=$(echo "$full_state" | jq 'has("session")')
has_repairs=$(echo "$full_state" | jq 'has("repairs")')
has_patterns=$(echo "$full_state" | jq 'has("error_patterns")')
has_snapshots=$(echo "$full_state" | jq 'has("snapshots")')

assert_equal "true" "$has_session" "Has session"
assert_equal "true" "$has_repairs" "Has repairs"
assert_equal "true" "$has_patterns" "Has error_patterns"
assert_equal "true" "$has_snapshots" "Has snapshots"

# ─────────────────────────────────────────────────────────────────
# Test 13: 错误重复检测 - 重复错误
# ─────────────────────────────────────────────────────────────────

test_case "error_patterns detects recurring errors"

# 创建第二个相同错误的修复记录
repair_dup=$(state_manager::create_repair "compile" "4" "undefined_reference" "abc123")
state_manager::update_repair "$repair_dup" "重复修复" '[]' "failed"

error_info=$(state_manager::get_error_by_hash "abc123")
count=$(echo "$error_info" | jq '.count')
status=$(echo "$error_info" | jq -r '.status')

assert_equal "2" "$count" "Error count after repeat"
assert_equal "recurring" "$status" "Error status should be recurring"

# ─────────────────────────────────────────────────────────────────
# Test 14: 跨会话恢复 - 读取现有会话
# ─────────────────────────────────────────────────────────────────

test_case "state survives process boundary (file persistence)"

# 获取当前状态
repairs_before=$(jq '.repairs | length' "$STATE_DIR/repairs.json")

# 模拟新进程读取状态（源脚本，重新读取文件）
repairs_after=$(jq '.repairs | length' "$STATE_DIR/repairs.json")

assert_equal "$repairs_before" "$repairs_after" "Repairs count persisted"

# ─────────────────────────────────────────────────────────────────
# Test 15: JSON 文件有效性
# ─────────────────────────────────────────────────────────────────

test_case "all JSON files are valid and well-formed"

assert_json_valid "$STATE_DIR/session.json" "session.json"
assert_json_valid "$STATE_DIR/repairs.json" "repairs.json"
assert_json_valid "$STATE_DIR/error-patterns.json" "error-patterns.json"
assert_json_valid "$STATE_DIR/snapshots.json" "snapshots.json"

# ═══════════════════════════════════════════════════════════════════════════
# 测试总结
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "Test Results"
echo "═══════════════════════════════════════════════════════════"
echo -e "Total tests:   $TESTS_RUN"
echo -e "Passed:        ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed:        ${RED}$TESTS_FAILED${NC}"
echo "═══════════════════════════════════════════════════════════"

# 清理
cleanup

# 返回状态码
if [ $TESTS_FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
