"""Finds halves of pairs that are missing, which no other instrument here can see.

The other four instruments measure what exists: coverage_gaps.py reports assertions
that exist and lack probes, api_surface.py reports declarations that exist and lack
comments, reachability.py reports types that exist and are unused, and mutate.py
mutates code that exists. None of them can report something that should exist and does
not, and that is where this project's expensive defects have been living - a saver that
did not match its loader, a constructor whose invariant only its readers asserted, a
forward layout function whose inverse three callers each re-derived.

Two checks, both mechanical. Neither decides whether a counterpart ought to exist -
that is judgment, and a checker that cries wolf gets ignored. They report asymmetry in
things that are already named, and reading settles the rest.

    python docs/pair_symmetry.py                 # both checks, worst first
    python docs/pair_symmetry.py --verbs         # only the unpaired-verb check
    python docs/pair_symmetry.py --guards        # only the guarded-but-unasserted check
    python docs/pair_symmetry.py --min-guards 3  # raise the bar on the second check

Output is a report, never a gate. Both counts are far from zero and a failing exit
would be ignored within a day.

WHAT IT CANNOT DO, stated here so nobody trusts it for this: it cannot find the
function nobody wrote. panel_text_row_at_y was missing for months and no tool could
have named it, because nothing referred to it. Only asking "what is the other half of
this" while writing the forward half finds that one.

CHECK ONE - unpaired verb. A function whose name starts with one half of a known verb
pair, where no function in the tree carries the other half with the same stem.
save_inventory with no load_inventory is the shape; it is how a saver and a loader come
to disagree about what a null means.

CHECK TWO - guarded but not asserted. A member that two or more sites null-check, and
that no assert anywhere mentions. That is the EquippedItem shape: ten readers reaching
through one pointer, four asserting, three checking defensively, three bare, and the
one place that could have established the invariant - the constructor - silent. A
member that is legitimately optional shows up here too, which is why it reports rather
than gates: ctx.renderer is guarded in code that can run headless and is correct.

FALSIFICATIONS, run 2026-10-01 before either zero was trusted. Both checks were driven
from absent to present and back; see the issue that introduced this file for the exact
edits.

Neither check reads a build. Both read src/*.cpp and src/*.h as text.
"""

import argparse
import pathlib
import re
import sys
from collections import defaultdict

# Verb pairs worth checking, as (writer, reader) and checked in that direction only.
# A writer with no reader is data written and never read back, which is a real defect.
# A reader with no writer is ordinarily correct: the six ability tables are reference
# data this game only ever reads, so load_charisma has no save_charisma by design.
#
# Deliberately short, and shorter than the first draft. These were tried and removed
# on 2026-10-01 because every hit was noise:
#
#   create/destroy   - a C idiom. RAII means nothing here has a destroy, so all
#                      eleven create_* functions reported and none was a finding.
#   acquire/release  - acquire_nearest is target acquisition, not ownership.
#   enter/exit       - exit_column is a map exit.
#   open/close, push/pop, show/hide, begin/end - all carry a common meaning here that
#                      has no counterpart, and std container vocabulary besides.
#
# And encode/decode was simply the wrong vocabulary: this tree pairs encode_X with
# parse_X, so all twenty-two encode_* functions reported while their readers sat in
# the same header. That was a bug in this checker, not a finding.
VERB_PAIRS = [
    ("save", "load"),
    ("serialize", "deserialize"),
    ("encode", "parse"),
    ("register", "unregister"),
    ("attach", "detach"),
    ("lock", "unlock"),
]

# A name used as a callable: an identifier followed by an open parenthesis, not
# preceded by a dot or arrow, which would make it a call on something else.
FUNCTION_NAME = re.compile(r"(?<![\w.>])([a-z_][a-z0-9_]{2,})\s*\(")

# The four shapes a null check takes in this tree.
GUARD_PATTERNS = [
    re.compile(r"if\s*\(\s*!\s*([A-Za-z_][\w.]*(?:->[\w.]+)*)\s*[)&|]"),
    re.compile(r"if\s*\(\s*([A-Za-z_][\w.]*(?:->[\w.]+)*)\s*&&"),
    re.compile(r"if\s*\(\s*([A-Za-z_][\w.]*(?:->[\w.]+)*)\s*==\s*nullptr\s*\)"),
    re.compile(r"if\s*\(\s*([A-Za-z_][\w.]*(?:->[\w.]+)*)\s*!=\s*nullptr\s*\)"),
]

# Proof the guarded thing is a pointer rather than a bool flag.
POINTER_USE = r"\b{}\s*->"


def source_files(root):
    """Every C++ source and header directly in src/, which is flat by design."""
    return sorted(root.glob("*.cpp")) + sorted(root.glob("*.h"))


def read_lines(path):
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def strip_comment(line):
    """The code half of a line. Loops and guards inside file blocks are prose."""
    cut = line.find("//")
    return line if cut < 0 else line[:cut]


def collect_function_names(files):
    """Every lowercase identifier used as a callable, with where it was first seen."""
    seen = {}
    for path in files:
        for number, line in enumerate(read_lines(path), 1):
            for match in FUNCTION_NAME.finditer(strip_comment(line)):
                seen.setdefault(match.group(1), (path.name, number))
    return seen


def unpaired_verbs(names):
    """Writers whose matching reader is nowhere in the tree.

    Checked one way only. A writer with no reader means something is written and
    never read back; a reader with no writer is ordinarily reference data.
    """
    findings = []
    for writer, reader in VERB_PAIRS:
        prefix = writer + "_"
        for name, (filename, number) in names.items():
            if not name.startswith(prefix):
                continue
            stem = name[len(prefix):]
            if not stem or f"{reader}_{stem}" in names:
                continue
            findings.append((name, f"{reader}_{stem}", filename, number))
    return sorted(findings)


def disagreeing_readers(files, minimum_guards):
    """Members that some sites assert and other sites null-check.

    Absence was the first design here and it did not work: keyed on a bare member
    name, `item` counted as asserted because some unrelated `assert(item && ...)`
    existed elsewhere, so deleting EquippedItem's constructor assertion changed
    nothing in the report. Disagreement is the signal that actually found that defect
    by hand - four readers asserting, three guarding, three bare, over one pointer.

    A member every site guards is reported by nothing here, and that is correct: a
    weapon a creature may not be holding is legitimately absent, and consistency is
    the evidence it is intended.
    """
    guards = defaultdict(list)
    asserts = defaultdict(list)

    for path in files:
        for number, line in enumerate(read_lines(path), 1):
            code = strip_comment(line)
            if re.match(r"\s*assert\s*\(", code):
                for expression in re.findall(r"[A-Za-z_][\w.]*(?:->[\w.]+)*", code):
                    member = re.split(r"\.|->", expression)[-1]
                    if len(member) >= 3:
                        asserts[member].append(f"{path.name}:{number}")
                continue
            for pattern in GUARD_PATTERNS:
                for match in pattern.finditer(code):
                    # The member is the last component: a guard on ctx.renderer and one
                    # on renderer are a guard on the same thing.
                    member = re.split(r"\.|->", match.group(1))[-1]
                    if len(member) >= 3:
                        guards[member].append(f"{path.name}:{number}")

    # Only things used as pointers. A bool flag guarded twice is not this defect.
    whole = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in files)

    findings = []
    for member, guard_sites in guards.items():
        assert_sites = asserts.get(member, [])
        if not assert_sites or len(guard_sites) < minimum_guards:
            continue
        if not re.search(POINTER_USE.format(re.escape(member)), whole):
            continue
        findings.append((len(guard_sites) + len(assert_sites), member, guard_sites, assert_sites))
    return sorted(findings, reverse=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--verbs", action="store_true", help="only the unpaired-verb check")
    parser.add_argument("--guards", action="store_true", help="only the disagreeing-readers check")
    parser.add_argument("--min-guards", type=int, default=2, help="guard sites before reporting (default 2)")
    parser.add_argument("--root", default="src", help="directory to read (default src)")
    arguments = parser.parse_args()

    root = pathlib.Path(arguments.root)
    if not root.is_dir():
        print(f"no such directory: {root}", file=sys.stderr)
        return 2

    files = source_files(root)
    run_verbs = arguments.verbs or not arguments.guards
    run_guards = arguments.guards or not arguments.verbs

    print(f"pair symmetry over {len(files)} files in {root}/")

    if run_verbs:
        findings = unpaired_verbs(collect_function_names(files))
        print(f"\nUNPAIRED VERB: {len(findings)}")
        for name, wanted, filename, number in findings:
            print(f"  {filename}:{number}  {name}  has no  {wanted}")

    if run_guards:
        findings = disagreeing_readers(files, arguments.min_guards)
        print(f"\nREADERS DISAGREE: {len(findings)} members asserted at some sites and guarded at others")
        for _, member, guard_sites, assert_sites in findings:
            print(f"  {member}  asserted {len(assert_sites)}x, guarded {len(guard_sites)}x")
            print(f"      asserts: {', '.join(assert_sites[:4])}{' ...' if len(assert_sites) > 4 else ''}")
            print(f"      guards:  {', '.join(guard_sites[:4])}{' ...' if len(guard_sites) > 4 else ''}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
