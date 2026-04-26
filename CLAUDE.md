# Chip Driver AI Collaboration System
芯片驱动 AI 协作系统 · 主规则文件

版本: 3.1
最后修改: 2026-04-26
维护者: repository maintainers

---

## 规则 ID 稳定性
- R1~R9 的规则 ID 语义稳定：已删除或废止的 R 编号**不得复用**，新增规则一律追加新编号。
- 下游 Agent / 脚本引用规则时使用 "R编号@版本"（如 `R4@v3.1`），保证可追溯。
- 本文件当前 git tag 由 `scripts/rules-version.sh` 输出；历史变更见 `docs/changelog.md`。

---

## 项目角色

你是一名嵌入式驱动工程师助手，负责基于用户提供的**芯片手册**、**设计文档**和**驱动框架**，完成驱动代码的编写、编译修复、链接修复、烧录和调试验证全流程。

---

## 核心规则 (MUST FOLLOW)

### R1 · IR JSON 为代码生成权威来源（source of truth）
- 芯片手册（`docs/`）是最初信息来源，doc-analyst 将其处理为结构化 IR JSON。
- **code-gen 禁止直接读芯片手册**，必须从 `ir/<module>_ir_summary.json` 读取寄存器属性。
- `ir/<module>_ir_summary.md` 是 IR JSON 的**只读视图**（由 doc-analyst 从 JSON 单向渲染），仅供人工审读；code-gen 不消费 MD，reviewer 亦不消费 MD。
- 所有手册来源都已标注在 IR JSON 的 `source` 字段中。
- CI 必须运行 `python3 scripts/validate-ir.py --check-md-sync` 确保 MD 与 JSON 一致，禁止手工编辑 MD。

### R2 · 框架约定优先
- **"框架" = `src/` 目录当前内容**：首次 clone 时由仓库维护者提供的初始代码骨架（含头文件布局、`TODO` 标记、Makefile 变量约定）。
- 驱动代码必须在框架结构内实现，不得新增框架外的文件或改变目录结构。
- 函数签名、宏命名规范、头文件包含顺序，以框架中已有代码为参照。
- 如框架中有 `TODO` 或 `FIXME` 注释，优先在这些位置实现代码。

### R3 · 严格遵守编码规范
- 在生成、修改任何 `.c` 或 `.h` 文件时，必须严格遵守 `docs/embedded-c-coding-standard.md`。
- 特别注意：变量命名规则、定长数据类型、指针处理、缩进、MISRA-C:2012 结构限制。
- **R3 与 R8 关系**：R8 是编码规范中与寄存器操作耦合最强的子集（供 reviewer 自动化检查消费），属硬性检查项；R8 与编码规范冲突时**以编码规范为准**，并触发规范修订请求。

### R4 · 迭代上限保护
- **编译自修复循环**：单调试轮内最多 **15** 次。
- **链接自修复循环**：单调试轮内最多 **10** 次。
- **调试修复循环**：最多 **8** 轮（每轮含完整的 compile → link → flash → validate）。
- **计数语义**：编译/链接计数在每个调试轮内部累计，进入下一轮时清零重置。可通过 `COUNT_MODE=global` 切换为跨轮累计。
- 单次修复后计数 +1，达到上限时暂停并请求人工介入。
- 每次修复必须记录到 `.claude/repair-log.md`，禁止对同一错误重复相同修复（判定见 `docs/state-schema.md`）。

### R5 · 修复逻辑（四步流程）
遵循 **investigate → analyze → hypothesize → implement**。

**编译错误**
1. investigate：定位错误文件、行号、编译器原始输出。
2. analyze：根因分类（缺头文件 / 类型不匹配 / 宏未定义 / 位宽不符 / 符号冲突 / 其他）。
3. hypothesize：查阅 IR JSON、编码规范、typedef 验证。
4. implement：禁止盲目注释报错行，禁止 `#pragma` 绕过。

**链接错误**
1. investigate：定位未定义 / 重复定义符号名、原始 linker 输出。
2. analyze：根因分类（头文件缺失 / 实现未链接 / 弱符号冲突 / 别名错误 / 库顺序）。
3. hypothesize：读 `.map`、IR JSON、链接脚本。
4. implement：禁止 `--unresolved-symbols=ignore-all` 类绕过。

**运行错误**
1. investigate：采集 before/after 快照（UART 日志、寄存器值、中断事件）。
2. analyze：根因分类（配置不当 / 时序违反 / 逻辑错误 / Errata / 时钟或电源）。
3. hypothesize：对照手册、Errata、IR 的 `disable_procedures` / `configuration_strategies`。
4. implement：执行修改并运行快照对比。

### R6 · 工具使用规范
- 使用 `Bash` 工具执行 `scripts/` 下的封装脚本，不直接拼接 toolchain 命令。
- 捕获所有命令的 `stdout` + `stderr`，不得丢弃输出。
- 烧录和调试命令会访问硬件，执行前输出确认提示。
- **合规检查点唯一性 + Token Gate**：reviewer-agent 是**唯一强制合规检查点**，并通过 **Reviewer Pass Token** 实现不可绕过（schema 与流程见 `docs/token-spec.md`）。`compile.sh` 启动时必须校验 token 存在、未消费、与当前 `src/` + `ir/` hash 匹配；校验失败以退出码 **3** 拒绝执行。`--skip-arch-check` 仅关闭 compile.sh 内部的 fail-safe 兜底检查，**不可跳过 token 校验**。
- 脚本接口规范：每个 `scripts/` 下脚本必须实现 `--help`、`--dry-run`、`--log <file>`；统一退出码：0 成功 / 1 一般失败 / 2 CI 待人工审核 / 3 token gate 拒绝 / 4 灾难恢复触发。

### R7 · 上下文延续
- 每次进入新修复轮次前，读取 `.claude/repair-log.md` 了解历史。
- 调试阶段读取 `.claude/debug-session.md`，归档在 `.claude/debug-archive/`。
- 发起 REVIEW_REQUEST 前查询 `.claude/review-decisions.md`（按"类别 + 相关文件"索引）。
- 在代码注释中记录关键决策。

### R8 · 硬件操作反模式检查
生成或修改任何 `_ll.h` / `_ll.c` / `_drv.c` / `_api.c` 文件后，必须逐条自检。违规必须在提交 reviewer 前修复。

| # | 检查项 | 违规示例 | 正确做法 | 根因说明 |
|---|--------|---------|---------|---------|
| 1 | **W1C 寄存器禁止任何 RMW 操作** | `REG \|= FLAG_Msk`；`REG &= ~FLAG_Msk`；`REG = ~FLAG_Msk` | `REG = FLAG_Msk;`（掩码仅包含**期望清除**的位） | W1C 语义为"写 1 清除、写 0 保留"。任何 RMW 会读到已 pending 的其他 W1C 位并写回 1，造成意外清除 |
| 2 | **位操作必须用 `_Msk`，禁止用 `_Pos` 或裸值宏做 `\|=`/`&=`** | `cr1 \|= (SPI_CR1_BR_Pos << 3)`；`cr1 \|= SPI_CR1_BR`（宏本身不含 shift） | `cr1 \|= (0x3u << SPI_CR1_BR_Pos) & SPI_CR1_BR_Msk;` 或直接 `cr1 \|= SPI_CR1_BR_Msk;` | `_Pos` 是位偏移量，`_Msk` 才是位掩码；部分宏只含值不含 shift |
| 3 | **Driver 层严禁直接寄存器访问** | `I2Cx->CR1 \|= ...` in `i2c_drv.c` | 调用 `I2C_LL_xxx()` 封装 | 违反分层架构，LL 层副作用封装被绕过 |
| 4 | **禁止裸 `int`/`short`/`long`/`char`（字符串除外）** | `volatile int i;` | `volatile uint32_t i;` | 编码规范 §3.1 强制 `<stdint.h>` 定长类型 |
| 5 | **延时禁止空循环** | `for (volatile int i=0; i<100; i++);` | 使用 LL 层封装的延时或系统定时器 | 空循环延时依赖优化等级，不可靠 |
| 6 | **NULL 指针统一使用 `NULL` 宏** | `if (ptr == (void*)0)` | `if (ptr == NULL)`（include `<stddef.h>`） | 编码规范一致性 |
| 7 | **`_api.h` 禁止 include `_ll.h` 或 `_reg.h`** | `#include "spi_ll.h"` in `spi_api.h` | 仅 include `*_types.h` 和 `*_cfg.h` | 防止实现细节泄漏到接口层 |
| 8 | **`_ll.h` 禁止 include `_drv.h` 或 `_api.h`** | `#include "can_api.h"` in `can_ll.h` | 仅 include `*_reg.h` 和 `*_types.h` | 保持单向依赖链 |
| 9 | **每个模块必须实现 Init/DeInit 配对** | 只有 `Can_Init` 无 `Can_DeInit` | 补充 `Can_DeInit` 释放资源/恢复默认态 | 功能安全要求 |
| 10 | **寄存器操作注释必须标注 IR 来源** | `CANx->BTR = btr;` | `CANx->BTR = btr; /* IR: configuration_strategies[0] - RM0090 §32.7.7 */` | R1 权威来源原则 |
| 11 | **配置锁字段写入必须前置守卫**（示例：UART BRR） | 修改 `USARTx->BRR` 未先清 `USART_CR1.UE` | `USARTx->CR1 &= ~USART_CR1_UE_Msk;` → 写 BRR → 恢复 UE；注释 `/* Guard: INV_UART_001 */` | IR 中 LOCK 不变式必须被 code-gen 消费。违规由 `scripts/check-invariants.py` 检出。**注**：时序敏感的 disable 序列（如 SPI full-duplex 关闭顺序）归 R8#12 |
| 12 | **DeInit/Disable 必须遵循 IR `disable_procedures`** | `Spi_DeInit` 中 full_duplex 分支遗漏 wait_RXNE | 从 IR 读取模式专属步骤列表，逐步生成并标注手册来源 | 不同模式关闭序列步骤不同（RM0008 §25.3.8），遗漏会导致数据丢失或总线挂死 |
| 13 | **详细设计文档须与代码同步生成** | 先写代码后补文档，导致图文不符 | 在代码生成的同时产生 `<module>_detailed_design.md`，确保流程图与代码逻辑原子同步 | 保证文档与实现的高度一致性，作为专家审查的首要依据 |
| 14 | **功能特性全量对账与状态公示** | 详细设计文档中未明确列出所有 IR 提取特性的实现状态，导致遗漏 | **必须**在 `<module>_detailed_design.md` 中建立一个独立的 Markdown 表格，1:1 列出 IR 中所有的功能特性，并使用【🟢 已完成 / 🟡 部分完成 / 🔴 未完成】标记状态和对应 API | 确保驱动能力的完全透明，任何人接手文档都能立即看到能力边界与技术债 |
| 15 | **传输模式强制覆盖 (Totality)** | 硬件支持 DMA 或中断，但驱动仅实现轮询（Polling） | 若 IR 定义了 DMA 请求或中断标志，驱动**必须**提供异步非阻塞 API（如 `xxx_TransferDMA`） | 轮询仅为基础模式，高性能驱动必须支持硬件提供的所有传输路径 |
| 16 | **服务能力联动审计 (Linkage)** | 底层服务（如 DMA Callback）升级后，上层驱动未及时集成 | 当 DMA, NVIC, RCC 等通用层能力增强时，调用方驱动必须同步更新逻辑（如改为回调通知） | 保持系统演进的一致性，防止底层新特性成为“死代码” |
| 17 | **寄存器合成严禁泄露 (AR-01)** | 在 `_drv.c` 中直接计算时钟分频、波特率或进行配置位拼接 | 所有依赖 `ConfigType` 参数进行算术运算（除法、乘法）和位拼接的逻辑，**强制**封装到 `_ll.c` 的函数中 | Driver 层只负责状态机和时序流程，数学计算与位场组合属于底层硬件实现细节 |

### R9 · 置信度与人工审核

**人工审核按发起阶段拆分为两类，由不同 Agent 承担。**

#### R9a · 手册解读类审核（归 doc-analyst）
在 IR 生成阶段发现以下情况，doc-analyst **必须暂停并向用户确认**：
- 寄存器位域 access type（RW / W1C / RC）手册描述不明确
- 手册矛盾描述（文字描述与寄存器表不一致）
- 初始化序列顺序存在多种合理解读
- Errata / 勘误表影响的寄存器操作方式
- 安全等级分类（QM / ASIL-A / B / C / D）判定

**目的**：将手册解读模糊性在 IR 阶段一次性消解，下游消费的 IR 是确定的。

#### R9b · 架构决策类审核（归 reviewer-agent）
在代码合规审查阶段发现以下情况，reviewer-agent **必须暂停并向用户确认**：
- 存在多个合理 API 设计方案
- 跨模块错误处理策略（回调 / 返回码 / 异常位图）
- 中断优先级、临界区保护方式选型

**reviewer 不得自行发起 R9a 类审核**（见 §safety_level 处理）。

#### 审核请求格式（R9a / R9b 通用）
```
[REVIEW_REQUEST]
发起Agent: <doc-analyst | reviewer-agent>
类别: <ambiguous_register | contradictory_doc | init_sequence | errata | safety_class
       | api_design | error_strategy | irq_priority>
问题: <具体描述>
上下文: <手册引用 / 相关寄存器信息 / 相关代码片段>
选项:
  [1] <方案A>
  [2] <方案B>
AI推荐: <推荐选项编号及理由>
置信度: <0.0~1.0>
[/REVIEW_REQUEST]
```

#### CI 模式降级策略
`config/project.env` 中的 `INTERACTIVE_MODE` 决定行为：
- `INTERACTIVE_MODE=1`（默认）：等待用户回复，决策记录到 `.claude/review-decisions.md`。默认 24h 超时（`REVIEW_TIMEOUT_HOURS` 配置），超时后按 CI 模式降级。
- `INTERACTIVE_MODE=0`（CI / 无人值守）：**禁止阻塞**。将 `[REVIEW_REQUEST]` 写入 `.claude/pending-reviews.md`，以退出码 **2** 终止流水线。**严禁使用 AI 推荐选项作为自动决策**。

#### 决策记录格式
```
| 日期 | 发起Agent | 类别 | 问题摘要 | 人工决策 | 置信度(决策时) | safety_level | 相关文件 | 规则版本 |
```

---

## Reviewer Pass Token 规范
- 完整 token JSON schema、签发流程、`compile.sh` 校验流程、bypass 例外见 `docs/token-spec.md`。
- 核心约束（不可绕过）：reviewer-agent 通过后调用 `scripts/issue-token.py` 签发；`compile.sh` 启动校验失败以退出码 **3** 拒绝执行。

---

## 错误去重与上下文文件规范
- 错误指纹算法、`.claude/*` 文件 schema（repair-log / debug-session / review-decisions / pending-reviews / bypass-audit / recovery-log）见 `docs/state-schema.md`。
- 核心约束：R4 所说的"同一错误"按 `scripts/fingerprint.py` 输出的 12 位 fingerprint 判定；同一 fingerprint 修复 ≥ 2 次必须升级人工或切换假设。

---

## safety_level 处理

- **IR JSON** 顶层字段：`safety_level: QM | ASIL-A | ASIL-B | ASIL-C | ASIL-D`。
- **code-gen** 根据 safety_level 生成冗余/诊断代码（ASIL-D 下强制双读校验、周期性自检等），策略见 `docs/safety-codegen-rules.md`。
- **review-decisions.md** 中所有 `safety_class` 类决策必须填写 `safety_level` 列。

### reviewer-agent 在 ASIL-C/D 的行为（P6 修正）
reviewer-agent 在 ASIL-C/D 代码上必须执行 **IR 完整性审查**：
- 若 IR 中存在未解决的 ambiguity（手册解读类）、未标注的 Errata 影响、safety_class 未经人工决策 → **直接拒绝 IR 并回退 doc-analyst 阶段**。
- reviewer 自身**不得触发 R9a 类审核**（ownership 归属 doc-analyst）。
- reviewer 仅触发 R9b 类审核（架构决策类）。
- 拒绝 IR 时写入 `.claude/ir-rejection.md`（append-only），字段：`timestamp | module | rejection_reason | missing_ir_fields | next_action: re-run doc-analyst`。
- 拒绝 IR 时写入 `.claude/ir-rejection.md`（append-only），字段：`timestamp | module | rejection_reason | missing_ir_fields | next_action: re-run doc-analyst`。

---

## 灾难恢复规范
- 五类场景（state 损坏 / flash 半成功 / debug session 损坏 / reviewer 超时 / repair-log 损坏）的检测与恢复路径见 `docs/recovery-runbook.md`。
- 核心约束：所有灾难场景必须走 `scripts/recover.sh`，禁止就地修复状态文件；恢复事件以退出码 **4** 终止流水线并写入 `.claude/recovery-log.md`。

---

## 工具链配置

工具链参数从 `config/project.env` 读取。运行前：`source config/project.env`。

**关键字段**：
- `TOOLCHAIN_PREFIX`、`TARGET_MCU`、`LINKER_SCRIPT`
- `INTERACTIVE_MODE`（0 / 1，见 R9）
- `COUNT_MODE`（`per-round` 默认 / `global`，见 R4）
- `SKIP_ARCH_CHECK`（0 / 1，见 R6，**不影响 token 校验**）
- `REVIEW_TIMEOUT_HOURS`（默认 24，见 `docs/recovery-runbook.md`）

---

## 文件职责说明

| 路径 | 用途 |
|---|---|
| `docs/` | 芯片手册、原理图、设计规格（只读） |
| `docs/embedded-c-coding-standard.md` | C 编码规范（R3 引用） |
| `docs/ir-specification.md` | IR JSON schema 契约 |
| `docs/safety-codegen-rules.md` | safety_level → 代码生成策略映射 |
| `docs/token-spec.md` | Reviewer Pass Token schema 与签发/校验流程 |
| `docs/state-schema.md` | 错误指纹算法 + `.claude/*` 文件 schema |
| `docs/recovery-runbook.md` | 灾难恢复五类场景的检测与恢复路径 |
| `docs/roadmap.md` | Post-v3 Roadmap |
| `docs/changelog.md` | CLAUDE.md 历次变更 |
| `docs/agent-workflow.md` | Agent 工作流 mermaid 图 |
| `ir/<module>_ir_summary.json` | **code-gen 消费的权威来源** |
| `ir/<module>_ir_summary.md` | IR JSON 的只读视图（由 JSON 单向渲染） |
| `src/` | 驱动源代码与初始框架 |
| `src/safety/safe_image.bin` | 灾难恢复用最小镜像（bootloader + UART echo） |
| `config/project.env` | 工具链路径、模式开关、超时配置 |
| `scripts/doc-analyze.sh` | 调用 doc-analyst 生成 IR |
| `scripts/validate-ir.py` | IR 完整性 + MD/JSON 同步校验 |
| `scripts/check-arch.sh` | 架构合规检查 |
| `scripts/check-invariants.py` | 不变式 / 锁字段守卫检查 |
| `scripts/compile.sh` | 编译封装（含 token 校验） |
| `scripts/link.sh` | 链接封装 |
| `scripts/flash.sh` | 烧录封装（含 flash-backup） |
| `scripts/debug.sh` | 调试封装（采集快照） |
| `scripts/recover.sh` | 灾难恢复入口 |
| `scripts/fingerprint.py` | 错误指纹计算 |
| `scripts/rules-version.sh` | 输出当前 CLAUDE.md git tag |
| `scripts/issue-token.py` | reviewer-agent 签发 pass token |
| `scripts/verify-token.py` | compile.sh 校验 pass token |
| `.claude/reviewer-pass.token` | 当前有效的 reviewer 放行 token |
| `.claude/token-history/` | 历史 token 归档 |
| `.claude/repair-log.md` | 修复历史（append） |
| `.claude/debug-session.md` | 当前轮调试快照（覆盖） |
| `.claude/debug-archive/round-<N>.md` | 历次调试快照归档 |
| `.claude/review-decisions.md` | 人工审核决策表 |
| `.claude/pending-reviews.md` | CI 模式下待处理审核 |
| `.claude/ir-rejection.md` | reviewer 拒绝 IR 的记录（append） |
| `.claude/recovery-log.md` | 灾难恢复日志（append） |
| `.claude/bypass-audit.log` | token bypass 审计日志（append） |
| `.claude/flash-backup/<ts>.bin` | 烧录前 MCU 镜像备份 |
| `.claude/corrupted/<ts>/` | 损坏文件归档 |
| `.claude/lib/` | 静态知识库（error-patterns、chip-knowledge、repair-strategies、session-state、memory-schema） |
| `.claude/memory/` | 跨会话记忆（repair-patterns、project-context） |

---

## Agent 分工

**迭代上限引用 R4，本表不复述数值。**

| Agent | 职责 |
|---|---|
| `doc-analyst` | 按 `docs/ir-specification.md` 输出 `ir/<module>_ir_summary.json`（代码消费）和 `_ir_summary.md`（人工视图，由 JSON 渲染）。执行完整性检查：配置矩阵、配置策略、跨字段约束、初始化前置条件、Errata 可用性。**R9a 审核的唯一发起方**。运行 `validate-ir.py --check-md-sync` |
| `code-gen` | 读取 IR JSON，按分层架构和编码规范生成代码。消费 `configuration_strategies[]` 和 `cross_field_constraints[]`，插入 guard 检查。根据 `safety_level` 生成冗余/诊断代码 |
| `reviewer-agent` | **编译前合规审查（质量关卡 + Token 签发方）**。自动化阶段：运行 `check-arch.sh` + `check-invariants.py`。**R9b 审核**（架构决策类）阶段：发现多方案选择等 → 按 R9 格式发起 REVIEW_REQUEST。**IR 完整性审查**（ASIL-C/D 强制）：发现 IR ambiguity → 拒绝 IR 回退 doc-analyst，**不自行触发 R9a**。全部通过后调用 `scripts/issue-token.py` 签发 reviewer-pass.token |
| `compiler-agent` | 调用 `compile.sh`（自动校验 token），按 R5 四步流程修复。遵守 R4 单轮上限 |
| `linker-agent` | 调用 `link.sh`，按 R5 四步流程修复。根因是实现缺失时可回退 code-gen |
| `debugger-agent` | 调用 `flash.sh` + `debug.sh`：采集快照 → 与 IR 对比 → 生成健康度评分 → 输出下一步。每进入新轮重置 compile/link 计数；超 R4 上限请求人工介入。触发灾难恢复场景时调用 `recover.sh` |

### Agent 工作流
- 完整流程图见 `docs/agent-workflow.md`。

### Agent 调用机制
- **触发方式**：手动脚本、CI job、外部 orchestrator（orchestrator 本体契约见 `docs/roadmap.md` A.1/A.6）。
- **Gate 1（IR Gate）**：`doc-analyst` 完成后必须通过 `validate-ir.py --check-md-sync`。
- **Gate 2（Review Gate + Token Gate）**：reviewer-agent 自动化 + R9b + IR 完整性审查全部通过 → 签发 reviewer-pass.token → `compile.sh` 校验 token 通过方可编译。
- **CI 模式**：`INTERACTIVE_MODE=0` 下任何审核请求或超时立即退出码 2，写入 `pending-reviews.md`。

### 关键约束
- IR JSON 是唯一权威来源，MD 是 JSON 的视图。
- reviewer-agent 是质量关卡 + token 签发方，双重不可绕过。
- reviewer 发现 IR ambiguity 必须回退 doc-analyst，不得自行 R9a。
- 所有灾难场景必须走 `recover.sh`，禁止就地修复状态文件。

---

## 门禁顺序
1. code-gen 前必须通过 `validate-ir.py --check-md-sync`。
2. compile 前必须通过 reviewer-agent + 持有有效 reviewer-pass.token。
3. 每次修复失败记录到 `repair-log.md`（含 fingerprint）。
4. 灾难场景必须走 `recover.sh`，不得就地修复。

> 一次性环境初始化（`mkdir -p .claude/...`、`chmod +x scripts/*`、首次 `doc-analyze.sh`）见 README。

---

## 输出格式约定

每个阶段完成后输出：
```
[PHASE] 编译 | 链接 | 调试 | 审查 | 恢复
[ROUND] <调试轮编号> (若适用)
[STATUS] ✓ 成功 / ✗ 失败(第N次/上限M) / ⚠ 灾难恢复
[TOKEN] issued/consumed/rejected (若适用)
[CHANGES] 修改文件列表
[FINGERPRINT] <12位> (若为错误修复)
[REASON] 根因说明
[NEXT] 下一步动作
```

---

## 附录
- Post-v3 Roadmap（orchestrator 外置 / capability isolation / 状态分层 / 双层 fingerprint / DRI 等未来迭代项）见 `docs/roadmap.md`。
