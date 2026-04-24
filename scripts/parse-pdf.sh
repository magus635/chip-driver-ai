#!/usr/bin/env bash
# scripts/parse-pdf.sh — PDF 预处理脚本（OpenDataLoader PDF）
# 将 docs/ 下的 PDF 手册转换为结构化 Markdown，供 doc-analyst 消费
#
# 用法:
#   ./scripts/parse-pdf.sh [OPTIONS] [file.pdf ...]
#   ./scripts/parse-pdf.sh                              # 转换 docs/ 下所有 PDF（text 模式）
#   ./scripts/parse-pdf.sh docs/rm0008.pdf              # 转换单个文件
#   ./scripts/parse-pdf.sh --mode ocr    docs/scan.pdf  # 扫描件全页 OCR
#   ./scripts/parse-pdf.sh --mode hybrid docs/mix.pdf   # 混合 PDF 动态 triage
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
FORCE=0
OUTPUT_DIR="docs/parsed"
# PDF 类型模式: text（纯向量 PDF）| ocr（扫描件）| hybrid（混合）
MODE="text"
# hybrid 后端服务地址（空 = 使用 opendataloader-pdf 默认本机地址）
HYBRID_URL=""

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
无参数时转换 docs/ 目录下所有 .pdf 文件（text 模式）。

OPTIONS:
  -h, --help              显示此帮助
  --dry-run               仅打印待执行命令，不实际转换
  --log <file>            同时将输出写入日志文件
  --force                 强制重新转换（跳过已存在检查）
  --output-dir <dir>      指定输出目录（默认: docs/parsed）
  --mode <text|ocr|hybrid>
                          PDF 类型模式（默认: text）:
                            text   — 纯向量 PDF（直接导出的手册），使用默认 Java 解析
                            ocr    — 扫描件 / 全图 PDF，全页送 OCR 后端处理
                            hybrid — 混合 PDF（部分页是图片），动态 triage 自动判断
  --hybrid-url <url>      hybrid 后端服务地址（ocr/hybrid 模式可选，
                          默认使用 opendataloader-pdf-hybrid 本机默认端口）

模式说明:
  text   → --table-method cluster --keep-line-breaks
           --markdown-page-separator "---\\n<!-- page %page-number% -->\\n"
  ocr    → 同 text 基础上加 --hybrid docling-fast --hybrid-mode full --hybrid-fallback
  hybrid → 同 text 基础上加 --hybrid docling-fast --hybrid-mode auto --hybrid-fallback

依赖:
  text 模式:   opendataloader-pdf (pip install opendataloader-pdf) + Java 11+
  ocr/hybrid:  同上 + opendataloader-pdf[hybrid] (pip install "opendataloader-pdf[hybrid]")
               并在另一终端启动: opendataloader-pdf-hybrid --port 5002

示例:
  ./scripts/parse-pdf.sh
  ./scripts/parse-pdf.sh docs/rm0008.pdf
  ./scripts/parse-pdf.sh --mode ocr  docs/scan.pdf
  ./scripts/parse-pdf.sh --mode hybrid --log .claude/parse-pdf.log docs/mixed.pdf
  ./scripts/parse-pdf.sh --mode hybrid --hybrid-url http://192.168.1.10:5002 docs/mixed.pdf
  ./scripts/parse-pdf.sh --force --log .claude/parse-pdf.log docs/rm0008.pdf
EOF
}

# ── 参数解析 ──────────────────────────────────────────────
PDF_FILES=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)        usage; exit 0 ;;
        --dry-run)        DRY_RUN=1; shift ;;
        --force)          FORCE=1; shift ;;
        --log)            LOG_FILE="$2"; shift 2 ;;
        --output-dir)     OUTPUT_DIR="$2"; shift 2 ;;
        --mode)
            MODE="$2"
            if [[ "$MODE" != "text" && "$MODE" != "ocr" && "$MODE" != "hybrid" ]]; then
                fail "无效的 --mode 值: $MODE（允许: text | ocr | hybrid）"
                exit 1
            fi
            shift 2 ;;
        --hybrid-url)     HYBRID_URL="$2"; shift 2 ;;
        -*)               fail "未知选项: $1"; usage; exit 1 ;;
        *)                PDF_FILES+=("$1"); shift ;;
    esac
done

# ── 日志文件初始化 ─────────────────────────────────────────
if [ -n "$LOG_FILE" ]; then
    mkdir -p "$(dirname "$LOG_FILE")"
    echo "=== parse-pdf.sh $(date '+%Y-%m-%d %H:%M:%S') ===" >> "$LOG_FILE"
fi

_log "══════════════════════════════════════════"
_log "  PDF 预处理 · OpenDataLoader PDF"
_log "  模式: ${MODE}"
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
ODL_CMD=""
if command -v opendataloader-pdf &>/dev/null; then
    ODL_VER=$(opendataloader-pdf --version 2>/dev/null || echo "unknown")
    pass "opendataloader-pdf 已安装（${ODL_VER}）"
    ODL_CMD="opendataloader-pdf"
elif python3 -c "import opendataloader_pdf" &>/dev/null; then
    pass "opendataloader_pdf Python 模块已安装"
    ODL_CMD="python3 -m opendataloader_pdf.cli"
else
    fail "opendataloader-pdf 未安装（运行: pip install opendataloader-pdf）"
    DEPS_OK=0
fi

# ocr / hybrid 模式额外检查 hybrid 后端
if [[ "$MODE" == "ocr" || "$MODE" == "hybrid" ]]; then
    # 检查 hybrid 包是否安装
    if python3 -c "import opendataloader_pdf.hybrid" &>/dev/null 2>&1 \
       || command -v opendataloader-pdf-hybrid &>/dev/null; then
        pass "opendataloader-pdf[hybrid] 已安装"
    else
        warn "opendataloader-pdf[hybrid] 未安装（运行: pip install 'opendataloader-pdf[hybrid]'）"
        warn "将继续尝试，若后端不可达则转换会失败"
    fi

    # 检查 hybrid 服务是否在监听
    # 优先使用用户指定 URL，否则探测默认端口 5002
    if [ -n "$HYBRID_URL" ]; then
        info "hybrid 后端地址: ${HYBRID_URL}（用户指定，跳过本机探测）"
    else
        if command -v curl &>/dev/null; then
            if curl -sf --max-time 2 "http://localhost:5002/" &>/dev/null; then
                pass "hybrid 后端服务运行中（localhost:5002）"
            else
                warn "hybrid 后端未在 localhost:5002 响应"
                warn "请在另一终端启动: opendataloader-pdf-hybrid --port 5002"
                if [ "$DRY_RUN" -eq 0 ]; then
                    fail "hybrid 后端不可达，中止执行（--dry-run 可跳过此检查）"
                    exit 2
                fi
            fi
        else
            warn "curl 未安装，无法探测 hybrid 后端，继续尝试..."
        fi
    fi
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

# ── 构造模式相关参数 ──────────────────────────────────────
# 公共参数（所有模式共享）：
#   --table-method cluster    border+cluster 双策略，提升寄存器表格识别率
#   --keep-line-breaks        保留换行，避免寄存器描述被合并成一行
#   --markdown-page-separator 保留页码，便于 doc-analyst 追溯手册出处（IR source 字段）
PAGE_SEP="---\n<!-- page %page-number% -->\n"
COMMON_OPTS="--table-method cluster --keep-line-breaks --markdown-page-separator \"${PAGE_SEP}\""

# 模式专属参数
case "$MODE" in
    text)
        MODE_OPTS=""
        ;;
    ocr)
        # 全页送 OCR 后端，--hybrid-fallback 保证后端异常时回退 Java
        MODE_OPTS="--hybrid docling-fast --hybrid-mode full --hybrid-fallback"
        [ -n "$HYBRID_URL" ] && MODE_OPTS="${MODE_OPTS} --hybrid-url \"${HYBRID_URL}\""
        ;;
    hybrid)
        # 动态 triage：auto 模式自动判断哪些页走 OCR
        MODE_OPTS="--hybrid docling-fast --hybrid-mode auto --hybrid-fallback"
        [ -n "$HYBRID_URL" ] && MODE_OPTS="${MODE_OPTS} --hybrid-url \"${HYBRID_URL}\""
        ;;
esac

# ── 转换执行 ──────────────────────────────────────────────
_log "[ 转换执行 ]"
_log "  输出目录: $OUTPUT_DIR"
_log "  公共参数: ${COMMON_OPTS}"
[ -n "$MODE_OPTS" ] && _log "  模式参数: ${MODE_OPTS}"
_log ""

TOTAL=${#PDF_FILES[@]}
SUCCESS=0
SKIP=0
FAIL_COUNT=0

# JAVA_TOOL_OPTIONS: 绕过 TimSort 比较器 bug (Comparison method violates its general contract)
# 发生在 HeaderFooterProcessor.sortPageContents，Java 21 TimSort 对非传递比较器报错
# useLegacyMergeSort=true 强制使用旧版归并排序，规避该问题
JAVA_OPT="JAVA_TOOL_OPTIONS='-Djava.util.Arrays.useLegacyMergeSort=true'"

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

    CMD="env ${JAVA_OPT} ${ODL_CMD} \"$pdf\" --output-dir \"$OUTPUT_DIR\" --format markdown -q ${COMMON_OPTS} ${MODE_OPTS}"

    if [ "$DRY_RUN" -eq 1 ]; then
        info "[DRY-RUN] $CMD"
        SUCCESS=$((SUCCESS + 1))
        continue
    fi

    # 执行转换，捕获 stdout+stderr
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
