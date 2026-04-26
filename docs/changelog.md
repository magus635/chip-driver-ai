# CLAUDE.md 变更日志

由 CLAUDE.md §规则 ID 稳定性 引用。当前 git tag 由 `scripts/rules-version.sh` 输出。

## v3.0 (2026-04-24)
- **P6 修复（自洽性 bug）**：剔除 safety_level 章节中 "reviewer 触发 R9a" 的 ownership inversion。reviewer-agent 发现 IR ambiguity 一律**拒绝 IR 并回退 doc-analyst**，不自行发起 R9a。
- **P4 落地（Reviewer Pass Token Gate）**：新增 §Reviewer Pass Token 规范；`compile.sh` 必须校验 token 存在且 `src/` + `ir/` hash 与 token 一致，否则拒绝执行。token 一次性、与代码树绑定、与规则版本绑定。
- **新增 §灾难恢复规范**：覆盖 state 文件损坏、flash 半成功、debug session 损坏、reviewer 卡死、repair-log 损坏五类场景，每类给出检测和恢复路径。
- **新增 §Post-v3 Roadmap（附录 A）**：列出 orchestrator 外置、capability isolation、状态分层存储、双层 fingerprint、DRI 机制等未来迭代项，避免本文档一次性承担超出职责的内容。
- R9 / safety_level / Agent 分工表同步修正 reviewer-agent 职责边界。
- `config/project.env` 新增 `REVIEW_TIMEOUT_HOURS` 字段。
- 脚本退出码扩展：2 = CI 待审核 / 3 = token gate 拒绝 / 4 = 灾难恢复触发。

## v3.1 (2026-04-26)
- **文档拆分**：将 Token 规范、状态文件 schema、灾难恢复 runbook、Roadmap、变更日志外迁至 `docs/`，CLAUDE.md 由 581 行压缩至 ~330 行；规则正文与 Agent 分工不变。

## v2.0 (2026-04-24)
- R4 迭代计数修正为 per-round；R5 补齐 analyze；R8 扩展 W1C / 修正示例；R9 拆分为 R9a + R9b；新增错误去重规范、上下文文件 schema、safety_level 落点；初始化步骤修正循环依赖。
