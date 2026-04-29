# GPIO + AFIO SRS（Software Requirements Specification）

> R11 · ASPICE SWE.1 工作产品。每条 SWR 通过 `<!-- SWR-XXX-NNN -->` 锚点被
> `ir/gpio_afio_feature_matrix.json` 中各 feature 的 `swr_refs[]` 引用。
> 校验：`python3 scripts/validate-feature-matrix.py ir/gpio_afio_feature_matrix.json --check-swr`

---

## SWR-GPIO-001 提供 GPIO 引脚生命周期管理

<!-- SWR-GPIO-001 -->

| 项 | 内容 |
|---|---|
| 优先级 | MUST |
| 来源   | 内部约定（驱动框架基线） |
| ASIL   | QM |
| 状态   | implemented |

**描述**：驱动须提供配对的 Init / DeInit 接口，覆盖 GPIO bank 的初始化与复位流程，保证应用可重复进入退出。

**验收准则**：
- AC-1：`GpioAfio_Init(NULL)` 必须返回 `GPIO_PARAM`
- AC-2：成功 `Init` 后再 `DeInit` 同 bank，CRL/CRH/ODR 应回到上电默认 (`0x44444444 / 0x44444444 / 0`)
- AC-3：`Init` 自带 RCC 时钟使能，调用方无需先开 IOPxEN

**关联**：
- Feature：`GPIO_AFIO-INIT`
- IR refs：`init_sequence`、`registers.GPIO_CRL/CRH/ODR`
- Test cases：（待补 TC-GPIO-001）

---

## SWR-GPIO-002 支持全部 16 种引脚模式

<!-- SWR-GPIO-002 -->

| 项 | 内容 |
|---|---|
| 优先级 | MUST |
| 来源   | 硬件能力（RM0008 §9.1.7-§9.1.11） |
| ASIL   | QM |
| 状态   | implemented |

**描述**：驱动须支持 RM0008 定义的 4 种输入模式（Analog / Floating / PullUp / PullDown）和 12 种输出模式（PP/OD × AF? × 三档速度）共 16 种 GPIO 模式。

**验收准则**：
- AC-1：`GpioAfio_Mode_e` 枚举值与 (MODE[1:0] << 2) | CNF[1:0] 拼装结果一致
- AC-2：每种模式 `Init` 后读 CRL/CRH 对应位段值与枚举值匹配
- AC-3：`INPUT_PUPD` 模式下，`pull` 字段决定 ODR 位（UP→1, DOWN→0）

**关联**：
- Feature：`GPIO_AFIO-MODE-INPUT-FLOATING`、`GPIO_AFIO-MODE-INPUT-PULL-UP`、`GPIO_AFIO-MODE-INPUT-PULL-DOWN`、`GPIO_AFIO-MODE-INPUT-ANALOG`、`GPIO_AFIO-MODE-OUTPUT-PP-10MHZ`、`GPIO_AFIO-MODE-OUTPUT-PP-2MHZ`、`GPIO_AFIO-MODE-OUTPUT-PP-50MHZ`、`GPIO_AFIO-MODE-OUTPUT-OD`、`GPIO_AFIO-MODE-OUTPUT-AF-PP`、`GPIO_AFIO-MODE-OUTPUT-AF-OD`
- IR refs：`operation_modes[]`、`registers.GPIO_CRL/CRH`
- Test cases：（待补）

---

## SWR-GPIO-003 强制硬件前置条件守卫

<!-- SWR-GPIO-003 -->

| 项 | 内容 |
|---|---|
| 优先级 | MUST |
| 来源   | 安全分析（防硬件锁死 / 静默失败） |
| ASIL   | QM（如未来升 ASIL-B 则该 SWR 状态升级到强制运行时校验） |
| 状态   | implemented |

**描述**：所有寄存器写入必须满足硬件前置条件——AFIOEN/IOPxEN 必须先使能；锁定的引脚不得修改。

**验收准则**：
- AC-1：未使能 IOPxEN 调用 `Init` 时，函数自动开启时钟（不静默失败）
- AC-2：bank 处于 LCKR 锁定状态时，`Init` 返回 `GPIO_LOCKED` 而非默默无效写入
- AC-3：`SetExtiSource` 必须先 `EnableClockAfio` 再写 EXTICRn

**关联**：
- Feature：`GPIO_AFIO-GUARD-INV-GPIO-001`、`GPIO_AFIO-GUARD-INV-GPIO-002`、`GPIO_AFIO-GUARD-INV-GPIO-003`
- IR refs：`invariants[INV_GPIO_001/002/003]`
- Test cases：（待补 TC-GPIO-003）
