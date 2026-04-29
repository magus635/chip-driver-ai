# 详细设计文档模板（R8#13 强制）

> 每个 driver 模块在 `src/drivers/<Module>/<module>_detailed_design.md` 必须按本模板组织。
> 文档由 codegen-agent 在 STEP 2 生成并随代码演进同步更新。
> `scripts/check-detailed-design.py` 校验所有强制章节存在；缺章 → STEP 2.D 拒绝放行。
>
> **每章必须与代码一一对应**：脱节即视为 R8#13 违规。

---

## 强制章节列表

| § | 标题 | 必须性 | 内容要求 | 对应代码 |
|:-:|---|:-:|---|---|
| 1 | 架构目标 | MUST | 模块职责、`safety_level`、外部依赖、设计约束 | IR JSON 顶层 |
| 2 | 文件清单 | MUST | 每个 .h/.c 一行，标注层级（reg/types/cfg/ll/api/drv/isr）+ 单句职责 | `src/drivers/<Module>/` |
| 3 | 功能特性矩阵 | MUST | `<!-- FEATURE_MATRIX:BEGIN/END -->` 自动渲染区块（R10） | `ir/<module>_feature_matrix.json` |
| 4 | 异步闭环设计 | MUST | ISR → driver handler → 用户回调的链路；DMA / 中断的解耦点；状态机转移；如纯轮询，写"无异步路径"声明 | `<module>_isr.c` + `<module>_drv.c` |
| 5 | 数据结构 | MUST | 每个 `ConfigType` / `HandleType` / `State_e` 字段表（名称/类型/语义/约束） | `<module>_types.h` |
| 6 | 接口函数说明 | MUST | 每个 public API：原型 / 参数 / 返回值 / 副作用 / 错误码 / 前置条件 / 调用线程上下文（thread/ISR） | `<module>_api.h` |
| 7 | 流程图 | MUST | 至少 3 张 mermaid：Init 流程、主传输路径、ISR/DeInit 之一 | `<module>_drv.c` 状态转移 |
| 8 | 调用示例 | MUST | 完整可编译 C 片段："include + Config 填充 + Init + 一次完整 IO + DeInit" | `<module>_api.h` |
| 9 | 不变式审计 | MUST | IR `invariants[]` 全表 + 每条标注守卫注入位置（文件:函数） | IR + LL/Drv 守卫注释 |

---

## 章节 ≠ 单纯凑齐

校验脚本只能查 H2 标题存在性。**人工评审重点**：

- §6 接口说明的"参数约束"必须能在 §9 不变式中找到对应（如 "port 必须 < GPIO_PORT_COUNT" ↔ INV_GPIO_xxx）
- §7 流程图分支必须对应 `drv.c` 中真实分支（每个判定节点能回溯到 if/switch）
- §8 调用示例必须能复制粘贴编译通过（reviewer 应该真的试一遍）
- §3 矩阵的 `target_apis` 必须在 §6 中有完整说明
- §5 字段名必须与 types.h 完全一致（diff 友好）

---

## 与现有规则的关系

- **R1 IR 真值源**：§1/§3/§9 直接派生自 IR JSON；任何手写偏差视为 IR 漂移
- **R8#13 同步约束**：本模板就是 R8#13 的具象化
- **R8#14 全量对账**：通过 §3 的自动渲染矩阵实现
- **R10 增量补齐**：§3 的 todo 项每补一个，对应在 §6/§7/§8 增加一段
- **R9b 架构决策**：若有多方案选择，决策结论写入 §1 末尾"架构决策记录"小节

---

## 反模式（reviewer 必拒）

- ❌ §6 写 "见代码"——必须在文档里展开
- ❌ §7 流程图与 `drv.c` 函数名不一致
- ❌ §8 写"略" / "略过"
- ❌ §9 只列 INV ID，不标注守卫位置
- ❌ §3 手工编辑而不通过 `feature-update.py --render`
