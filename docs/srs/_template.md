# SWR 模板（R11 · ASPICE 追溯）

> 每个模块的 SRS 文件命名为 `docs/srs/<module>_srs.md`（如 `gpio_afio_srs.md`）。
> SWR ID 命名：`SWR-<MODULE_UPPER>-<NNN>`（NNN 三位数字，001 起）。
> 每条 SWR 必须包含 HTML 锚点注释 `<!-- SWR-XXX-NNN -->`，校验脚本依赖此锚点。

---

## SWR-XXX-NNN 标题（动宾短语）

<!-- SWR-XXX-NNN -->

| 项 | 内容 |
|---|---|
| **优先级** | MUST / SHOULD / NICE |
| **来源**   | 客户需求 / 安全分析 / 硬件能力 / 内部约定 |
| **关联 ASIL** | QM / ASIL-A / B / C / D |
| **状态**   | proposed / accepted / implemented / verified |

**描述**：1-3 句话说明这条 SWR 要求驱动做什么、不做什么。

**验收准则**：
- AC-1：可观测、可测试的条件 1
- AC-2：条件 2
- AC-3：条件 3（每条 SWR 至少 2 条 AC）

**关联**：
- Feature：`<MODULE>-XXX-YYY`（实现此 SWR 的 feature_id；可一对多）
- IR refs：相关 IR 字段（registers / atomic_sequences / invariants 等）
- Test cases：`TC-XXX-NNN`（SWE.4 测试用例 ID，未实现时空）

---

## 反模式（reviewer 必拒）

- ❌ SWR 表述含糊（"应该工作正常"）—— AC 必须可测
- ❌ 一条 SWR 同时绑定 ≥ 5 个 feature —— SWR 太粗，应拆分
- ❌ feature 的 `swr_refs[]` 引用了 SRS 里不存在的 SWR ID
- ❌ SWR 状态为 `verified` 但 Test cases 列空
