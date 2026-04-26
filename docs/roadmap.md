# Post-v3 Roadmap

以下项目在 v3 中**有意未纳入主体规则**，避免 CLAUDE.md 承担超出"规则文件"职责的内容。每项标注未来的归属文档。

## A.1 Orchestrator 外置（对应 P1）
- 当前 v3 承认 orchestrator 是独立组件，其契约（状态机定义、legal transitions、状态持有者）归属**未来的** `docs/orchestrator-spec.md`。
- CLAUDE.md 只定义 Agent 语义和 Agent 间协作契约，不承担状态机实现规范。
- orchestrator 实现层可选：Python 脚本 / Claude Code slash command / CI workflow DSL。

## A.2 Capability Isolation（对应 P3）
- prompt 层的"禁止"只是 reminder，runtime enforcement 应在 sub-agent 定义层实现。
- 未来在 `.claude/agents/<name>.md` 的 frontmatter 中配置 `tools:` 白名单：
  ```yaml
  # .claude/agents/code-gen.md
  ---
  tools:
    - Read(ir/**)
    - Read(src/**)
    - Write(src/**)
  # 不给 docs/** 的读权限
  ---
  ```
- R1 的"禁止直接读 docs"将由此机制 runtime 强制。

## A.3 状态分层存储（对应 P2）
- 当前 v3 用 YAML frontmatter + MD 做折中，适合 Claude Code 串行调度场景。
- 未来若引入并发多 Agent，将可变计数器（`current_round`、`compile_count`、`last_fingerprint`）迁至 `.claude/state.json`，日志类文件保持 MD。
- 迁移触发条件：出现真实的并发写 race condition 证据。

## A.4 双层 Fingerprint（对应 P5）
- 当前单层 fingerprint 混入 AI 推理产出的 `root_cause_class`，存在稳定性问题。
- 未来方案：
  ```
  objective_fp = sha1(error_code + symbol + phase + ast_context_hash)[:12]
  subjective_fp = sha1(objective_fp + root_cause_class)[:12]
  ```
  - `objective_fp` 用于 R4 的"同一错误"判定
  - `subjective_fp` 用于避免同根因重复同修复
  - `objective_fp` 同但 `subjective_fp` 不同 → 允许尝试（切换假设）

## A.5 DRI 机制（对应 P7）
- ASPICE 合规和 ISO 26262 追溯需要，但对运行时行为无直接影响。
- 未来外置 `docs/module-owners.yaml`：
  ```yaml
  SPI:
    ir_owner: Alice
    review_owner: Bob
    release_owner: Carol
  ```
- reviewer 发起 REVIEW_REQUEST 时按 `module` 查表 @mention 对应 owner。

## A.6 Orchestrator 状态模型草图（占位）
未来 `docs/orchestrator-spec.md` 应至少定义：
```yaml
state_source: .claude/state.json
legal_transitions:
  ir_validated -> code_generated
  code_generated -> reviewed
  reviewed(pass) -> compiling
  reviewed(ir_ambiguity) -> ir_draft   # P6 修正路径
  compile_fail -> compile_retry
  compile_retry(count>=15) -> escalation
  link_fail(symbol_missing) -> code_generated
  flash_partial_success -> disaster_recovery
illegal_transitions:
  debugger -> reviewer    # 必须走完整轮次
  code_generated -> compiling  # 必须经过 reviewer
```
