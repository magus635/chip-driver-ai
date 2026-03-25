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

### STEP 2 · 代码生成

读取以下上下文后生成驱动代码：
1. `.claude/doc-summary.md`（文档摘要）
2. `src/` 目录下已有的框架文件（识别 TODO/stub 位置）
3. `config/project.env`（芯片型号、时钟频率等）

生成规则：
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
如返回 `COMPILE_FAILED_MAX_ITER`，停止并输出诊断报告。

### STEP 4 · 链接修复循环 (调用 linker-agent)

使用 Task 工具启动 `linker-agent` agent：
```
max_iterations=10
link_script=scripts/link.sh
map_file=build/output.map
log_file=.claude/repair-log.md
```

等待 agent 返回。如返回 `LINK_SUCCESS`，进入 STEP 5。
如返回 `LINK_FAILED_MAX_ITER`，停止并输出诊断报告。

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
