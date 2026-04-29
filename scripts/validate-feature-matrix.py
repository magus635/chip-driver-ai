#!/usr/bin/env python3
"""
Feature Matrix 校验脚本（R10 配套工具）

校验内容：
  1. JSON schema 完整性（必填字段、状态枚举、ID 唯一性）
  2. depends_on 图无环且引用合法
  3. ir_refs 在对应 ir/<module>_ir_summary.json 中可解析（点路径，存在性检查）
  4. MD 同步：<module>_detailed_design.md 中的功能矩阵表与 JSON 一致

退出码：0 通过 / 1 失败 / 2 用法错误

用法：
  python3 scripts/validate-feature-matrix.py ir/dma_feature_matrix.json
  python3 scripts/validate-feature-matrix.py --all
  python3 scripts/validate-feature-matrix.py --help
  python3 scripts/validate-feature-matrix.py ir/dma_feature_matrix.json --dry-run --log /tmp/v.log
"""
import argparse
import json
import re
import sys
from pathlib import Path

VALID_STATUS = {"todo", "partial", "done"}
STATUS_EMOJI = {"todo": "🔴", "partial": "🟡", "done": "🟢"}
REQUIRED_FIELDS = [
    "feature_id", "category", "name", "status",
    "target_apis", "depends_on", "ir_refs", "acceptance",
]
REPO_ROOT = Path(__file__).resolve().parent.parent


def load_json(path: Path):
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


SUPPORTED_SCHEMA_VERSIONS = ("1.0", "1.1")  # 1.1 加入可选 swr_refs


def validate_schema(matrix, errors):
    sv = matrix.get("schema_version")
    if sv not in SUPPORTED_SCHEMA_VERSIONS:
        errors.append(f"schema_version 必须 ∈ {SUPPORTED_SCHEMA_VERSIONS}，实际: {sv!r}")
    if not matrix.get("module"):
        errors.append("缺少 module 字段")
    feats = matrix.get("features")
    if not isinstance(feats, list) or not feats:
        errors.append("features 必须为非空数组")
        return
    seen = set()
    for i, f in enumerate(feats):
        prefix = f"features[{i}] ({f.get('feature_id', '?')})"
        for key in REQUIRED_FIELDS:
            if key not in f:
                errors.append(f"{prefix}: 缺少字段 {key}")
        if f.get("status") not in VALID_STATUS:
            errors.append(f"{prefix}: status 非法，必须是 {sorted(VALID_STATUS)}")
        for k in ("target_apis", "depends_on", "ir_refs"):
            if k in f and not isinstance(f[k], list):
                errors.append(f"{prefix}: {k} 必须为数组")
        fid = f.get("feature_id")
        if fid in seen:
            errors.append(f"{prefix}: feature_id 重复")
        seen.add(fid)


def validate_deps(matrix, errors):
    feats = matrix.get("features", [])
    ids = {f["feature_id"] for f in feats if "feature_id" in f}
    for f in feats:
        for dep in f.get("depends_on", []):
            if dep not in ids:
                errors.append(f"{f['feature_id']}: depends_on 引用未知 feature_id {dep!r}")
    # 拓扑环检测
    graph = {f["feature_id"]: list(f.get("depends_on", [])) for f in feats}
    WHITE, GRAY, BLACK = 0, 1, 2
    color = {k: WHITE for k in graph}

    def dfs(n, stack):
        if color[n] == GRAY:
            errors.append(f"depends_on 出现环: {' -> '.join(stack + [n])}")
            return
        if color[n] == BLACK:
            return
        color[n] = GRAY
        for m in graph.get(n, []):
            if m in graph:
                dfs(m, stack + [n])
        color[n] = BLACK

    for n in list(graph):
        if color[n] == WHITE:
            dfs(n, [])


def validate_ir_refs(matrix, errors):
    ir_path = REPO_ROOT / matrix.get("ir_source", "")
    if not ir_path.exists():
        errors.append(f"ir_source 不存在: {ir_path}")
        return
    ir = load_json(ir_path)

    def find_token(node, token, max_depth=4):
        """BFS 搜索：dict key、dict.name、list 中 name==token 的项。"""
        from collections import deque
        q = deque([(node, 0)])
        while q:
            n, d = q.popleft()
            if isinstance(n, dict):
                if token in n:
                    return n[token]
                if n.get("name") == token or n.get("id") == token:
                    return n
                if d < max_depth:
                    for v in n.values():
                        if isinstance(v, (dict, list)):
                            q.append((v, d + 1))
            elif isinstance(n, list):
                for it in n:
                    if isinstance(it, dict) and (it.get("name") == token or it.get("id") == token):
                        return it
                    if d < max_depth and isinstance(it, (dict, list)):
                        q.append((it, d + 1))
        return None

    def resolve(obj, parts):
        cur = obj
        for p in parts:
            cur = find_token(cur, p)
            if cur is None:
                return None
        return cur

    peripheral = ir.get("peripheral", ir)
    for f in matrix.get("features", []):
        for ref in f.get("ir_refs", []):
            parts = ref.split(".")
            if resolve(peripheral, parts) is None and resolve(ir, parts) is None:
                errors.append(f"{f['feature_id']}: ir_refs {ref!r} 在 {ir_path.name} 中无法解析")


# MD 表行格式: | 类别 | 名称 | 状态 emoji + (status) | 备注 |
TABLE_HEADER_RE = re.compile(r"^\|\s*特性分类\s*\|", re.MULTILINE)


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


BEGIN_MARK = "<!-- FEATURE_MATRIX:BEGIN -->"
END_MARK = "<!-- FEATURE_MATRIX:END -->"


def validate_md_sync(matrix, errors):
    md_path = REPO_ROOT / matrix.get("design_doc", "")
    if not md_path.exists():
        errors.append(f"design_doc 不存在: {md_path}（可用 feature-update.py --render 生成）")
        return
    md = md_path.read_text(encoding="utf-8")
    if BEGIN_MARK not in md or END_MARK not in md:
        errors.append(
            f"{md_path.name}: 缺少 {BEGIN_MARK} / {END_MARK} 标记块。"
            f"请在功能矩阵章节包裹 JSON 渲染区块（feature-update.py --render 会写入）"
        )
        return
    inside = md.split(BEGIN_MARK, 1)[1].split(END_MARK, 1)[0].strip()
    expected = render_md_table(matrix).strip()
    if inside != expected:
        errors.append(
            f"{md_path.name}: 功能矩阵区块与 JSON 不同步。"
            f"请运行 scripts/feature-update.py --render {matrix['module']} 重新渲染"
        )


_SWR_ANCHOR_RE = re.compile(r"<!--\s*(SWR-[A-Z0-9_-]+)\s*-->")


def extract_swr_anchors(srs_path: Path):
    """读 SRS markdown，抽出所有 <!-- SWR-XXX-NNN --> 锚点"""
    if not srs_path.exists():
        return None
    text = srs_path.read_text(encoding="utf-8")
    return set(_SWR_ANCHOR_RE.findall(text))


def validate_swr(matrix, errors, check_swr: bool):
    """R11 · 校验 feature.swr_refs 引用全部存在于 docs/srs/<module>_srs.md。
    - 任何 feature 含非空 swr_refs → 隐式启用检查
    - 显式 --check-swr → 即使全空也强制检查存在 SRS 文件"""
    feats = matrix.get("features", [])
    has_any_swr = any(f.get("swr_refs") for f in feats)
    if not (check_swr or has_any_swr):
        return
    module = matrix.get("module", "")
    srs_path = REPO_ROOT / "docs" / "srs" / f"{module}_srs.md"
    anchors = extract_swr_anchors(srs_path)
    if anchors is None:
        if check_swr or has_any_swr:
            errors.append(
                f"R11 · SRS 缺失: {srs_path.relative_to(REPO_ROOT)} 不存在；"
                f"feature 含 swr_refs 时必须提供 SRS"
            )
        return
    for f in feats:
        for swr in f.get("swr_refs", []) or []:
            if swr not in anchors:
                errors.append(
                    f"R11 · {f['feature_id']}: swr_refs {swr!r} 在 {srs_path.name} 中找不到锚点"
                )


def run_one(path: Path, check_swr: bool = False):
    errors = []
    try:
        matrix = load_json(path)
    except Exception as e:
        return [f"无法加载 {path}: {e}"]
    validate_schema(matrix, errors)
    if errors:
        return errors
    validate_deps(matrix, errors)
    validate_ir_refs(matrix, errors)
    validate_md_sync(matrix, errors)
    validate_swr(matrix, errors, check_swr)
    return errors


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", nargs="?", help="ir/<module>_feature_matrix.json")
    ap.add_argument("--all", action="store_true", help="校验所有 ir/*_feature_matrix.json")
    ap.add_argument("--check-swr", action="store_true",
                    help="R11 · 强制校验 swr_refs 引用与 docs/srs/<module>_srs.md 锚点一致")
    ap.add_argument("--dry-run", action="store_true", help="不做任何写操作（本脚本只读，仅为接口对齐）")
    ap.add_argument("--log", help="将输出额外写入指定日志文件")
    args = ap.parse_args()

    if not args.path and not args.all:
        ap.error("必须提供路径参数或 --all")

    targets = []
    if args.all:
        targets = sorted((REPO_ROOT / "ir").glob("*_feature_matrix.json"))
    else:
        targets = [Path(args.path)]

    if not targets:
        print("[INFO] 未找到任何 feature_matrix.json")
        return 0

    log_lines = []
    failed = 0
    for p in targets:
        errs = run_one(p, check_swr=args.check_swr)
        if errs:
            failed += 1
            msg = f"❌ {p}\n  - " + "\n  - ".join(errs)
        else:
            msg = f"✅ {p}"
        print(msg)
        log_lines.append(msg)

    if args.log:
        Path(args.log).write_text("\n".join(log_lines) + "\n", encoding="utf-8")

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
