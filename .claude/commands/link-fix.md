# /link-fix — 链接自修复命令

**调用方式：** `/link-fix [--max <次数>]`

单独运行链接→自动修复循环。默认最大迭代次数 10。

---

## 执行逻辑

### 准备
```bash
source config/project.env
```
读取 `.claude/repair-log.md` 了解历史链接修复记录。

### 每次迭代

1. 执行链接：
   ```bash
   bash scripts/link.sh 2>&1 | tee .claude/last-link.log
   ```

2. 检查返回码：
   - `0` → 链接成功，跳出循环，输出 `LINK_SUCCESS`
   - 其他 → 进入错误分析

3. 错误分析（链接错误通常比编译错误更难定位）：

   **读取 `.map` 文件：**
   ```bash
   cat build/output.map
   ```

   **按错误类型分类处理：**

   - `undefined reference to 'xxx'`
     1. 在 `src/` 中搜索 `xxx` 的声明位置
     2. 如有声明无实现 → 补充函数体
     3. 如实现存在但未被链接 → 检查 `CMakeLists.txt` 或 `Makefile` 的 `SRC` 变量
     4. 如是弱符号覆盖问题 → 检查 `__weak` 定义

   - `multiple definition of 'xxx'`
     1. 在 `.map` 文件中找到两个定义的位置
     2. 检查是否因头文件被多次包含（加 `#pragma once` 或 include guard）
     3. 或将定义移入 `.c` 文件，头文件只保留 `extern` 声明

   - `region ... overflowed by ... bytes`
     1. 读取链接脚本（`config/` 或 `src/` 中的 `.ld` 文件）确认各段大小限制
     2. 分析 `.map` 文件中占用最多空间的符号
     3. 优化方案：移入 `.ccmram`/`.bss`，或启用 `-fdata-sections -ffunction-sections --gc-sections`

   - `cannot find -lxxx`
     1. 检查 `config/project.env` 中 `LIBS` 变量
     2. 检查 toolchain sysroot 路径

4. 修复并追加日志到 `.claude/repair-log.md`

5. 如修复前后无变化，停止并报警

### 超出迭代上限

```
[LINK_FAILED_MAX_ITER]
剩余链接错误：N 个
.map 文件中最大段占用：...
建议人工检查链接脚本：config/*.ld
```
