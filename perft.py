#!/usr/bin/env python3
"""Perft regression harness for Aestra.

Run AFTER building (`make basic`). Drives the exe over UCI — `position fen <fen>`,
`go perft N` — parses the last output line (`Total nodes: N`) and compares it
against the published perft counts below.

Usage:
    python perft.py [exe] [--smoke]

    exe     path to the built engine (default: Aestra.exe)
    --smoke run every position at depth 3 against its published depth-3 count
            (fast wiring check; the default full run uses the deep counts)
"""

import argparse
import re
import subprocess
import sys

POSITIONS = [
    # fen, full-depth expected nodes, depth-3 expected nodes
    ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324, 8902),
    ("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690, 97862),
    ("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, 11030083, 2812),
    ("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, 15833292, 9467),
    ("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194, 62379),
    ("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5, 164075551, 89890),
]

TIMEOUT = 900  # seconds per position


def run_perft(exe: str, fen: str, depth: int) -> int:
    proc = subprocess.Popen(
        [exe],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    commands = f"position fen {fen}\ngo perft {depth}\nquit\n"
    out, _ = proc.communicate(commands, timeout=TIMEOUT)
    lines = [line for line in out.splitlines() if line.strip()]
    last = lines[-1] if lines else ""
    match = re.search(r"Total nodes:\s*(\d+)", last)
    if not match:
        raise RuntimeError(f"no 'Total nodes' line found; last line: {last!r}")
    return int(match.group(1))


def main() -> int:
    parser = argparse.ArgumentParser(description="Aestra perft regression harness")
    parser.add_argument("exe", nargs="?", default="Aestra.exe", help="engine binary (default: Aestra.exe)")
    parser.add_argument("--smoke", action="store_true", help="run all positions at depth 3")
    args = parser.parse_args()

    failures = 0
    for fen, depth, expected, smoke_expected in POSITIONS:
        if args.smoke:
            depth, expected = 3, smoke_expected
        try:
            got = run_perft(args.exe, fen, depth)
        except Exception as exc:  # noqa: BLE001 - harness reports any failure
            print(f"FAIL  depth {depth}: {exc}")
            failures += 1
            continue
        status = "PASS" if got == expected else "FAIL"
        if got != expected:
            failures += 1
        print(f"{status} depth {depth}: got {got}, expected {expected}  ({fen[:32]}...)")

    print(f"\n{len(POSITIONS) - failures}/{len(POSITIONS)} positions passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
