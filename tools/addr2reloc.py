#!/usr/bin/env python3
"""Convert between legacy SKSE64 (P: framework) game addresses and CommonLib
relocation IDs for Skyrim SE 1.7.99.

Mapping
-------
The P: framework relocates with offsets from the DLL image base, e.g.
    RelocAddr<_CreateBSTriShape> CreateBSTriShape(0x00EF1ED0);
CommonLib's offsets table (offsets-1-7-99-0.txt) stores ABSOLUTE addresses at
the standard x64 image base 0x140000000. So:

    file_address = p_address + 0x140000000

Each line of the offsets table is "<relocID> <hexAddress>"; the LEFT column is
the relocation ID used with REL::RelocationID(0, <id>).

Verified anchor: P: NiAllocate 0x00EAC530 -> file 0x140EAC530 -> relocID 68994.

Usage
-----
    # address(es) -> relocation ID(s). The image base is auto-detected:
    #   - a value <  0x140000000 is treated as a P: offset (base added)
    #   - a value >= 0x140000000 is treated as already absolute (used as-is)
    addr2reloc.py 0x00EF1ED0 0x140EF1ED0

    # force the interpretation regardless of auto-detection
    addr2reloc.py --no-base 0x140EF1ED0     # always add the base
    addr2reloc.py --with-base 0x00EF1ED0    # never add the base

    # reverse: relocation ID(s) -> file address(es)
    addr2reloc.py --reverse 70655 68994

    # custom offsets table
    addr2reloc.py --offsets /path/to/offsets-1-7-99-0.txt 0x00EF1ED0

Addresses may be given with or without a 0x prefix, in hex or decimal.
"""
from __future__ import annotations

import argparse
import os
import sys

IMAGE_BASE = 0x140000000


def _default_offsets_path() -> str:
    # tools/addr2reloc.py -> project root/offsets-1-7-99-0.txt
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.normpath(os.path.join(here, os.pardir, "offsets-1-7-99-0.txt"))


def _parse_addr(text: str) -> int:
    t = text.strip()
    try:
        if t.lower().startswith("0x"):
            return int(t, 16)
        # If it looks hex-ish (contains a-f) and no 0x, treat as hex.
        if any(c in "abcdefABCDEF" for c in t):
            return int(t, 16)
        return int(t, 10)
    except ValueError:
        raise SystemExit(f"error: cannot parse address {text!r}")


def _load_table(path: str):
    addr_to_id = {}
    id_to_addr = {}
    try:
        fh = open(path, "r", encoding="utf-8")
    except OSError as exc:
        raise SystemExit(f"error: cannot read offsets table {path!r}: {exc}")
    with fh:
        for line in fh:
            parts = line.split()
            if len(parts) < 2:
                continue
            try:
                rid = int(parts[0], 10)
            except ValueError:
                continue
            try:
                addr = int(parts[1], 16)
            except ValueError:
                continue
            addr_to_id.setdefault(addr, rid)
            id_to_addr.setdefault(rid, addr)
    return addr_to_id, id_to_addr


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("values", nargs="+", help="addresses (forward) or relocation IDs (with --reverse)")
    ap.add_argument("--reverse", action="store_true", help="interpret values as relocation IDs and print file addresses")
    base_grp = ap.add_mutually_exclusive_group()
    base_grp.add_argument("--no-base", dest="base", action="store_const", const=False,
                          help="value is a P: offset; always add the image base")
    base_grp.add_argument("--with-base", dest="base", action="store_const", const=True,
                          help="value already includes the image base; never add it")
    ap.add_argument("--offsets", default=_default_offsets_path(), help="path to the offsets-1-7-99-0.txt table")
    args = ap.parse_args(argv)

    if not os.path.exists(args.offsets):
        raise SystemExit(f"error: offsets table not found: {args.offsets}")

    addr_to_id, id_to_addr = _load_table(args.offsets)

    rc = 0
    for v in args.values:
        n = _parse_addr(v)
        if args.reverse:
            addr = id_to_addr.get(n)
            if addr is None:
                print(f"{v} (relocID {n}) -> NOT FOUND")
                rc = 1
            else:
                print(f"{v} (relocID {n}) -> 0x{addr:X}")
        else:
            if args.base is None:
                # auto-detect: values at/above the image base are already absolute
                has_base = n >= IMAGE_BASE
            else:
                has_base = args.base
            file_addr = n if has_base else (n + IMAGE_BASE)
            note = "base applied" if has_base else f"+0x{IMAGE_BASE:X}"
            rid = addr_to_id.get(file_addr)
            if rid is None:
                print(f"0x{n:X} ({note} -> 0x{file_addr:X}) -> NOT FOUND")
                rc = 1
            else:
                print(f"0x{n:X} ({note} -> 0x{file_addr:X}) -> relocID {rid}")
    return rc


if __name__ == "__main__":
    sys.exit(main())
