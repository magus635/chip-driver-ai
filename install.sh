#!/usr/bin/env bash
# install.sh — chip-driver-ai 安装脚本
# 将此协作系统安装到指定的嵌入式项目目录中
#
# 用法：
#   bash install.sh                    # 安装到当前目录
#   bash install.sh /path/to/project   # 安装到指定项目目录
#
# 安装后在项目目录执行 `claude` 即可启动 Claude Code

set -euo pipefail

INSTALL_SRC="$(cd "$(dirname "$0")" && pwd)"
TARGET_DIR="${1:-$(pwd)}"

# ── 颜色输出 ─────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
BLUE='\033[0;34m'; BOLD='\033[1m'; RESET='\033[0m'
info()    { echo -e "${BLUE}[INFO]${RESET}  $*"; }
success() { echo -e "${GREEN}[OK]${RESET}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${RESET}  $*"; }
error()   { echo -e "${RED}[ERROR]${RESET} $*"; exit 1; }
header()  { echo -e "\n${BOLD}$*${RESET}"; }

# ── 欢迎 ─────────────────────────────────────────────────
echo ""
echo -e "${BOLD}╔══════════════════════════════════════════════╗${RESET}"
echo -e "${BOLD}║   chip-driver-ai · Claude Code 安装程序      ║${RESET}"
echo -e "${BOLD}║   芯片驱动 AI 协作系统                        ║${RESET}"
echo -e "${BOLD}╚══════════════════════════════════════════════╝${RESET}"
echo ""
info "安装源：$INSTALL_SRC"
info "目标目录：$TARGET_DIR"

# ── 前置检查 ──────────────────────────────────────────────
header "[ 1/5 ] 前置检查"

# 检查 Claude Code 是否安装
if ! command -v claude &>/dev/null; then
    warn "未检测到 claude 命令"
    warn "请先安装 Claude Code："
    echo "       npm install -g @anthropic-ai/claude-code"
    echo ""
    read -r -p "继续安装文件（稍后再安装 Claude Code）？[y/N] " confirm
    [[ "$confirm" =~ ^[Yy]$ ]] || error "安装中止"
else
    CLAUDE_VER=$(claude --version 2>/dev/null || echo "未知版本")
    success "Claude Code 已安装 ($CLAUDE_VER)"
fi

# 检查目标目录
if [ ! -d "$TARGET_DIR" ]; then
    read -r -p "目录 $TARGET_DIR 不存在，是否创建？[y/N] " confirm
    [[ "$confirm" =~ ^[Yy]$ ]] || error "安装中止"
    mkdir -p "$TARGET_DIR"
    success "已创建目录：$TARGET_DIR"
else
    success "目标目录存在：$TARGET_DIR"
fi

# ── 目录结构创建 ──────────────────────────────────────────
header "[ 2/5 ] 创建目录结构"

DIRS=(
    ".claude/commands"
    ".claude/agents"
    "scripts"
    "config"
    "docs"
    "src"
    "build"
)

for dir in "${DIRS[@]}"; do
    FULL_PATH="$TARGET_DIR/$dir"
    if [ ! -d "$FULL_PATH" ]; then
        mkdir -p "$FULL_PATH"
        success "创建 $dir/"
    else
        info "已存在 $dir/ （跳过）"
    fi
done

# ── 复制核心文件 ──────────────────────────────────────────
header "[ 3/5 ] 安装系统文件"

# CLAUDE.md — 如果目标已有，追加而不是覆盖
install_or_append() {
    local src="$1" dst="$2" label="$3"
    if [ -f "$dst" ]; then
        warn "$label 已存在，备份为 ${dst}.bak"
        cp "$dst" "${dst}.bak"
    fi
    cp "$src" "$dst"
    success "安装 $label"
}

install_or_append "$INSTALL_SRC/CLAUDE.md"                       "$TARGET_DIR/CLAUDE.md"                       "CLAUDE.md（主规则）"
install_or_append "$INSTALL_SRC/.claude/settings.json"           "$TARGET_DIR/.claude/settings.json"           "settings.json（权限配置）"
install_or_append "$INSTALL_SRC/.claude/repair-log.md"           "$TARGET_DIR/.claude/repair-log.md"           "repair-log.md（修复日志）"
install_or_append "$INSTALL_SRC/.claude/debug-session.md"        "$TARGET_DIR/.claude/debug-session.md"        "debug-session.md（调试记录）"

# Commands
for cmd in dev-driver compile-fix link-fix flash-debug; do
    install_or_append \
        "$INSTALL_SRC/.claude/commands/${cmd}.md" \
        "$TARGET_DIR/.claude/commands/${cmd}.md" \
        "命令 /${cmd}"
done

# Agents
for agent in doc-analyst compiler-agent linker-agent debugger-agent; do
    install_or_append \
        "$INSTALL_SRC/.claude/agents/${agent}.md" \
        "$TARGET_DIR/.claude/agents/${agent}.md" \
        "Agent ${agent}"
done

# Scripts
for script in compile.sh link.sh flash.sh debug-snapshot.sh check-env.sh; do
    install_or_append \
        "$INSTALL_SRC/scripts/${script}" \
        "$TARGET_DIR/scripts/${script}" \
        "脚本 scripts/${script}"
    chmod +x "$TARGET_DIR/scripts/${script}"
done

# Config（只在不存在时安装，避免覆盖用户已配置的项目）
if [ ! -f "$TARGET_DIR/config/project.env" ]; then
    cp "$INSTALL_SRC/config/project.env" "$TARGET_DIR/config/project.env"
    success "安装 config/project.env（模板，请按项目修改）"
else
    info "config/project.env 已存在，保留用户配置"
fi

# ── VSCode 集成 ───────────────────────────────────────────
header "[ 4/5 ] 配置 VS Code 集成"

mkdir -p "$TARGET_DIR/.vscode"

# tasks.json
cat > "$TARGET_DIR/.vscode/tasks.json" << 'TASKS_EOF'
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "🔨 Compile",
      "type": "shell",
      "command": "bash scripts/compile.sh",
      "group": "build",
      "presentation": { "reveal": "always", "panel": "shared" },
      "problemMatcher": {
        "owner": "gcc",
        "fileLocation": ["relative", "${workspaceFolder}"],
        "pattern": {
          "regexp": "^(.+):(\\d+):(\\d+):\\s+(warning|error):\\s+(.+)$",
          "file": 1, "line": 2, "column": 3, "severity": 4, "message": 5
        }
      }
    },
    {
      "label": "🔗 Link",
      "type": "shell",
      "command": "bash scripts/link.sh",
      "group": "build",
      "dependsOn": "🔨 Compile",
      "presentation": { "reveal": "always", "panel": "shared" }
    },
    {
      "label": "⚡ Flash",
      "type": "shell",
      "command": "bash scripts/flash.sh",
      "group": "test",
      "dependsOn": "🔗 Link",
      "presentation": { "reveal": "always", "panel": "shared" }
    },
    {
      "label": "📡 Debug Snapshot",
      "type": "shell",
      "command": "bash scripts/debug-snapshot.sh | tee .claude/runtime-output.txt",
      "group": "test",
      "presentation": { "reveal": "always", "panel": "shared" }
    },
    {
      "label": "✅ Full Build (Compile + Link)",
      "dependsOn": ["🔨 Compile", "🔗 Link"],
      "dependsOrder": "sequence",
      "group": { "kind": "build", "isDefault": true }
    },
    {
      "label": "🚀 Build + Flash + Debug",
      "dependsOn": ["🔨 Compile", "🔗 Link", "⚡ Flash", "📡 Debug Snapshot"],
      "dependsOrder": "sequence",
      "group": "test"
    },
    {
      "label": "🔍 Check Environment",
      "type": "shell",
      "command": "bash scripts/check-env.sh",
      "presentation": { "reveal": "always", "panel": "shared" }
    }
  ]
}
TASKS_EOF
success "安装 .vscode/tasks.json"

# launch.json（GDB 调试配置）
cat > "$TARGET_DIR/.vscode/launch.json" << 'LAUNCH_EOF'
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug on Target (OpenOCD)",
      "type": "cortex-debug",
      "request": "launch",
      "servertype": "openocd",
      "configFiles": ["config/openocd.cfg"],
      "executable": "build/firmware.elf",
      "runToEntryPoint": "main",
      "showDevDebugOutput": "none",
      "svdFile": "config/SVD_FILE.svd"
    },
    {
      "name": "Debug on Target (JLink)",
      "type": "cortex-debug",
      "request": "launch",
      "servertype": "jlink",
      "executable": "build/firmware.elf",
      "device": "${env:CHIP_MODEL}",
      "interface": "swd",
      "runToEntryPoint": "main"
    }
  ]
}
LAUNCH_EOF
success "安装 .vscode/launch.json"

# c_cpp_properties.json
cat > "$TARGET_DIR/.vscode/c_cpp_properties.json" << 'CPP_EOF'
{
  "configurations": [
    {
      "name": "ARM Embedded",
      "includePath": [
        "${workspaceFolder}/src/**",
        "${workspaceFolder}/src/include/**"
      ],
      "defines": [
        "USE_HAL_DRIVER",
        "__ARM_ARCH_7EM__"
      ],
      "compilerPath": "/usr/bin/arm-none-eabi-gcc",
      "cStandard": "c11",
      "cppStandard": "c++14",
      "intelliSenseMode": "gcc-arm"
    }
  ],
  "version": 4
}
CPP_EOF
success "安装 .vscode/c_cpp_properties.json"

# ── 安装摘要 ─────────────────────────────────────────────
header "[ 5/5 ] 安装完成"

echo ""
echo -e "${GREEN}${BOLD}✓ chip-driver-ai 安装成功！${RESET}"
echo ""
echo -e "${BOLD}已安装的文件：${RESET}"
echo "  CLAUDE.md                      — AI 主规则（自动被 Claude Code 读取）"
echo "  .claude/commands/dev-driver    — /dev-driver 全流程命令"
echo "  .claude/commands/compile-fix   — /compile-fix 编译自修复命令"
echo "  .claude/commands/link-fix      — /link-fix 链接自修复命令"
echo "  .claude/commands/flash-debug   — /flash-debug 烧录调试命令"
echo "  .claude/agents/doc-analyst     — 文档分析 Agent"
echo "  .claude/agents/compiler-agent  — 编译修复 Agent"
echo "  .claude/agents/linker-agent    — 链接修复 Agent"
echo "  .claude/agents/debugger-agent  — 调试验证 Agent"
echo "  scripts/*.sh                   — 工具链封装脚本"
echo "  .vscode/                       — VS Code 任务和调试配置"
echo ""
echo -e "${BOLD}下一步操作：${RESET}"
echo ""
echo "  1. 修改工具链配置："
echo "     ${YELLOW}vim $TARGET_DIR/config/project.env${RESET}"
echo ""
echo "  2. 将芯片手册放入 docs/ 目录："
echo "     ${YELLOW}cp /path/to/datasheet.pdf $TARGET_DIR/docs/${RESET}"
echo ""
echo "  3. 将驱动框架代码放入 src/ 目录"
echo ""
echo "  4. 进入项目目录并启动 Claude Code："
echo "     ${YELLOW}cd $TARGET_DIR && claude${RESET}"
echo ""
echo "  5. 在 Claude Code 中运行开发命令："
echo "     ${GREEN}/dev-driver <模块名>${RESET}        # 全流程自动开发"
echo "     ${GREEN}/compile-fix${RESET}               # 仅编译修复"
echo "     ${GREEN}/link-fix${RESET}                  # 仅链接修复"
echo "     ${GREEN}/flash-debug${RESET}               # 烧录并调试验证"
echo ""
echo -e "${BOLD}参考文档：${RESET}"
echo "  $TARGET_DIR/CLAUDE.md          — 完整规则和配置说明"
echo ""
