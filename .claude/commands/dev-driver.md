# /dev-driver — 驱动开发全流程命令 (V2.0)

**调用方式：** `/dev-driver <模块名> [--from-step <step>] [--review-level <strict|normal|relaxed>] [--resume] [--reset]`

**示例：**
- `/dev-driver can_bus`          从头开始开发 CAN 总线驱动
- `/dev-driver uart --from-step compile`  从编译阶段恢复
- `/dev-driver spi --review-level relaxed`  原型开发模式（宽松审查）
- `/dev-driver spi --resume`     恢复上次中断的 SPI 开发会话

---

## 知识库与状态依赖（V2.0 新增）

执行前自动加载：
- `.claude/lib/session-state.json` — 会话状态（断点恢复）
- `.claude/lib/error-patterns.json` — 错误模式分类库
- `.claude/lib/repair-strategies.json` — 修复策略决策库
- `.claude/memory/repair-patterns.json` — 学习到的修复模式
- `.claude/memory/chip-knowledge.json` — 芯片特定知识
- `.claude/memory/project-context.json` — 项目上下文

---

## 执行步骤

收到此命令后，按以下顺序完整执行。每步完成后输出阶段摘要，**保存断点**，再进入下一步。

### PRE-STEP · 会话恢复检测（V2.0 新增）

**自动执行**：检查 `.claude/lib/session-state.json`

```
如果检测到未完成会话（status != "completed" && status != "idle"）：
  ╔══════════════════════════════════════════════════════════════╗
  ║  检测到未完成的会话                                            ║
  ╠══════════════════════════════════════════════════════════════╣
  ║  模块：{module}                                               ║
  ║  上次进度：STEP {current_step} - {step_name}                  ║
  ║  中断时间：{last_updated}                                     ║
  ║  已完成：doc-analysis ✓, review ✓, codegen ✗                  ║
  ╠══════════════════════════════════════════════════════════════╣
  ║  回复 "恢复" → 从 STEP {current_step} 继续                    ║
  ║  回复 "重新开始" → 丢弃进度，从头开始                           ║
  ╚══════════════════════════════════════════════════════════════╝
```

**如果用户选择恢复**：
1. 加载 `session-state.json` 中的 `checkpoint`
2. 恢复各 agent 的状态（`agent_states`）
3. 验证 `artifacts` 中的文件是否存在
4. 跳转到对应的 STEP

**如果是新会话**：
1. 生成会话 ID
2. 初始化 `session-state.json`
3. 记录到 `project-context.json` 的 `session_history`

### STEP 0 · 环境检查
```bash
source config/project.env && bash scripts/check-env.sh
```
验证工具链、目标板连接、必要文档是否就绪。如有缺失，列出清单并停止。

**增强检查项：**
- 检测 `docs/` 目录下是否存在 Errata 文档（`*errata*`、`*ES0*`）
- 检测是否配置了 `CMSIS_HEADER_PATH`（可选增强验证）
- 输出检测到的文档清单供用户确认

### STEP 1 · 文档分析 (调用 doc-analyst agent)

使用 Task 工具启动 `doc-analyst` agent，传入参数：
```
module=$ARGUMENTS
docs_dir=docs/
output=ir/{module}_ir_summary.md
output_json=ir/{module}_ir_summary.json
```

**处理返回状态：**
- `DOC_CACHE_HIT`：文档未变化，复用缓存，输出提示后直接进入 STEP 1.5
- `DOC_ANALYSIS_COMPLETE`：正常完成，继续流程
- `DOC_INCOMPLETE`：文档不足，列出缺失项，询问用户是否继续

等待 agent 完成后，读取 `ir/{module}_ir_summary.md` 和 `ir/{module}_ir_summary.json`，确认以下内容已提取：
- [ ] 外设基地址和寄存器偏移表
- [ ] 初始化序列（寄存器写入顺序）
- [ ] 关键时序参数
- [ ] 中断/DMA 相关配置
- [ ] Errata 已知缺陷（如有）
- [ ] JSON 格式输出有效

### STEP 1.5 · 寄存器级交叉审查 (Review & HitL Confirm)

使用 Task 工具启动 `reviewer-agent` agent 进行多维度质量审查：
```
doc_summary=ir/{module}_ir_summary.md
doc_summary_json=ir/{module}_ir_summary.json
module=$ARGUMENTS
review_level=${REVIEW_LEVEL:-strict}
cmsis_header=${CMSIS_HEADER_PATH:-auto}
```

**审查内容（V2.0 增强）：**
1. 位域重叠与掩码错误检查
2. 内存偏移连续性检查
3. W1C/RC 副作用标注检查
4. **官方头文件交叉验证**（若存在 CMSIS）
5. **初始化序列逻辑验证**（时钟/模式依赖）
6. **时序参数合理性检查**（超时值、波特率）
7. **JSON 格式完整性验证**

**处理返回状态：**

- 若返回 `REVIEW_FAILED`：
  1. 输出详细的审查报告和修复指控
  2. 自动触发 `doc-analyst` 返工（最多重试 2 次）
  3. 若 2 次返工后仍失败，停止流程，请求人工介入

- 若返回 `REVIEW_PASSED`：**必须在此立刻暂停整个流程，等待人工复核**

  输出以下信息供人工审阅：
  ```
  ╔══════════════════════════════════════════════════════════════╗
  ║  🚨 [HITL 等待人工决断]                                       ║
  ╠══════════════════════════════════════════════════════════════╣
  ║  AI 已完成底层"地基"设计及自动化机审：                          ║
  ║  ✓ 位域完整性检查通过                                         ║
  ║  ✓ 内存对齐检查通过                                           ║
  ║  ✓ 初始化序列逻辑验证通过                                      ║
  ║  ✓ 时序参数合理性检查通过                                      ║
  ║  ✓ CMSIS 交叉验证：[已执行/已跳过]                             ║
  ╠══════════════════════════════════════════════════════════════╣
  ║  请扫视以下文件确认无误：                                      ║
  ║  • ir/{module}_ir_summary.md    — 人类可读摘要                   ║
  ║  • ir/{module}_ir_summary.json  — 机器解析格式                   ║
  ║  • .claude/review-report.md  — 审查报告详情                   ║
  ╠══════════════════════════════════════════════════════════════╣
  ║  回复 "继续" 或 "Continue" 进入代码生成阶段                    ║
  ║  回复 "停止" 或 "Stop" 终止流程                               ║
  ║  回复具体问题可要求 AI 解释或修正                              ║
  ╚══════════════════════════════════════════════════════════════╝
  ```

等待用户回复确认指令后，方可进入 STEP 2。

### STEP 2 · 业务代码硬核生成 (调用 codegen-agent)

使用 Agent 工具启动 `codegen-agent` agent：
```
module=$ARGUMENTS
doc_summary_json=ir/{module}_ir_summary.json
doc_summary_md=ir/{module}_ir_summary.md
target_dir=src/drivers/{Module}/
mode=full
```

**codegen-agent 职责**：
- 严格遵循四层架构（_reg.h → _ll.h → _drv.c → _api.h）
- 从 `ir/<module>_ir_summary.json` 提取寄存器定义、初始化序列、错误处理
- 实现 Errata workaround（若存在）
- 确保代码符合 `docs/embedded-c-coding-standard.md` 规范

**处理返回状态**：
- `CODEGEN_SUCCESS`：代码生成完成，进入 STEP 3
- `CODEGEN_PARTIAL`：部分成功，展示警告列表，询问用户是否继续
- `CODEGEN_FAILED`：生成失败，停止流程并输出诊断

**代码生成原则**（由 codegen-agent 强制执行）：
1. **拒绝"玩具级"驱动**：必须提供 `ConfigType` 配置结构体
2. **拒绝"死等"逻辑**：所有等待必须有超时退出
3. **全量错误应对**：状态机 + `GetStatus` API
4. **资源利用最大化**：多邮箱/多通道/DMA 支持
5. **Errata 规避**：实现 workaround 并注释来源

等待 agent 返回后，输出生成的文件清单

### STEP 3 · 编译修复循环 (调用 compiler-agent)

使用 Task 工具启动 `compiler-agent` agent：
```
max_iterations=15
build_script=scripts/compile.sh
src_dir=src/
log_file=.claude/repair-log.md
```

等待 agent 返回。如返回 `COMPILE_SUCCESS`，进入 STEP 4。
如返回 `COMPILE_FAILED_MAX_ITER` 或 `COMPILE_STUCK`，停止并输出诊断报告。

### STEP 4 · 链接修复循环 (调用 linker-agent)

使用 Task 工具启动 `linker-agent` agent：
```
max_iterations=10
link_script=scripts/link.sh
map_file=build/output.map
log_file=.claude/repair-log.md
```

等待 agent 返回。如返回 `LINK_SUCCESS`，进入 STEP 5。
如返回 `LINK_FAILED_MAX_ITER` 或 `LINK_STUCK`，停止并输出诊断报告。

### STEP 5 · 烧录与调试验证 (调用 debugger-agent)

使用 Task 工具启动 `debugger-agent` agent：
```
flash_script=scripts/flash.sh
debug_script=scripts/debug-snapshot.sh
doc_summary=ir/{module}_ir_summary.md
doc_summary_json=ir/{module}_ir_summary.json
session_file=.claude/debug-session.md
max_rounds=8
```

**增强验证（基于 JSON）：**
- 使用 `ir/<module>_ir_summary.json` 中的 `timing_constraints` 验证实际超时行为
- 使用 `errors[]` 定义验证错误处理是否正确触发
- 若有 `errata[]`，验证 workaround 是否生效

如 agent 返回 `DEBUG_PASS`，流程结束，输出完成摘要。
如 agent 返回 `DEBUG_FAIL_NEED_REWORK`，读取 `.claude/debug-session.md`
中的错误分析，修改源码后从 **STEP 3** 重新开始。

---

## 流程结束输出

```
╔══════════════════════════════════════════════════════════════╗
║   驱动模块开发完成                                             ║
╠══════════════════════════════════════════════════════════════╣
║   模块：$ARGUMENTS                                            ║
║   芯片：$CHIP_MODEL                                           ║
║   文档缓存：命中 / 重新解析                                     ║
║   CMSIS 验证：已执行 / 已跳过                                   ║
║   Errata 处理：N 条 / 无                                       ║
║   编译修复次数：N                                              ║
║   链接修复次数：N                                              ║
║   调试轮次：N                                                  ║
║   修改文件：file1, file2, ...                                  ║
╠══════════════════════════════════════════════════════════════╣
║   生成产物：                                                   ║
║   • ir/{module}_ir_summary.md      — 文档摘要                    ║
║   • ir/{module}_ir_summary.json    — 结构化数据                  ║
║   • .claude/review-report.md    — 审查报告                    ║
║   • .claude/repair-log.md       — 修复历史                    ║
╚══════════════════════════════════════════════════════════════╝
```

---

## 断点保存机制（V2.0 新增）

**每个 STEP 完成后自动执行**：

```
保存到 .claude/lib/session-state.json：
{
  "current_session": {
    "id": "{session_id}",
    "module": "{module}",
    "current_step": "{step_number}",
    "status": "in_progress"
  },
  "checkpoint": {
    "step": "{step_number}",
    "data": { /* step-specific data */ }
  },
  "agent_states": {
    "{agent_name}": {
      "status": "{status}",
      "iterations": N
    }
  }
}
```

**中断恢复**：
- 用户中断（Ctrl+C）：状态已保存，下次可恢复
- Agent 失败：记录失败点，支持从失败点重试
- 意外崩溃：最后一个成功的断点可恢复

---

## 记忆集成（V2.0 新增）

### 修复模式学习
```
compiler-agent / linker-agent 成功修复错误后：
1. 提取错误特征签名
2. 记录成功的修复方法
3. 更新 .claude/memory/repair-patterns.json
4. 下次遇到相似错误时优先尝试已学习的模式
```

### 芯片知识积累
```
debugger-agent 发现硬件特性时：
1. 分类：quirk / best_practice / common_mistake
2. 关联芯片系列
3. 更新 .claude/memory/chip-knowledge.json
4. 后续项目自动参考
```

### 项目上下文更新
```
流程结束时：
1. 更新模块状态（completed / failed）
2. 记录生成的文件列表
3. 添加到会话历史
4. 保存到 .claude/memory/project-context.json
```

---

## 命令行参数说明

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `<模块名>` | 要开发的驱动模块（如 can_bus, spi, uart） | 必填 |
| `--from-step` | 从指定步骤恢复执行 | 从头开始 |
| `--review-level` | 审查严格度：strict/normal/relaxed | strict |
| `--skip-cache` | 强制重新解析文档，忽略缓存 | false |
| `--no-hitl` | 跳过人工确认步骤（仅用于 CI/自动化测试） | false |
| `--resume` | 恢复上次中断的会话（V2.0 新增） | false |
| `--reset` | 重置会话状态，强制从头开始（V2.0 新增） | false |
| `--no-memory` | 禁用记忆系统（调试用）（V2.0 新增） | false |
