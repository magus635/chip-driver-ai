# /dev-driver — 驱动开发全流程命令 (V3.1)
# 遵循 CLAUDE.md v3.1 规约

**调用方式：** `/dev-driver <模块名> [--from-step <step>] [--review-level <strict|normal|relaxed>] [--resume] [--reset]`

**示例：**
- `/dev-driver can_bus`          从头开始开发 CAN 总线驱动
- `/dev-driver uart --from-step compile`  从编译阶段恢复
- `/dev-driver spi --resume`     恢复上次中断的会话（自动处理 Token）

---

## 知识库与状态依赖

执行前自动加载：
- `.claude/lib/session-state.json` — 会话状态与 Token 状态
- `.claude/lib/error-patterns.json` — 错误模式库
- `.claude/memory/repair-patterns.json` — 指纹修复经验
- `.claude/lib/chip-knowledge.json` — 芯片特性库（Errata, Quirks）

---

## 执行步骤

### PRE-STEP · 会话与 Token 检查 (V3.1)
1. 检查 `.claude/lib/session-state.json` 是否存在。
2. **Token 状态检查**：
   - 如果 `--resume` 且当前已进入 `compile` 阶段，检查 `.claude/reviewer-pass.token`。
   - 如果 Token 已消耗或已失效（代码变动），提醒用户需重新进入 `Step 1.5` 进行审查。

### STEP 0 · 环境检查
```bash
source config/project.env && bash scripts/check-env.sh
```
- 检查 `scripts/rules-version.sh` 输出是否为 `v3.1`。
- 检查 `src/safety/safe_image.bin`：**存在则就绪，不存在则警告**"灾难恢复路径不可用"，不阻断主流程。

### STEP 1 · 文档分析与 IR 生成 (doc-analyst)
1. **PDF 预处理（可选降级）**：
   ```bash
   bash scripts/parse-pdf.sh --log .claude/parse-pdf.log
   ```
   - 将 `docs/*.pdf` 转为 `docs/parsed/*.md`（已存在则跳过）。
   - 若需强制重新转换，使用 `--force`。
   - **Exit 2（依赖缺失）**：降级为 AI 直接读取 `docs/*.pdf`，流程不中断。
   - **Exit 1（转换失败）**：同样降级，但记录警告到 `.claude/parse-pdf.log`。
2. 调用 doc-analyst，优先从 `docs/parsed/` 读取预处理后的 Markdown，根据 $CHIP_MODEL 和 $CHIP_FAMILY 定位手册章节，生成 `ir/{module}_ir_summary.json`。
3. **IR Gate (V3.1 强制)**：
   运行 `python3 scripts/validate-ir.py --check-md-sync`。
   - **失败**：MD 与 JSON 不一致，强制返工 `doc-analyst`。
   - **成功**：进入下一步。
4. **R9a 审核发起**：
   如果 doc-analyst 发现手册模糊（ambiguous_register 等），按 R9a 格式发起请求。


### STEP 1.5 · IR 质量审查 (reviewer-agent · pre-codegen)

> 本阶段**只审 IR**，不审代码。代码层审查（`check-arch.sh` / `check-invariants.py`）已迁至 STEP 2.D，因为 STEP 1.5 时 `src/drivers/<Module>/` 还没有代码，跑代码层检查没有意义。

1. **IR 完整性审查 (ASIL-C/D)**：
   如果 `safety_level` ≥ ASIL-C，检查 IR 是否存在未经人工决策的模糊点。
   - **发现模糊**：直接退回 STEP 1，不自行发起 R9a。
2. **R9b 审核发起**：发现架构设计多方案选择，按 R9b 格式请求人工。
3. **HITL 人工确认（必须暂停）**：
   审查通过后，输出以下信息并等待用户确认：
   ```
   ╔══════════════════════════════════════════════════════════════╗
   ║  🚨 [HITL 等待人工决断]                                       ║
   ╠══════════════════════════════════════════════════════════════╣
   ║  AI 已完成 IR 审查及自动化机审：                                ║
   ║  ✓ 架构合规性检查                                             ║
   ║  ✓ 不变式校验                                                 ║
   ║  ✓ CMSIS 交叉验证：[已执行/已跳过]                             ║
   ╠══════════════════════════════════════════════════════════════╣
   ║  请扫视以下文件确认无误：                                      ║
   ║  • ir/{module}_ir_summary.md    — 人类可读摘要                 ║
   ║  • ir/{module}_ir_summary.json  — 机器解析格式                 ║
   ╠══════════════════════════════════════════════════════════════╣
   ║  回复 "继续" 进入代码生成阶段                                  ║
   ║  回复 "停止" 终止流程                                         ║
   ╚══════════════════════════════════════════════════════════════╝
   ```
   等待用户回复确认指令后，方可进入 STEP 1.6。

### STEP 1.6 · 签发 Token (V3.1 核心)
> **注意**：Token 签发**必须延迟到 STEP 2.D 代码层审查通过之后**。STEP 1.5 只完成 IR 审查，无法签发覆盖代码哈希的 token。本节保留作为流程标记位，实际 `issue-token.py` 调用在 STEP 2.D 末尾。

### STEP 2 · 代码生成 (codegen-agent)
1. 读取 `ir/{module}_ir_summary.json`。
2. 严禁读取 PDF 手册，保证 JSON 是唯一源。
3. 生成代码时，针对 `safety_level` 插入冗余校验。
4. **首轮 Feature Matrix bootstrap (R10)**：
   ```bash
   python3 scripts/feature-bootstrap.py ir/{module}_ir_summary.json
   python3 scripts/feature-update.py ir/{module}_feature_matrix.json --render
   ```
   - 若文件已存在则跳过；codegen-agent 在首轮代码落盘后，**应** 把已实现的 feature 通过 `feature-update.py --status done` 翻转。
   - 状态值：`todo`（默认） / `partial`（部分 API 实现） / `done`。

### STEP 2.D · 代码层审查 (reviewer-agent · post-codegen)
> 从 STEP 1.5 迁来的代码检查。每轮 STEP 2.5 的 update 之前也会跑这两条，本节是首轮 STEP 2 输出后的初查。

```bash
bash scripts/check-arch.sh
python3 scripts/check-invariants.py ir/${module}_ir_summary.json \
    src/drivers/${module^}/include/*_ll.h \
    src/drivers/${module^}/src/*.c
```
失败则退回 STEP 2 让 codegen-agent 修；通过后进入 STEP 2.5。

### STEP 2.5 · Feature Completion Loop (R10) — **MUST**

> **强制语义**：本步骤是循环，不是单次执行。在 `scripts/check-feature-matrix-clean.py` 返回 0 之前，**禁止进入 STEP 3**。
> 主控 Claude 必须按下方流程逐轮驱动，每轮只补 1 个 feature。

**循环不变式**：每一轮开始时，`scripts/check-feature-matrix-clean.py ir/{module}_feature_matrix.json` 退出码必须为 1（仍有未 done）；否则跳出循环进入 STEP 3。

**单轮执行（MUST 全部执行，不得跳步）**：
1. `next=$(python3 scripts/feature-next.py ir/{module}_feature_matrix.json --format id | head -n1)`
   - 退出码 1 → 全部 done，跳出循环 → STEP 3
   - 退出码 3 → 依赖死锁 → 立即中止流水线，请求人工调整 `depends_on`，**不得自动绕过**
2. `python3 scripts/feature-next.py ir/{module}_feature_matrix.json --format prompt` 打印 codegen 提示
3. **调用 codegen-agent**：`mode=feature, feature_id=$next` —— 仅消费该 feature 的 `ir_refs` 切片，仅生成 `target_apis` 列出的接口
4. `bash scripts/check-arch.sh && python3 scripts/check-invariants.py ir/{module}_ir_summary.json src/drivers/{Module}/include/*_ll.h src/drivers/{Module}/src/*.c`
5. `python3 scripts/issue-token.py`
6. `bash scripts/compile.sh`（消费 token；失败按 R4/R5 修复）
7. `python3 scripts/feature-update.py ir/{module}_feature_matrix.json --id $next --status done`（部分实现用 `--status partial`）
8. 回到步骤 1

**禁止行为**（违反即视为 R10 违规）：
- ❌ 跑完 STEP 2 首轮 codegen 直接进 STEP 3
- ❌ 一轮处理多个 feature_id
- ❌ 在 STEP 3 入口前不调用 `check-feature-matrix-clean.py`
- ❌ 用 `partial` 状态作为永久收尾（partial 必须最终翻转为 done 或回滚为 todo）

**STEP 3 入口门禁（MUST）**：
```bash
python3 scripts/check-feature-matrix-clean.py ir/{module}_feature_matrix.json
# exit 0 → 进入 STEP 3
# exit 1 → 退回 STEP 2.5 循环
# exit 2 → 矩阵文件缺失，退回 STEP 2 重跑 bootstrap
```

**上限**：单个 feature_id 重复进入 loop ≥ 3 次仍未 done，或循环轮数 > `MAX_FEATURE_ROUNDS` → 退出码 2，请求人工介入；与 R4 的 compile/link/debug 计数互不影响。

### STEP 2.6 · 签发 Token （从原 STEP 1.6 迁来）
STEP 2.5 全部 feature done + STEP 2.D 代码层审查通过后：
```bash
python3 scripts/issue-token.py
```
此 token 覆盖当前 `src/` + `ir/` 哈希，仅本次 compile 可消费。

### STEP 3 · 编译修复循环 (compiler-agent)
**入口双门禁**：
1. `scripts/check-feature-matrix-clean.py` 退出码 0（R10）
2. 持有 STEP 2.6 签发的有效 reviewer-pass.token（R6）
任一不满足，禁止进入。

1. **Token 校验**：调用 `scripts/compile.sh`。
   - 内部会自动运行 `verify-token.py --consume`。
   - **Exit 3**：Token 校验失败（代码被篡改或过期），必须退回 STEP 1.5。
2. **迭代上限**：遵循 R4@v3.1（编译循环单轮上限）。
3. **指纹去重**：
   - 编译报错后，调用 `scripts/fingerprint.py` 计算错误指纹。
   - 记录到 `.claude/repair-log.md`。
   - 同一指纹尝试 ≥ 2 次时，强制要求重写 `hypothesis`。

### STEP 4 · 链接修复循环 (linker-agent)
- 调用 `scripts/link.sh`。
- **迭代上限**：遵循 R4@v3.1（链接循环单轮上限）。

### STEP 5 · 烧录与验证 (debugger-agent)
- **调试轮次上限**：遵循 R4@v3.1（每轮含完整的 compile → link → flash → validate）。
- 运行前自动执行 `flash.sh`。
- **Exit 4 (灾难恢复)**：如果烧录返回 Exit 4 或检测到硬件半成功，跳转至 **RECOVERY-FLOW**。
- 采集运行快照，与 IR 的期望行为比对。

---

## 异常与灾难处理流程 (V3.1)

### RECOVERY-FLOW (灾难恢复)
当任何脚本返回 **Exit 4** 时，AI 必须执行：
1. 识别场景（state/flash/debug/repair）。
2. 调用 `bash scripts/recover.sh <scenario>`。
3. 根据恢复结果决定：
   - **Rollback**：回退 Git commit 并清理 `.claude` 缓存。
   - **Safe-Flash**：烧录 `safe_image.bin` 恢复硬件响应。
4. 记录到 `.claude/recovery-log.md` 并请求用户人工确认。

### CI 挂起处理
当脚本返回 **Exit 2** 时：
1. 识别待审核项（R9a/R9b）。
2. 将请求写入 `.claude/pending-reviews.md`。
3. 退出会话，保存断点，等待人工清除 pending。

---

## 记忆系统集成

- **指纹关联**：将成功的 `fingerprint -> repair_strategy` 存入 `memory/repair-patterns.json`。
- **IR 知识**：将 R9a 决策积累到 `memory/chip-knowledge.json`。

---

## 输出规范

每个 Step 完成后输出：
```
[PHASE] 阶段名
[STATUS] ✓ 成功 / ⚠ 待审 / ✗ 失败
[TOKEN] <reviewer_run_id> / consumed
[RECOVERY] 无 / 已触发 (场景: xxx)
[FINGERPRINT] <指纹列表>
[NEXT] 下一步动作
```
