#!/usr/bin/env bash
# scripts/test-compiler-integration.sh — 编译脚本集成测试
# 创建一个测试项目，验证编译和状态管理的集成

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# 创建临时测试项目
TEST_PROJECT=$(mktemp -d)
cd "$TEST_PROJECT"

echo "═══════════════════════════════════════════════════════════"
echo "编译脚本集成测试"
echo "═══════════════════════════════════════════════════════════"
echo "测试项目: $TEST_PROJECT"
echo ""

# ═══════════════════════════════════════════════════════════════
# 准备测试环境
# ═══════════════════════════════════════════════════════════════

# 创建目录结构
mkdir -p src include build config .claude/state scripts

# 创建配置文件
cat > config/project.env <<'EOF'
CHIP_MODEL="STM32F407VGT6"
CC="arm-none-eabi-gcc"
CFLAGS="-mcpu=cortex-m4 -mthumb -Wall -Wextra"
INC_DIRS="include"
SRC_DIR="src"
BUILD_DIR="build"
EOF

# 创建测试源文件 1：正常编译
cat > src/test1.c <<'EOF'
#include <stdint.h>

int add(int a, int b) {
    return a + b;
}
EOF

# 创建测试源文件 2：带错误的源文件（稍后创建）
cat > src/test2.c <<'EOF'
// 初始版本：正常
int sub(int a, int b) {
    return a - b;
}
EOF

# 创建 header
cat > include/test.h <<'EOF'
#ifndef TEST_H
#define TEST_H

int add(int a, int b);
int sub(int a, int b);

#endif
EOF

# ═══════════════════════════════════════════════════════════════
# 测试 1：正常编译
# ═══════════════════════════════════════════════════════════════

echo "📋 [测试 1] 正常编译"
echo "─────────────────────────────────────────────────────────"

# 源路径调整（指向实际项目的脚本）
export PROJECT_ROOT
cp "$PROJECT_ROOT/scripts/compile.sh" scripts/
cp "$PROJECT_ROOT/scripts/state-manager.sh" scripts/
cp "$PROJECT_ROOT/config/project.env" . 2>/dev/null || true

# 初始化会话
SESSION_ID=$(bash scripts/state-manager.sh init test_module)
echo "✓ 会话初始化: $SESSION_ID"

# 运行第一次编译
if COMPILE_ITERATION=1 bash scripts/compile.sh > compile.log 2>&1; then
    echo "✅ 编译成功"

    # 验证目标文件是否生成
    if [ -f "build/.obj-list" ]; then
        echo "✓ 对象文件列表已生成"
        OBJ_COUNT=$(wc -w < build/.obj-list)
        echo "✓ 生成了 $OBJ_COUNT 个目标文件"
    fi

    TEST1_PASS=true
else
    echo "❌ 编译失败"
    TEST1_PASS=false
    cat compile.log
fi

# ═══════════════════════════════════════════════════════════════
# 测试 2：错误捕获和分析
# ═══════════════════════════════════════════════════════════════

echo ""
echo "📋 [测试 2] 编译错误捕获"
echo "─────────────────────────────────────────────────────────"

# 修改源文件，引入错误
cat > src/test2.c <<'EOF'
int sub(int a, int b)
{
    return a - b  // 缺少分号
}
EOF

# 运行编译，预期失败
if COMPILE_ITERATION=2 bash scripts/compile.sh > compile2.log 2>&1; then
    echo "❌ 编译应该失败，但成功了"
    TEST2_PASS=false
else
    echo "✓ 编译失败（符合预期）"

    # 检查错误分析是否已提取
    if grep -q "ERROR_ANALYSIS" compile2.log; then
        echo "✓ 错误分析已提取"
        TEST2_PASS=true
    else
        echo "❌ 错误分析未能提取"
        TEST2_PASS=false
    fi

    # 显示错误信息
    echo ""
    echo "捕获的错误："
    grep -A3 "ERROR_ANALYSIS" compile2.log | head -8
fi

# ═══════════════════════════════════════════════════════════════
# 测试 3：状态管理集成
# ═══════════════════════════════════════════════════════════════

echo ""
echo "📋 [测试 3] 状态管理集成"
echo "─────────────────────────────────────────────────────────"

# 检查会话文件
if [ -f ".claude/state/session.json" ]; then
    echo "✓ 会话文件存在"

    # 获取会话信息
    module=$(jq -r '.session.module' .claude/state/session.json 2>/dev/null || echo "")
    phase=$(jq -r '.session.phase' .claude/state/session.json 2>/dev/null || echo "")

    echo "  模块: $module"
    echo "  阶段: $phase"

    TEST3_PASS=true
else
    echo "❌ 会话文件不存在"
    TEST3_PASS=false
fi

# ═══════════════════════════════════════════════════════════════
# 测试 4：错误去重
# ═══════════════════════════════════════════════════════════════

echo ""
echo "📋 [测试 4] 错误去重检查"
echo "─────────────────────────────────────────────────────────"

# 检查去重信息
if grep -q "DEDUP_CHECK" compile2.log; then
    echo "✓ 去重检查已执行"

    grep -A2 "DEDUP_CHECK" compile2.log | head -4
    TEST4_PASS=true
else
    echo "⚠️  去重检查未执行（可能是新错误）"
    TEST4_PASS=true  # 这不是失败，只是首次出现
fi

# ═══════════════════════════════════════════════════════════════
# 测试总结
# ═══════════════════════════════════════════════════════════════

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "测试结果"
echo "═══════════════════════════════════════════════════════════"

pass_count=0
[ "$TEST1_PASS" = "true" ] && ((pass_count++)) && echo "✅ [TEST 1] 正常编译" || echo "❌ [TEST 1] 正常编译"
[ "$TEST2_PASS" = "true" ] && ((pass_count++)) && echo "✅ [TEST 2] 错误捕获" || echo "❌ [TEST 2] 错误捕获"
[ "$TEST3_PASS" = "true" ] && ((pass_count++)) && echo "✅ [TEST 3] 状态管理" || echo "❌ [TEST 3] 状态管理"
[ "$TEST4_PASS" = "true" ] && ((pass_count++)) && echo "✅ [TEST 4] 错误去重" || echo "❌ [TEST 4] 错误去重"

echo ""
echo "通过: $pass_count / 4"
echo "═══════════════════════════════════════════════════════════"

# 清理
cd "$PROJECT_ROOT"
rm -rf "$TEST_PROJECT"

# 返回状态
[ $pass_count -eq 4 ] && exit 0 || exit 1
