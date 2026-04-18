# 第一阶段：基础架构 - 集成分析与实现方案

**日期**: 2026-03-26
**阶段目标**: 建立统一状态管理系统（MVP），连接四个 Agent 与状态层
**预计工作量**: 8-12 小时

---

## 1. 当前架构分析

### 1.1 四个 Agent 的现状

| Agent | 现有能力 | 缺陷 | 集成需求 |
|-------|--------|------|--------|
| **doc-analyst** | ✅ 结构化输出 (`ir/<module>_ir_summary.json`) | 无状态记录 | 记录：分析完成时间、覆盖范围、信心度 |
| **compiler-agent** | ✅ 日志追加、MD5 校验、错误模式表 | 自由格式日志、无去重 | 记录每轮的错误哈希、修复 diff、假设 |
| **linker-agent** | ✅ 读 .map 文件、修复决策树 | 自由格式日志、无跨阶段追踪 | 记录错误类型、根因分析、决策路径 |
| **debugger-agent** | ✅ 分层分析、快照采集 | 快照无持久化、无修复追踪 | 记录快照数据、验证结论、回溯链接 |

### 1.2 信息流现状

```
dev-driver (主命令)
├─ STEP 0: 环境检查 ────────────────────────→ 无状态
├─ STEP 1: doc-analyst ────────────────────→ 输出 `ir/<module>_ir_summary.json`
├─ STEP 2: 代码生成 ────────────────────────→ 修改源文件（无记录）
├─ STEP 3: compiler-agent ──────────────────→ 追加 repair-log.md（无结构）
├─ STEP 4: linker-agent ────────────────────→ 追加 repair-log.md（无结构）
└─ STEP 5: debugger-agent ──────────────────→ 追加 debug-session.md（无结构）

问题：每个 Agent 独立工作，缺少跨阶段的状态关联
```

---

## 2. MVP 数据结构设计（最小可行）

### 2.1 必须字段分析

基于三个 Agent 的需求，定义**最小集合**（不冗余）：

```json
{
  "session": {
    "id": "sess_20260326_061200",          // 唯一标识，用于跨会话恢复
    "started": "2026-03-26T06:12:00Z",     // 会话开始时间
    "module": "can_bus",                   // 开发的驱动模块
    "phase": "compile",                    // 当前阶段: check_env|doc_analysis|code_gen|compile|link|flash|debug
    "status": "in_progress",               // not_started|in_progress|completed|failed|stopped_by_user
    "iterations": {                        // 当前各阶段的迭代计数
      "compile": 3,
      "link": 0,
      "debug": 0
    },
    "limits": {                            // 各阶段的上限
      "compile": 15,
      "link": 10,
      "debug": 8
    }
  },

  "repairs": [                             // 所有修复操作的记录
    {
      "id": "repair_001",
      "timestamp": "2026-03-26T06:12:30Z",
      "phase": "compile",
      "iteration": 1,
      "error": {
        "type": "undefined_reference",     // 错误分类
        "message": "undefined reference to `can_init'",
        "file": "src/can_driver.c",
        "line": 42,
        "hash": "abc123"                   // 错误签名（用于去重）
      },
      "hypothesis": "can_init 函数未实现", // AI 的假设
      "plan": "在 can_driver.c 中添加实现",
      "changes": [
        {
          "file": "src/can_driver.c",
          "type": "add_function",
          "summary": "添加 can_init 函数"
        }
      ],
      "result": "success",                 // success|failed|no_change
      "next_action": "进入编译第 2 轮"
    }
  ],

  "error_patterns": [                      // 错误去重表（关键功能）
    {
      "hash": "abc123",
      "type": "undefined_reference",
      "count": 1,                          // 出现次数
      "first_repair": "repair_001",
      "status": "resolved"                 // resolved|recurring|failed
    }
  ],

  "snapshots": [                           // 调试快照（debugger-agent 产出）
    {
      "id": "snapshot_001",
      "timestamp": "2026-03-26T06:15:00Z",
      "iteration": 1,
      "phase": "debug",
      "serial_output": "CAN init: OK\n",
      "registers": {"CAN1_MCR": "0x10"},
      "variables": {"g_can_init": 1},
      "analysis": "初始化成功",
      "status": "pass"                     // pass|fail
    }
  ]
}
```

### 2.2 JSON vs Markdown 混合存储方案

**推荐方案**：
- **主存储**: `session.json` （机器读写，结构化）
- **辅助**: `repairs.json` 和 `snapshots.json` （细节存储）
- **展示**: `logs/repairs.md` （自动从 JSON 生成，人类可读）

**目录结构**：
```
.claude/
├── state/                    # 新增：状态管理层
│   ├── session.json          # 当前会话（单一真实来源）
│   ├── repairs.json          # 修复记录（细节）
│   ├── error-patterns.json   # 错误去重表
│   └── snapshots.json        # 调试快照
├── logs/                     # 新增：展示层（可选，用于快速浏览）
│   ├── repairs.md            # 从 repairs.json 生成
│   └── session-summary.md    # 会话摘要
├── repair-log.md             # ← 废弃（保留兼容性）
└── debug-session.md          # ← 废弃（保留兼容性）
```

---

## 3. 集成点分析（四个 Agent 如何对接状态系统）

### 3.1 doc-analyst 集成点

**当前**: 输出 `ir/<module>_ir_summary.md`

**需要添加**:
```json
// 在 session.json 中记录
{
  "doc_analysis": {
    "completed_at": "2026-03-26T06:12:10Z",
    "module": "can_bus",
    "files_read": ["docs/STM32F407_RM.pdf"],
    "coverage": "高",  // 高|中|低
    "extracted": {
      "registers_count": 8,
      "init_steps": 6,
      "interrupts": 2
    }
  }
}
```

**实现点**:
- [ ] doc-analyst 完成后，调用 `state_manager.update_session("doc_analysis", {...})`
- [ ] 文件位置: 在 doc-analyst.md 的"输出文件"章节添加指导

### 3.2 compiler-agent 集成点

**当前**: 追加 `repair-log.md`（自由格式）

**需要修改**:
```bash
# 在编译循环中的关键位置调用状态管理函数

# 每轮编译开始
state_manager.create_repair(
  phase="compile",
  iteration=$ITERATION,
  error_type="$ERROR_TYPE",
  error_message="$ERROR_MSG",
  error_hash="$ERROR_HASH"
)

# 修复完成后
state_manager.update_repair(
  repair_id="$REPAIR_ID",
  hypothesis="$HYPOTHESIS",
  changes=[...],
  result="$RESULT"
)

# 错误去重检查
if state_manager.error_exists($ERROR_HASH):
  echo "警告：此错误在 $PREV_REPAIR_ID 修复过，当前是重复吗？"
```

**实现点**:
- [ ] 在 compiler-agent.md 的 FIX 阶段添加状态记录步骤
- [ ] 创建 `scripts/state-manager.sh` 工具函数
- [ ] 修改 `scripts/compile.sh` 集成调用

### 3.3 linker-agent 集成点

**当前**: 追加 `repair-log.md`

**需要修改**:
```bash
# 每次链接前记录当前阶段
state_manager.set_phase("link", iteration=$ITERATION)

# 读取 .map 文件并分析
SYMBOL_STATUS=$(analyze_map_file build/output.map)
state_manager.create_repair(
  phase="link",
  error_type="$ERROR_TYPE",
  symbol="$SYMBOL_NAME",
  analysis="$MAP_ANALYSIS"
)

# 修复后更新状态和关联信息
state_manager.update_repair(
  repair_id="$REPAIR_ID",
  related_compile_repair="$COMPILE_REPAIR_ID"  # 关键：跨阶段追踪
)
```

**实现点**:
- [ ] 在 linker-agent.md 的 ANALYZE 阶段添加状态记录
- [ ] 实现 `.map` 文件解析到 JSON 的转换
- [ ] 修改 `scripts/link.sh` 集成调用

### 3.4 debugger-agent 集成点

**当前**: 追加 `debug-session.md`

**需要修改**:
```bash
# 烧录前标记阶段
state_manager.set_phase("flash")

# 采集快照后保存到结构化存储
DEBUG_SNAPSHOT=$(bash scripts/debug-snapshot.sh)
state_manager.create_snapshot(
  phase="debug",
  iteration=$ITERATION,
  serial_output="$SERIAL_OUTPUT",
  registers="{...}",
  variables="{...}"
)

# 分析验收
state_manager.update_snapshot(
  snapshot_id="$SNAPSHOT_ID",
  analysis="$ANALYSIS",
  status="pass|fail",
  next_action="继续编译|调试通过"
)
```

**实现点**:
- [ ] 修改 `scripts/debug-snapshot.sh` 输出 JSON 格式
- [ ] 在 debugger-agent.md 的"结果分析"添加状态更新步骤
- [ ] 实现快照的 JSON 存储和检索

---

## 4. MVP 实现路线图（优先级）

### Phase 1A: 状态管理框架（必做，不可跳过）
```
工作量: 2-3 小时
验收: state-manager.sh 工具链完整可用

- [ ] 创建 .claude/state/ 目录和初始化脚本
- [ ] 实现 state-manager.sh 的核心函数：
      - init_session()
      - create_repair()
      - update_repair()
      - query_error_by_hash()
      - get_session()
      - set_phase()
- [ ] 编写单元测试（echo 验证）
```

### Phase 1B: compiler-agent 集成
```
工作量: 2-3 小时
验收: 编译修复时自动记录状态，错误去重函数可用

- [ ] 在 compiler-agent.md 中标记集成点（注释说明）
- [ ] 修改 scripts/compile.sh 集成 state_manager.sh
- [ ] 实现错误哈希计算函数
- [ ] 测试：运行一个有 5 轮修复的编译，验证 repairs.json 正确生成
```

### Phase 1C: linker-agent 集成
```
工作量: 2-3 小时
验收: 链接修复时自动记录状态，.map 解析正确

- [ ] 修改 scripts/link.sh 集成 state_manager.sh
- [ ] 实现 .map 文件解析为 JSON（extract_map_info.sh）
- [ ] 记录修复与编译阶段的关联（修复 #X 可能回溯修复 #Y）
- [ ] 测试：运行一个链接错误修复，验证 symbol 追踪正确
```

### Phase 1D: debugger-agent 集成
```
工作量: 2 小时
验收: 调试快照自动存储，可跨会话检索

- [ ] 修改 scripts/debug-snapshot.sh 输出 JSON
- [ ] 集成 state_manager.sh 的 create_snapshot()
- [ ] 测试：烧录→采集快照→验证 snapshots.json 内容
```

### Phase 1E: 展示层（辅助，可选）
```
工作量: 1-2 小时
验收: logs/repairs.md 可以快速浏览修复历史

- [ ] 创建 generate-logs.sh 脚本，从 JSON 生成可读 MD
- [ ] 在 dev-driver 命令末尾调用，输出友好摘要
```

---

## 5. 核心工具实现细节

### 5.1 state-manager.sh 函数签名

```bash
# 初始化会话（在 dev-driver STEP 0 调用一次）
state_manager::init_session() {
  local module=$1
  # 生成 UUID，初始化 session.json
  # 返回: 成功 0，失败 1
}

# 创建修复记录（在每轮修复开始时调用）
state_manager::create_repair() {
  local phase=$1              # compile|link|debug
  local iteration=$2
  local error_type=$3         # undefined_reference|type_mismatch|...
  local error_hash=$4         # md5(error_message)
  # 返回: repair_id（供后续 update 使用）
}

# 更新修复记录（修复完成后调用）
state_manager::update_repair() {
  local repair_id=$1
  local hypothesis=$2         # 修复前的假设
  local changes=$3            # JSON 数组 [{file, type, summary}]
  local result=$4             # success|failed|no_change
}

# 错误去重查询（修复前调用）
state_manager::query_error_by_hash() {
  local error_hash=$1
  # 返回: 如果存在，输出 repair_id；否则空
  # exit code: 0=found, 1=not_found
}

# 获取当前会话状态（任何时刻调用）
state_manager::get_session() {
  # 输出 session.json 的内容（JSON 格式）
}

# 设置当前阶段（STEP 切换时调用）
state_manager::set_phase() {
  local phase=$1
  local iteration=$2
  # 更新 session.status 为 in_progress，phase 为当前值
}
```

### 5.2 error_hash 计算方法

```bash
# 使用：错误消息 + 文件 + 行号
error_hash=$(echo "${error_msg}:${file}:${line}" | md5sum | cut -d' ' -f1)

# 示例：
# error_msg = "undefined reference to `can_init'"
# file = "src/can_driver.c"
# line = 42
# hash = "abc123def456"  （前 12 位足够区分）
```

---

## 6. 实现检查清单

### 实现前的准备
- [ ] 确认 JSON 工具可用（jq、Python 或 bash 数组）
- [ ] 确认所有脚本具有写权限（`.claude/state/`）
- [ ] 备份现有的 `repair-log.md` 和 `debug-session.md`

### 测试验收标准

**测试 1: 状态初始化**
```bash
# 执行
bash scripts/state-manager.sh init_session can_bus

# 验证
[ -f .claude/state/session.json ]
jq '.session.module' .claude/state/session.json | grep -q "can_bus"
```

**测试 2: 错误记录**
```bash
# 执行 compiler-agent 一个修复循环
bash scripts/compile.sh

# 验证
jq '.repairs | length' .claude/state/repairs.json | grep -q "^1$"
jq '.repairs[0].phase' .claude/state/repairs.json | grep -q "compile"
```

**测试 3: 错误去重**
```bash
# 执行两轮编译，第二轮故意产生相同错误
# 验证
error_hash=$(jq -r '.repairs[0].error.hash' .claude/state/repairs.json)
jq --arg h "$error_hash" '.error_patterns[] | select(.hash == $h).count' \
  .claude/state/error-patterns.json | grep -q "^2$"
```

**测试 4: 跨阶段追踪**
```bash
# 执行完整流程到 linker-agent
# 验证链接修复中是否记录了编译阶段的修复 ID
jq '.repairs[] | select(.phase == "link") | .related_compile_repair' \
  .claude/state/repairs.json
```

---

## 7. 集成点的文档修改清单

| 文件 | 修改类型 | 说明 |
|-----|--------|------|
| `.claude/agents/doc-analyst.md` | 添加 | STEP 4 输出中记录 `doc_analysis` 到 session |
| `.claude/agents/compiler-agent.md` | 修改 | FIX 阶段后添加"状态记录"小节 |
| `.claude/agents/linker-agent.md` | 修改 | FIX 记录后添加"状态记录"小节 |
| `.claude/agents/debugger-agent.md` | 修改 | 阶段 4 和 5 中添加"快照存储"步骤 |
| `scripts/compile.sh` | 修改 | 在编译循环中集成 state_manager 调用 |
| `scripts/link.sh` | 修改 | 在链接循环中集成 state_manager 调用 |
| `scripts/debug-snapshot.sh` | 修改 | 输出结果转换为 JSON 格式 |
| `scripts/state-manager.sh` | 新建 | 状态管理核心库 |
| `.claude/state/README.md` | 新建 | 状态系统使用文档 |
| `CLAUDE.md` | 修改 | 更新规则 R6，说明新的上下文管理流程 |

---

## 8. 风险和缓解措施

| 风险 | 发生概率 | 影响 | 缓解 |
|-----|--------|------|-----|
| JSON 文件损坏导致无法读取 | 低 | 需要手动修复 | 每个操作前备份，JSON 验证 |
| 性能下降（频繁 JSON 读写） | 低 | 修复变慢 | 内存缓存，仅在关键点持久化 |
| 旧格式兼容性破裂 | 低 | 用户困惑 | 保留 `.md` 文件，提供迁移脚本 |
| state_manager 函数 bug | 中 | 状态记录错误 | 详细单元测试，快速快照工具 |

---

## 9. 成功指标

✅ **MVP 完成的标志**：

1. 创建了 `.claude/state/` 目录和 JSON 文件
2. `state-manager.sh` 所有核心函数可用且测试通过
3. 完整的 dev-driver 流程运行，所有 Agent 自动填充状态
4. 删除 session 后，读取 JSON 可以 100% 恢复上下文
5. 错误去重功能有效（相同错误第二次出现时被识别）
6. 链接修复可以追踪回编译修复（跨阶段关联）

---

## 10. 代码框架示例

### 示例：state-manager.sh 的骨架

```bash
#!/usr/bin/env bash
# scripts/state-manager.sh - 状态管理核心库

set -euo pipefail

STATE_DIR=".claude/state"
SESSION_FILE="$STATE_DIR/session.json"
REPAIRS_FILE="$STATE_DIR/repairs.json"
PATTERNS_FILE="$STATE_DIR/error-patterns.json"

# ═══════════════════════════════════════════════════════════
# 初始化
# ═══════════════════════════════════════════════════════════

state_manager::init_session() {
  local module=$1
  mkdir -p "$STATE_DIR"

  local session_id="sess_$(date +%Y%m%d_%H%M%S)"

  cat > "$SESSION_FILE" <<EOF
{
  "session": {
    "id": "$session_id",
    "started": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "module": "$module",
    "phase": "check_env",
    "status": "in_progress",
    "iterations": {"compile": 0, "link": 0, "debug": 0},
    "limits": {"compile": 15, "link": 10, "debug": 8}
  },
  "repairs": [],
  "error_patterns": [],
  "snapshots": []
}
EOF

  echo "$session_id"
}

# ═══════════════════════════════════════════════════════════
# 修复记录
# ═══════════════════════════════════════════════════════════

state_manager::create_repair() {
  local phase=$1 iteration=$2 error_type=$3 error_hash=$4

  # 生成 repair ID
  local repair_id="repair_$(printf "%03d" $(($(jq '.repairs | length' "$REPAIRS_FILE") + 1)))"

  # 追加修复记录
  jq \
    --arg rid "$repair_id" \
    --arg ts "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
    --arg ph "$phase" \
    --arg it "$iteration" \
    --arg et "$error_type" \
    --arg eh "$error_hash" \
    '.repairs += [{
      "id": $rid,
      "timestamp": $ts,
      "phase": $ph,
      "iteration": ($it | tonumber),
      "error": {"type": $et, "hash": $eh},
      "hypothesis": null,
      "result": null
    }]' "$REPAIRS_FILE" > "$REPAIRS_FILE.tmp"

  mv "$REPAIRS_FILE.tmp" "$REPAIRS_FILE"
  echo "$repair_id"
}

# ═══════════════════════════════════════════════════════════
# 错误去重
# ═══════════════════════════════════════════════════════════

state_manager::query_error_by_hash() {
  local error_hash=$1

  local found=$(jq -r --arg h "$error_hash" \
    '.error_patterns[] | select(.hash == $h) | .first_repair // empty' \
    "$PATTERNS_FILE" 2>/dev/null)

  if [ -n "$found" ]; then
    echo "$found"
    return 0
  else
    return 1
  fi
}

# ═══════════════════════════════════════════════════════════
# 其他函数... (update_repair, set_phase, get_session, etc.)
# ═══════════════════════════════════════════════════════════

```

---

**下一步建议**:

1. **审查此方案**是否符合你的需求（数据结构、集成点、优先级）
2. **确认实现顺序**：是否按 1A → 1B → 1C → 1D → 1E 执行
3. **分配工具**：使用 `jq` 还是其他 JSON 工具？
4. **测试环境**：用哪个项目做试点？

准备好开始实现了吗？
