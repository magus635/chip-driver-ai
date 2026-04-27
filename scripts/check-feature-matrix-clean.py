#!/usr/bin/env python3
"""
门禁脚本：feature_matrix.json 中所有 feature 必须 status=done 才允许进入 STEP 3 编译。

用法：
  python3 scripts/check-feature-matrix-clean.py ir/dma_feature_matrix.json
  python3 scripts/check-feature-matrix-clean.py --all
  python3 scripts/check-feature-matrix-clean.py --module dma

退出码：
  0  全部 done，允许进入 STEP 3
  1  仍有 todo/partial，禁止进入 STEP 3
  2  用法/文件错误
"""
import argparse
import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent


def check(path: Path):
    try:
        m = json.loads(path.read_text(encoding="utf-8"))
    except Exception as e:
        print(f"[ERROR] 无法读取 {path}: {e}", file=sys.stderr)
        return 2
    pending = [f for f in m.get("features", []) if f.get("status") != "done"]
    if not pending:
        print(f"✅ {path.name}: 全部 feature 已 done，允许进入 STEP 3 编译")
        return 0
    print(f"❌ {path.name}: 仍有 {len(pending)} 个 feature 未 done，禁止进入 STEP 3", file=sys.stderr)
    for f in pending:
        print(f"  - {f['feature_id']} [{f['status']}] {f['name']}", file=sys.stderr)
    print("", file=sys.stderr)
    print("修复路径（R10）：", file=sys.stderr)
    print("  1. python3 scripts/feature-next.py {} --format prompt".format(path), file=sys.stderr)
    print("  2. 调用 codegen-agent --mode feature --feature-id <ID>", file=sys.stderr)
    print("  3. python3 scripts/feature-update.py {} --id <ID> --status done".format(path), file=sys.stderr)
    print("  4. 重复直至本脚本返回 0", file=sys.stderr)
    return 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", nargs="?", help="ir/<module>_feature_matrix.json")
    ap.add_argument("--module", help="按模块名定位 ir/<module>_feature_matrix.json")
    ap.add_argument("--all", action="store_true", help="校验所有 ir/*_feature_matrix.json，全 done 才返回 0")
    ap.add_argument("--dry-run", action="store_true", help="只读，仅为接口对齐")
    ap.add_argument("--log", help="额外写入日志文件")
    args = ap.parse_args()

    if args.all:
        targets = sorted((REPO_ROOT / "ir").glob("*_feature_matrix.json"))
    elif args.module:
        targets = [REPO_ROOT / "ir" / f"{args.module}_feature_matrix.json"]
    elif args.path:
        targets = [Path(args.path)]
    else:
        ap.error("必须提供 path / --module / --all 之一")

    rc = 0
    for t in targets:
        r = check(t)
        rc = max(rc, r)
    return rc


if __name__ == "__main__":
    sys.exit(main())
