#!/usr/bin/env python3
"""
从 <module>_types.h / <module>_api.h 提取数据结构和 API 表格，注入到
detailed_design.md 的锚点区块（与 R10 FEATURE_MATRIX 同模式）。

锚点：
  <!-- DATA_STRUCTURES:BEGIN -->  ...  <!-- DATA_STRUCTURES:END -->   (§5)
  <!-- API_FUNCTIONS:BEGIN -->    ...  <!-- API_FUNCTIONS:END -->     (§6)

支持解析（启发式正则，非完整 C AST）：
  - typedef enum { name = value, /* comment */ } Name_e;
  - typedef struct { type field; /* comment */ } Name_t;
  - ReturnType FuncName(args);  (含前导 doxygen 风格注释)

用法：
  python3 scripts/extract-design-tables.py --module gpio_afio
  python3 scripts/extract-design-tables.py --all
  python3 scripts/extract-design-tables.py --module gpio_afio --dry-run

退出码：0 注入成功 / 1 失败 / 2 用法错误
"""
import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

DS_BEGIN, DS_END = "<!-- DATA_STRUCTURES:BEGIN -->", "<!-- DATA_STRUCTURES:END -->"
API_BEGIN, API_END = "<!-- API_FUNCTIONS:BEGIN -->", "<!-- API_FUNCTIONS:END -->"


def strip_comments_keep_inline(text: str):
    """移除 /* ... */ 块注释但保留 // 行注释（用于内联说明）。"""
    return re.sub(r"/\*(?:[^*]|\*(?!/))*\*/", "", text, flags=re.DOTALL)


def parse_enums(header_text: str):
    """匹配 typedef enum { ... } Name; 块，返回 [(name, [(member, value, comment)])]"""
    out = []
    pattern = re.compile(
        r"typedef\s+enum\s*\{(?P<body>.*?)\}\s*(?P<name>\w+)\s*;",
        re.DOTALL,
    )
    for m in pattern.finditer(header_text):
        members = []
        for line in m.group("body").split("\n"):
            line = line.strip()
            if not line or line.startswith("//"):
                continue
            # member [= value][,] [/* comment */ | // comment]
            mm = re.match(
                r"(?P<n>\w+)\s*(?:=\s*(?P<v>[^,/]+?))?\s*,?\s*(?:/\*(?P<c>.*?)\*/|//\s*(?P<c2>.+))?\s*$",
                line,
            )
            if mm and mm.group("n"):
                comment = (mm.group("c") or mm.group("c2") or "").strip()
                members.append((mm.group("n"), (mm.group("v") or "").strip(), comment))
        if members:
            out.append((m.group("name"), members))
    return out


def parse_structs(header_text: str):
    """匹配 typedef struct { ... } Name; 块，返回 [(name, [(type, field, comment)])]"""
    out = []
    pattern = re.compile(
        r"typedef\s+struct\s*\{(?P<body>.*?)\}\s*(?P<name>\w+)\s*;",
        re.DOTALL,
    )
    for m in pattern.finditer(header_text):
        fields = []
        for raw in m.group("body").split("\n"):
            # 保留行尾注释用于语义
            inline_comment = ""
            m2 = re.search(r"/\*(.*?)\*/", raw)
            if m2:
                inline_comment = m2.group(1).strip()
                raw = raw[: m2.start()] + raw[m2.end() :]
            m3 = re.search(r"//\s*(.+)$", raw)
            if m3:
                inline_comment = inline_comment or m3.group(1).strip()
                raw = raw[: m3.start()]
            line = raw.strip().rstrip(";").strip()
            if not line:
                continue
            # 期望 "type [const|*]... fieldName[ array ]"
            mm = re.match(r"^(?P<t>.+?)\s+(?P<f>\w+)\s*(?P<arr>\[[^\]]*\])?$", line)
            if mm:
                ftype = mm.group("t").strip()
                fname = mm.group("f")
                if mm.group("arr"):
                    ftype += " " + mm.group("arr")
                fields.append((ftype, fname, inline_comment))
        if fields:
            out.append((m.group("name"), fields))
    return out


def _strip_non_doxygen_block_comments(text: str) -> str:
    """移除 /* ... */ 但保留 /** ... */ doxygen 块。
    避免普通块注释里的字符（如 'until reset')被声明正则吞掉。"""
    def repl(m):
        s = m.group(0)
        return s if s.startswith("/**") else ""
    return re.sub(r"/\*(?:[^*]|\*(?!/))*\*/", repl, text, flags=re.DOTALL)


def parse_apis(header_text: str):
    """匹配函数声明（含其前导 doxygen 注释），返回 [(brief, return_type, name, params)]"""
    out = []
    # 移除非 doxygen 块注释，避免普通注释里的英文短语被解析为函数声明
    text = _strip_non_doxygen_block_comments(header_text)
    # 移除行注释也是必要的（防止 // 后的内容污染下一行匹配）
    text = re.sub(r"//[^\n]*", "", text)
    # 找形如 "type name(...);" 的声明
    decl_re = re.compile(
        r"(?P<doc>/\*\*(?:[^*]|\*(?!/))*\*/\s*)?"
        r"(?P<rt>(?:\w[\w\s\*]*?))\s+(?P<n>\w+)\s*\((?P<args>[^;{]*)\)\s*;",
        re.DOTALL,
    )
    for m in decl_re.finditer(text):
        rt = re.sub(r"\s+", " ", m.group("rt").strip())
        # 过滤宏 / 关键字误匹配
        if rt in ("typedef", "struct", "enum", "if", "while", "for") or not rt:
            continue
        if m.group("n") in ("if", "while", "for", "return", "switch", "sizeof"):
            continue
        brief = ""
        if m.group("doc"):
            doc_lines = m.group("doc").splitlines()
            for line in doc_lines:
                line = line.strip().lstrip("/").lstrip("*").strip()
                if line.startswith("@brief"):
                    brief = line.split("@brief", 1)[1].strip()
                    break
                if line and not brief and not line.startswith("@") and not line.startswith("/"):
                    brief = line
        params = re.sub(r"\s+", " ", m.group("args").strip()) or "void"
        out.append((brief, rt, m.group("n"), params))
    return out


def render_data_structures(enums, structs) -> str:
    lines = ["### Enums", ""]
    if not enums:
        lines.append("_无_")
    for ename, members in enums:
        lines.append(f"#### `{ename}`")
        lines.append("")
        lines.append("| 成员 | 值 | 说明 |")
        lines.append("|---|---|---|")
        for n, v, c in members:
            v_disp = v if v else "—"
            lines.append(f"| `{n}` | {v_disp} | {c or '—'} |")
        lines.append("")
    lines.append("### Structs")
    lines.append("")
    if not structs:
        lines.append("_无_")
    for sname, fields in structs:
        lines.append(f"#### `{sname}`")
        lines.append("")
        lines.append("| 字段 | 类型 | 说明 |")
        lines.append("|---|---|---|")
        for t, f, c in fields:
            lines.append(f"| `{f}` | `{t}` | {c or '—'} |")
        lines.append("")
    return "\n".join(lines).rstrip()


def render_apis(apis) -> str:
    if not apis:
        return "_无 public API_"
    lines = [
        "| 函数 | 返回 | 参数 | 简介 |",
        "|---|---|---|---|",
    ]
    for brief, rt, n, params in apis:
        b = (brief or "—").replace("|", "\\|")
        params_disp = params.replace("|", "\\|")
        lines.append(f"| `{n}` | `{rt}` | `{params_disp}` | {b} |")
    return "\n".join(lines)


def inject_block(md: str, begin: str, end: str, content: str) -> str:
    block = f"{begin}\n{content}\n{end}"
    if begin in md and end in md:
        head = md.split(begin, 1)[0]
        tail = md.split(end, 1)[1]
        return head + block + tail
    # 否则追加到文末
    return md.rstrip() + "\n\n" + block + "\n"


def process_module(module: str, dry_run: bool) -> int:
    camel = "".join(p.capitalize() for p in module.split("_"))
    drv_dir = REPO_ROOT / "src" / "drivers" / camel
    # 头文件命名优先 <module>_types.h；若不存在尝试无下划线变体（与 check-arch #9 一致）
    candidates_t = [drv_dir / "include" / f"{module}_types.h",
                    drv_dir / "include" / f"{module.replace('_', '')}_types.h"]
    candidates_a = [drv_dir / "include" / f"{module}_api.h",
                    drv_dir / "include" / f"{module.replace('_', '')}_api.h"]
    types_h = next((p for p in candidates_t if p.exists()), candidates_t[0])
    api_h   = next((p for p in candidates_a if p.exists()), candidates_a[0])
    design_md = drv_dir / f"{module}_detailed_design.md"

    for p in (types_h, api_h, design_md):
        if not p.exists():
            print(f"[ERROR] missing: {p}", file=sys.stderr)
            return 1

    types_text = types_h.read_text(encoding="utf-8")
    api_text = api_h.read_text(encoding="utf-8")

    # 直接喂原文：parse_enums / parse_structs 已逐行处理 /* ... */ 行尾注释
    enums = parse_enums(types_text)
    structs = parse_structs(types_text)
    apis = parse_apis(api_text)

    ds_block = render_data_structures(enums, structs)
    api_block = render_apis(apis)

    md = design_md.read_text(encoding="utf-8")
    new_md = inject_block(md, DS_BEGIN, DS_END, ds_block)
    new_md = inject_block(new_md, API_BEGIN, API_END, api_block)

    summary = (
        f"[EXTRACT] {module}: enums={len(enums)} structs={len(structs)} apis={len(apis)} "
        f"-> {design_md.relative_to(REPO_ROOT)}"
    )
    print(summary)

    if dry_run:
        print("[DRY-RUN] 未写盘")
    else:
        design_md.write_text(new_md, encoding="utf-8")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--module", help="module name (e.g. gpio_afio)")
    ap.add_argument("--all", action="store_true", help="所有 src/drivers/* 模块")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--log", help="额外写入日志文件")
    args = ap.parse_args()

    targets = []
    if args.all:
        # 以 ir/*_ir_summary.json 为真值源，避免 camel↔snake 反推歧义
        for ir_file in sorted((REPO_ROOT / "ir").glob("*_ir_summary.json")):
            mod = ir_file.stem.replace("_ir_summary", "")
            # 仅包含已有 detailed_design.md 的模块（其余模块尚未生成设计文档）
            camel = "".join(p.capitalize() for p in mod.split("_"))
            if (REPO_ROOT / "src" / "drivers" / camel / f"{mod}_detailed_design.md").exists():
                targets.append(mod)
    elif args.module:
        targets = [args.module]
    else:
        ap.error("必须 --module 或 --all")

    rc = 0
    for m in targets:
        r = process_module(m, args.dry_run)
        rc = max(rc, r)
    return rc


if __name__ == "__main__":
    sys.exit(main())
