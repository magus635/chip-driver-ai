#!/usr/bin/env bash
# feature-loop.sh — R10 增量 feature 补齐循环驱动
#
# 流程：每轮 feature-next -> 输出 codegen prompt -> (人工/agent 完成) ->
#       reviewer-agent (--check 模式) -> compile.sh -> feature-update -> 下一轮
#
# 上限来自 R10：MAX_FEATURE_ROUNDS（默认 = 未完成 feature 数 × 2，可被 project.env 覆盖）
#
# 用法：
#   scripts/feature-loop.sh ir/dma_feature_matrix.json
#   scripts/feature-loop.sh ir/dma_feature_matrix.json --dry-run
#   scripts/feature-loop.sh --help
#
# 退出码：0 全部完成 / 1 一般失败 / 2 上限触达需人工介入 / 3 死锁

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

DRY_RUN=0
LOG_FILE=""
MATRIX=""

usage() {
    sed -n '2,18p' "$0"
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --help|-h) usage ;;
        --dry-run) DRY_RUN=1; shift ;;
        --log) LOG_FILE="$2"; shift 2 ;;
        ir/*_feature_matrix.json) MATRIX="$1"; shift ;;
        *) echo "[ERROR] 未知参数: $1" >&2; exit 2 ;;
    esac
done

if [[ -z "$MATRIX" ]]; then
    echo "[ERROR] 必须提供 ir/<module>_feature_matrix.json 路径" >&2
    exit 2
fi
if [[ ! -f "$REPO_ROOT/$MATRIX" ]]; then
    echo "[ERROR] 文件不存在: $MATRIX" >&2
    exit 1
fi

cd "$REPO_ROOT"
[[ -f config/project.env ]] && source config/project.env

log() {
    local line="$1"
    echo "$line"
    if [[ -n "$LOG_FILE" ]]; then
        echo "$line" >> "$LOG_FILE"
    fi
    return 0
}

unfinished=$(python3 -c "
import json,sys
m=json.load(open('$MATRIX'))
print(sum(1 for f in m['features'] if f['status']!='done'))
")
if [[ "$unfinished" == "0" ]]; then
    log "[FEATURE-LOOP] 所有 feature 已完成 ✅"
    exit 0
fi

MAX_ROUNDS="${MAX_FEATURE_ROUNDS:-$((unfinished * 2))}"
log "[FEATURE-LOOP] 开始：未完成=${unfinished} 上限=${MAX_ROUNDS} 轮 dry_run=${DRY_RUN}"

round=0
while :; do
    round=$((round + 1))
    if (( round > MAX_ROUNDS )); then
        log "[FEATURE-LOOP] ⚠ 达到 MAX_FEATURE_ROUNDS=$MAX_ROUNDS，请求人工介入 (R10)"
        exit 2
    fi

    log "──────── ROUND $round / $MAX_ROUNDS ────────"

    set +e
    next_out=$(python3 scripts/feature-next.py "$MATRIX" --format prompt 2>&1)
    rc=$?
    set -e
    log "$next_out"

    case $rc in
        0) ;;
        1) log "[FEATURE-LOOP] ✅ 全部 feature 完成"; exit 0 ;;
        3) log "[FEATURE-LOOP] ✗ 依赖死锁，请检查 depends_on"; exit 3 ;;
        *) log "[FEATURE-LOOP] feature-next 异常: $rc"; exit 1 ;;
    esac

    next_id=$(python3 scripts/feature-next.py "$MATRIX" --format id | head -n1)
    log "[FEATURE-LOOP] 本轮目标: $next_id"

    if (( DRY_RUN == 1 )); then
        log "[FEATURE-LOOP] --dry-run：跳过 codegen / reviewer / compile / update"
        log "[FEATURE-LOOP] dry-run 仅演示一轮，结束"
        exit 0
    fi

    log "[FEATURE-LOOP] 等待 codegen-agent 实现 $next_id..."
    log "  完成后请运行: scripts/feature-update.py $MATRIX --id $next_id --status done"
    log "  或运行 reviewer + compile 后再 update"
    log "[FEATURE-LOOP] 非交互模式不自动调用 codegen-agent，退出等待手工触发下一轮"
    exit 0
done
