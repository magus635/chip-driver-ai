#!/usr/bin/env python3
"""
从 ir/<module>_ir_summary.json 反推一份初版 feature_matrix.json。

冷启动场景使用：doc-analyst 完成 IR 后、首轮 codegen 之前调用本脚本。
所有 feature 状态默认 `todo`；codegen-agent 首轮跑完后通过 feature-update.py
把已落实的 feature 翻成 done。

启发式来源（按优先级合并）：
  1. peripheral.operating_modes / operation_modes / functional_model.operating_modes
  2. peripheral.functionality_matrix (I2C 风格)
  3. peripheral.features (ADC 风格)
  4. peripheral.interrupts          → 一个 IRQ 处理 feature
  5. peripheral.dma_channels        → 一个 DMA 集成 feature
  6. peripheral.errata              → 每条 errata 一个 workaround feature
  7. peripheral.invariants          → 每条 invariant 一个 guard feature
  8. 基础 Init/DeInit                → 始终生成

用法：
  python3 scripts/feature-bootstrap.py ir/dma_ir_summary.json
  python3 scripts/feature-bootstrap.py ir/dma_ir_summary.json --force
  python3 scripts/feature-bootstrap.py ir/dma_ir_summary.json --dry-run

退出码：0 已生成 / 1 失败 / 2 已存在且未 --force
"""
import argparse
import json
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent


def slug(s: str) -> str:
    s = re.sub(r"[^A-Za-z0-9]+", "-", s).strip("-").upper()
    return s[:32] if s else "X"


def derive_features(module: str, ir: dict):
    p = ir.get("peripheral", ir)
    mod_up = module.upper()
    feats = []

    def add(fid, category, name, apis, refs, acceptance, notes="", deps=None):
        feats.append({
            "feature_id": fid,
            "category": category,
            "name": name,
            "status": "todo",
            "target_apis": apis,
            "depends_on": deps or [],
            "ir_refs": refs,
            "acceptance": acceptance,
            "notes": notes,
        })

    # 1. 基础 Init/DeInit
    init_id = f"{mod_up}-INIT"
    add(init_id, "生命周期", "Init / DeInit",
        [f"{module.title()}_Init", f"{module.title()}_DeInit"],
        ["init_sequence"] if "init_sequence" in p else [],
        f"{module.title()}_Init 完成寄存器初始化序列；{module.title()}_DeInit 释放资源")

    # 2. operating modes
    modes = (
        p.get("operating_modes")
        or p.get("operation_modes")
        or (p.get("functional_model") or {}).get("operating_modes")
        or []
    )
    for m in modes if isinstance(modes, list) else []:
        if not isinstance(m, dict):
            continue
        nm = m.get("name") or m.get("id") or "mode"
        fid = f"{mod_up}-MODE-{slug(nm)}"
        add(fid, "操作模式", nm,
            [f"{module.title()}_Configure"],
            [f"operating_modes.{nm}"],
            f"配置 {nm} 模式，符合 IR 中字段约束",
            deps=[init_id])

    # 3. functionality_matrix (I2C 风格)
    fm = p.get("functionality_matrix")
    if isinstance(fm, list):
        for item in fm:
            if not isinstance(item, dict):
                continue
            nm = item.get("name") or item.get("feature") or "feature"
            fid = f"{mod_up}-FM-{slug(nm)}"
            add(fid, "功能矩阵", nm,
                item.get("apis") or [],
                [f"functionality_matrix.{nm}"],
                item.get("description") or f"实现 {nm}",
                deps=[init_id])

    # 4. features (ADC 风格)
    feat_list = p.get("features")
    if isinstance(feat_list, list):
        for item in feat_list:
            if isinstance(item, str):
                nm = item
                refs = []
            elif isinstance(item, dict):
                nm = item.get("name") or item.get("id") or "feature"
                refs = [f"features.{nm}"]
            else:
                continue
            fid = f"{mod_up}-FEAT-{slug(nm)}"
            add(fid, "外设特性", nm, [], refs, f"实现 {nm}", deps=[init_id])

    # 5. interrupts
    intrs = p.get("interrupts")
    if isinstance(intrs, list) and intrs:
        names = [i.get("name") for i in intrs if isinstance(i, dict) and i.get("name")]
        add(f"{mod_up}-IRQ", "中断与事件", "中断处理与回调分发",
            [f"{nm}" for nm in names] + [f"{module.title()}_HandleInterrupt", f"{module.title()}_RegisterCallback"],
            [f"interrupts.{nm}" for nm in names] or ["interrupts"],
            "实现所有 IRQHandler 入口并将事件分发到回调",
            deps=[init_id])

    # 6. dma_channels
    dch = p.get("dma_channels")
    if isinstance(dch, list) and dch:
        add(f"{mod_up}-DMA", "数据通路", "DMA 集成 (TX/RX)",
            [f"{module.title()}_TransferDMA"],
            ["dma_channels"],
            "通过 DMA 完成异步传输并经回调通知完成",
            deps=[init_id])

    # 7. errata
    errata = p.get("errata")
    if isinstance(errata, list):
        for e in errata:
            if not isinstance(e, dict):
                continue
            eid = e.get("id") or e.get("name") or "ERRATA"
            add(f"{mod_up}-ERR-{slug(eid)}", "Errata 规避", f"{eid} workaround",
                [], [f"errata.{eid}"],
                e.get("workaround") or "实现手册建议的 workaround",
                notes=e.get("description", ""))

    # 8. invariants
    invs = p.get("invariants")
    if isinstance(invs, list):
        for inv in invs:
            if not isinstance(inv, dict):
                continue
            iid = inv.get("id") or inv.get("name") or "INV"
            add(f"{mod_up}-GUARD-{slug(iid)}", "硬件保护", f"{iid} 守卫",
                [], [f"invariants.{iid}"],
                inv.get("description") or "在写入受保护字段前注入守卫",
                deps=[init_id])

    return feats


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("ir_path", help="ir/<module>_ir_summary.json")
    ap.add_argument("--force", action="store_true", help="覆盖已存在的 feature_matrix.json")
    ap.add_argument("--dry-run", action="store_true", help="只打印不写盘")
    ap.add_argument("--log", help="额外写入日志文件")
    args = ap.parse_args()

    ir_path = Path(args.ir_path)
    if not ir_path.exists():
        print(f"[ERROR] IR 不存在: {ir_path}", file=sys.stderr)
        return 1
    m = re.match(r"(.+)_ir_summary\.json$", ir_path.name)
    if not m:
        print(f"[ERROR] 文件名应为 <module>_ir_summary.json: {ir_path.name}", file=sys.stderr)
        return 1
    module = m.group(1)
    out_path = ir_path.parent / f"{module}_feature_matrix.json"
    if out_path.exists() and not args.force and not args.dry_run:
        print(f"[ERROR] 已存在 {out_path}，使用 --force 覆盖", file=sys.stderr)
        return 2

    ir = json.loads(ir_path.read_text(encoding="utf-8"))
    features = derive_features(module, ir)

    # 推断 design_doc 路径（与现有约定保持一致）
    drv_dir = REPO_ROOT / "src" / "drivers" / module.title()
    design_doc = f"src/drivers/{module.title()}/{module}_detailed_design.md"

    matrix = {
        "schema_version": "1.0",
        "module": module,
        "ir_source": str(ir_path.relative_to(REPO_ROOT)) if ir_path.is_absolute() else str(ir_path),
        "design_doc": design_doc,
        "features": features,
    }

    out_text = json.dumps(matrix, ensure_ascii=False, indent=2) + "\n"

    msg = f"[BOOTSTRAP] module={module} features={len(features)} -> {out_path}"
    print(msg)
    for f in features:
        print(f"  + {f['feature_id']} [{f['category']}] {f['name']}")

    if args.dry_run:
        print("[DRY-RUN] 未写盘")
    else:
        out_path.write_text(out_text, encoding="utf-8")
        print(f"[BOOTSTRAP] 已写入 {out_path}")

    if args.log:
        Path(args.log).write_text(msg + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
