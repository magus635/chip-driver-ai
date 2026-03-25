# /compile-fix — 编译自修复命令

**调用方式：** `/compile-fix [--max <次数>]`

单独运行编译→自动修复循环，适合在代码手动修改后使用。
默认最大迭代次数 15，可通过 `--max` 覆盖。

---

## 执行逻辑

### 准备
```bash
source config/project.env
```
读取 `.claude/repair-log.md` 了解本模块历史修复记录，避免重复同样的错误。

### 循环体

**每次迭代执行：**

1. 运行编译：
   ```bash
   bash scripts/compile.sh 2>&1 | tee .claude/last-compile.log
   ```

2. 检查返回码：
   - 返回码 `0` → 编译成功，跳出循环，输出 `COMPILE_SUCCESS`
   - 其他 → 进入错误分析

3. 错误分析流程：
   - 解析 `.claude/last-compile.log`，提取所有 `error:` 行
   - 按文件分组，优先处理出现频率最高的错误类型
   - 对每个错误：读取对应源文件上下文（错误行 ±20 行）
   - 判断错误类型：
     - `undeclared identifier` → 检查头文件包含，补充 `#include` 或前向声明
     - `implicit declaration` → 补充函数声明或移动实现位置
     - `incompatible types` → 检查类型定义，添加正确转换
     - `too few arguments` → 补充缺失的函数参数
     - `expected ... before` → 语法错误，精确定位并修正
     - 未知类型 → 搜索 `src/` 和框架中类似用法作为参照

4. 执行修复（直接修改源文件）

5. 记录到日志：
   ```
   ## 编译修复 · 第N次 · YYYY-MM-DD HH:MM
   **错误：** <原始错误信息>
   **文件：** <文件路径>:<行号>
   **根因：** <分析说明>
   **修复：** <修改描述>
   ```
   追加到 `.claude/repair-log.md`

6. 如本次迭代未修复任何错误（修复前后 diff 为空），停止并报告死循环风险

### 超出迭代上限

输出诊断摘要：
```
[COMPILE_FAILED_MAX_ITER]
剩余错误数：N
最难解决的错误类型：...
建议人工检查：...
相关文档章节：...
```
