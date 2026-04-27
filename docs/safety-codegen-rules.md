# Safety Level → Code Generation Rules

> 本文件定义 IR JSON 顶层 `safety_level` 字段如何驱动 codegen-agent 生成额外的冗余/诊断代码。被 CLAUDE.md `safety_level 处理` 章节引用。

## 字段语义

`safety_level ∈ { QM | ASIL-A | ASIL-B | ASIL-C | ASIL-D }`，按 ISO 26262 分级。未声明视为 `QM`。

## 代码生成增量

| safety_level | LL 层增量 | Driver 层增量 | 测试增量 |
|---|---|---|---|
| `QM` | 无额外要求 | 无 | 单元测试覆盖 init/transfer 主路径 |
| `ASIL-A` | 关键写后回读校验 | 错误码暴露给上层 | 加 1 条 fault-injection 用例 |
| `ASIL-B` | 全部关键寄存器写后回读 | 周期性自检 API（`<Module>_PeriodicCheck`） | 边界值 + 错误注入 ≥ 3 条 |
| `ASIL-C` | 双读多数表决（关键状态字段） | 状态机锁定 + 看门狗刷新点；reviewer 必须 IR 完整性审查 | 强制覆盖率 ≥ 90% 分支 |
| `ASIL-D` | 双读 + CRC 校验 + 冗余镜像变量 | 双通道执行流 + 互锁；reviewer 拒绝任何 IR ambiguity | MC/DC 覆盖 + 形式化属性证明 |

## reviewer-agent 行为联动

- `safety_level ≥ ASIL-C`：reviewer 在代码层审查中必须**额外**运行 IR 完整性审查（见 CLAUDE.md `safety_level 处理 · reviewer-agent 在 ASIL-C/D 的行为`）。
- 任何 IR ambiguity / 未标注 Errata / safety_class 未经人工决策 → 直接拒绝并写 `.claude/ir-rejection.md`。

## 落地约束

1. codegen-agent 在生成 `_drv.c` 时按 `safety_level` 表挑选增量并标注：`/* SAFETY: ASIL-C — 双读多数表决 */`
2. 增量代码必须可被 `#ifdef SAFETY_LEVEL_ASIL_C` 之类的开关关闭，便于单元测试在 QM 模式下复用主路径
3. 本文件变更必须同步更新 CLAUDE.md `safety_level 处理` 章节中的引用
