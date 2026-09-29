"""Reports, per source file, which of the three coverages it is missing.

The project measures coverage three ways, each with its own instrument, and each
instrument answers for the whole tree rather than per file. So "which files are
worst" was being worked out by hand. This does it once:

  - **a mutation plan**, from tests/**/*mutations*.json. Line coverage is not the
    notion used here; a plan whose sweep catches is. A file named by no plan has
    nothing saying its tests would notice a change.
  - **an assert probe**, from tests/assert_probes.ps1. A file holding asserts that
    no probe targets has assertions nobody has watched fire.
  - **documentation**, from api_surface.collect - imported rather than
    reimplemented, so "documented" has one definition in this repository.

It reports; it does not gate. Every count is far from zero today, and a checker
that fails every run is one nobody reads.

Usage:

    python docs/coverage_gaps.py                 # worst-first, then totals
    python docs/coverage_gaps.py --file Trap     # one file
    python docs/coverage_gaps.py --missing plan  # only files with no plan

Example:

    python docs/coverage_gaps.py --file Trap
      Trap                 plan yes   probe no    docs 4/11

Reading a row: the file is named by a mutation plan, holds at least one assert
that no probe targets, and 4 of its 11 required declarations carry a comment.

Falsified 2026-09-29, each column driven from absent to present and back, with
the tree restored byte for byte:

  - plan:  a throwaway plan naming src/Persistent.cpp turned "plan no" into
           "plan yes"; deleting it turned it back.
  - probe: an entry for src/Trap.cpp in assert_probes.ps1 turned "probe no" into
           "probe yes".
  - docs:  stripping the comment lines from src/Trap.h took it from 6/15 to
           0/15.

Without those three the report could have been printing constants.
"""

import argparse
import json
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).parent))

ROOT = pathlib.Path(__file__).resolve().parent.parent


def files_named_by_plans():
    """Every source file stem that a mutation plan touches."""
    named = set()
    for plan in (ROOT / "tests").rglob("*mutations*.json"):
        for mutation in json.loads(plan.read_text(encoding="utf-8"))["mutations"]:
            named.add(pathlib.Path(mutation["file"]).stem)
    return named


def files_named_by_probes():
    """Every source file stem that assert_probes.ps1 targets."""
    harness = (ROOT / "tests/assert_probes.ps1").read_text(encoding="utf-8")
    return set(re.findall(r'file\s*=\s*"src/(\w+)\.cpp"', harness))


def files_holding_asserts():
    """Every source file stem with at least one assert to falsify."""
    holding = set()
    for source in (ROOT / "src").glob("*.cpp"):
        if "assert(" in source.read_bytes().decode("utf-8", "replace"):
            holding.add(source.stem)
    return holding


def documentation_by_stem():
    """Required declarations and how many carry a comment, per file stem.

    Imported from api_surface so "required" and "documented" mean here exactly
    what they mean in the coverage check that already exists.
    """
    from api_surface import collect

    counts = {}
    for entry in collect(ROOT / "src"):
        stem = pathlib.Path(entry["file"]).stem
        required = [d for d in entry["declarations"] if d["required"]]
        documented = sum(1 for d in required if d["comment"])
        written, total = counts.get(stem, (0, 0))
        counts[stem] = (written + documented, total + len(required))
    return counts


def rows():
    """One row per source file, worst first."""
    planned = files_named_by_plans()
    probed = files_named_by_probes()
    asserting = files_holding_asserts()
    documentation = documentation_by_stem()

    collected = []
    for source in sorted((ROOT / "src").glob("*.cpp")):
        stem = source.stem
        written, total = documentation.get(stem, (0, 0))
        collected.append({
            "file": stem,
            "plan": stem in planned,
            "probe": None if stem not in asserting else stem in probed,
            "documented": written,
            "required": total,
        })

    def gap(row):
        share = row["documented"] / row["required"] if row["required"] else 1.0
        return (row["plan"], row["probe"] is not False, share)

    return sorted(collected, key=gap)


def describe(row):
    probe = "n/a" if row["probe"] is None else ("yes" if row["probe"] else "no ")
    share = f'{row["documented"]}/{row["required"]}' if row["required"] else "-"
    return f'  {row["file"]:<24} plan {"yes" if row["plan"] else "no "}  probe {probe}  docs {share}'


def main():
    parser = argparse.ArgumentParser(description="Per-file mutation, assert-probe and documentation gaps.")
    parser.add_argument("--file", default="", help="report one file by stem")
    parser.add_argument("--missing", default="", choices=["", "plan", "probe", "docs"], help="only files missing this")
    arguments = parser.parse_args()

    collected = rows()

    if arguments.file:
        collected = [row for row in collected if row["file"] == arguments.file]
        if not collected:
            print(f"no source file named {arguments.file}")
            return 2
    if arguments.missing == "plan":
        collected = [row for row in collected if not row["plan"]]
    if arguments.missing == "probe":
        collected = [row for row in collected if row["probe"] is False]
    if arguments.missing == "docs":
        collected = [row for row in collected if row["documented"] < row["required"]]

    for row in collected:
        print(describe(row))

    everything = rows()
    noPlan = sum(1 for row in everything if not row["plan"])
    noProbe = sum(1 for row in everything if row["probe"] is False)
    written = sum(row["documented"] for row in everything)
    required = sum(row["required"] for row in everything)
    # api_surface counts every header; this attributes declarations to the .cpp
    # beside them, so header-only files are named apart and the two totals add up
    # rather than appearing to disagree.
    from api_surface import collect
    everyRequired = sum(1 for entry in collect(ROOT / "src")
                        for declaration in entry["declarations"] if declaration["required"])

    print(f"\n{len(everything)} source files: {noPlan} with no mutation plan, "
          f"{noProbe} holding unprobed asserts, {written} of {required} declarations documented")
    print(f"{everyRequired - required} more required declarations sit in headers with no .cpp "
          f"beside them, which is why api_surface reports {everyRequired}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
