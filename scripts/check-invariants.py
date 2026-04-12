#!/usr/bin/env python3
"""
check-invariants.py — IR hardware invariant static checker.

Loads invariants from a peripheral IR JSON (ir/<module>_ir.json) and scans
target C files for violations. Used by reviewer-agent before compilation.

Scope (MVP):
  - LOCK invariants:  "<guard> implies !writable(REG.FIELD) [&& ...]"
    Verifies any write to REG.FIELD is preceded by a guard that clears or
    checks the precondition within the same function.
  - CONSISTENCY invariants:  "A==v && B==v implies C==v"
    Verifies that any assignment expression setting A and B also sets C.
  - Anything else is reported as ADVISORY (not failed).

Limitations:
  - No full C AST. Uses function-level text windows + regex.
  - Only direct register access patterns: `REG->FIELD [|&^]?=` and `FIELD_Msk`
    references on RHS. LL function call sites are NOT transitively expanded.
  - Scope analysis is intra-procedural, textual.
"""
from __future__ import annotations
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

# ---------------------------------------------------------------- parsing ---

IMPLIES_RE = re.compile(r"^\s*(.+?)\s+implies\s+(.+?)\s*$")
LOCK_RE = re.compile(r"!\s*writable\(\s*(\w+)\.(\w+)\s*\)")
CMP_RE = re.compile(r"(\w+)\.(\w+)\s*(==|!=|<=|>=|<|>)\s*(\w+)")


@dataclass
class LockInvariant:
    id: str
    guard_expr: str           # e.g. "CR1.SPE==1"
    guard_reg: str            # e.g. "CR1"
    guard_field: str          # e.g. "SPE"
    guard_value: str          # e.g. "1"
    locked_fields: list[tuple[str, str]]   # [(REG, FIELD), ...]
    source: str


@dataclass
class ConsistencyInvariant:
    id: str
    preconds: list[tuple[str, str, str]]   # [(REG, FIELD, value), ...]
    required: tuple[str, str, str]         # (REG, FIELD, value)
    source: str


@dataclass
class Violation:
    invariant_id: str
    file: str
    line: int
    function: str
    kind: str       # LOCK_UNGUARDED | CONSISTENCY_MISSING | ADVISORY
    message: str


def classify_invariant(inv: dict):
    expr = inv.get("expr", "")
    m = IMPLIES_RE.match(expr)
    if not m:
        return ("ADVISORY", None)
    lhs, rhs = m.group(1).strip(), m.group(2).strip()

    # LOCK pattern
    locks = LOCK_RE.findall(rhs)
    if locks and "writable" in rhs and "!" in rhs:
        g = CMP_RE.search(lhs)
        if g:
            return (
                "LOCK",
                LockInvariant(
                    id=inv["id"],
                    guard_expr=lhs,
                    guard_reg=g.group(1),
                    guard_field=g.group(2),
                    guard_value=g.group(4),
                    locked_fields=[(r, f) for r, f in locks],
                    source=inv.get("source", ""),
                ),
            )
    # CONSISTENCY pattern
    if "implies" in expr and LOCK_RE.search(rhs) is None:
        pre = CMP_RE.findall(lhs)
        req = CMP_RE.search(rhs)
        if pre and req and all(op == "==" for _, _, op, _ in pre):
            return (
                "CONSISTENCY",
                ConsistencyInvariant(
                    id=inv["id"],
                    preconds=[(r, f, v) for r, f, op, v in pre if op == "=="],
                    required=(req.group(1), req.group(2), req.group(4)),
                    source=inv.get("source", ""),
                ),
            )
    return ("ADVISORY", None)


# ----------------------------------------------------------- source scan ---

FUNC_RE = re.compile(r"^[A-Za-z_][\w\s\*]*\b(\w+)\s*\([^;{]*\)\s*\{", re.MULTILINE)
WRITE_RE = re.compile(r"(\w+)\s*->\s*(\w+)\s*(=|\|=|&=|\^=)")


def split_functions(src: str) -> list[tuple[str, int, int, str]]:
    """Return [(func_name, start_line, end_line, body)]. Brace-counting."""
    out = []
    for m in FUNC_RE.finditer(src):
        name = m.group(1)
        start = m.end() - 1  # position of '{'
        depth = 0
        i = start
        while i < len(src):
            c = src[i]
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    body = src[start : i + 1]
                    start_line = src.count("\n", 0, m.start()) + 1
                    end_line = src.count("\n", 0, i) + 1
                    out.append((name, start_line, end_line, body))
                    break
            i += 1
    return out


SELF_ASSIGN_RE = re.compile(r"(\w+)\s*->\s*(\w+)\s*=\s*\1\s*->\s*\2\s*;")


def find_field_writes(body: str, reg: str, field: str) -> list[tuple[int, str]]:
    """Find writes in `body` that touch REG.FIELD. Returns [(rel_line, line_text)]."""
    hits: list[tuple[int, str]] = []
    field_msk = f"{reg}_{field}_Msk"
    for idx, line in enumerate(body.splitlines(), 1):
        # Self-assignment `X->REG = X->REG` is a deliberate no-op write
        # (e.g. RM-prescribed MODF clear). Bit values unchanged → skip.
        if SELF_ASSIGN_RE.search(line):
            continue
        wm = WRITE_RE.search(line)
        if not wm:
            continue
        _, lhs_reg, op = wm.group(1), wm.group(2), wm.group(3)
        if lhs_reg != reg:
            continue
        # Direct assign `REG = X` touches ALL fields of REG.
        # RMW `REG |= X` / `&= X` only touches fields whose mask appears on RHS.
        if op == "=":
            hits.append((idx, line.strip()))
        elif field_msk in line:
            hits.append((idx, line.strip()))
    return hits


def has_lock_guard(body: str, write_lineno: int, lock: LockInvariant) -> bool:
    """Does body, before `write_lineno`, break the guard precondition?

    Guard `REG.FIELD==V` is "broken" (safe to write locked fields) if earlier
    in the same function we see either:
      - explicit clear:  `REG &= ~..._FIELD_Msk`      (when V==1)
      - explicit set:    `REG |= ..._FIELD_Msk`       (when V==0)
      - conditional bail:`if (... _FIELD_Msk ...) return/goto ...`

    Uses per-line substring matching on the token `REG_FIELD_Msk` to tolerate
    peripheral-prefixed macros (e.g. `SPI_CR1_SPE_Msk`). Textual heuristic,
    not dataflow analysis.
    """
    msk_token = f"{lock.guard_reg}_{lock.guard_field}_Msk"
    reg = lock.guard_reg
    clear_re = re.compile(rf"\b{reg}\s*&=\s*~")
    set_re = re.compile(rf"\b{reg}\s*\|=")
    bail_re = re.compile(r"\b(return|goto)\b")
    for line in body.splitlines()[: write_lineno - 1]:
        if msk_token not in line:
            continue
        if lock.guard_value == "1":
            if clear_re.search(line):
                return True
            if "if" in line and bail_re.search(line):
                return True
        else:
            if set_re.search(line):
                return True
    return False


def check_lock(
    path: Path, funcs: list, inv: LockInvariant
) -> Iterable[Violation]:
    for name, start_line, _end, body in funcs:
        for reg, field in inv.locked_fields:
            for rel_line, text in find_field_writes(body, reg, field):
                if not has_lock_guard(body, rel_line, inv):
                    yield Violation(
                        invariant_id=inv.id,
                        file=str(path),
                        line=start_line + rel_line - 1,
                        function=name,
                        kind="LOCK_UNGUARDED",
                        message=(
                            f"write to {reg}.{field} without clearing/checking "
                            f"guard `{inv.guard_expr}` — {text!r}"
                        ),
                    )


def check_consistency(
    path: Path, funcs: list, inv: ConsistencyInvariant
) -> Iterable[Violation]:
    """For `A && B implies C`: any statement setting A and B must also set C."""
    pre_msks = [f"{r}_{f}_Msk" for r, f, _ in inv.preconds]
    req_reg, req_field, _ = inv.required
    req_msk = f"{req_reg}_{req_field}_Msk"
    # Look for assignments/RMWs whose RHS contains ALL precond masks but not required.
    for name, start_line, _end, body in funcs:
        for rel_idx, line in enumerate(body.splitlines(), 1):
            if all(p in line for p in pre_msks) and req_msk not in line:
                # Could still be set by a later statement in same function; do a
                # cheap check: is req_msk referenced anywhere in the function?
                if req_msk in body:
                    continue
                yield Violation(
                    invariant_id=inv.id,
                    file=str(path),
                    line=start_line + rel_idx - 1,
                    function=name,
                    kind="CONSISTENCY_MISSING",
                    message=(
                        f"sets {pre_msks} but never sets {req_msk} "
                        f"(invariant requires {req_reg}.{req_field})"
                    ),
                )


# ------------------------------------------------------------------- main ---


def load_invariants(ir_path: Path) -> list[dict]:
    data = json.loads(ir_path.read_text())
    fm = data.get("peripheral", {}).get("functional_model", {})
    out = []
    for inv in fm.get("invariants", []):
        if "reviewer-agent:static" in inv.get("enforced_by", []):
            out.append(inv)
    return out


def run(ir_path: Path, c_paths: list[Path]) -> int:
    invs = load_invariants(ir_path)
    if not invs:
        print(f"[check-invariants] no static invariants in {ir_path}")
        return 0

    lock_invs: list[LockInvariant] = []
    cons_invs: list[ConsistencyInvariant] = []
    advisory: list[str] = []
    for inv in invs:
        kind, parsed = classify_invariant(inv)
        if kind == "LOCK":
            lock_invs.append(parsed)
        elif kind == "CONSISTENCY":
            cons_invs.append(parsed)
        else:
            advisory.append(inv["id"])

    violations: list[Violation] = []
    for cp in c_paths:
        src = cp.read_text()
        funcs = split_functions(src)
        for li in lock_invs:
            violations.extend(check_lock(cp, funcs, li))
        for ci in cons_invs:
            violations.extend(check_consistency(cp, funcs, ci))

    print(f"[check-invariants] ir={ir_path}")
    print(
        f"[check-invariants] parsed: {len(lock_invs)} LOCK, "
        f"{len(cons_invs)} CONSISTENCY, {len(advisory)} ADVISORY "
        f"({','.join(advisory) or '-'})"
    )
    print(f"[check-invariants] scanned {len(c_paths)} file(s)")

    if not violations:
        print("[check-invariants] ✓ no violations")
        return 0

    # Dedupe: group by (file, line, invariant_id, kind). Multi-field LOCK
    # violations collapse to one report listing all affected fields.
    grouped: dict[tuple, list[Violation]] = {}
    for v in violations:
        grouped.setdefault(
            (v.file, v.line, v.invariant_id, v.kind, v.function), []
        ).append(v)
    print(f"[check-invariants] ✗ {len(grouped)} violation site(s):")
    for (file, line, inv_id, kind, func), vs in sorted(grouped.items()):
        if len(vs) == 1:
            print(
                f"  [{kind}] {inv_id} @ {file}:{line} in {func}()\n"
                f"    {vs[0].message}"
            )
        else:
            fields = sorted({v.message.split()[2] for v in vs})
            print(
                f"  [{kind}] {inv_id} @ {file}:{line} in {func}()\n"
                f"    unguarded write touching {len(fields)} locked fields: "
                f"{', '.join(fields)}"
            )
    return 1


def main() -> int:
    if len(sys.argv) < 3:
        print(
            "usage: check-invariants.py <ir_json> <c_file> [<c_file>...]",
            file=sys.stderr,
        )
        return 2
    ir_path = Path(sys.argv[1])
    c_paths = [Path(p) for p in sys.argv[2:]]
    for p in [ir_path, *c_paths]:
        if not p.exists():
            print(f"error: {p} not found", file=sys.stderr)
            return 2
    return run(ir_path, c_paths)


if __name__ == "__main__":
    sys.exit(main())
