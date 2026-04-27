#!/usr/bin/env python3
"""
按 depends_on 拓扑排序，输出下一个待实现的 feature（status != done）。

用法：
  python3 scripts/feature-next.py ir/dma_feature_matrix.json
  python3 scripts/feature-next.py ir/dma_feature_matrix.json --count 2
  python3 scripts/feature-next.py ir/dma_feature_matrix.json --format prompt

退出码：0 找到 / 1 已全部完成 / 2 用法错误 / 3 依赖死锁（环或全部依赖未完成）
"""
import argparse
import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent


def load(p):
    with open(p, "r", encoding="utf-8") as f:
        return json.load(f)


def topo_pending(matrix):
    feats = {f["feature_id"]: f for f in matrix["features"]}
    pending = []
    for fid, f in feats.items():
        if f["status"] == "done":
            continue
        unmet = [d for d in f.get("depends_on", []) if feats.get(d, {}).get("status") != "done"]
        if not unmet:
            pending.append(f)
    pending.sort(key=lambda x: (0 if x["status"] == "partial" else 1, x["feature_id"]))
    return pending


def feature_to_prompt(feature, matrix):
    ir_path = matrix.get("ir_source", "")
    apis = ", ".join(feature.get("target_apis", [])) or "(待 code-gen 决定)"
    refs = "\n".join(f"  - {r}" for r in feature.get("ir_refs", [])) or "  - (无)"
    deps = ", ".join(feature.get("depends_on", [])) or "(无)"
    return f"""[FEATURE-LOOP] 待实现 feature
====================================================================
模块         : {matrix['module']}
Feature ID   : {feature['feature_id']}
类别         : {feature['category']}
名称         : {feature['name']}
当前状态     : {feature['status']}
依赖         : {deps}
目标 API     : {apis}
验收标准     : {feature['acceptance']}
IR 引用      :
{refs}
权威 IR JSON : {ir_path}
====================================================================
请 codegen-agent 仅针对此 feature 在框架内补齐实现：
  1. 读取 {ir_path} 中的 ir_refs 字段切片，禁止读其他无关字段
  2. 仅添加 target_apis 列表中的接口，禁止顺手重构其他模块
  3. 完成后运行 scripts/feature-update.py 标记为 done
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", help="ir/<module>_feature_matrix.json")
    ap.add_argument("--count", type=int, default=1, help="一次输出 N 个候选 (默认 1)")
    ap.add_argument("--format", choices=["json", "prompt", "id"], default="json")
    ap.add_argument("--dry-run", action="store_true", help="只读，仅为接口对齐")
    ap.add_argument("--log", help="额外写入日志文件")
    args = ap.parse_args()

    matrix = load(args.path)
    pending = topo_pending(matrix)

    # 是否还有 todo/partial 但被依赖卡住
    unfinished = [f for f in matrix["features"] if f["status"] != "done"]
    if not unfinished:
        print("[FEATURE-LOOP] 所有 feature 已完成 ✅", file=sys.stderr)
        return 1
    if not pending:
        print("[FEATURE-LOOP] 检测到死锁：仍有未完成 feature，但全部 depends_on 未满足。", file=sys.stderr)
        for f in unfinished:
            print(f"  - {f['feature_id']} 依赖 {f.get('depends_on')}", file=sys.stderr)
        return 3

    chosen = pending[: args.count]
    out = []
    if args.format == "json":
        print(json.dumps(chosen, ensure_ascii=False, indent=2))
    elif args.format == "id":
        for f in chosen:
            line = f["feature_id"]
            print(line)
            out.append(line)
    else:
        for f in chosen:
            line = feature_to_prompt(f, matrix)
            print(line)
            out.append(line)

    if args.log:
        Path(args.log).write_text("\n".join(out) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
