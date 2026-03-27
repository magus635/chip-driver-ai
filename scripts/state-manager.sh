#!/usr/bin/env bash
# scripts/state-manager.sh — 统一状态管理库
# 为所有 Agent 提供状态记录、查询、去重功能
#
# 使用示例：
#   source scripts/state-manager.sh
#   repair_id=$(state_manager::create_repair "compile" 1 "undefined_reference" "abc123")
#   state_manager::update_repair "$repair_id" "补充实现" '{"file":"src/xxx.c"}' "success"

set -euo pipefail

# ═══════════════════════════════════════════════════════════════════════════
# 配置
# ═══════════════════════════════════════════════════════════════════════════

STATE_DIR="${STATE_DIR:-./.claude/state}"
SESSION_FILE="$STATE_DIR/session.json"
REPAIRS_FILE="$STATE_DIR/repairs.json"
PATTERNS_FILE="$STATE_DIR/error-patterns.json"
SNAPSHOTS_FILE="$STATE_DIR/snapshots.json"

# JSON 工具选择（优先 jq，降级到 python3，降级到 bash）
_json_tool=""
if command -v jq &>/dev/null; then
    _json_tool="jq"
elif command -v python3 &>/dev/null; then
    _json_tool="python3"
else
    _json_tool="bash"  # 仅用于简单操作，有限制
fi

# ═══════════════════════════════════════════════════════════════════════════
# 内部工具函数
# ═══════════════════════════════════════════════════════════════════════════

# 计算错误哈希（MD5 后取前 12 位）
_compute_error_hash() {
    local error_msg=$1
    local file=$2
    local line=$3
    echo "${error_msg}:${file}:${line}" | md5sum | cut -c1-12
}

# 确保 JSON 文件存在
_ensure_json_file() {
    local file=$1
    local template=$2

    if [ ! -f "$file" ]; then
        mkdir -p "$(dirname "$file")"
        echo "$template" > "$file"
    fi
}

# 使用 jq 更新 JSON（如果可用）
_jq_update() {
    local file=$1
    shift
    # 剩余参数作为 jq 的参数
    jq "$@" "$file" > "$file.tmp" && mv "$file.tmp" "$file"
}

# 使用 jq 查询 JSON
_jq_query() {
    local file=$1
    shift
    jq "$@" "$file"
}

# ═══════════════════════════════════════════════════════════════════════════
# API: 会话管理
# ═══════════════════════════════════════════════════════════════════════════

# 初始化新会话（在 dev-driver STEP 0 调用一次）
state_manager::init_session() {
    local module=$1

    mkdir -p "$STATE_DIR"

    local session_id="sess_$(date +%Y%m%d_%H%M%S)_$$"
    local timestamp=$(date -u +%Y-%m-%dT%H:%M:%SZ)

    # 初始化 session.json
    cat > "$SESSION_FILE" <<'EOF'
{
  "session": {
    "id": "",
    "started": "",
    "module": "",
    "phase": "check_env",
    "status": "in_progress",
    "iterations": {"compile": 0, "link": 0, "debug": 0},
    "limits": {"compile": 15, "link": 10, "debug": 8}
  }
}
EOF

    _jq_update "$SESSION_FILE" \
        --arg id "$session_id" \
        --arg ts "$timestamp" \
        --arg mod "$module" \
        '.session.id = $id | .session.started = $ts | .session.module = $mod'

    # 初始化其他 JSON 文件
    echo '{"repairs": []}' > "$REPAIRS_FILE"
    echo '{"error_patterns": []}' > "$PATTERNS_FILE"
    echo '{"snapshots": []}' > "$SNAPSHOTS_FILE"

    echo "$session_id"
    return 0
}

# 获取当前会话信息
state_manager::get_session() {
    if [ ! -f "$SESSION_FILE" ]; then
        echo '{"error": "No active session"}' >&2
        return 1
    fi
    _jq_query "$SESSION_FILE" '.session'
}

# 获取完整会话状态（包含 repairs, patterns, snapshots）
state_manager::get_full_state() {
    if [ ! -f "$SESSION_FILE" ]; then
        echo '{"error": "No active session"}' >&2
        return 1
    fi

    # 合并所有状态文件
    jq -s '{
        session: .[0].session,
        repairs: (.[1].repairs // []),
        error_patterns: (.[2].error_patterns // []),
        snapshots: (.[3].snapshots // [])
    }' \
        <(cat "$SESSION_FILE") \
        <(cat "$REPAIRS_FILE") \
        <(cat "$PATTERNS_FILE") \
        <(cat "$SNAPSHOTS_FILE")
}

# 设置当前阶段
state_manager::set_phase() {
    local phase=$1
    local iteration=${2:-0}

    _ensure_json_file "$SESSION_FILE" '{}'

    _jq_update "$SESSION_FILE" \
        --arg ph "$phase" \
        --arg it "$iteration" \
        '.session.phase = $ph | .session.iterations[$ph] = ($it | tonumber)'
}

# ═══════════════════════════════════════════════════════════════════════════
# API: 修复记录
# ═══════════════════════════════════════════════════════════════════════════

# 创建修复记录（修复开始时调用）
state_manager::create_repair() {
    local phase=$1
    local iteration=$2
    local error_type=$3
    local error_hash=$4

    _ensure_json_file "$REPAIRS_FILE" '{"repairs": []}'

    local repair_count=$(_jq_query "$REPAIRS_FILE" '.repairs | length')
    local repair_id=$(printf "repair_%03d" $((repair_count + 1)))
    local timestamp=$(date -u +%Y-%m-%dT%H:%M:%SZ)

    _jq_update "$REPAIRS_FILE" \
        --arg rid "$repair_id" \
        --arg ts "$timestamp" \
        --arg ph "$phase" \
        --arg it "$iteration" \
        --arg et "$error_type" \
        --arg eh "$error_hash" \
        '.repairs += [{
            "id": $rid,
            "timestamp": $ts,
            "phase": $ph,
            "iteration": ($it | tonumber),
            "error": {
                "type": $et,
                "message": "",
                "file": "",
                "line": 0,
                "hash": $eh
            },
            "hypothesis": null,
            "plan": null,
            "changes": [],
            "result": null,
            "next_action": null
        }]'

    # 更新迭代计数
    state_manager::set_phase "$phase" "$iteration"

    echo "$repair_id"
}

# 更新修复记录（修复完成时调用）
state_manager::update_repair() {
    local repair_id=$1
    local hypothesis=${2:-""}
    local changes=${3:-"[]"}
    local result=${4:-""}

    _ensure_json_file "$REPAIRS_FILE" '{"repairs": []}'

    _jq_update "$REPAIRS_FILE" \
        --arg rid "$repair_id" \
        --arg hyp "$hypothesis" \
        --argjson chg "$changes" \
        --arg res "$result" \
        '(.repairs[] | select(.id == $rid)) |=
            . + {hypothesis: $hyp, changes: $chg, result: $res}'

    # 更新错误模式表
    local error_hash=$(_jq_query "$REPAIRS_FILE" \
        --arg rid "$repair_id" \
        '.repairs[] | select(.id == $rid) | .error.hash' 2>/dev/null || echo "")

    if [ -n "$error_hash" ]; then
        _update_error_pattern "$error_hash" "$repair_id" "$result"
    fi
}

# ═══════════════════════════════════════════════════════════════════════════
# API: 错误去重
# ═══════════════════════════════════════════════════════════════════════════

# 更新错误模式表（内部函数）
_update_error_pattern() {
    local error_hash=$1
    local repair_id=$2
    local result=$3
    local pattern_status="resolved"

    _ensure_json_file "$PATTERNS_FILE" '{"error_patterns": []}'

    # 检查是否已存在
    local exists=$(_jq_query "$PATTERNS_FILE" \
        --arg hash "$error_hash" \
        '.error_patterns | map(select(.hash == $hash)) | length')

    if [ "$exists" -eq 0 ]; then
        # 新增模式
        [ "$result" != "success" ] && pattern_status="failed"

        _jq_update "$PATTERNS_FILE" \
            --arg hash "$error_hash" \
            --arg rid "$repair_id" \
            --arg sta "$pattern_status" \
            '.error_patterns += [{
                "hash": $hash,
                "count": 1,
                "first_repair": $rid,
                "last_repair": $rid,
                "status": $sta
            }]'
    else
        # 更新现有模式
        _jq_update "$PATTERNS_FILE" \
            --arg hash "$error_hash" \
            --arg rid "$repair_id" \
            --arg res "$result" \
            '(.error_patterns[] | select(.hash == $hash)) |=
                (.count += 1) |
                (.last_repair = $rid) |
                if .status == "resolved" and $res != "success" then
                    .status = "recurring"
                elif .status == "failed" and $res == "success" then
                    .status = "resolved"
                else
                    .
                end'
    fi
}

# 查询错误（用于去重检查）
state_manager::query_error_by_hash() {
    local error_hash=$1

    _ensure_json_file "$PATTERNS_FILE" '{"error_patterns": []}'

    local found=$(_jq_query "$PATTERNS_FILE" \
        --arg hash "$error_hash" \
        '.error_patterns[] | select(.hash == $hash) | .first_repair // empty' 2>/dev/null)

    if [ -n "$found" ]; then
        echo "$found"
        return 0
    else
        return 1
    fi
}

# 获取错误详情
state_manager::get_error_by_hash() {
    local error_hash=$1

    _ensure_json_file "$PATTERNS_FILE" '{"error_patterns": []}'

    _jq_query "$PATTERNS_FILE" \
        --arg hash "$error_hash" \
        '.error_patterns[] | select(.hash == $hash)' 2>/dev/null || echo "{}"
}

# ═══════════════════════════════════════════════════════════════════════════
# API: 快照（调试相关）
# ═══════════════════════════════════════════════════════════════════════════

# 创建调试快照
state_manager::create_snapshot() {
    local iteration=$1
    local serial_output=${2:-""}
    local registers=${3:-"{}"}
    local variables=${4:-"{}"}

    _ensure_json_file "$SNAPSHOTS_FILE" '{"snapshots": []}'

    local snapshot_count=$(_jq_query "$SNAPSHOTS_FILE" '.snapshots | length')
    local snapshot_id=$(printf "snapshot_%03d" $((snapshot_count + 1)))
    local timestamp=$(date -u +%Y-%m-%dT%H:%M:%SZ)

    _jq_update "$SNAPSHOTS_FILE" \
        --arg sid "$snapshot_id" \
        --arg ts "$timestamp" \
        --arg it "$iteration" \
        --arg so "$serial_output" \
        --argjson reg "$registers" \
        --argjson var "$variables" \
        '.snapshots += [{
            "id": $sid,
            "timestamp": $ts,
            "iteration": ($it | tonumber),
            "serial_output": $so,
            "registers": $reg,
            "variables": $var,
            "analysis": null,
            "status": null
        }]'

    echo "$snapshot_id"
}

# 更新快照分析结果
state_manager::update_snapshot() {
    local snapshot_id=$1
    local analysis=${2:-""}
    local status=${3:-""}

    _ensure_json_file "$SNAPSHOTS_FILE" '{"snapshots": []}'

    _jq_update "$SNAPSHOTS_FILE" \
        --arg sid "$snapshot_id" \
        --arg ana "$analysis" \
        --arg sta "$status" \
        '(.snapshots[] | select(.id == $sid)) |=
            (.analysis = $ana) |
            (.status = $sta)'
}

# 获取最后一个快照
state_manager::get_last_snapshot() {
    _ensure_json_file "$SNAPSHOTS_FILE" '{"snapshots": []}'

    _jq_query "$SNAPSHOTS_FILE" '.snapshots[-1] // {}'
}

# ═══════════════════════════════════════════════════════════════════════════
# 工具函数
# ═══════════════════════════════════════════════════════════════════════════

# 输出格式化的状态摘要
state_manager::print_summary() {
    local state=$(state_manager::get_full_state)

    echo "═══════════════════════════════════════════════════════════"
    echo "会话状态摘要"
    echo "═══════════════════════════════════════════════════════════"

    local session_id=$(echo "$state" | jq -r '.session.id // "unknown"')
    local module=$(echo "$state" | jq -r '.session.module // "unknown"')
    local phase=$(echo "$state" | jq -r '.session.phase // "unknown"')
    local compile_iter=$(echo "$state" | jq -r '.session.iterations.compile // 0')
    local link_iter=$(echo "$state" | jq -r '.session.iterations.link // 0')
    local debug_iter=$(echo "$state" | jq -r '.session.iterations.debug // 0')
    local repairs=$(echo "$state" | jq '.repairs | length')

    echo "Session ID: $session_id"
    echo "Module: $module"
    echo "Phase: $phase"
    echo "Iterations: compile=$compile_iter, link=$link_iter, debug=$debug_iter"
    echo "Total repairs: $repairs"
    echo "═══════════════════════════════════════════════════════════"
}

# 导出状态为 JSON（用于后续分析）
state_manager::export_state() {
    local output_file=${1:-.claude/state-export.json}
    state_manager::get_full_state > "$output_file"
    echo "State exported to $output_file"
}

# 清空会话（谨慎使用）
state_manager::clear_session() {
    echo "WARNING: Clearing session state..."
    rm -f "$SESSION_FILE" "$REPAIRS_FILE" "$PATTERNS_FILE" "$SNAPSHOTS_FILE"
    echo "Session cleared."
}

# ═══════════════════════════════════════════════════════════════════════════
# 主程序入口（支持命令行调用）
# ═══════════════════════════════════════════════════════════════════════════


# 主程序入口（仅在脚本直接执行时运行，不影响 source）
_run_as_command() {
    command=${1:-help}
    case "$command" in
        init)
            state_manager::init_session "${2:-unknown}"
            ;;
        get-session)
            state_manager::get_session
            ;;
        get-state)
            state_manager::get_full_state
            ;;
        set-phase)
            state_manager::set_phase "$2" "${3:-0}"
            ;;
        create-repair)
            state_manager::create_repair "$2" "$3" "$4" "$5"
            ;;
        update-repair)
            state_manager::update_repair "$2" "$3" "$4" "$5"
            ;;
        query-error)
            state_manager::query_error_by_hash "$2"
            ;;
        get-error)
            state_manager::get_error_by_hash "$2"
            ;;
        create-snapshot)
            state_manager::create_snapshot "$2" "$3" "$4" "$5"
            ;;
        update-snapshot)
            state_manager::update_snapshot "$2" "$3" "$4"
            ;;
        summary)
            state_manager::print_summary
            ;;
        export)
            state_manager::export_state "${2:-.claude/state-export.json}"
            ;;
        clear)
            state_manager::clear_session
            ;;
        *)
            echo "Usage: $0 <command> [args]"
            echo ""
            echo "Commands:"
            echo "  init <module>                 - Initialize new session"
            echo "  get-session                   - Get current session info"
            echo "  get-state                     - Get full state (JSON)"
            echo "  set-phase <phase> [iter]      - Set current phase"
            echo "  create-repair <phase> <iter> <type> <hash> - Create repair record"
            echo "  update-repair <id> <hyp> <changes> <result> - Update repair"
            echo "  query-error <hash>            - Check if error exists (for dedup)"
            echo "  get-error <hash>              - Get error pattern details"
            echo "  create-snapshot <iter> [serial] [regs] [vars] - Create debug snapshot"
            echo "  update-snapshot <id> <analysis> <status> - Update snapshot"
            echo "  summary                       - Print state summary"
            echo "  export [file]                 - Export state to JSON"
            echo "  clear                         - Clear session (use with caution)"
            exit 1
            ;;
    esac
}

# 仅在脚本直接执行时运行（检查调用栈）
if [[ ${#BASH_SOURCE[@]} -eq 1 ]]; then
    _run_as_command "$@"
fi

