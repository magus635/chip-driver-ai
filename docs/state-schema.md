# 上下文文件 Schema 与错误指纹规范

由 CLAUDE.md R4 / R7 引用。

## 错误指纹（fingerprint）

R4 所说的"同一错误"按错误指纹判定：

```
fingerprint = sha1(
    normalize(compiler_or_linker_code) +   # 如 "error: E0308"
    normalize(symbol_or_type) +            # 剥离文件行号，仅保留符号/类型名
    normalize(phase) +                     # compile | link | runtime
    normalize(root_cause_class)            # R5 analyze 阶段产出的根因分类
)[:12]
```

- **行号不参与指纹**：避免因修复导致行号前移被误判为新错误。
- 相同指纹的修复尝试 ≥ 2 次，视为"同一错误的重复修复"，必须升级到人工或切换根因假设。
- 指纹由 `scripts/fingerprint.py` 计算，`.claude/repair-log.md` 条目必须包含 `fingerprint` 字段。
- **已知局限**：`root_cause_class` 由 AI 推理产出，存在稳定性问题。v3 暂保留当前算法，双层方案见 `docs/roadmap.md` A.4。

---

## `.claude/repair-log.md`
- **模式**：append-only。
- **单条格式**（YAML frontmatter + 正文）：
  ```yaml
  ---
  timestamp: 2026-04-24T10:30:00Z
  phase: compile | link | runtime
  round: <调试轮编号>
  attempt: <该轮内尝试序号>
  fingerprint: <12位哈希>
  files: [path/to/file.c, ...]
  error_code: <编译器/链接器原始错误码>
  root_cause_class: <R5 analyze 输出>
  hypothesis: <简述>
  diff_summary: <修复 diff 的核心变更>
  result: success | fail
  ---
  <可选自由文本>
  ```

## `.claude/debug-session.md`
- **模式**：每轮**覆盖**，进入新轮前归档至 `.claude/debug-archive/round-<N>.md`。
- **快照字段**：
  ```yaml
  round: <N>
  timestamp: <ISO-8601>
  hardware_state:
    mcu_variant: <芯片型号>
    firmware_hash: <镜像 sha256>
  uart_log: <关键日志，最多 200 行>
  register_snapshot:
    - name: <REG_NAME>
      addr: 0x...
      before: 0x...
      after: 0x...
      expected: 0x...
  irq_events: [<事件时序列表>]
  verdict: pass | warn | fail
  next_action: <下一步计划>
  ```

## `.claude/review-decisions.md`
- **模式**：append。
- **检索约束**：按 `(类别, 相关文件)` 二元组索引。
- **表头**：见 CLAUDE.md R9。

## `.claude/pending-reviews.md`
- **模式**：CI 模式下由 AI 写入，由人工清空。

## `.claude/bypass-audit.log`
- **模式**：append-only。
- **字段**：`timestamp | operator | reason | git_commit | outcome`。

## `.claude/recovery-log.md`
- **模式**：append-only。
- **字段**：`timestamp | scenario | detection_signal | actions_taken | outcome | follow_up`。
