#!/usr/bin/env bash
# scripts/check-arch.sh — 架构合规性自动检查脚本
# 被 reviewer-agent 在编译前调用，返回码 0 = 全部通过, 1 = 存在违规
#
# 检查项：
#   1. Driver 层直接寄存器访问（绕过 LL 层）
#   2. Include 方向违规（向上依赖 / 实现泄漏）
#   3. 裸 int/short/long/char 类型使用
#   4. W1C 寄存器 |= 操作
#   5. _Pos 误用为位掩码
#   6. (void*)0 替代 NULL
#   7. 空循环延时
#   8. Init/DeInit 配对完整性
#
# 用法：bash scripts/check-arch.sh [--fix-hint]
#   --fix-hint  输出修复建议（默认只输出违规）

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC_DIR="$PROJECT_ROOT/src"

SHOW_HINTS=false
if [[ "${1:-}" == "--fix-hint" ]]; then
    SHOW_HINTS=true
fi

VIOLATIONS=0
WARNINGS=0

# 临时文件用于跨子 shell 计数（管道中的 while 运行在子 shell，无法修改父 shell 变量）
VCOUNT_FILE=$(mktemp)
WCOUNT_FILE=$(mktemp)
echo 0 > "$VCOUNT_FILE"
echo 0 > "$WCOUNT_FILE"
trap 'rm -f "$VCOUNT_FILE" "$WCOUNT_FILE"' EXIT

# ═══════════════════════════════════════════════════════════════════════════
# 工具函数
# ═══════════════════════════════════════════════════════════════════════════

report_violation() {
    local check_id="$1"
    local file="$2"
    local line="$3"
    local message="$4"
    local hint="${5:-}"

    # 跨子 shell 安全计数
    local cnt
    cnt=$(cat "$VCOUNT_FILE")
    echo $((cnt + 1)) > "$VCOUNT_FILE"

    echo "  [FAIL] #${check_id} ${file}:${line}"
    echo "         ${message}"
    if $SHOW_HINTS && [[ -n "$hint" ]]; then
        echo "         FIX: ${hint}"
    fi
    echo ""
}

report_warning() {
    local check_id="$1"
    local message="$2"

    local cnt
    cnt=$(cat "$WCOUNT_FILE")
    echo $((cnt + 1)) > "$WCOUNT_FILE"

    echo "  [WARN] #${check_id} ${message}"
    echo ""
}

section_header() {
    echo "───────────────────────────────────────"
    echo " 检查 #$1: $2"
    echo "───────────────────────────────────────"
}

# ═══════════════════════════════════════════════════════════════════════════
# 检查 #1: Driver 层直接寄存器访问
# 规则: *_drv.c 和 *_api.c 中不得出现 ->REGISTER 形式的直接寄存器操作
# 允许: -> 用于访问配置结构体成员（如 Config->baudrate）
# ═══════════════════════════════════════════════════════════════════════════

section_header 1 "Driver 层直接寄存器访问"

# 查找 *_drv.c 和 *_api.c 文件
DRV_FILES=$(find "$SRC_DIR" -name "*_drv.c" -o -name "*_api.c" 2>/dev/null)

for f in $DRV_FILES; do
    # 策略：只检测对硬件外设寄存器的直接访问
    #
    # 真正的寄存器访问特征：
    #   - 目标是 XXX_TypeDef* 指针（如 I2Cx->CR1, CANx->MCR, SPIx->DR）
    #   - 寄存器名通常是 2~5 个大写字母+数字（CR1, SR, BTR, OAR1, CMCON）
    #   - 操作方式是 |=, &=, = （赋值/位操作）
    #
    # 排除：
    #   - 驱动句柄成员（hi2c->State, hi2c->ErrorCode, hi2c->XferCount 等）
    #     这些名称含小写字母（PascalCase/camelCase），不是纯大写寄存器名
    #   - LL 函数调用中的参数（如 I2C_LL_WriteData(I2Cx, ...)）
    #   - 配置结构体成员（Config->xxx, PduInfo->xxx）
    #   - 注释行和宏定义行
    #
    # 匹配模式：箭头后跟 2~6 个纯大写字母/数字的寄存器名，且该行含 |=, &=, = 赋值操作
    # 这排除了 ->State, ->ErrorCode, ->TxCpltCallback 等含小写字母的句柄成员

    grep -n '\->[A-Z][A-Z0-9]\{1,5\}\b' "$f" 2>/dev/null | \
        grep -v '^\s*//' | grep -v '^\s*\*' | grep -v '#define' | \
        grep -v '_LL_\|_ll_' | \
        grep '[|&]\?=' | \
        while IFS=: read -r lineno content; do
            report_violation 1 "$(basename "$f")" "$lineno" \
                "Driver 层直接访问寄存器: $(echo "$content" | sed 's/^[[:space:]]*//')" \
                "将寄存器操作封装到对应的 *_ll.h 函数中，Driver 层只调用 LL API"
        done
done

echo "  检查 #1 完成"
echo ""

# ═══════════════════════════════════════════════════════════════════════════
# 检查 #2: Include 方向违规
# 规则:
#   *_api.h 不得 include *_ll.h 或 *_reg.h
#   *_ll.h  不得 include *_drv.h, *_drv.c, *_api.h
#   *_reg.h 不得 include 任何项目头文件（只允许标准库）
# ═══════════════════════════════════════════════════════════════════════════

section_header 2 "Include 方向违规"

# 2a: *_api.h 不得 include *_ll.h 或 *_reg.h
API_HEADERS=$(find "$SRC_DIR" -name "*_api.h" 2>/dev/null)
for f in $API_HEADERS; do
    grep -n '#include.*_ll\.h\|#include.*_reg\.h' "$f" 2>/dev/null | while IFS=: read -r lineno content; do
        report_violation 2 "$(basename "$f")" "$lineno" \
            "API 层 include 了实现层头文件: $(echo "$content" | sed 's/^[[:space:]]*//')" \
            "*_api.h 只允许 include *_types.h 和 *_cfg.h"
    done
done

# 2b: *_ll.h 不得 include *_api.h 或 *_drv.h
LL_HEADERS=$(find "$SRC_DIR" -name "*_ll.h" 2>/dev/null)
for f in $LL_HEADERS; do
    grep -n '#include.*_api\.h\|#include.*_drv\.h' "$f" 2>/dev/null | while IFS=: read -r lineno content; do
        report_violation 2 "$(basename "$f")" "$lineno" \
            "LL 层向上依赖: $(echo "$content" | sed 's/^[[:space:]]*//')" \
            "*_ll.h 只允许 include *_reg.h 和 *_types.h"
    done
done

# 2c: *_reg.h 不得 include 项目头文件（只允许 <stdint.h> 等标准库）
REG_HEADERS=$(find "$SRC_DIR" -name "*_reg.h" 2>/dev/null)
for f in $REG_HEADERS; do
    grep -n '#include\s*"' "$f" 2>/dev/null | while IFS=: read -r lineno content; do
        report_violation 2 "$(basename "$f")" "$lineno" \
            "寄存器层 include 了项目头文件: $(echo "$content" | sed 's/^[[:space:]]*//')" \
            "*_reg.h 只允许 include <stdint.h> 等 C 标准库头文件"
    done
done

echo "  检查 #2 完成"
echo ""

# ═══════════════════════════════════════════════════════════════════════════
# 检查 #3: 裸 int/short/long 类型
# 规则: 禁止使用裸 int/short/long/unsigned，必须用 stdint.h 定长类型
# 排除: 函数返回的 int（如 main），注释，字符串中的 int
# ═══════════════════════════════════════════════════════════════════════════

section_header 3 "裸 int/short/long 类型使用"

ALL_SRC=$(find "$SRC_DIR" -name "*.c" -o -name "*.h" 2>/dev/null)
for f in $ALL_SRC; do
    # 匹配独立的 int/short/long 关键字作为类型声明
    # 排除: uint32_t, int32_t 等已有前缀的, 注释行, #include 行
    grep -n '\bint\b\|\bshort\b\|\blong\b' "$f" 2>/dev/null | \
        grep -v 'int[0-9]\|uint[0-9]\|int_t\|uint_t\|_least\|_fast\|intptr\|uintptr\|intmax\|printf\|sprint\|oint\|hint\|print\|interrupt' | \
        grep -v '^\s*//' | grep -v '^\s*\*' | grep -v '#include' | \
        grep -v 'MISRA\|misra\|lint\|NOLINT' | \
        while IFS=: read -r lineno content; do
            report_violation 3 "$(basename "$f")" "$lineno" \
                "使用了裸类型: $(echo "$content" | sed 's/^[[:space:]]*//')" \
                "替换为 <stdint.h> 定长类型（如 int32_t, uint32_t）"
        done
done

echo "  检查 #3 完成"
echo ""

# ═══════════════════════════════════════════════════════════════════════════
# 检查 #4: W1C 寄存器 |= 操作
# 规则: 对 W1C 类寄存器（如 RF0R, SR, ICR, ISCR 等状态/标志清除寄存器）
#       不得使用 |= 操作，必须直接赋值
# 注意: 这是启发式检查，基于常见 W1C 寄存器名称模式
# ═══════════════════════════════════════════════════════════════════════════

section_header 4 "W1C 寄存器 |= 操作"

# W1C 寄存器的常见名称模式（可按芯片扩展）
W1C_PATTERNS="RF0R\|RF1R\|TSR\|ISCR\|ICR\|IFCR\|LIFCR\|HIFCR\|CLRFR\|CLRR"

LL_FILES=$(find "$SRC_DIR" -name "*_ll.h" -o -name "*_ll.c" 2>/dev/null)
for f in $LL_FILES; do
    # 匹配: REG->W1C_REG |=
    grep -n "|=" "$f" 2>/dev/null | grep "$W1C_PATTERNS" | \
        grep -v '^\s*//' | grep -v '^\s*\*' | \
        while IFS=: read -r lineno content; do
            report_violation 4 "$(basename "$f")" "$lineno" \
                "W1C 寄存器使用了 |= 操作: $(echo "$content" | sed 's/^[[:space:]]*//')" \
                "W1C 寄存器必须用直接赋值（=），|= 会意外清除其他 pending 标志"
        done
done

echo "  检查 #4 完成"
echo ""

# ═══════════════════════════════════════════════════════════════════════════
# 检查 #5: _Pos 误用为位掩码
# 规则: |= 和 &= 操作中不得直接使用 _Pos 后缀的宏
#       _Pos 是位偏移量，_Msk 才是位掩码
# ═══════════════════════════════════════════════════════════════════════════

section_header 5 "_Pos 误用为位掩码"

for f in $ALL_SRC; do
    # 匹配: |= XXX_Pos 或 &= XXX_Pos（不含移位操作 << _Pos）
    grep -n '|=.*_Pos\b\|&=.*_Pos\b' "$f" 2>/dev/null | \
        grep -v '<<' | grep -v '^\s*//' | grep -v '^\s*\*' | \
        while IFS=: read -r lineno content; do
            report_violation 5 "$(basename "$f")" "$lineno" \
                "_Pos 被误用为位掩码: $(echo "$content" | sed 's/^[[:space:]]*//')" \
                "位操作用 _Msk（如 SPI_CR1_MSTR_Msk），_Pos 仅用于移位（<< _Pos）"
        done
done

echo "  检查 #5 完成"
echo ""

# ═══════════════════════════════════════════════════════════════════════════
# 检查 #6: (void*)0 替代 NULL
# 规则: 空指针统一使用 NULL 宏，不使用 (void*)0
# ═══════════════════════════════════════════════════════════════════════════

section_header 6 "(void*)0 替代 NULL"

for f in $ALL_SRC; do
    grep -n '(void\s*\*)\s*0\|(void\s*\*)(0)' "$f" 2>/dev/null | \
        grep -v '^\s*//' | grep -v '^\s*\*' | \
        while IFS=: read -r lineno content; do
            report_violation 6 "$(basename "$f")" "$lineno" \
                "使用了 (void*)0: $(echo "$content" | sed 's/^[[:space:]]*//')" \
                "统一使用 NULL 宏（需 #include <stddef.h>）"
        done
done

echo "  检查 #6 完成"
echo ""

# ═══════════════════════════════════════════════════════════════════════════
# 检查 #7: 空循环延时
# 规则: 禁止使用 for/while 空循环作为延时手段
# ═══════════════════════════════════════════════════════════════════════════

section_header 7 "空循环延时"

for f in $ALL_SRC; do
    # 匹配: for(...) ; 或 for(...) {} 空循环体
    grep -n 'for\s*(.*volatile.*)\s*;' "$f" 2>/dev/null | \
        grep -v '^\s*//' | grep -v '^\s*\*' | \
        while IFS=: read -r lineno content; do
            report_violation 7 "$(basename "$f")" "$lineno" \
                "空循环延时: $(echo "$content" | sed 's/^[[:space:]]*//')" \
                "使用硬件定时器或 LL 层封装的延时函数，空循环依赖编译器优化等级，不可靠"
        done
done

echo "  检查 #7 完成"
echo ""

# ═══════════════════════════════════════════════════════════════════════════
# 检查 #8: Init/DeInit 配对完整性
# 规则: 每个模块的 _api.h 中有 Init 函数就必须有对应的 DeInit 函数
# ═══════════════════════════════════════════════════════════════════════════

section_header 8 "Init/DeInit 配对完整性"

for f in $API_HEADERS; do
    module=$(basename "$f" | sed 's/_api\.h//')
    has_init=$(grep -c '_Init\b' "$f" 2>/dev/null | tr -d '[:space:]')
    has_deinit=$(grep -c '_DeInit\b\|_Deinit\b\|_deInit\b' "$f" 2>/dev/null | tr -d '[:space:]')
    has_init=${has_init:-0}
    has_deinit=${has_deinit:-0}

    if [[ "$has_init" -gt 0 && "$has_deinit" -eq 0 ]]; then
        report_warning 8 "模块 ${module}: 有 Init 但缺少 DeInit（功能安全要求 Init/DeInit 配对）"
    fi
done

echo "  检查 #8 完成"
echo ""

# ═══════════════════════════════════════════════════════════════════════════
# 检查 #9: 模块文件清单完整性与命名合规
# 规则:
#   - 每个 drivers/<Module>/include/ 下必须有 _reg.h / _ll.h / _types.h / _cfg.h / _api.h
#   - 每个 drivers/<Module>/src/    下必须有 _drv.c (ISR 需求另外检查)
#   - 禁止文件: _api.c / _reg.c / <module>.c (不分层的扁平命名)
#   - src/ 下应有 _isr.c (若 _api.h 未标注 /* @no-isr */，则视为需要)
# ═══════════════════════════════════════════════════════════════════════════

section_header 9 "模块文件清单完整性与命名合规"

DRIVERS_ROOT="$SRC_DIR/drivers"
if [[ -d "$DRIVERS_ROOT" ]]; then
    for mod_dir in "$DRIVERS_ROOT"/*/; do
        [[ -d "$mod_dir" ]] || continue
        module_cap=$(basename "$mod_dir")
        module=$(echo "$module_cap" | tr '[:upper:]' '[:lower:]')
        inc="$mod_dir/include"
        src="$mod_dir/src"

        # 必须存在 include/ 和 src/ 子目录
        if [[ ! -d "$inc" ]]; then
            report_violation 9 "$mod_dir" "-" \
                "模块 ${module_cap} 缺少 include/ 子目录" \
                "mkdir -p ${mod_dir}include && 将 .h 文件移入"
            continue
        fi
        if [[ ! -d "$src" ]]; then
            report_violation 9 "$mod_dir" "-" \
                "模块 ${module_cap} 缺少 src/ 子目录" \
                "mkdir -p ${mod_dir}src && 将 .c 文件移入"
            continue
        fi

        # 必须存在的头文件
        for required_h in "${module}_reg.h" "${module}_ll.h" "${module}_types.h" "${module}_cfg.h" "${module}_api.h"; do
            if [[ ! -f "$inc/$required_h" ]]; then
                report_violation 9 "$inc/$required_h" "-" \
                    "模块 ${module_cap} 缺失必须头文件 ${required_h}" \
                    "由 codegen-agent 生成对应层；参见 architecture-guidelines.md line 357-366"
            fi
        done

        # 必须存在的 drv.c
        if [[ ! -f "$src/${module}_drv.c" ]]; then
            report_violation 9 "$src/${module}_drv.c" "-" \
                "模块 ${module_cap} 缺失 ${module}_drv.c (驱动逻辑层实现)" \
                "由 codegen-agent Phase 2.4 生成"
        fi

        # isr.c 检查（允许通过注释豁免: /* @no-isr */ 出现在 _api.h 顶部）
        if [[ ! -f "$src/${module}_isr.c" ]]; then
            if [[ -f "$inc/${module}_api.h" ]] && grep -q "@no-isr" "$inc/${module}_api.h" 2>/dev/null; then
                :  # 显式豁免，跳过
            else
                report_violation 9 "$src/${module}_isr.c" "-" \
                    "模块 ${module_cap} 缺失 ${module}_isr.c (ISR 入口)" \
                    "由 codegen-agent Phase 2.6 生成；若模块确无中断，在 ${module}_api.h 顶部加注释 /* @no-isr */"
            fi
        fi

        # 禁止的文件命名
        for bad in "$src/${module}_api.c" "$src/${module}_reg.c" "$src/${module}.c"; do
            if [[ -f "$bad" ]]; then
                report_violation 9 "$bad" "-" \
                    "禁止的文件命名: $(basename "$bad") (不符合四层架构命名)" \
                    "api 层应为纯头文件，实现放 _drv.c；reg 层无实现代码；禁止扁平命名 <module>.c"
            fi
        done

        # include/ 和 src/ 下不应有其他非法 .h/.c
        for f in "$inc"/*.h; do
            [[ -f "$f" ]] || continue
            base=$(basename "$f")
            case "$base" in
                ${module}_reg.h|${module}_ll.h|${module}_types.h|${module}_cfg.h|${module}_api.h|${module}_drv.h) ;;
                *)
                    report_warning 9 "模块 ${module_cap}: include/ 下存在非标准头文件 ${base}"
                    ;;
            esac
        done
    done
else
    report_warning 9 "未找到 src/drivers/ 目录，跳过文件清单检查"
fi

echo "  检查 #9 完成"
echo ""

# ═══════════════════════════════════════════════════════════════════════════
# 汇总报告
# ═══════════════════════════════════════════════════════════════════════════

# 从临时文件读取最终计数
VIOLATIONS=$(cat "$VCOUNT_FILE")
WARNINGS=$(cat "$WCOUNT_FILE")

echo "═══════════════════════════════════════"
echo " 架构合规性检查报告"
echo "═══════════════════════════════════════"
echo " 违规 (FAIL): ${VIOLATIONS}"
echo " 警告 (WARN): ${WARNINGS}"
echo "═══════════════════════════════════════"

if [[ $VIOLATIONS -gt 0 ]]; then
    echo ""
    echo " [BLOCKED] 存在 ${VIOLATIONS} 项违规，禁止进入编译阶段"
    echo " 请修复所有 FAIL 项后重新运行此检查"
    exit 1
else
    if [[ $WARNINGS -gt 0 ]]; then
        echo ""
        echo " [PASS with WARNINGS] 无违规，但有 ${WARNINGS} 项警告建议处理"
    else
        echo ""
        echo " [PASS] 全部检查通过"
    fi
    exit 0
fi
