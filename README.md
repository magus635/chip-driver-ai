# chip-driver-ai

**基于 Claude Code 的嵌入式芯片驱动 AI 协作开发系统**

---

## 系统架构

```
chip-driver-ai/
├── CLAUDE.md                        # AI 主规则（Claude Code 自动读取）
├── install.sh                       # 安装脚本
│
├── .claude/
│   ├── settings.json                # Claude Code 权限配置
│   ├── commands/
│   │   ├── dev-driver.md            # /dev-driver  全流程开发命令
│   │   ├── compile-fix.md           # /compile-fix 编译自修复
│   │   ├── link-fix.md              # /link-fix    链接自修复
│   │   └── flash-debug.md           # /flash-debug 烧录+调试验证
│   ├── agents/
│   │   ├── doc-analyst.md           # 文档分析 Sub-agent
│   │   ├── compiler-agent.md        # 编译修复 Sub-agent
│   │   ├── linker-agent.md          # 链接修复 Sub-agent
│   │   └── debugger-agent.md        # 调试验证 Sub-agent
│   ├── repair-log.md                # 自动追加的修复历史
│   ├── debug-session.md             # 调试会话记录
│   ├── debug-registers.txt          # 调试时监控的寄存器列表
│   └── debug-variables.txt          # 调试时监控的变量列表
│
├── config/
│   ├── project.env                  # 工具链和芯片配置（必须修改）
│   ├── openocd.cfg                  # OpenOCD 探针配置
│   └── STM32F103C8T6.ld             # 链接脚本
│
├── scripts/
│   ├── check-env.sh                 # 环境检查
│   ├── compile.sh                   # 编译封装
│   ├── link.sh                      # 链接封装
│   ├── flash.sh                     # 烧录封装
│   └── debug-snapshot.sh            # 调试快照采集
│
├── docs/                            # 放入芯片手册、设计文档（AI 读取）
├── src/                             # 驱动源代码（AI 生成和修改）
│   ├── include/
│   └── can_driver.c                 # 框架示例
└── .vscode/                         # VS Code 任务和调试配置
```

---

## 快速开始

### 1. 安装到项目

```bash
# 克隆或下载 chip-driver-ai
git clone https://github.com/yourorg/chip-driver-ai.git

# 安装到你的嵌入式项目目录
bash chip-driver-ai/install.sh /path/to/your/project
```

### 2. 配置项目

```bash
cd /path/to/your/project

# 必须修改：芯片型号、工具链路径、串口、调试探针
vim config/project.env

# 放入芯片手册（PDF 或 Markdown 均可）
cp /path/to/stm32f103_reference_manual.pdf docs/
cp /path/to/design_spec.md docs/

# 放入驱动框架（已有 TODO 的框架文件）
cp /path/to/your/driver_framework/* src/

# 配置需要监控的寄存器（可选，用于调试验证）
vim .claude/debug-registers.txt
vim .claude/debug-variables.txt
```

### 3. 启动 Claude Code

```bash
claude
```

### 4. 使用 AI 命令

```
# 全流程：文档分析→代码生成→编译→链接→烧录→调试
/dev-driver can_bus

# 单独运行各阶段
/compile-fix
/link-fix
/flash-debug --criteria "CAN 初始化完成，可以收发报文"

# 从中断步骤恢复
/dev-driver uart --from-step compile
```

---

## 命令详解

### `/dev-driver <模块名>`

全流程自动化命令，按顺序执行：
1. **环境检查** — 验证工具链、文档、硬件连接
2. **文档分析** — AI 读取手册，提取寄存器表和初始化序列
3. **代码生成** — 在框架 TODO 处填写实现
4. **编译修复** — 循环编译→分析→修复，最多 15 次
5. **链接修复** — 循环链接→分析 map→修复，最多 10 次
6. **烧录调试** — 烧录→采集运行输出→对照文档验证→如有问题修改代码重来

### `/compile-fix`

单独运行编译修复循环。适合手动修改代码后快速验证。

### `/link-fix`

单独运行链接修复循环。会读取 `.map` 文件进行深度分析。

### `/flash-debug`

烧录固件并验证运行结果。AI 通过串口输出、寄存器读值、全局变量判断正确性。

---

## 工作原理

### 四个 Sub-Agent 分工

| Agent | 职责 | 工具 |
|---|---|---|
| `doc-analyst` | 读取手册，输出结构化摘要 | Read 文件工具 |
| `compiler-agent` | 编译→分析错误→修复→循环 | Bash + Edit 工具 |
| `linker-agent` | 链接→读 map→修复→循环 | Bash + Edit 工具 |
| `debugger-agent` | 烧录→采集快照→AI 分析→输出结论 | Bash + Read 工具 |

### 三个自修复循环

```
编译循环（最多15次）:  编译 → [失败] → 分析错误 → 修改源码 → 循环
链接循环（最多10次）:  链接 → [失败] → 读 .map  → 修改配置 → 循环
调试循环（最多8轮）:   烧录 → 采集  → AI 分析  → 修改代码 → 回编译
```

### CLAUDE.md 规则驱动

Claude Code 启动时自动读取 `CLAUDE.md`，其中定义了：
- AI 必须以文档为权威来源（不得猜测寄存器地址）
- 每次修复必须记录日志，防止死循环
- 迭代上限保护，超出后停止并请求人工介入
- 工具链命令的调用方式

---

## 自定义扩展

### 添加新的验证逻辑

编辑 `.claude/debug-error-flags.sh`，添加针对你的芯片的错误读取命令：

```bash
# 示例：读取 CAN 总线错误状态
CAN_ESR=$(openocd -c "mdw 0x40006418" 2>/dev/null | awk '{print $2}')
echo "CAN_ESR: $CAN_ESR"
echo "TEC (发送错误计数): $(( (0x$CAN_ESR >> 24) & 0xFF ))"
```

### 修改迭代上限

编辑 `CLAUDE.md` 中的规则 R3，修改迭代次数。

### 添加新命令

在 `.claude/commands/` 目录下创建新的 `.md` 文件，文件名即命令名（无需 `/` 前缀）。

---

## 常见问题

**Q: AI 在编译循环中卡住了？**
查看 `.claude/repair-log.md` 中最后几条记录，如果同一个错误反复出现且 diff 为空，说明 AI 遇到了其能力范围外的问题，需要人工介入。

**Q: 调试快照没有串口输出？**
检查 `config/project.env` 中的 `SERIAL_PORT` 和 `SERIAL_BAUD` 配置，以及串口线是否连接正确。

**Q: OpenOCD 无法连接目标板？**
检查 `config/openocd.cfg` 中的探针类型和目标芯片配置，运行 `bash scripts/check-env.sh` 查看诊断信息。

**Q: 链接脚本报 Flash 溢出？**
AI 会自动尝试优化（开启 `-Os`、`--gc-sections`），如仍溢出，检查 `config/project.env` 中的 `FLASH_SIZE` 是否与实际芯片一致。

---

## 支持的工具链

- **编译器**: `arm-none-eabi-gcc` （Arm GNU Toolchain）
- **调试探针**: ST-Link, J-Link, CMSIS-DAP
- **调试软件**: OpenOCD, pyOCD, JLinkExe
- **IDE**: VS Code + Cortex-Debug 插件

## 支持的芯片系列

理论上支持所有 ARM Cortex-M 芯片，已在以下系列验证：
- STM32F1/F2/F4/F7/H7
- STM32G0/G4/L4
- GD32F1/F3/F4
- nRF52/nRF5340

---

## 许可证

MIT License
