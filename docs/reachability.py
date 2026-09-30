"""Reports types declared in src/ that nothing in src/ reaches.

Three separate finds on 2026-09-30 were the same shape: a type compiled into the
binary, carrying tests and coverage, reached by no production code. Handlers that
no caller constructs, an event route nothing subscribes to. Each time the check
that would have caught it was "grep for the production caller", and each time it
was either skipped or run with too narrow a pattern.

What it reports, per type, is where its name appears outside the .h/.cpp pair
that declares it:

  - **dead**      - nowhere in src/, and nowhere in tests/ either.
  - **tests only** - nowhere in src/ outside its own pair, but tests use it. This
                    is the worst case for a coverage programme, because the file
                    looks covered and the coverage measures nothing the game runs.
  - **self only**  - used inside its own header or .cpp and nowhere else. Often
                    legitimate: a helper a nearby inline function needs.

It reports; it does not gate. Deleting a type is a decision about the design, and
a name being absent from the tree is evidence rather than a verdict - a factory
reached only through a base pointer still has to be constructed somewhere, so it
shows up here, but a type named only in a string or a build file does not.

Usage:

    python docs/reachability.py                # every finding, worst first
    python docs/reachability.py --kind dead    # one bucket
    python docs/reachability.py --name Creature  # ask about one type

Example:

    python docs/reachability.py --name MonsterDeathHandler
    MonsterDeathHandler          dead        src 0    tests 0    own pair 0    DeathHandler.h

    python docs/reachability.py --name TrapState
    TrapState                    self only   src 0    tests 4    own pair 2    Trap.h

Reading the first row: nothing anywhere names MonsterDeathHandler - not another
file in src/, not a test, and not its own header beyond declaring it. The second
is the shape that is fine: TrapState is used inside Trap.h and Trap.cpp and by
four test files, and no other source file needs to know it exists.

Falsified 2026-09-30, each bucket driven from absent to present and back, with the
tree restored byte for byte:

  - dead:       a struct added to a header and named nowhere reported "dead";
                deleting it took it out of the report.
  - self only:  giving that same struct one use inside its own header moved it to
                "self only", which is how an encapsulated type is kept out of the
                findings.
  - tests only: a throwaway test file constructing MonsterDeathHandler moved it
                from "dead" to "tests only". That is the case this tool exists
                for, reproduced: until e530fbc the death tests named exactly that
                class and no production code did.

Both of the first classifier's buckets were wrong and the report was read before
it was believed. It called 30 types dead, of which 28 were module-internal - a
Dijkstra frontier node, PlayerController's MouseMode - because it counted a type's
own declaration and its Name::member definitions as mentions, so a type nothing
constructs looked as busy as one in constant use. It then called nine types "tests
only" that production used perfectly well inside their own file. Counting uses
rather than mentions, and asking whether production touches the type anywhere at
all, took the findings from 39 to 2 with nothing real lost.

"""

import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

# A type declaration that opens a body, so a forward declaration is not counted
# as the thing itself.
DECLARATION = re.compile(
    r"^\s*(?:class|struct|enum\s+class|enum)\s+(\w+)\s*(?:final\s*)?(?::[^;{]*)?\{",
    re.MULTILINE,
)


def declared_types():
    """Every type declared with a body in a src/ header, and the file declaring it."""
    declared = {}
    for header in sorted((ROOT / "src").glob("*.h")):
        text = header.read_text(encoding="utf-8", errors="replace")
        for name in DECLARATION.findall(text):
            declared.setdefault(name, header)
    return declared


def files_naming(name, folder, skip):
    """How many files under `folder` mention `name`, ignoring those in `skip`."""
    word = re.compile(r"\b" + re.escape(name) + r"\b")
    count = 0
    for source in folder.rglob("*"):
        if source.suffix not in {".h", ".cpp", ".ps1", ".json"}:
            continue
        if source in skip:
            continue
        if word.search(source.read_text(encoding="utf-8", errors="replace")):
            count += 1
    return count


def uses_in(paths, name):
    """How many times `name` is *used* across `paths`, rather than declared.

    A type's own files always mention it: once to declare it, and once per
    out-of-line member definition as `Name::member`. Neither is a use, and
    counting them makes a type nothing constructs look busy. What is left is a
    member of that type, a parameter, a construction - the mentions that mean
    somebody needs it.
    """
    word = re.compile(r"\b" + re.escape(name) + r"\b")
    qualified = re.compile(r"\b" + re.escape(name) + r"\s*::")
    declaration = re.compile(
        r"\b(?:class|struct|enum\s+class|enum)\s+" + re.escape(name) + r"\b")

    used = 0
    for path in paths:
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.lstrip().startswith("//"):
                continue
            if declaration.search(line):
                continue
            hits = len(word.findall(line)) - len(qualified.findall(line))
            used += max(hits, 0)
    return used


def classify(name, header):
    """Where a type is reached from, as one row."""
    own = {header, header.with_suffix(".cpp")}
    in_source = files_naming(name, ROOT / "src", own)
    in_tests = files_naming(name, ROOT / "tests", set())
    own_pair = uses_in(own, name)

    # What production does with it, wherever that happens: another file naming it,
    # or its own module using it. A type used inside its own header and nowhere
    # else is encapsulated, which is a design, not a defect.
    used_by_production = in_source + own_pair

    if used_by_production > 0:
        kind = "reachable" if in_source > 0 else "self only"
    elif in_tests > 0:
        kind = "tests only"
    else:
        kind = "dead"
    return {
        "name": name,
        "kind": kind,
        "source": in_source,
        "tests": in_tests,
        "own": own_pair,
        "header": header.name,
    }


def rows():
    """One row per declared type, worst first."""
    order = {"dead": 0, "tests only": 1, "self only": 2, "reachable": 3}
    collected = [classify(name, header) for name, header in declared_types().items()]
    collected.sort(key=lambda row: (order[row["kind"]], row["source"], row["name"]))
    return collected


def print_row(row):
    print("  %-28s %-11s src %-4d tests %-4d own pair %-4d %s"
          % (row["name"], row["kind"], row["source"], row["tests"], row["own"], row["header"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--kind", choices=["dead", "tests only", "self only", "reachable"], default=None,
                        help="only this bucket")
    parser.add_argument("--name", default=None, help="ask about one type")
    arguments = parser.parse_args()

    collected = rows()

    if arguments.name is not None:
        matching = [row for row in collected if row["name"] == arguments.name]
        if not matching:
            print("no type named %s is declared with a body in src/" % arguments.name)
            return 0
        for row in matching:
            print_row(row)
        return 0

    shown = [row for row in collected if arguments.kind is None or row["kind"] == arguments.kind]
    for row in shown:
        if arguments.kind is None and row["kind"] in {"reachable", "self only"}:
            continue
        print_row(row)

    dead = sum(1 for row in collected if row["kind"] == "dead")
    tests_only = sum(1 for row in collected if row["kind"] == "tests only")
    print()
    contained = sum(1 for row in collected if row["kind"] == "self only")
    print("%d types declared in src/ headers: %d reached by nothing, %d reached only by tests,"
          " %d used only inside their own file" % (len(collected), dead, tests_only, contained))
    return 0


if __name__ == "__main__":
    sys.exit(main())
