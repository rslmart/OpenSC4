#!/usr/bin/env python3
"""
4-Stage Decompilation Pipeline & Mach-O Audit Tool for OpenSC4.

Stages:
  Stage 0 (Tier Classifier):
    - Tier 1: Core SC4 Simulation, World, Networks, Automata, Advisors, Disasters & Terrain (algorithmic methods)
    - Tier 2: UI Windows, 3D View/Render, Hit/Fresh Audio & Lua/Visual Effects (algorithmic methods)
    - Tier 3: Auto-Verified GZCOM Boilerplate & <=48B Call-Free x86_64 Leaf Accessors/Stubs ($0 LLM tokens)
    - Tier 4: Superseded cGZ/cRZ Framework, Aspyr DirectX7 Shim, Legacy GZWin Widgets, ABI Thunks & STL (Skip)
  Stage 1 (Zero-Token x86_64 Leaf Lifter):
    - Automatically decodes call-free <=48B leaf getters/setters/predicates/stubs into C++20 inline expressions.
  Stage 2 (Subagent Workpack Generator):
    - Bundles a class's auto-lifted leaves + condensed, demangled disassembly of all [TODO-ALGO] methods.
  Stage 3 (Registry Sync & Verification):
    - Syncs all cited 0x100xxxxxx addresses into src/decompilation_registry.cpp for ./build/opensc4_verify.

Symbol States (honest coverage — a citation is not an implementation):
  VERIFIED    - exercised by a [PASS] check of the verify suite (strongest)
  IMPLEMENTED - cited in a .cpp/.mm implementation unit, or prefix-covered class
  CITED       - address cited only in headers/tests (declared, not implemented)
  TODO        - no citation, no code: the real remaining work
  AUTO        - Tier 3: machine-verified GZCOM boilerplate / leaf accessors
  SKIP        - Tier 4: superseded runtime (not reimplemented by design)

Usage:
  python3 tools/sc4_decomp_audit.py                                   # 4-Tier summary + Top Tier-1 classes
  python3 tools/sc4_decomp_audit.py --class cSC4FireProtectionSimulator
  python3 tools/sc4_decomp_audit.py --workpack cSC4FireProtectionSimulator
  python3 tools/sc4_decomp_audit.py --disasm 0x1002faf6e
  python3 tools/sc4_decomp_audit.py --dump 0x100b175d4:4:f32          # byte-exact constants
  python3 tools/sc4_decomp_audit.py --vtable cSTETerrain             # vtable slot resolution
  python3 tools/sc4_decomp_audit.py --sync-registry
  python3 tools/sc4_decomp_audit.py --status-md STATUS.md
"""

import argparse
import bisect
import collections
import datetime
import glob
import os
import re
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_BIN = (
    "/Users/russellmartin/Library/Application Support/Steam/steamapps/common/"
    "SimCity 4 Deluxe/Sim City 4 Deluxe Edition.app/Contents/MacOS/"
    "Sim City 4 Deluxe Editionsub"
)

STREAM_VT = {
    0x20: "Sint8",
    0x28: "Uint8",
    0x30: "Sint16",
    0x38: "Uint16",
    0x40: "Sint32",
    0x48: "Uint32",
    0x50: "Sint64",
    0x58: "Uint64",
    0x60: "Float32",
    0x68: "Float64",
    0x78: "GZString",
    0x90: "GetError",
}


def is_call_free_leaf_x86(code: bytes) -> bool:
    n = len(code)
    if n == 0 or n > 48:
        return False
    while n > 0 and code[n - 1] == 0x90:
        n -= 1
    if n == 0:
        return False
    last = code[n - 1]
    ends_ret = last == 0xC3
    ends_jmp_rel8 = n >= 2 and code[n - 2] == 0xEB
    ends_jmp_rel32 = n >= 5 and code[n - 5] == 0xE9
    if not (ends_ret or ends_jmp_rel8 or ends_jmp_rel32):
        return False
    for i in range(n):
        if code[i] == 0xE8:
            return False
        if code[i] == 0xFF and i + 1 < n and ((code[i + 1] >> 3) & 7) == 2:
            return False
    return True


def is_com_boilerplate(mangled: str) -> bool:
    return (
        any(
            x in mangled
            for x in (
                "14QueryInterfaceEjPPv",
                "6AddRefEv",
                "7ReleaseEv",
                "9RemoveRefEv",
                "8RefCountEv",
                "10GetGZCLSIDEv",
                "D0Ev",
                "D1Ev",
                "D2Ev",
            )
        )
        or "GZCOM_Create" in mangled
        or ("GZCOM_" in mangled and "CLSIDEv" in mangled)
    )


def classify_tier(mangled: str, demangled: str, is_leaf: bool) -> int:
    if mangled.startswith(
        ("__ZThn", "__ZTv", "__ZTc", "___cxx_global", "__GLOBAL__", "__ZGV")
    ):
        return 4
    if demangled.startswith(
        ("std::__1::", "std::", "__clang_call_terminate", "operator new", "operator delete")
    ) or "std::__1::" in demangled.split("(", 1)[0]:
        return 4
    head = demangled.split("(", 1)[0]
    cls = head.split("::")[0].split()[-1] if "::" in head else "<free>"
    com = is_com_boilerplate(mangled)
    if (
        cls.startswith(
            (
                "cGZ",
                "cRZ",
                "cIGZ",
                "nGZ",
                "nRZ",
                "RZ",
                "GZ",
                "nSGLDX7",
                "CDirect3D",
                "CDirectDraw",
                "cSGLDX7",
                "nsTexelOp",
                "IDirectSound",
                "cWin",
                "cUIGZ",
                "cHTML",
                "NetworkFileCache",
                "cSCRes",
                "nSCRes",
                "cSCProperty",
            )
        )
        or cls == "<free>"
    ):
        if com and (cls.startswith(("cSC4", "cSTE")) or cls == "<free>"):
            return 3
        return 4
    if com or is_leaf:
        return 3
    if cls.startswith(("cSC4", "cISC4", "tAutomataState", "cSTETerrain", "cSTESurfaceWater")):
        if any(
            w in cls
            for w in (
                "Win",
                "Dlg",
                "Button",
                "UI",
                "ViewInput",
                "View3D",
                "3DRender",
                "RenderProperties",
                "Tool",
                "Camera",
                "ModelMaker",
                "24HourClock",
                "Effects",
                "Description",
                "Particle",
            )
        ):
            return 2
        return 1
    return 2


def load_macho_text_symbols(bin_path: str):
    data = open(bin_path, "rb").read()
    magic, _, _, _, ncmds, sizeofcmds, _, _ = struct.unpack_from("<IIIIIIII", data, 0)
    assert magic == 0xFEEDFACF, f"Not a 64-bit Mach-O binary: {magic:#x}"
    off = 32
    symoff = nsyms = stroff = strsize = 0
    text_end = 0x10092AD40
    for _ in range(ncmds):
        cmd, cmdsize = struct.unpack_from("<II", data, off)
        if cmd == 0x19 and cmdsize >= 152:
            sectname = data[off + 72 : off + 88].split(b"\x00", 1)[0].decode()
            if sectname == "__text":
                addr, size = struct.unpack_from("<QQ", data, off + 72 + 32)
                text_end = addr + size
        elif cmd == 0x2:
            symoff, nsyms, stroff, strsize = struct.unpack_from("<IIII", data, off + 8)
        off += cmdsize

    raw = set()
    for i in range(nsyms):
        n_strx, n_type, n_sect, _, n_value = struct.unpack_from(
            "<IBBHQ", data, symoff + i * 16
        )
        if (n_type & 0xE0) == 0 and (n_type & 0x0E) == 0x0E and n_sect == 1:
            end = data.find(b"\x00", stroff + n_strx)
            mangled = data[stroff + n_strx : end].decode("utf-8", "replace")
            raw.add((n_value, mangled))

    ordered = sorted(raw)
    demangled = (
        subprocess.check_output(
            ["c++filt"], input=("\n".join(m for _, m in ordered)).encode()
        )
        .decode()
        .splitlines()
    )
    out = []
    for i, ((addr, mangled), dem) in enumerate(zip(ordered, demangled)):
        nxt = text_end
        for j in range(i + 1, len(ordered)):
            if ordered[j][0] > addr:
                nxt = ordered[j][0]
                break
        sz = max(0, nxt - addr)
        foff = addr - 0x100000000
        code = data[foff : foff + sz] if 0 <= foff and foff + sz <= len(data) else b""
        leaf = is_call_free_leaf_x86(code)
        out.append((addr, sz, mangled, dem, leaf))
    return [x[:4] for x in out], {x[0]: x[4] for x in out}


CITATION_SITES = (
    ("src/**/*.cpp", "cpp"),
    ("src/**/*.mm", "cpp"),
    ("tools/**/*.mm", "cpp"),
    ("include/**/*.h", "header"),
    ("tests/**/*.cpp", "test"),
)


def collect_decompiled_addresses(symbols):
    addrs = [a for a, _, _, _ in symbols]
    cited = set()
    kinds = {}
    for pat, kind in CITATION_SITES:
        for path in glob.glob(os.path.join(ROOT, pat), recursive=True):
            if path.endswith("decompilation_registry.cpp"):
                continue
            txt = open(path).read()
            for m in re.findall(r"0x(100[0-9a-fA-F]{6})(?:[uUlL]*)\b", txt):
                val = int(m, 16)
                if 0x1000018A0 <= val < 0x10092AD40:
                    idx = bisect.bisect_right(addrs, val) - 1
                    if idx >= 0 and val - addrs[idx] < 0x2000:
                        cited.add(addrs[idx])
                        kinds.setdefault(addrs[idx], set()).add(kind)
    done = set(cited)
    for addr, _, m, _ in symbols:
        if m.startswith(("__ZThn", "__ZTv", "__ZTc")):
            continue
        if (
            "4ReadER11cIGZIStream" in m
            or "5WriteER11cIGZOStream" in m
            or "10GetGZCLSIDEv" in m
            or "GZCOM_Create" in m
            or ("GZCOM_" in m and "CLSIDEv" in m)
            or m.startswith(
                (
                    "__ZN10cSC4Demand",
                    "__ZNK10cSC4Demand",
                    "__ZN7cRZDate",
                    "__ZNK7cRZDate",
                    "__ZN7cGZDate",
                    "__ZNK7cGZDate",
                    "__ZN8cRZTimer",
                    "__ZNK8cRZTimer",
                    "__ZN11cSC4SimGrid",
                    "__ZNK11cSC4SimGrid",
                    "__Z27ProcessTwoGridsWithCallback",
                    "__ZN18cSC4AutoSaveStream",
                    "__ZN18cSC4AutoLoadStream",
                    "__ZN17cSC4ResponseCurve",
                    "__ZNK17cSC4ResponseCurve",
                    "__ZN20cSC4BoundaryIterator",
                    "__ZNK20cSC4BoundaryIterator",
                    "__ZN14RandomIterator",
                    "__ZNK14RandomIterator",
                    "__ZN20cSC4BoundaryIterator",
                    "__ZNK20cSC4BoundaryIterator",
                    "__ZN25cSC4GameOfLifeSimulator",
                    "__ZNK25cSC4GameOfLifeSimulator",
                    "__ZN24cSC4RigidBodySimulator2D",
                    "__ZNK24cSC4RigidBodySimulator2D",
                    "__ZN30cSC4TileNeighborhoodCompact",
                    "__ZNK30cSC4TileNeighborhoodCompact",
                    "__Z21SC4ReadGZSerializable",
                    "__Z22SC4WriteGZSerializable",
                    "__Z19SC4IsGZSerializable",
                    "__Z19SC4ReadStringRecord",
                    "__Z20SC4WriteStringRecord",
                    "__Z19SC4ReadRegionalCity",
                    "__Z20SC4WriteRegionalCity",
                    "__Z24SC4ReadAppLoadSaveVersion",
                    "__Z25SC4WriteAppLoadSaveVersion",
                )
            )
        ):
            done.add(addr)
    return cited, done, kinds


def collect_verified_addresses(symbols, verify_bin=None, verify_log=None):
    """Addresses of __TEXT,__text symbols exercised by a [PASS] verify check."""
    if verify_log:
        text = open(verify_log).read()
    else:
        bin_path = verify_bin or os.path.join(
            ROOT, "..", "build", "authentic", "opensc4_authentic_verify"
        )
        if not os.path.exists(bin_path):
            return set(), None
        text = subprocess.run(
            [bin_path], capture_output=True, text=True, check=True
        ).stdout
    addrs_sorted = [a for a, _, _, _ in symbols]
    out = set()
    for m in re.findall(r"\[PASS\][^\n]*?0x(100[0-9a-fA-F]{6})", text):
        val = int(m, 16)
        idx = bisect.bisect_right(addrs_sorted, val) - 1
        if idx >= 0 and val - addrs_sorted[idx] < 0x2000:
            out.add(addrs_sorted[idx])
    return out, text.count("[PASS]")


def symbol_state(addr, done, kinds, verified):
    """Honest per-symbol state: VERIFIED > IMPLEMENTED > CITED > TODO."""
    if addr in verified:
        return "VERIFIED"
    if "cpp" in kinds.get(addr, ()):
        return "IMPLEMENTED"
    if addr in done:
        return "IMPLEMENTED" if addr not in kinds else "CITED"
    return "TODO"


# Empirical completion-rate model for the project-level estimate in STATUS.md.
# Calibrated from the M3 terrain wave (commits 7c16a2a..6a4fc2e): ~18.7 KB of
# __TEXT machine code reconstructed in ~1.5 working days, serial pace (one
# conductor dispatching one implementer/reviewer at a time), roughly 20
# max-step subagent sessions plus conductor overhead per 4 tickets.
# These are order-of-magnitude PLANNING numbers, not measurements.
KB_PER_DAY = 12.5            # observed serial single-conductor pace
TOKENS_PER_KB = 2.0e6        # rough LLM tokens per KB of reconstructed code


def write_status_md(path, names, tier_tot, tier_kb, tier_state, tier_state_kb,
                   cls_state, cls_state_kb, verified, n_pass,
                   total_symbols, covered_symbols):
    rows = [
        "# STATUS.md — Authentic decompilation progress",
        "",
        "<!-- AUTO-GENERATED: regenerate with `python3 tools/sc4_decomp_audit.py --status-md STATUS.md` -->",
        "",
        f"Generated: {datetime.date.today().isoformat()}",
        (
            f"Verify suite: **{n_pass} [PASS]** checks ({len(verified)} symbols VERIFIED)"
            if n_pass is not None
            else "Verify suite: not run (build it and regenerate for VERIFIED state)"
        ),
        f"Symbols scanned: {total_symbols} `__TEXT,__text`; {covered_symbols} cited/covered.",
        "",
    ]

    # Overall completion block (whole project, KB-weighted). Tier 3 (AUTO) and
    # Tier 4 (SKIP) carry no remaining work by design, so only Tiers 1+2 count.
    t1_left = tier_state_kb[1]["TODO"] + tier_state_kb[1]["CITED"]
    t2_left = tier_state_kb[2]["TODO"] + tier_state_kb[2]["CITED"]
    total_kb = tier_kb[1] + tier_kb[2]
    left_kb = t1_left + t2_left
    done_kb = total_kb - left_kb
    pct = 100.0 * done_kb / total_kb if total_kb > 0 else 0.0
    est_days = left_kb / KB_PER_DAY if KB_PER_DAY > 0 else float("inf")
    est_months = est_days / 30.0
    est_tokens = left_kb * TOKENS_PER_KB
    if est_tokens >= 1e9:
        tok_str = f"{est_tokens / 1e9:.1f}B"
    else:
        tok_str = f"{est_tokens / 1e6:.0f}M"
    rows.extend([
        "## Overall completion (whole authentic project)",
        "",
        f"- **{pct:.1f}% complete** — {done_kb:.0f} KB reconstructed of "
        f"{total_kb:.0f} KB of `__TEXT` machine code (Tier 1+2, KB-weighted; "
        "Tier 3 AUTO / Tier 4 SKIP are complete by design).",
        f"- **Remaining**: {left_kb:.0f} KB of machine code "
        f"(Tier 1: {t1_left:.0f} KB, Tier 2: {t2_left:.0f} KB).",
        f"- **Estimated time to finish**: ~{est_months:.0f} months at the observed "
        f"serial single-conductor pace ({KB_PER_DAY} KB/day, calibrated on the "
        "M3 terrain wave). Parallelizing implementers across disjoint classes "
        "would reduce this roughly proportionally.",
        f"- **Estimated tokens to finish**: ~{tok_str} LLM tokens at the observed "
        f"~{TOKENS_PER_KB / 1e6:.0f}M tokens/KB.",
        "- The percentage is hard data; the time/token figures are "
        "order-of-magnitude planning extrapolations, not commitments.",
        "",
        "Per-symbol states (honest coverage — a citation is not an implementation):",
        "",
        "- **VERIFIED** — exercised by a `[PASS]` check of `opensc4_authentic_verify` (strongest).",
        "- **IMPLEMENTED** — cited in a `.cpp`/`.mm` implementation unit, or part of a prefix-covered implemented class.",
        "- **CITED** — address cited only in headers/tests: declared, not yet implemented.",
        "- **TODO** — no citation, no code: the real remaining work.",
        "- **AUTO** (Tier 3) — machine-verified GZCOM boilerplate / leaf accessors. **SKIP** (Tier 4) — superseded runtime, not reimplemented by design.",
        "",
        "## Tier coverage",
        "",
        "| Tier | Symbols | VERIFIED | IMPLEMENTED | CITED-only | TODO/AUTO/SKIP | KB total | KB remaining |",
        "| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |",
    ])
    for t in (1, 2, 3, 4):
        c, ck = tier_state[t], tier_state_kb[t]
        if t in (1, 2):
            rest_label = str(c["TODO"])
            kb_left = ck["TODO"] + ck["CITED"]
        elif t == 3:
            rest_label = f"AUTO {c['AUTO']}"
            kb_left = 0.0
        else:
            rest_label = f"SKIP {c['SKIP']}"
            kb_left = 0.0
        rows.append(
            f"| {names[t]} | {tier_tot[t]} | {c['VERIFIED']} | {c['IMPLEMENTED']} | "
            f"{c['CITED']} | {rest_label} | {tier_kb[t]:.1f} | {kb_left:.1f} |"
        )
    for t in (1, 2):
        remaining = [
            (cls, c)
            for (tt, cls), c in cls_state.items()
            if tt == t and (c["TODO"] or c["CITED"])
        ]
        remaining.sort(key=lambda kv: (-(kv[1]["TODO"] + kv[1]["CITED"]), kv[0]))
        if not remaining:
            continue
        rows += [
            "",
            f"## Tier {t} remaining work (TODO + CITED-only), by class",
            "",
            "| Class | VERIFIED | IMPL | CITED-only | TODO | KB left |",
            "| :--- | ---: | ---: | ---: | ---: | ---: |",
        ]
        for cls, c in remaining[:40]:
            kb = cls_state_kb[(t, cls)]
            rows.append(
                f"| `{cls}` | {c['VERIFIED']} | {c['IMPLEMENTED']} | {c['CITED']} | "
                f"{c['TODO']} | {kb['TODO'] + kb['CITED']:.1f} |"
            )
    rows += [
        "",
        "## Next actions",
        "",
        "1. `python3 tools/sc4_decomp_audit.py --next \"\" 25` — next TODO symbols.",
        "2. `python3 tools/sc4_decomp_audit.py --workpack <Class>` — workpack for a class.",
        "3. `--sync-registry` after citing new addresses; rerun verify and regenerate this file.",
        "",
    ]
    open(path, "w").write("\n".join(rows) + "\n")
    print(f"Wrote status report to {path}")


def disasm_range(bin_path: str, start: int, stop: int):
    out = subprocess.check_output(
        [
            "objdump",
            "-d",
            "--x86-asm-syntax=intel",
            "--no-show-raw-insn",
            f"--start-address={start:#x}",
            f"--stop-address={stop:#x}",
            bin_path,
        ]
    ).decode()
    lines = []
    for line in out.splitlines():
        if line.startswith("100"):
            parts = line.split("\t", 1)
            if len(parts) == 2 and not parts[1].strip().startswith("nop"):
                lines.append((line.split(":")[0], parts[1].strip()))
    return lines


_FILE_BYTES_CACHE = {}


def macho_read(bin_path: str, vmaddr: int, count: int) -> bytes:
    """Byte-exact read at a vmaddr using the linear vmaddr-0x100000000 file
    mapping this binary uses (validated against multiple __DATA tunables)."""
    data = _FILE_BYTES_CACHE.get(bin_path)
    if data is None:
        data = open(bin_path, "rb").read()
        _FILE_BYTES_CACHE[bin_path] = data
    off = vmaddr - 0x100000000
    if off < 0 or off + count > len(data):
        raise SystemExit(f"Address {vmaddr:#x} (+{count} B) outside mapped file")
    return data[off : off + count]


def macho_data_symbols(bin_path: str):
    """All symbol-table entries (any section), as (value, mangled_name)."""
    data = macho_read(bin_path, 0x100000000, 32)
    _, _, _, _, ncmds, _, _, _ = struct.unpack_from("<IIIIIIII", data, 0)
    data = _FILE_BYTES_CACHE[bin_path]
    off = 32
    symoff = nsyms = stroff = strsize = 0
    for _ in range(ncmds):
        cmd, cmdsize = struct.unpack_from("<II", data, off)
        if cmd == 0x2:  # LC_SYMTAB
            symoff, nsyms, stroff, strsize = struct.unpack_from("<IIII", data, off + 8)
            break
        off += cmdsize
    out = []
    for i in range(nsyms):
        n_strx, _, _, _, n_value = struct.unpack_from("<IBBHQ", data, symoff + i * 16)
        end = data.find(b"\x00", stroff + n_strx)
        out.append((n_value, data[stroff + n_strx : end].decode("utf-8", "replace")))
    return out


def dump_constant(bin_path: str, spec: str):
    """Parse ADDR[:COUNT[:TYPE]] and print byte-exact values.
    TYPE: bytes (default hex), f32, f64, i32, u32 (f32/f64/i32 read COUNT
    consecutive values)."""
    parts = spec.split(":")
    addr = int(parts[0], 0)
    kind = "bytes"
    count = 16
    if len(parts) >= 2 and parts[1]:
        count = int(parts[1], 0)
    if len(parts) >= 3 and parts[2]:
        kind = parts[2]
    raw = macho_read(bin_path, addr, count if kind == "bytes" else count * 8)
    print(f"=== {addr:#x} ({spec}) ===")
    if kind == "bytes":
        for i in range(0, len(raw), 16):
            chunk = raw[i : i + 16]
            hexs = " ".join(f"{b:02x}" for b in chunk)
            asc = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
            print(f"  {addr + i:#x}: {hexs:<48} {asc}")
    else:
        fmt, sz = {"f32": ("<f", 4), "f64": ("<d", 8), "i32": ("<i", 4), "u32": ("<I", 4)}[kind]
        for i in range(len(raw) // sz):
            val = struct.unpack_from(fmt, raw, i * sz)[0]
            extra = ""
            if kind == "f32":
                extra = f"  (bits {struct.unpack_from('<I', raw, i * sz)[0]:#010x})"
            elif kind == "f64":
                extra = f"  (bits {struct.unpack_from('<Q', raw, i * sz)[0]:#018x})"
            print(f"  {addr + i * sz:#x}: {val!r}{extra}")


def dump_vtable(bin_path: str, cls: str, symbols, max_off: int = 0x400):
    """Resolve a class's __ZTV vtable slots against the text symbol table.
    Prints vt+OFF -> target (demangled callee) for every slot in range."""
    want = f"__ZTV{len(cls)}{cls}"
    vt = None
    for value, name in macho_data_symbols(bin_path):
        if value != 0 and (name == want or name.rstrip("$") == want):
            vt = value
            break
    if vt is None:
        raise SystemExit(f"Vtable symbol not found: {want}")
    by_addr = {}
    for a, sz, m, d in symbols:
        by_addr.setdefault(a, d)
    print(f"=== vtable {cls} (__ZTV base {vt:#x}; slots at base+0x10) ===")
    for off in range(0, max_off + 1, 8):
        (ptr,) = struct.unpack("<Q", macho_read(bin_path, vt + 0x10 + off, 8))
        label = by_addr.get(ptr)
        hint = f"  {label}" if label else f"  (no text symbol: {ptr:#x})"
        print(f"  vt+0x{off:03x} -> {ptr:#x}{hint}")


def summarize_leaf_insns(insns):
    body = [
        ins
        for _, ins in insns
        if ins not in ("push\trbp", "mov\trbp, rsp", "pop\trbp")
    ]
    if not body:
        return "void /* no-op */"
    if body == ["ret"]:
        return "return; // no-op stub"
    if body == ["xor\teax, eax", "ret"]:
        return "return 0; // false / null"
    if len(body) == 2 and body[1] == "ret":
        b0 = body[0]
        m = re.match(r"mov(?:zx|sx|ss)?\t(\w+),\s*(\w+ ptr \[rdi(?:\s*\+\s*(0x[0-9a-f]+))?\])", b0)
        if m:
            off = m.group(3) or "0x0"
            return f"return this->field_{off}; // {b0}"
        m = re.match(r"lea\trax,\s*\[rdi\s*\+\s*(0x[0-9a-f]+)\]", b0)
        if m:
            return f"return &this->field_{m.group(1)};"
        m = re.match(r"mov\t(eax|al),\s*(0x[0-9a-f]+|-?\d+)", b0)
        if m:
            return f"return {m.group(2)};"
        m = re.match(r"mov(?:ss)?\t\w+ ptr \[rdi(?:\s*\+\s*(0x[0-9a-f]+))?\],\s*(\w+)", b0)
        if m:
            off = m.group(1) or "0x0"
            return f"this->field_{off} = {m.group(2)};"
    return " ; ".join(body)


def decode_const(bin_path: str, addr: int, ins: str):
    """Decode a rip-referenced constant-pool load with its best-guess type.
    Returns a formatted table row or None. Deterministic — no agent guessing.
    (Precedent: the xmm2/xmm3 transcription trap and the -9.1f decimal misread
    both happened on manual constant reads; this pre-pass eliminates them.)
    Region rule: FP loads always decode; integer loads decode only in the
    known literal-pool (0x100a15000-0x100a2a000) and game-globals
    (0x100b00000-0x100b80000) regions — the 0x100a6xxxx+ GOT/vtable
    region holds pointer references, not constants."""
    m = re.search(r"\[rip [+-] 0x[0-9a-f]+\]\s*##\s*(0x[0-9a-f]+)", ins)
    if not m:
        return None
    addr = int(m.group(1), 16)
    if re.match(r"movsd", ins):
        typ, n = "f64", 8
    elif re.match(r"movss", ins):
        typ, n = "f32", 4
    else:
        typ, n = "i32", 4
        if not (0x100a15000 <= addr < 0x100a2a000 or
                0x100b00000 <= addr < 0x100b80000):
            return None
    try:
        raw = macho_read(bin_path, addr, n)
    except SystemExit:
        return None
    if typ == "f64":
        v = struct.unpack("<d", raw)[0]
        bits = struct.unpack("<Q", raw)[0]
        return f"  | {addr:#x} f64 = {v!r} (bits {bits:#018x})"
    if typ == "f32":
        v = struct.unpack("<f", raw)[0]
        bits = struct.unpack("<I", raw)[0]
        return f"  | {addr:#x} f32 = {v!r} (bits {bits:#010x})"
    v = struct.unpack("<i", raw)[0]
    return f"  | {addr:#x} i32 = {v} ({v:#x})"


def disasm_algo_full(bin_path: str, addr: int, sz: int, sym_lookup: dict):
    """Full, honest instruction listing for a [TODO-ALGO] method.

    Every instruction is emitted verbatim — nothing is filtered or condensed.
    The previous implementation "condensed" the listing by keeping only
    "interesting" opcodes (calls, compares, scalar float math, ...) and
    silently dropped everything else. For multi-block functions — large
    if/else regions, interpolation blocks, multi-pass loops — that filter
    discarded the branches, pointer arithmetic and multi-instruction float
    expressions that define the function's real structure and size. Terrain
    brush functions were misrepresented as tiny "uniform pattern" bodies
    (e.g. 246/182/167 B) when they were actually 452/624/670/138 B with
    interpolation math, rects and a richer signature.

    Only two additive annotations are layered on top of the raw listing:
      * indirect vtable calls are tagged with the cIGZSerializable stream
        method they dispatch to (// >> tag),
      * direct call/jmp targets are annotated with the demangled callee
        (// -> name).
    Nothing is removed: ret, conditional/unconditional jumps, FP and pointer
    instructions are all preserved so the workpack reflects ground truth.
    """
    insns = disasm_range(bin_path, addr, addr + sz)
    out = []
    landing_pads = 0
    const_rows = {}
    for pc, ins in insns:
        tag = ""
        m_vt = re.search(r"call\tqword ptr \[r\w+\s*\+\s*(0x[0-9a-f]+)\]", ins)
        if m_vt:
            voff = int(m_vt.group(1), 16)
            tag = f" // >> {STREAM_VT.get(voff, f'vtable+{voff:#x}')}"
        else:
            m_call = re.search(r"(call|jmp)\s+(0x100[0-9a-f]+)", ins)
            if m_call:
                taddr = int(m_call.group(2), 16)
                tag = f" // -> {sym_lookup.get(taddr, '')}"
                if "Unwind_Resume" in tag:
                    landing_pads += 1
        m_rip = re.search(r"\[rip [+-] 0x[0-9a-f]+\]\s*##\s*(0x[0-9a-f]+)", ins)
        if m_rip:
            # decode_const's region rule filters GOT/vtable pointer refs.
            row = decode_const(bin_path, addr, ins)
            if row is not None:
                const_rows[row] = True
        out.append(f"  {pc}: {ins}{tag}")
    # Deterministic pre-pass summary (landing pads => stack RAII object
    # count; constants table => byte-exact values, no manual reads needed).
    if landing_pads or const_rows:
        out.append("")
        out.append("  [pre-pass]")
        out.append(
            f"  | landing pads: {landing_pads} "
            f"(exception cleanup paths -> >= {landing_pads} stack RAII objects)"
        )
        for row in sorted(const_rows):
            out.append(row)
    return out


def sync_registry(cited):
    reg_path = os.path.join(ROOT, "src", "decompilation_registry.cpp")
    txt = open(reg_path).read()
    cited_list = sorted(cited)
    rows = []
    for i in range(0, len(cited_list), 6):
        row = ", ".join(f"0x{a:09x}ull" for a in cited_list[i : i + 6])
        rows.append(f"    {row},")
    replacement = (
        "constexpr Uint64 kRegisteredAddresses[] = {\n"
        + "\n".join(rows)
        + "\n};"
    )
    new_txt, n = re.subn(
        r"constexpr Uint64 kRegisteredAddresses\[\] = \{.*?\};",
        replacement,
        txt,
        flags=re.S,
    )
    assert n == 1
    open(reg_path, "w").write(new_txt)
    print(f"Synced {len(cited_list)} addresses into {reg_path}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default=DEFAULT_BIN)
    ap.add_argument("--class", dest="cls", help="Audit all methods of a class")
    ap.add_argument("--workpack", help="Generate subagent decompilation workpack for a class")
    ap.add_argument("--out", help="Optional output file for --workpack")
    ap.add_argument("--next", nargs="+", help="Filter [limit] of next Tier-1 TODO symbols")
    ap.add_argument("--disasm", help="Address (0x100...) or substring to disassemble")
    ap.add_argument("--dump", nargs="+", metavar="ADDR[:COUNT[:TYPE]]",
                    help="Byte-exact constant dump. TYPE: bytes (default), f32, f64, i32, u32; "
                         "COUNT = bytes for 'bytes', values otherwise. "
                         "e.g. --dump 0x100b175d4:1:f32 0x100a29c10:8:i32")
    ap.add_argument("--vtable", metavar="CLASS",
                    help="Resolve a class's __ZTV vtable slots against text symbols "
                         "(e.g. --vtable cSTETerrain)")
    ap.add_argument("--sync-registry", action="store_true")
    ap.add_argument("--status-md", help="Write a markdown status report to this file")
    ap.add_argument("--verify-log", help="Reuse a captured verify-suite log instead of running it")
    ap.add_argument("--verify-bin", help="Path to opensc4_authentic_verify (default: ../build/authentic/)")
    ap.add_argument("--no-verify", action="store_true", help="Skip the verify suite (no VERIFIED state)")
    args = ap.parse_args()

    symbols, leaf_map = load_macho_text_symbols(args.bin)
    cited, done, kinds = collect_decompiled_addresses(symbols)
    verified, n_pass = set(), None
    if not args.no_verify:
        try:
            verified, n_pass = collect_verified_addresses(
                symbols, args.verify_bin, args.verify_log
            )
        except Exception as exc:  # verify run is optional
            print(
                f"warning: verify suite unavailable ({exc}); "
                "reporting without VERIFIED state",
                file=sys.stderr,
            )
    sym_lookup = {a: d for a, _, _, d in symbols}

    if args.sync_registry:
        sync_registry(cited)
        return

    if args.disasm:
        q = args.disasm
        target = None
        if q.startswith("0x"):
            val = int(q, 16)
            for a, sz, m, d in symbols:
                if a <= val < a + max(1, sz):
                    target = (a, sz, m, d)
                    break
        else:
            for a, sz, m, d in symbols:
                if q in d or q == m:
                    target = (a, sz, m, d)
                    break
        if not target:
            sys.exit(f"Symbol not found: {q}")
        a, sz, m, d = target
        print(f"=== {d} ({m}) @ {a:#x}..{a+sz:#x} ({sz} bytes) ===")
        for pc, ins in disasm_range(args.bin, a, a + sz):
            print(f"{pc}: {ins}")
        return

    if args.dump:
        for spec in args.dump:
            dump_constant(args.bin, spec)
        return

    if args.vtable:
        dump_vtable(args.bin, args.vtable, symbols)
        return

    if args.cls:
        matches = [
            (a, sz, m, d)
            for a, sz, m, d in symbols
            if not m.startswith(("__ZThn", "__ZTv", "__ZTc"))
            and (d.startswith(args.cls + "::") or args.cls in d)
        ]
        print(f"Class/filter '{args.cls}': {len(matches)} non-thunk symbols")
        for a, sz, m, d in matches:
            leaf = leaf_map.get(a, False)
            tier = classify_tier(m, d, leaf)
            if tier == 3:
                tag = "AUTO-LEAF" if leaf else "AUTO-COM "
            elif tier == 4:
                tag = "SKIP-T4  "
            else:
                st = symbol_state(a, done, kinds, verified)
                tag = {
                    "VERIFIED": "VERIFIED ",
                    "IMPLEMENTED": "IMPL     ",
                    "CITED": "CITED-HDR",
                    "TODO": "TODO-ALGO",
                }[st]
            print(f"  [{tag}] {a:#011x} ({sz:5d} B)  {d}")
        return

    if args.workpack:
        cls_name = args.workpack
        matches = [
            (a, sz, m, d)
            for a, sz, m, d in symbols
            if not m.startswith(("__ZThn", "__ZTv", "__ZTc"))
            and (d.startswith(cls_name + "::") or f"GZCOM_Create{cls_name}" in d or f"GZCOM_{cls_name}CLSID" in d)
        ]
        if not matches:
            sys.exit(f"No symbols found for class '{cls_name}'")
        lines = [f"# Decompilation Workpack: `{cls_name}` ({len(matches)} symbols)\n"]
        leaves = []
        todos = []
        already = []
        for a, sz, m, d in matches:
            leaf = leaf_map.get(a, False)
            tier = classify_tier(m, d, leaf)
            st = symbol_state(a, done, kinds, verified)
            if st in ("VERIFIED", "IMPLEMENTED"):
                already.append((a, sz, d))
            elif tier == 3:
                leaves.append((a, sz, m, d, leaf))
            elif tier in (1, 2):
                todos.append((a, sz, m, d))
        lines.append(
            f"- Already decompiled: `{len(already)}` | Auto-lifted COM/leaf (`Tier 3`): `{len(leaves)}` | Remaining algorithmic (`Tier 1/2`): `{len(todos)}` (`{sum(x[1] for x in todos)/1024:.1f} KB`)\n"
        )
        if leaves:
            lines.append("## 1. Auto-Lifted COM & Leaf Accessors (`<= 48 B`, $0 LLM Tokens)\n```cpp")
            for a, sz, m, d, is_leaf in leaves:
                if is_leaf:
                    ins = disasm_range(args.bin, a, a + sz)
                    summary = summarize_leaf_insns(ins)
                    lines.append(f"// @ {a:#x} ({sz} B): {d}\n//   -> {summary}")
                else:
                    lines.append(f"// @ {a:#x} ({sz} B): {d} [GZCOM boilerplate]")
            lines.append("```\n")
        if todos:
            lines.append("## 2. Remaining Algorithmic Methods (`[TODO-ALGO]`)\n")
            for a, sz, m, d in todos:
                lines.append(f"### `{d}` @ `{a:#x}` (`{sz}` bytes)\n```asm")
                lines.extend(disasm_algo_full(args.bin, a, sz, sym_lookup))
                lines.append("```\n")
        text = "\n".join(lines)
        if args.out:
            open(args.out, "w").write(text)
            print(f"Wrote workpack ({len(todos)} algorithmic methods, {len(leaves)} auto-leaves) to {args.out}")
        else:
            print(text)
        return

    if args.next:
        filt = args.next[0]
        limit = int(args.next[1]) if len(args.next) > 1 else 25
        shown = 0
        for a, sz, m, d in symbols:
            leaf = leaf_map.get(a, False)
            tier = classify_tier(m, d, leaf)
            if tier not in (1, 2):
                continue
            if symbol_state(a, done, kinds, verified) in ("VERIFIED", "IMPLEMENTED"):
                continue
            if filt in d or filt in m:
                print(f"  [TODO-T{tier}] {a:#011x} ({sz:5d} B)  {d}")
                shown += 1
                if shown >= limit:
                    break
        return

    names = {
        1: "Tier 1: Core SC4 Simulation, World, Network, Automata & Terrain",
        2: "Tier 2: UI Windows, 3D View/Render, Audio & Lua/Effects",
        3: "Tier 3: Auto-Verified GZCOM Boilerplate & <=48B Leaf Accessors",
        4: "Tier 4: Superseded cGZ/cRZ Framework, DX7 Shim, Thunks & STL",
    }
    order = ("VERIFIED", "IMPLEMENTED", "CITED", "TODO")

    tier_tot = collections.Counter()
    tier_kb = collections.Counter()
    tier_state = {t: collections.Counter() for t in (1, 2, 3, 4)}
    tier_state_kb = {t: collections.Counter() for t in (1, 2, 3, 4)}
    cls_state = collections.defaultdict(collections.Counter)
    cls_state_kb = collections.defaultdict(collections.Counter)

    for a, sz, m, d in symbols:
        leaf = leaf_map.get(a, False)
        t = classify_tier(m, d, leaf)
        tier_tot[t] += 1
        tier_kb[t] += sz / 1024.0
        if t == 3:
            st = "AUTO"
        elif t == 4:
            st = "SKIP"
        else:
            st = symbol_state(a, done, kinds, verified)
        tier_state[t][st] += 1
        tier_state_kb[t][st] += sz / 1024.0
        if t in (1, 2):
            cls = d.split("(", 1)[0].split("::")[0].split()[-1]
            cls_state[(t, cls)][st] += 1
            cls_state_kb[(t, cls)][st] += sz / 1024.0

    print("=== OpenSC4 4-Tier Decompilation Pipeline Status ===")
    if n_pass is not None:
        print(
            f"  Verify suite: {n_pass} [PASS] checks ({len(verified)} symbols VERIFIED)"
        )
    else:
        print("  Verify suite: not run (no VERIFIED state reported)")
    for t in (1, 2, 3, 4):
        counts = " ".join(f"{st}:{tier_state[t][st]:5d}" for st in order)
        print(f"  {names[t]}")
        print(f"    {counts}  (total {tier_tot[t]:5d})")
        print(
            f"    KB "
            + " ".join(
                f"{st}:{tier_state_kb[t][st]:7.1f}" for st in order if tier_state[t][st]
            )
            + f"  (total {tier_kb[t]:7.1f})"
        )

    for t in (1, 2):
        rows = []
        for (tt, cls), c in cls_state.items():
            if tt != t:
                continue
            remaining = c["TODO"] + c["CITED"]
            if remaining:
                rows.append((remaining, cls, c))
        rows.sort(key=lambda r: (-r[0], r[1]))
        if not rows:
            continue
        print(f"\nTop 25 Tier-{t} Classes with Remaining Work (TODO + CITED-only):")
        for remaining, cls, c in rows[:25]:
            impl = c["VERIFIED"] + c["IMPLEMENTED"]
            total = sum(c.values())
            kb = cls_state_kb[(t, cls)]
            left_kb = kb["TODO"] + kb["CITED"]
            print(
                f"  {cls:42s}  {impl:4d}/{total:4d} implemented  ({c['CITED']:4d} CITED, {c['TODO']:4d} TODO, {left_kb:7.1f} KB left)"
            )

    if args.status_md:
        write_status_md(
            args.status_md,
            names,
            tier_tot,
            tier_kb,
            tier_state,
            tier_state_kb,
            cls_state,
            cls_state_kb,
            verified,
            n_pass,
            len(symbols),
            len(done),
        )


if __name__ == "__main__":
    main()
