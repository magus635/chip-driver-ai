#!/usr/bin/env python3
"""
详细设计文档章节完整性校验（R8#13 配套门禁）。

校验对象：src/drivers/<Module>/<module>_detailed_design.md
校验规则：必含 9 个强制章节（H2 级别）+ FEATURE_MATRIX 渲染区块。
未通过 → STEP 2.D 拒绝放行。

用法：
  python3 scripts/check-detailed-design.py src/drivers/GpioAfio/gpio_afio_detailed_design.md
  python3 scripts/check-detailed-design.py --all
  python3 scripts/check-detailed-design.py --module gpio_afio

退出码：0 通过 / 1 缺章 / 2 文件错误
"""
import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

# 章节关键词（H2 标题包含这些子串即认作匹配）
REQUIRED_SECTIONS = [
    ("架构目标", "1"),
    ("文件清单", "2"),
    ("功能特性矩阵", "3"),
    ("异步闭环", "4"),
    ("数据结构", "5"),
    ("接口函数", "6"),
    ("流程图", "7"),
    ("调用示例", "8"),
    ("不变式审计", "9"),
]
MATRIX_BEGIN = "<!-- FEATURE_MATRIX:BEGIN -->"
MATRIX_END = "<!-- FEATURE_MATRIX:END -->"
H2_RE = re.compile(r"^##\s+(.+?)\s*$", re.MULTILINE)
ANTI_PATTERNS = [
    (r"##\s+\d?\.?\s*接口函数.*\n+.*见代码", "§6 写'见代码'，必须展开"),
    (r"##\s+\d?\.?\s*调用示例.*\n+.*略过|##\s+\d?\.?\s*调用示例.*\n+.*\(略\)", "§8 写'略'"),
]


def check_file(path: Path):
    if not path.exists():
        return [f"文件不存在: {path}"]
    md = path.read_text(encoding="utf-8")
    errors = []

    h2s = H2_RE.findall(md)
    for keyword, num in REQUIRED_SECTIONS:
        if not any(keyword in h for h in h2s):
            errors.append(f"§{num} 缺失：未找到 H2 标题包含 '{keyword}' 的章节")

    if MATRIX_BEGIN not in md or MATRIX_END not in md:
        errors.append(f"§3 缺少 {MATRIX_BEGIN} / {MATRIX_END} 渲染区块（运行 feature-update.py --render 注入）")

    for pat, msg in ANTI_PATTERNS:
        if re.search(pat, md):
            errors.append(f"反模式：{msg}")

    return errors


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("path", nargs="?", help="<module>_detailed_design.md 路径")
    ap.add_argument("--module", help="按模块名定位（自动 camelize）")
    ap.add_argument("--all", action="store_true", help="校验 src/drivers/*/*_detailed_design.md 全部")
    ap.add_argument("--dry-run", action="store_true", help="只读，仅为接口对齐")
    ap.add_argument("--log", help="额外写入日志文件")
    args = ap.parse_args()

    if args.all:
        targets = sorted((REPO_ROOT / "src" / "drivers").glob("*/*_detailed_design.md"))
    elif args.module:
        m = args.module
        camel = "".join(p.capitalize() for p in m.split("_"))
        targets = [REPO_ROOT / "src" / "drivers" / camel / f"{m}_detailed_design.md"]
    elif args.path:
        targets = [Path(args.path)]
    else:
        ap.error("必须提供 path / --module / --all 之一")

    if not targets:
        print("[INFO] 未找到任何 detailed_design.md")
        return 0

    rc = 0
    log_lines = []
    for t in targets:
        errs = check_file(t)
        if errs:
            rc = 1
            msg = f"❌ {t.relative_to(REPO_ROOT) if t.is_relative_to(REPO_ROOT) else t}\n  - " + "\n  - ".join(errs)
        else:
            msg = f"✅ {t.relative_to(REPO_ROOT) if t.is_relative_to(REPO_ROOT) else t}"
        print(msg)
        log_lines.append(msg)

    if args.log:
        Path(args.log).write_text("\n".join(log_lines) + "\n", encoding="utf-8")
    return rc


if __name__ == "__main__":
    sys.exit(main())
