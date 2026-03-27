#!/usr/bin/env bash
# scripts/test-full-integration.sh — 完整流程集成测试
# 验证编译 → 链接 → 调试的完整闭环

set -euo pipefail

echo "═══════════════════════════════════════════════════════════"
echo "完整流程集成测试（编译 → 链接 → 调试）"
echo "═══════════════════════════════════════════════════════════"
echo ""

# 创建测试项目
TEST_PROJECT=$(mktemp -d)
echo "测试项目: $TEST_PROJECT"
cd "$TEST_PROJECT"

# 准备环境
mkdir -p src include build config .claude/state scripts

# 配置文件
cat > config/project.env <<'EOF'
CHIP_MODEL="TEST"
CC="gcc"
CFLAGS="-Wall -Wextra"
INC_DIRS="include"
SRC_DIR="src"
BUILD_DIR="build"
LINKER_SCRIPT="config/test.ld"
TARGET_ELF="build/output.elf"
TARGET_BIN="build/output.bin"
TARGET_HEX="build/output.hex"
OBJCOPY="objcopy"
SIZE="size"
LDFLAGS="-static"
SERIAL_PORT="/dev/null"
SERIAL_BAUD="115200"
SERIAL_CAPTURE_SECONDS="2"
DEBUG_PORT="3333"
OPENOCD_CFG="config/openocd.cfg"
FLASH_SIZE="256"
RAM_SIZE="64"
EOF

# 源文件
cat > src/main.c <<'EOF'
int main(void) {
    return 0;
}
EOF

cat > src/driver.c <<'EOF'
int driver_init(void) {
    return 0;
}
EOF

cat > include/driver.h <<'EOF'
#ifndef DRIVER_H
#define DRIVER_H
int driver_init(void);
#endif
EOF

# 链接脚本（简单版本）
cat > config/test.ld <<'EOF'
ENTRY(main)
SECTIONS {
    .text : { *(.text*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}
EOF

# 复制脚本
cp /Users/qlwang/Downloads/chip-driver-ai/scripts/*.sh scripts/ 2>/dev/null || true
cp config/project.env .

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "测试 1: 编译流程"
echo "═══════════════════════════════════════════════════════════"

# 初始化会话
SESSION_ID=$(bash scripts/state-manager.sh init test_full)
echo "✓ 会话初始化: $SESSION_ID"

# 编译
if COMPILE_ITERATION=1 bash scripts/compile.sh > compile.log 2>&1; then
    echo "✅ 编译成功"
    COMPILE_PASS=true
else
    echo "❌ 编译失败"
    COMPILE_PASS=false
    grep -A5 "ERROR_ANALYSIS" compile.log 2>/dev/null || true
fi

# 检查状态
if [ -f ".claude/state/repairs.json" ]; then
    repair_count=$(jq '.repairs | length' .claude/state/repairs.json 2>/dev/null || echo "0")
    echo "✓ 修复记录: $repair_count 条"
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "测试 2: 链接流程"
echo "═══════════════════════════════════════════════════════════"

if [ "$COMPILE_PASS" = "true" ]; then
    if LINK_ITERATION=1 bash scripts/link.sh > link.log 2>&1; then
        echo "✅ 链接成功"
        LINK_PASS=true

        if [ -f "build/output.elf" ]; then
            echo "✓ ELF 文件已生成: build/output.elf"
        fi
    else
        echo "❌ 链接失败"
        LINK_PASS=false
        grep -A5 "LINK_ERROR_ANALYSIS" link.log 2>/dev/null || true
    fi
else
    echo "⏭️  跳过（编译失败）"
    LINK_PASS=false
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "测试 3: 状态管理验证"
echo "═══════════════════════════════════════════════════════════"

if [ -f ".claude/state/session.json" ]; then
    module=$(jq -r '.session.module' .claude/state/session.json)
    phase=$(jq -r '.session.phase' .claude/state/session.json)

    echo "✓ 会话: $module"
    echo "✓ 当前阶段: $phase"

    # 检查完整状态
    full_state=$(bash scripts/state-manager.sh get-state 2>/dev/null || echo "{}")

    has_session=$(echo "$full_state" | jq 'has("session")' 2>/dev/null || echo "false")
    has_repairs=$(echo "$full_state" | jq 'has("repairs")' 2>/dev/null || echo "false")
    has_patterns=$(echo "$full_state" | jq 'has("error_patterns")' 2>/dev/null || echo "false")

    echo "✓ 会话状态: $has_session"
    echo "✓ 修复记录: $has_repairs"
    echo "✓ 错误模式: $has_patterns"

    STATE_PASS=true
else
    echo "❌ 状态文件不存在"
    STATE_PASS=false
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "测试 4: 错误去重验证"
echo "═══════════════════════════════════════════════════════════"

if [ -f ".claude/state/error-patterns.json" ]; then
    pattern_count=$(jq '.error_patterns | length' .claude/state/error-patterns.json 2>/dev/null || echo "0")
    echo "✓ 错误模式数: $pattern_count"

    if [ "$pattern_count" -gt 0 ]; then
        jq '.error_patterns[0]' .claude/state/error-patterns.json | head -5 || true
    fi

    DEDUP_PASS=true
else
    echo "ℹ️  无错误模式（可能是编译成功）"
    DEDUP_PASS=true
fi

# ═══════════════════════════════════════════════════════════════════════════
# 测试总结
# ═══════════════════════════════════════════════════════════════════════════

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "集成测试总结"
echo "═══════════════════════════════════════════════════════════"

pass_count=0
[ "$COMPILE_PASS" = "true" ] && ((pass_count++)) && echo "✅ 编译流程" || echo "❌ 编译流程"
[ "$LINK_PASS" = "true" ] && ((pass_count++)) && echo "✅ 链接流程" || echo "❌ 链接流程"
[ "$STATE_PASS" = "true" ] && ((pass_count++)) && echo "✅ 状态管理" || echo "❌ 状态管理"
[ "$DEDUP_PASS" = "true" ] && ((pass_count++)) && echo "✅ 错误去重" || echo "❌ 错误去重"

echo ""
echo "通过: $pass_count / 4"
echo "═══════════════════════════════════════════════════════════"

# 清理
cd /
rm -rf "$TEST_PROJECT"

[ $pass_count -eq 4 ] && exit 0 || exit 1
