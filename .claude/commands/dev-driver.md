# /dev-driver — 驱动开发全流程命令

**调用方式：** `/dev-driver <模块名> [--from-step <step>]`

**示例：**
- `/dev-driver can_bus`          从头开始开发 CAN 总线驱动
- `/dev-driver uart --from-step compile`  从编译阶段恢复

---

## 执行步骤

收到此命令后，按以下顺序完整执行。每步完成后输出阶段摘要，再进入下一步。

### STEP 0 · 环境检查
```bash
source config/project.env && bash scripts/check-env.sh
```
验证工具链、目标板连接、必要文档是否就绪。如有缺失，列出清单并停止。

### STEP 1 · 文档分析 (调用 doc-analyst agent)

使用 Task 工具启动 `doc-analyst` agent，传入参数：
```
module=$ARGUMENTS
docs_dir=docs/
output=.claude/doc-summary.md
```

等待 agent 完成后，读取 `.claude/doc-summary.md`，确认以下内容已提取：
- [ ] 外设基地址和寄存器偏移表
- [ ] 初始化序列（寄存器写入顺序）
- [ ] 关键时序参数
- [ ] 中断/DMA 相关配置

### STEP 1.5 · 寄存器级交叉审查 (Review & HitL Confirm)

使用 Task 工具启动 `reviewer-agent` agent 进行静态防呆和内存对齐分析：
```
doc_summary=.claude/doc-summary.md
module=$ARGUMENTS
```

等待 agent 审查返回结果：
- 若返回 `REVIEW_FAILED`：停止流程，根据诊断报告修改约束或让系统打回重做。
- 若返回 `REVIEW_PASSED`：**必须在此立刻暂停整个流程，等待人工复核**。弹窗/打印提示：
> 🚨 `[HITL 等待人工决断]` AI 已经完成底层“地基”设计及自动化机审，未发现位段重叠或偏移越界。
> 请作为 Owner 的你最后扫视一眼 `.claude/doc-summary.md`。如果架构和物理地址提取均无误，请回复“继续”(Continue)，系统将进入海量代码生成与编译修复阶段。

等待用户回复确认指令后，方可进入 STEP 2。

### STEP 2 · 业务代码硬核生成

读取以下上下文后生成驱动代码：
1. `.claude/doc-summary.md`（经过审查的高亮规则）
2. `src/` 目录下已有的框架文件（识别 TODO/stub 位置）
3. `config/project.env`（芯片型号、时钟频率等）
4. **`docs/architecture-guidelines.md` 或相关架构规范（绝不能跨层违规：底层操作比如 |= 必须封装在 _ll 层的 static inline 中，高层业务只能调底层的 wrapper API）**
5. **`docs/embedded-c-coding-standard.md`（确保 MISRA-C 与标准合规）**

生成规则：
- 强制遵守【硬件潜能开发原则】：
    1. **拒绝“玩具级”驱动**：严禁生成只包含单一模式或固定波特率的 API，必须提供 `ConfigType` 配置结构体。
    2. **拒绝“死等”逻辑**：除初始化外，数据收发层严禁使用无限 `while`，必须引入超时退出或中断/DMA 事件。
    3. **全量错误应对**：代码必须包含状态机，能实时处理 OVR、CRC 错误、总线挂死，并提供 `GetStatus` API 暴露给上层诊断。
    4. **资源利用最大化**：例如 CAN 模块必须生成多邮箱管理算法（非固定 0 号邮箱），SPI 必须包含 8/16位动态切换。
- 只在框架预留位置填写实现，不新增文件
- 每个关键操作旁注明文档依据（章节号）
- 生成后输出修改文件清单

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
doc_summary=.claude/doc-summary.md
session_file=.claude/debug-session.md
max_rounds=8
```

如 agent 返回 `DEBUG_PASS`，流程结束，输出完成摘要。
如 agent 返回 `DEBUG_FAIL_NEED_REWORK`，读取 `.claude/debug-session.md` 
中的错误分析，修改源码后从 **STEP 3** 重新开始。

---

## 流程结束输出

```
╔══════════════════════════════════════╗
║   驱动模块开发完成                    ║
║   模块：$ARGUMENTS                   ║
║   编译修复次数：N                     ║
║   链接修复次数：N                     ║
║   调试轮次：N                         ║
║   修改文件：file1, file2, ...         ║
╚══════════════════════════════════════╝
```
