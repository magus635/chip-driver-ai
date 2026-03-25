# /flash-debug — 烧录与调试验证命令

**调用方式：** `/flash-debug [--criteria "<验证条件>"]`

烧录固件到目标板，捕获运行输出，AI 分析验证正确性。
`--criteria` 可指定自定义验证条件，否则从 `.claude/doc-summary.md` 中推断。

---

## 执行逻辑

### 阶段 1 · 烧录

```bash
source config/project.env
bash scripts/flash.sh
```

检查返回码：
- 非 0 → 报告烧录失败（通常是调试探针连接问题），停止并提示人工检查

### 阶段 2 · 运行快照采集

```bash
bash scripts/debug-snapshot.sh | tee .claude/runtime-output.txt
```

`debug-snapshot.sh` 完成以下动作（由用户根据项目配置）：
- 通过串口/RTT 采集 N 秒的打印输出
- 通过 OpenOCD/pyOCD 读取关键寄存器值
- 读取关键全局变量（地址从 `.map` 文件解析）
- 将以上数据保存为结构化文本

### 阶段 3 · AI 分析运行结果

读取以下上下文：
1. `.claude/runtime-output.txt`（本次运行快照）
2. `.claude/doc-summary.md`（期望行为描述）
3. `src/` 中相关驱动源文件（对照代码逻辑）
4. `--criteria` 参数（用户自定义验证条件）

**验证维度：**

| 验证项 | 方法 |
|---|---|
| 初始化完成标志 | 检查 `printf("XXX init OK")` 或对应寄存器 READY 位 |
| 寄存器配置正确性 | 读取寄存器值与手册期望值比对 |
| 数据收发正确性 | 检查发送/接收计数、CRC 错误计数 |
| 中断触发 | 确认中断服务函数被执行 |
| 时序合规 | 分析时间戳间隔是否在手册允许范围内 |

**输出判断：**

```
[DEBUG_ANALYSIS]
验证通过项：N/M
验证失败项：
  - 项目1：期望 XXX，实际 YYY → 推测原因：...
  - 项目2：...

假设根因：
  1. 寄存器 CAN_BTR 的 BRP 字段计算错误（参考 DS §12.4.2）
  2. 未等待 INAK 标志置位即退出初始化（参考 DS §12.5.1）

建议修改：
  - 文件：src/can_driver.c 第 45 行
  - 修改内容：...
```

若所有验证通过，输出 `DEBUG_PASS`。
若有失败项，输出 `DEBUG_FAIL_NEED_REWORK`，并更新 `.claude/debug-session.md`。

### 阶段 4 · 更新调试会话记录

将本轮快照、分析结果、假设根因追加到 `.claude/debug-session.md`：
```markdown
## 调试轮次 N · YYYY-MM-DD HH:MM
**固件版本：** <git hash 或时间戳>
**运行输出摘要：** ...
**寄存器读值：** ...
**验证结论：** PASS / FAIL
**根因假设：** ...
**下次修改方向：** ...
```
