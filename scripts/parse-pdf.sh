#!/usr/bin/env bash
# scripts/parse-pdf.sh — PDF 预处理脚本（OpenDataLoader PDF）
# 将 docs/ 下的 PDF 手册转换为结构化 Markdown，供 doc-analyst 消费
#
# 用法:
#   ./scripts/parse-pdf.sh [OPTIONS] [file.pdf ...]
#   ./scripts/parse-pdf.sh               # 转换 docs/ 下所有 PDF
#   ./scripts/parse-pdf.sh docs/rm0008.pdf
#   ./scripts/parse-pdf.sh docs/rm0008.pdf docs/errata.pdf
#
# 输出目录: docs/parsed/<basename>.md
#
# 退出码: 0 成功, 1 失败, 2 依赖缺失

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

# ── 默认参数 ──────────────────────────────────────────────
DRY_RUN=0
LOG_FILE=""
FORCE=0          # 强制重新转换（即使目标已存在）
OUTPUT_DIR="docs/parsed"

# ── 输出函数 ──────────────────────────────────────────────
_log()  { local msg="$*"; echo "$msg"; [ -n "$LOG_FILE" ] && echo "$msg" >> "$LOG_FILE"; }
pass()  { _log "  ✓ $*"; }
warn()  { _log "  ⚠ $*"; }
fail()  { _log "  ✗ $*"; }
info()  { _log "  · $*"; }

# ── 帮助信息 ──────────────────────────────────────────────
usage() {
    cat <<EOF
用法: $(basename "$0") [OPTIONS] [file.pdf ...]

将 PDF 手册转换为结构化 Markdown（使用 OpenDataLoader PDF）。
无参数时转换 docs/ 目录下所有 .pdf 文件。

OPTIONS:
  -h, --help          显示此帮助
  --dry-run           仅打印待执行命令，不实际转换
  --log <file>        同时将输出写入日志文件
  --force             强制重新转换（跳过已存在检查）
  --output-dir <dir>  指定输出目录（默认: docs/parsed）

依赖:
  - opendataloader-pdf  (pip install opendataloader-pdf)
  - Java 11+            (java -version)

示例:
  ./scripts/parse-pdf.sh
  ./scripts/parse-pdf.sh docs/rm0008.pdf
  ./scripts/parse-pdf.sh --force --log .claude/parse-pdf.log docs/rm0008.pdf
EOF
}

# ── 参数解析 ──────────────────────────────────────────────
PDF_FILES=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)      usage; exit 0 ;;
        --dry-run)      DRY_RUN=1; shift ;;
        --force)        FORCE=1; shift ;;
        --log)          LOG_FILE="$2"; shift 2 ;;
        --output-dir)   OUTPUT_DIR="$2"; shift 2 ;;
        -*)             fail "未知选项: $1"; usage; exit 1 ;;
        *)              PDF_FILES+=("$1"); shift ;;
    esac
done

# ── 日志文件初始化 ─────────────────────────────────────────
if [ -n "$LOG_FILE" ]; then
    mkdir -p "$(dirname "$LOG_FILE")"
    echo "=== parse-pdf.sh $(date '+%Y-%m-%d %H:%M:%S') ===" >> "$LOG_FILE"
fi

_log "══════════════════════════════════════════"
_log "  PDF 预处理 · OpenDataLoader PDF"
_log "══════════════════════════════════════════"
_log ""

# ── 依赖检查 ──────────────────────────────────────────────
_log "[ 依赖检查 ]"

DEPS_OK=1

# 检查 Java
if command -v java &>/dev/null; then
    JAVA_VER=$(java -version 2>&1 | awk -F '"' '/version/ {print $2}' | cut -d. -f1)
    # Java 9+ 版本号格式为 "11"，Java 8 为 "1.8"
    if [ "$JAVA_VER" = "1" ]; then
        JAVA_VER=$(java -version 2>&1 | awk -F '"' '/version/ {print $2}' | cut -d. -f2)
    fi
    if [ "${JAVA_VER:-0}" -ge 11 ] 2>/dev/null; then
        pass "Java ${JAVA_VER} 已安装"
    else
        fail "Java 版本过低（需要 11+，当前 ${JAVA_VER:-未知}）"
        DEPS_OK=0
    fi
else
    fail "Java 未安装（需要 JDK 11+，参考: https://adoptium.net）"
    DEPS_OK=0
fi

# 检查 opendataloader-pdf
if command -v opendataloader-pdf &>/dev/null; then
    ODL_VER=$(opendataloader-pdf --version 2>/dev/null || echo "unknown")
    pass "opendataloader-pdf 已安装（${ODL_VER}）"
elif python3 -c "import opendataloader_pdf" &>/dev/null; then
    pass "opendataloader_pdf Python 模块已安装"
else
    fail "opendataloader-pdf 未安装（运行: pip install opendataloader-pdf）"
    DEPS_OK=0
fi

if [ "$DEPS_OK" -eq 0 ]; then
    _log ""
    fail "依赖缺失，无法继续。请安装后重试。"
    exit 2
fi
_log ""

# ── 收集待处理 PDF ────────────────────────────────────────
_log "[ 收集 PDF ]"

if [ ${#PDF_FILES[@]} -eq 0 ]; then
    # 未指定文件，扫描 docs/ 目录（排除 docs/parsed/）
    while IFS= read -r f; do
        PDF_FILES+=("$f")
    done < <(find docs/ -maxdepth 1 -name "*.pdf" 2>/dev/null | sort)

    if [ ${#PDF_FILES[@]} -eq 0 ]; then
        warn "docs/ 目录下没有找到 PDF 文件"
        exit 0
    fi
fi

for f in "${PDF_FILES[@]}"; do
    if [ ! -f "$f" ]; then
        fail "文件不存在: $f"
        exit 1
    fi
    info "待处理: $f"
done
_log ""

# ── 创建输出目录 ──────────────────────────────────────────
if [ "$DRY_RUN" -eq 0 ]; then
    mkdir -p "$OUTPUT_DIR"
fi

# ── 转换执行 ──────────────────────────────────────────────
_log "[ 转换执行 ]"
_log "  输出目录: $OUTPUT_DIR"
_log ""

TOTAL=${#PDF_FILES[@]}
SUCCESS=0
SKIP=0
FAIL_COUNT=0

for pdf in "${PDF_FILES[@]}"; do
    BASENAME=$(basename "$pdf" .pdf)
    TARGET_MD="${OUTPUT_DIR}/${BASENAME}.md"

    # 跳过已存在（除非 --force）
    if [ -f "$TARGET_MD" ] && [ "$FORCE" -eq 0 ]; then
        warn "跳过（已存在）: $TARGET_MD  （使用 --force 强制重新转换）"
        SKIP=$((SKIP + 1))
        continue
    fi

    info "转换: $pdf → $TARGET_MD"

    # 构造命令
    # JAVA_TOOL_OPTIONS: 绕过 TimSort 比较器 bug (Comparison method violates its general contract)
    # 发生在 HeaderFooterProcessor.sortPageContents，Java 21 TimSort 对非传递比较器报错
    # useLegacyMergeSort=true 强制使用旧版归并排序，规避该问题
    JAVA_OPT="JAVA_TOOL_OPTIONS='-Djava.util.Arrays.useLegacyMergeSort=true'"
    if command -v opendataloader-pdf &>/dev/null; then
        CMD="env ${JAVA_OPT} opendataloader-pdf \"$pdf\" --output-dir \"$OUTPUT_DIR\" --format markdown -q"
    else
        CMD="env ${JAVA_OPT} python3 -m opendataloader_pdf.cli \"$pdf\" --output-dir \"$OUTPUT_DIR\" --format markdown -q"
    fi

    if [ "$DRY_RUN" -eq 1 ]; then
        info "[DRY-RUN] $CMD"
        SUCCESS=$((SUCCESS + 1))
        continue
    fi

    # 执行转换，捕获输出
    if eval "$CMD" >> "${LOG_FILE:-/dev/null}" 2>&1; then
        pass "完成: $TARGET_MD"
        SUCCESS=$((SUCCESS + 1))
    else
        fail "转换失败: ${pdf} (查看日志了解详情)"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
done

# ── 汇总 ──────────────────────────────────────────────────
_log ""
_log "══════════════════════════════════════════"
_log "  结果: 成功 ${SUCCESS}，跳过 ${SKIP}，失败 ${FAIL_COUNT}（共 ${TOTAL} 个）"

if [ "$FAIL_COUNT" -gt 0 ]; then
    fail "部分文件转换失败，请检查日志"
    exit 1
fi

if [ "$DRY_RUN" -eq 1 ]; then
    _log "  [DRY-RUN] 未实际执行转换"
fi

pass "全部完成 → $OUTPUT_DIR"
exit 0
