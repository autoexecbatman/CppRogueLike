"""Refuses mutation-plan entries whose mutated text holds a token no edit meant.

A repair that moves an anchor can drop the whitespace between two tokens.
`return true` then becomes `returntrue` and `{ 18, 51 }` becomes
`{ 18,5 1 }`; neither mutant compiles, and a sweep reads a failed build as a
caught mutant. This reads the plans and the sources only, so it needs no
snapshot of the plans before the repair and no build.

    python docs/fused_tokens.py                  # every plan in the tree
    python docs/fused_tokens.py --plans DIR      # plans under DIR instead
    python docs/fused_tokens.py --self-test      # the fixture's findings

It exits 1 when it refuses an entry and 0 when it refuses none. Only entries
on .cpp and .h files are read, and string and char literals and // comments
are blanked first. Three rules, on the entry's mutated text:

    unknown   an identifier no source in src/ or tests/ holds, that the
              entry's original does not hold, and that it does not declare
    split     two numbers with only whitespace between them
    stale     a name in the entry's "introduces" list the text does not hold

A mutant that brings in a name on purpose declares it in its plan entry,
beside the edit it explains: "introduces": ["anyBuff"]. mutate.py and
MutationPlanTest read only the keys they name, so the field changes neither.

Example, over the plans as they stood before the 2b anchor repair was carried
again, and over the tree at this commit:

    $ python docs/fused_tokens.py --plans plans_before_recarry
    tests/Actor/carry_limit_mutations.json #2  unknown 'returnitem'
    ...
    tests/Factories/monster_ability_table_mutations.json #1  split '5 1'
    ...
    1618 entries, 30 refused
    $ python docs/fused_tokens.py
    1648 entries, 0 refused

The functions, for a caller other than main: identifiers(text) reads names
with literals blanked, source_identifiers(root) collects the known ones,
entry_findings(entry, known) applies the rules to one entry, and
findings(plans_root, known) applies them to every plan.

FALSIFIED, 2026-10-23, against docs/fused_tokens_fixture.json, read by
--self-test with the known names taken from a temporary tree: the two
fixture sources as a src/ and a tests/ file, and a src/ note holding
returntrue, which must stay unknown. The red step, an entry_findings that
returned nothing, missed all 5 findings. Silent cases: a declared name, a
name from the entry's original, a name only the tests/ half defines,
returntrue inside a string and inside a comment, 18, 51, "18 5" inside a
string, and a Python entry from a docs/ plan. Every rule is then broken by
docs/fused_tokens_mutations.json through mutate.py: 12 of 12 caught. On real
data it refuses the 30 entries of the plans before the re-carry: 23 fused
identifiers that could not compile, monster_ability_table #1, and the six
deliberate names that were then undeclared.

WHAT IT CANNOT SEE. A space lost inside a string literal, the shape of two of
the 2b repair's 25, which only a token compare against a snapshot finds. A
fusion that happens to form a name the sources hold. A mutant on a file that
is not C++.
"""

import argparse
import dataclasses
import json
import pathlib
import re
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from dispatch_audit import blank

ROOT = pathlib.Path(__file__).resolve().parent.parent
DOCS = pathlib.Path(__file__).resolve().parent
FIXTURE_PLAN = DOCS / "fused_tokens_fixture.json"
FIXTURE_SOURCE = DOCS / "fused_tokens_fixture.cpp.txt"
FIXTURE_TEST_SOURCE = DOCS / "fused_tokens_fixture_test.cpp.txt"
FIXTURE_FINDINGS = {
    (1, "unknown", "returntrue"),
    (2, "split", "5 1"),
    (3, "unknown", "conststd"),
    (4, "stale", "anyBuff"),
    (13, "unknown", "total2"),
}


@dataclasses.dataclass(frozen=True)
class Finding:
    """One refused entry: its plan, its 1-based number, the rule, the token."""

    plan: str
    number: int
    rule: str
    token: str


IDENTIFIER = re.compile(r"\b[A-Za-z_]\w*\b")
SPLIT_NUMBER = re.compile(r"\b\d+\s+\d+\b")
CODE_FILE = re.compile(r".*\.(?:cpp|h)")


def identifiers(text):
    """The identifiers in text, with literals and comments blanked first.

    Example:
        identifiers('log("returntrue"); // conststd')   # -> {'log'}
    """
    return set(IDENTIFIER.findall(blank(text)))


def source_identifiers(root):
    """Every identifier in the .cpp and .h files under root's src/ and tests/.

    Example:
        len(source_identifiers(ROOT))   # -> 9350 at bdd41e64
    """
    known = set()
    for folder in ("src", "tests"):
        for path in sorted((root / folder).rglob("*")):
            if CODE_FILE.fullmatch(path.name):
                known |= identifiers(path.read_bytes().decode("utf-8"))
    return known


def entry_findings(entry, known):
    """(rule, token) for each way one entry's mutated text is refused.

    An identifier is refused when nothing knows it: not the sources, not the
    entry's own original, and not the entry's "introduces" list. A declared
    name the mutated text does not hold is refused as stale. Only entries on
    .cpp and .h files are read.

    Example:
        entry = {"file": "src/A.cpp", "original": "\\treturn false;",
                 "mutated": "\\treturntrue;"}
        entry_findings(entry, {"return", "true", "false"})
        # -> [('unknown', 'returntrue')]
        entry["introduces"] = ["returntrue"]
        entry_findings(entry, {"return", "true", "false"})   # -> []
    """
    if not CODE_FILE.fullmatch(entry["file"]):
        return []
    declared = entry.get("introduces", [])
    mutated_names = identifiers(entry["mutated"])
    allowed = known | identifiers(entry["original"]) | set(declared)
    found = [("unknown", name) for name in sorted(mutated_names - allowed)]
    found += [("split", pair)
              for pair in SPLIT_NUMBER.findall(blank(entry["mutated"]))]
    # A declaration must name something the mutant still introduces.
    found += [("stale", name) for name in declared
              if name not in mutated_names]
    return found


def plan_paths(root):
    """Every mutation plan under root's tests/ and docs/, in sorted order."""
    return sorted(list((root / "tests").rglob("*_mutations.json"))
                  + list((root / "docs").glob("*_mutations.json")))


def findings(plans_root, known):
    """Every refused entry in the plans under plans_root.

    Example:
        findings(ROOT, source_identifiers(ROOT))   # -> [] at this commit
    """
    found = []
    for path in plan_paths(plans_root):
        plan = json.loads(path.read_bytes().decode("utf-8"))
        name = path.relative_to(plans_root).as_posix()
        for number, entry in enumerate(plan["mutations"], start=1):
            for rule, token in entry_findings(entry, known):
                found.append(Finding(name, number, rule, token))
    return found


def self_test():
    """Returns 0 when the fixture gives exactly FIXTURE_FINDINGS, else 1.

    The known identifiers come from a temporary tree, not from the checkout:
    the two fixture sources as src/ and tests/ files, and a note in src/ that
    names returntrue, which must stay unknown because it is not C++.

    Example:
        self_test()
        # prints "self-test: 4 of 4 findings, none missed, none extra" -> 0
    """
    with tempfile.TemporaryDirectory() as folder:
        root = pathlib.Path(folder)
        (root / "src").mkdir()
        (root / "tests").mkdir()
        (root / "src" / "Fixture.cpp").write_bytes(FIXTURE_SOURCE.read_bytes())
        (root / "tests" / "FixtureTest.cpp").write_bytes(
            FIXTURE_TEST_SOURCE.read_bytes())
        (root / "src" / "notes.md").write_bytes(b"returntrue conststd\n")
        known = source_identifiers(root)
    plan = json.loads(FIXTURE_PLAN.read_bytes().decode("utf-8"))
    reported = {(number, rule, token)
                for number, entry in enumerate(plan["mutations"], start=1)
                for rule, token in entry_findings(entry, known)}
    missed = sorted(FIXTURE_FINDINGS - reported)
    extra = sorted(reported - FIXTURE_FINDINGS)
    if missed or extra:
        print(f"self-test FAILED: missed {missed}, extra {extra}")
        return 1
    print(f"self-test: {len(FIXTURE_FINDINGS)} of {len(FIXTURE_FINDINGS)} "
          "findings, none missed, none extra")
    return 0


def main():
    """Checks the plans the command line names and returns the exit code.

    Example:
        main()   # with no arguments, at this commit, prints the totals -> 0
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--plans", help="read the plans under this folder")
    parser.add_argument("--self-test", action="store_true",
                        help="check the fixture's exact findings")
    arguments = parser.parse_args()
    if arguments.self_test:
        return self_test()
    plans_root = pathlib.Path(arguments.plans) if arguments.plans else ROOT
    found = findings(plans_root, source_identifiers(ROOT))
    for finding in found:
        print(f"{finding.plan} #{finding.number}  {finding.rule} "
              f"{finding.token!r}")
    entries = sum(len(json.loads(path.read_bytes().decode("utf-8"))
                      ["mutations"]) for path in plan_paths(plans_root))
    refused = len({(finding.plan, finding.number) for finding in found})
    print(f"{entries} entries, {refused} refused")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
