#!/usr/bin/env bash
# scripts/compile.sh — 编译脚本
# 被 compiler-agent 调用，返回码 0 = 成功

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# 加载配置
if [ -f "config/project.env" ]; then
    source config/project.env
else
    echo "[ERROR] config/project.env 不存在，请先配置项目" >&2
    exit 1
fi

# 创建输出目录
mkdir -p "$BUILD_DIR"

echo "═══════════════════════════════════════"
echo " 编译开始 · $(date '+%Y-%m-%d %H:%M:%S')"
echo " 目标芯片: $CHIP_MODEL"
echo " 工具链:   $CC"
echo "═══════════════════════════════════════"

# 收集源文件
SRC_FILES=$(find "$SRC_DIR" -name "*.c" | sort)
INC_FLAGS=$(for d in $INC_DIRS; do echo -n "-I$d "; done)

if [ -z "$SRC_FILES" ]; then
    echo "[ERROR] src/ 目录下未找到 .c 文件" >&2
    exit 1
fi

# 编译每个源文件
COMPILE_ERRORS=0
OBJ_FILES=""

for src in $SRC_FILES; do
    obj="${BUILD_DIR}/$(basename ${src%.c}.o)"
    echo "  CC  $src"
    if $CC $CFLAGS $INC_FLAGS -c "$src" -o "$obj" 2>&1; then
        OBJ_FILES="$OBJ_FILES $obj"
    else
        COMPILE_ERRORS=$((COMPILE_ERRORS + 1))
    fi
done

if [ $COMPILE_ERRORS -gt 0 ]; then
    echo ""
    echo "═══════════════════════════════════════"
    echo " 编译失败 · 错误数: $COMPILE_ERRORS"
    echo "═══════════════════════════════════════"
    exit 1
fi

# 保存目标文件列表供 link.sh 使用
echo $OBJ_FILES > "$BUILD_DIR/.obj-list"

echo ""
echo "═══════════════════════════════════════"
echo " 编译成功 · 目标文件: $(echo $OBJ_FILES | wc -w) 个"
echo "═══════════════════════════════════════"
exit 0
