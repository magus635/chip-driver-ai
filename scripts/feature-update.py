#!/usr/bin/env python3
"""
更新 feature_matrix.json 中单个 feature 的状态，并自动回写 detailed_design.md
中由 <!-- FEATURE_MATRIX:BEGIN/END --> 包裹的渲染区块。

用法：
  python3 scripts/feature-update.py ir/dma_feature_matrix.json --id DMA-IRQ-HTTE --status done
  python3 scripts/feature-update.py ir/dma_feature_matrix.json --render
  python3 scripts/feature-update.py --help

退出码：0 成功 / 1 失败 / 2 用法错误
"""
import argparse
import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
VALID_STATUS = {"todo", "partial", "done"}
STATUS_EMOJI = {"todo": "🔴", "partial": "🟡", "done": "🟢"}
BEGIN_MARK = "<!-- FEATURE_MATRIX:BEGIN -->"
END_MARK = "<!-- FEATURE_MATRIX:END -->"


def render_md_table(matrix) -> str:
    lines = [
        "| 特性ID | 类别 | 功能特性 | 状态 | API | 备注 |",
        "| :--- | :--- | :--- | :--- | :--- | :--- |",
    ]
    for f in matrix["features"]:
        emoji = STATUS_EMOJI[f["status"]]
        apis = ", ".join(f.get("target_apis", [])) or "-"
        notes = (f.get("notes") or "").replace("|", "\\|")
        lines.append(
            f"| `{f['feature_id']}` | {f['category']} | {f['name']} | {emoji} {f['status']} | `{apis}` | {notes} |"
        )
    return "\n".join(lines)


def write_md(matrix):
    md_path = REPO_ROOT / matrix["design_doc"]
    if not md_path.exists():
        print(f"[ERROR] design_doc 不存在: {md_path}", file=sys.stderr)
        return False
    md = md_path.read_text(encoding="utf-8")
    table = render_md_table(matrix)
    block = f"{BEGIN_MARK}\n{table}\n{END_MARK}"
    if BEGIN_MARK in md and END_MARK in md:
        head = md.split(BEGIN_MARK, 1)[0]
        tail = md.split(END_MARK, 1)[1]
        new_md = head + block + tail
    else:
        # 首次注入：追加到文档末尾
        new_md = md.rstrip() + "\n\n## 功能特性矩阵 (Auto-rendered from feature_matrix.json)\n\n" + block + "\n"
    md_path.write_text(new_md, encoding="utf-8")
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", help="ir/<module>_feature_matrix.json")
    ap.add_argument("--id", help="目标 feature_id")
    ap.add_argument("--status", choices=sorted(VALID_STATUS))
    ap.add_argument("--notes", help="同时更新 notes")
    ap.add_argument("--render", action="store_true", help="只重渲染 MD，不修改 JSON")
    ap.add_argument("--dry-run", action="store_true", help="不写盘，仅打印将要的变更")
    ap.add_argument("--log", help="额外写入日志文件")
    args = ap.parse_args()

    p = Path(args.path)
    matrix = json.loads(p.read_text(encoding="utf-8"))

    log_lines = []
    if not args.render:
        if not args.id or not args.status:
            ap.error("非 --render 模式必须同时提供 --id 与 --status")
        target = next((f for f in matrix["features"] if f["feature_id"] == args.id), None)
        if target is None:
            print(f"[ERROR] feature_id 未找到: {args.id}", file=sys.stderr)
            return 1
        old = target["status"]
        target["status"] = args.status
        if args.notes is not None:
            target["notes"] = args.notes
        msg = f"[UPDATE] {args.id}: {old} -> {args.status}"
        print(msg)
        log_lines.append(msg)
        if not args.dry_run:
            p.write_text(json.dumps(matrix, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    if args.dry_run:
        print("[DRY-RUN] 跳过 MD 渲染")
    else:
        if write_md(matrix):
            msg = f"[RENDER] 已更新 {matrix['design_doc']}"
            print(msg)
            log_lines.append(msg)
        else:
            return 1

    if args.log:
        Path(args.log).write_text("\n".join(log_lines) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
