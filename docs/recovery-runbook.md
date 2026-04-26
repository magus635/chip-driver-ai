# 灾难恢复 Runbook

由 CLAUDE.md R6 引用。工业级系统必须覆盖 recovery path，不仅是 happy path。

以下五类场景由 `scripts/recover.sh` 封装，所有灾难恢复事件以退出码 **4** 终止当前流水线并写入 `.claude/recovery-log.md`。

## 1. state 文件损坏
- **检测**：`.claude/*.json` 读取失败（JSON parse error）或 checksum 不匹配。
- **恢复**：
  1. 归档损坏文件至 `.claude/corrupted/<timestamp>/`。
  2. 查找最近的有效 `reviewer-pass.token`（按 `consumed_at` 倒序）。
  3. `git stash` 当前未提交改动 → `git checkout <token.git_commit>`。
  4. 从 `.claude/token-history/` 恢复对应时刻的 state 快照（若存在）。
  5. 提示用户人工确认后方可继续。

## 2. flash 半成功
- **检测**：`flash.sh` 返回非 0，或烧录后 UART 连续 3 秒无 boot banner。
- **前置要求**：每次 `flash.sh` 执行前必须将 MCU 当前镜像 dump 到 `.claude/flash-backup/<timestamp>.bin`。
- **恢复**：
  1. 自动尝试烧录 `src/safety/safe_image.bin`（最小 bootloader + UART echo，由仓库维护者预构建）。
  2. safe_image 烧录成功 → MCU 回到已知态，可重试原 flash 或人工介入。
  3. safe_image 烧录失败 → 硬件问题，立即退出，请求人工介入。
  4. 所有尝试记录到 `.claude/recovery-log.md`。

## 3. debug session 损坏
- **检测**：`debug-session.md` YAML frontmatter 解析失败或 `round` 字段缺失。
- **恢复**：
  1. 归档损坏文件至 `.claude/corrupted/`。
  2. 从 `.claude/debug-archive/round-<N-1>.md` 恢复为当前 session，round 计数回退至 N-1。
  3. R4 的 compile / link 计数重置。
  4. 若 archive 也损坏 → round 清零，从调试 round 1 重新开始。

## 4. reviewer 卡死 / REVIEW_REQUEST 超时
- **检测**：`INTERACTIVE_MODE=1` 下，REVIEW_REQUEST 发起后超过 `REVIEW_TIMEOUT_HOURS`（默认 24h）未收到回复。
- **恢复**：
  1. 自动降级为 CI 模式行为：将待审请求转存 `.claude/pending-reviews.md`。
  2. 以退出码 **2** 终止流水线。
  3. **禁止**自动采纳 AI 推荐选项。
  4. 若是 R9a（doc-analyst 阶段） → IR 标记为 draft 状态，code-gen 不得消费。
  5. 若是 R9b（reviewer 阶段） → reviewer-pass.token 不签发。

## 5. repair-log 损坏
- **检测**：YAML frontmatter 解析失败或 fingerprint 字段缺失。
- **恢复**：
  1. 保留损坏文件为 `repair-log.md.broken.<timestamp>`。
  2. 新建空 `repair-log.md`。
  3. 当前调试轮的 compile / link 计数清零。
  4. 已有修复不回滚（代码改动保留），但历史可检测性丢失，建议人工审阅 broken 文件后补录关键条目。
