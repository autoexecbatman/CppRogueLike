"""Finds functions that dereference a GameContext member they never assert.

The house rule is to assert an invariant before reaching through it, and every member
of GameContext is a pointer a caller may have left unwired. This reads every .cpp
directly in src/ and reports each (function, member) pair where the function's body
dereferences `ctx.<member>` and nowhere asserts it. It is the count behind
issues/0194's bare-dereference half.

    python docs/bare_derefs.py            # the totals, per member, most first
    python docs/bare_derefs.py --list     # the same, then one line per pair

The totals open with the number of pairs, then one line per member:

    560 (function, member) pairs dereferenced with no assert in the function
      messageSystem          103

With --list, each pair is one line: the file and the line of the function's head, the
member, and the head itself.

    Actor.cpp:94  renderer             void Actor::render(const GameContext& ctx) const noexcept

There is no --check mode. A gate exits 0 on a clean tree, and this tree is far from
clean, so a gate would only ever be red; the count is reported, not enforced.

HOW IT READS. A function body runs from a `{` alone at column 0 to the next `}` alone at
column 0, which is how this tree's Allman style lays out every definition, inside
namespaces too. Its head is the last line before that `{` that starts with a letter and
holds a `(` with no `;` before it. Within a body, a dereference is `ctx.<member>->` or
`*ctx.<member>` not followed by a call, and an assertion is `assert(ctx.<member>`. A pair
with a dereference and no assertion is a finding.

WHAT IT CANNOT SEE. An assertion in a helper the function calls, or an invariant set up
at construction; a context reached under any name but `ctx`; a lambda, which is counted
as part of the function holding it; and headers, whose inline bodies are not read. It
reads text, not a build, and does not skip comments, so a dereference written in a
comment counts as one and an assert written in a comment hides one. Every count is a
floor for what it finds and a ceiling for what it reports clean.

FALSIFICATION, run 2026-10-17 before the count was trusted. The tree at af4d30ee
reported 560 pairs, none of them for TrapDisarm::disarm_at. With src/Trap.cpp copied
aside and its line `assert(ctx.dice && "TrapDisarm::disarm_at: called without dice");`
deleted, the report rose to 561 and --list showed the new pair:

    Trap.cpp:312  dice                 std::optional<int> TrapDisarm::disarm_at(Player& thief, Vector2D target, GameContext& ctx)

The file restored from the copy, byte-identical by cmp, the report was 560 again with
that pair gone.
"""

import argparse
import collections
import pathlib
import re

DEREFERENCE = re.compile(r"(?:\bctx\.([A-Za-z_]\w*)\s*->|\*\s*ctx\.([A-Za-z_]\w*)\b(?!\s*\())")
ASSERTION = re.compile(r"assert\(\s*ctx\.([A-Za-z_]\w*)\b")
HEAD = re.compile(r"^[A-Za-z][^;]*\(")


def functions(lines):
    """Yields each function's head and body lines, by the column-0 braces.

    Example:
        list(functions(["int f()", "{", "\\treturn 1;", "}"]))
        # -> [((0, 'int f()'), ['{', '\\treturn 1;', '}'])]
        list(functions(["int x = 1;"]))   # -> [], no body
    """
    head = None
    start = None
    for number, line in enumerate(lines):
        # Outside a body: remember the latest head, and open on a lone '{'.
        if start is None:
            if HEAD.match(line):
                head = (number, line.strip())
            elif line == "{" and head is not None:
                start = number
        # Inside a body: a lone '}' closes it.
        elif line == "}":
            yield head, lines[start:number + 1]
            start = None
            head = None


def findings_in(path):
    """Returns (file name, head line, head text, member) for each bare pair in a file.

    Example, a file holding only:
        void draw(GameContext& ctx)
        {
            ctx.renderer->draw();
        }
    findings_in(path)   # -> [('Draw.cpp', 1, 'void draw(GameContext& ctx)', 'renderer')]
    The same file with `assert(ctx.renderer && "...");` in the body returns [].
    """
    lines = path.read_text(encoding="utf-8").replace("\r\n", "\n").split("\n")
    found = []
    for (head_line, head_text), body in functions(lines):
        text = "\n".join(body)
        # Either group of the pattern holds the member, depending on which form matched.
        dereferenced = {arrow or star for arrow, star in DEREFERENCE.findall(text)}
        asserted = set(ASSERTION.findall(text))
        for member in sorted(dereferenced - asserted):
            found.append((path.name, head_line + 1, head_text[:90], member))
    return found


def main():
    """Prints the totals for src/, and every pair with --list."""
    parser = argparse.ArgumentParser(description="GameContext members dereferenced with no assert.")
    parser.add_argument("--list", action="store_true", help="also print one line per pair")
    arguments = parser.parse_args()

    # src/ is the sibling of this script's folder, wherever it is run from.
    source = pathlib.Path(__file__).resolve().parent.parent / "src"
    findings = []
    for path in sorted(source.glob("*.cpp")):
        findings.extend(findings_in(path))

    by_member = collections.Counter(member for *_, member in findings)
    print(f"{len(findings)} (function, member) pairs dereferenced with no assert in the function")
    for member, count in by_member.most_common():
        print(f"  {member:<22} {count}")
    if arguments.list:
        for name, line, head, member in findings:
            print(f"{name}:{line}  {member:<20} {head}")


if __name__ == "__main__":
    main()
