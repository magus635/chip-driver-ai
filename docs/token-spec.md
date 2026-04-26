# Reviewer Pass Token 规范

由 CLAUDE.md R6 引用。

## 目的
确保 reviewer-agent 的放行不可绕过：没有有效 token，`compile.sh` 拒绝执行。

## Token 格式（`.claude/reviewer-pass.token`，JSON）
```json
{
  "schema_version": "1.0",
  "issued_at": "2026-04-24T10:30:00Z",
  "rules_version": "v3.1",
  "git_commit": "<当前 HEAD 的 sha>",
  "src_tree_hash": "<src/ 目录的 sha256 tree hash>",
  "ir_tree_hash": "<ir/ 目录的 sha256 tree hash>",
  "reviewer_run_id": "<uuid>",
  "findings_summary": {
    "arch_check": "pass",
    "invariants_check": "pass",
    "r9b_reviews_resolved": 2,
    "r9b_reviews_open": 0
  },
  "consumed": false,
  "consumed_at": null
}
```

## 签发流程（reviewer-agent）
1. 自动化检查（`check-arch.sh` + `check-invariants.py`）全部 pass。
2. R9b 审核全部 resolved（`r9b_reviews_open == 0`）。
3. 若检测到 ASIL-C/D 代码引用的 IR 存在未解决 ambiguity → **不签发 token，拒绝 IR，回退 doc-analyst**（见 CLAUDE.md §safety_level 处理）。
4. 计算 `src/` 和 `ir/` 的 tree hash（使用 `git ls-files` + `sha256sum`，保证顺序确定）。
5. 写入 `.claude/reviewer-pass.token`，归档旧 token 至 `.claude/token-history/<issued_at>.token`。

## 校验流程（`compile.sh` 启动时）
1. `.claude/reviewer-pass.token` 不存在 → 退出 **3**，提示 "reviewer-agent has not approved"。
2. `consumed == true` → 退出 **3**，提示 "token already consumed, re-run reviewer-agent"。
3. `rules_version` 与 `scripts/rules-version.sh` 输出不一致 → 退出 **3**，提示 "rules version drift"。
4. 重新计算当前 `src/` / `ir/` 的 tree hash，与 token 中 hash 不一致 → 退出 **3**，提示 "code or IR changed since review"。
5. 全部校验通过后，原子更新 `consumed=true` + `consumed_at=<now>`，然后执行编译。
6. 编译完成（无论成败）token 保持 consumed 状态；下次编译前必须重新过 reviewer。

## 例外（仅调试用）
- `compile.sh --bypass-token` 仅在 `INTERACTIVE_MODE=1` 下可用，且要求 stdin 输入确认字符串 `I UNDERSTAND THIS BYPASS IS FORBIDDEN IN CI`，所有 bypass 事件追加记录到 `.claude/bypass-audit.log`（append-only）。
