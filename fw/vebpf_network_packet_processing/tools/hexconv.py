#!/usr/bin/env python3
"""Convert `objcopy -O verilog` output into the memory-image formats this
project's hardware and simulations consume.

Background: `riscv32-unknown-elf-objcopy -O verilog` emits an address
marker line (`@00000000`) followed by lines of space-separated bytes in
little-endian order:

    @00000000
    37 01 01 00 EF 40 00 10 ...

That layout is what the UART bootloader streams verbatim, but Verilog's
`$readmemh` needs one whole word per line, most-significant nibble first.
Historically that conversion was done by `load_BigEndian_Simulation.py`
and `load_BigEndian_Simulation_in_bytes_for_vivado.py`, both of which had
their input AND output filenames hardcoded in the script body -- so
producing a different app's memory image meant hand-editing two Python
files. This script takes them as arguments instead, so the Makefile can
emit every format from one `make APP=<name>` invocation and they can
never disagree about which build they came from.

Formats
-------
uart    : passthrough of the objcopy output (what `load.py` streams over UART)
sim     : one 32-bit word per line, e.g. `100040EF`  -- Verilog `$readmemh`.
          Used by BOTH cocotb simulation and synthesis-time BRAM preload;
          the name is historical, it really means "readmemh format".
vivado  : the same words, but split into one byte per line, least-significant
          byte of each word first.

Byte-for-byte compatible with the two original scripts, including their
handling of a trailing partial word (dropped -- `len(bytes)//4`).
"""
from __future__ import annotations

import argparse
import sys
from typing import List


def parse_objcopy_verilog(path: str) -> List[str]:
    """Read `objcopy -O verilog` output -> list of 8-hex-digit word strings.

    Each group of 4 little-endian bytes is reversed into one big-endian
    word. A trailing group of fewer than 4 bytes is dropped, matching the
    original scripts' `int(len(...)/4)` truncation exactly.
    """
    words: List[str] = []
    with open(path) as f:
        for line in f:
            if "@" in line:
                continue  # address marker; offsets are implicit and contiguous here
            fields = line.split("\n")[0].split(" ")
            if len(fields) == 0:
                continue
            for i in range(len(fields) // 4):
                words.append(
                    fields[4 * i + 3] + fields[4 * i + 2]
                    + fields[4 * i + 1] + fields[4 * i]
                )
    return words


def write_sim(words: List[str], path: str) -> None:
    """One 32-bit word per line -- Verilog $readmemh format."""
    with open(path, "w") as f:
        for word in words:
            f.write(word)
            f.write("\n")


def write_vivado(words: List[str], path: str) -> None:
    """One byte per line, least-significant byte of each word first."""
    with open(path, "w") as f:
        for word in words:
            f.write(word[6]); f.write(word[7]); f.write("\n")
            f.write(word[4]); f.write(word[5]); f.write("\n")
            f.write(word[2]); f.write(word[3]); f.write("\n")
            f.write(word[0]); f.write(word[1]); f.write("\n")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", help="objcopy -O verilog output (.hex)")
    ap.add_argument("output", help="destination file")
    ap.add_argument("--mode", required=True, choices=["sim", "vivado"],
                     help="output format ('uart' needs no conversion -- use the objcopy output directly)")
    args = ap.parse_args()

    words = parse_objcopy_verilog(args.input)
    if not words:
        print(f"error: no data words parsed from {args.input}", file=sys.stderr)
        return 1

    if args.mode == "sim":
        write_sim(words, args.output)
    else:
        write_vivado(words, args.output)

    print(f"hexconv: {args.input} -> {args.output} ({args.mode}, {len(words)} words)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
