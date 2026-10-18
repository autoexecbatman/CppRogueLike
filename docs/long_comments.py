"""Finds comments inside function bodies that run to three lines or more.

The owner's rule, 2026-10-18: "In function comments should be limited to 1-2 lines max with
a strong preference for short one liners." A header block above a function and a file's
opening block may be longer; only a comment inside a body is held to two lines.

    python docs/long_comments.py                    # per file, worst first, then the total
    python docs/long_comments.py --list             # every run as file:line, with its length
    python docs/long_comments.py --file other.cpp   # read that file instead of src/
    python docs/long_comments.py --compare          # runs a brace-depth count would add, and why
    python docs/long_comments.py --self-test        # the fixture must give exactly its runs

It exits 1 when it finds a run and 0 when it finds none. It reports and does not gate; whether
it gates is the owner's call once the count is 0.

Example, on a file holding one long block in a body:

    $ python docs/long_comments.py --list --file fixture.cpp
    fixture.cpp:16  3 lines
    1 run in 1 file

HOW A BODY IS TOLD FROM A SCOPE. Brace depth alone cannot do it: a block above a function in
a namespace is at depth 1, and so is a block inside a file-scope function. So each `{` is
classified by the code before it, back to the last `;` or the last body or scope brace, with
strings and comments blanked first so that a brace inside "..." moves nothing:

    ) then qualifiers (const, noexcept, override, final,
      mutable, -> type, requires ...)                   a function body, or a lambda's
    ) : initializer list, ending in } or )              a constructor body
    ] alone, as in [&] {                                a lambda body
    anything else                                       not a body: a namespace, a class,
                                                        = { or x{ 0 }

A brace that is not a body does not end the statement, so `Widget() : count{ 1 } {` still
reads as a constructor. A `//` line is in a body when any brace around it is a body, so a
lambda, a case block or an initializer inside a function counts. A run is consecutive lines
that hold only a `//` comment; a code line, a blank line or a trailing comment ends it.

FALSIFIED, 2026-10-18, against docs/long_comments_fixture.cpp.txt, which --self-test runs
after any change to the classifier. It holds 11 runs that must report and 9 that must not.
Report: a free function, a body after strings holding braces, an inline member, a
constructor after an initializer list, a namespace-scope lambda with and without a parameter
list, a lambda in a function, a case block, a lambda passed as a call argument, an
initializer inside a body, and a [&] lambda. Silent: a file opening block, a namespace
header block, a class header block, class-body blocks above a member, a `= default` and a
`= 0`, a file-scope initializer and the comment inside it, and a /* */ block. Each rule was
then broken one at a time in a copy, and --self-test failed every time:

    ) qualifiers not a body              8 of 11 missed
    ) : initializer list not a body      the constructor body missed
    ] alone not a body                   the namespace-scope [] lambda missed
    a non-body brace ends the statement  the constructor body missed
    every brace a body                   7 silent blocks reported
    strings not blanked                  6 silent blocks reported
    only the innermost brace decides     the case block and the initializer in a body missed

A separate scope rule for namespace, class and enum was written first and removed: breaking
it changed nothing, because none of those tails ends in `)` or `]`.

WHAT IT CANNOT DO. It reads text, not a parse tree. A macro that expands to a brace, or a
function whose `)` and `{` are separated by something it does not know as a qualifier, is
misread; the qualifier list is the place to extend. A comment that should be one line but is
two is not its business.
"""

import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE = ROOT / "src"
LONGEST_ALLOWED = 2

# The fixture's REPORT lines; every other run in it, or a missing one, fails --self-test.
FIXTURE = pathlib.Path(__file__).resolve().parent / "long_comments_fixture.cpp.txt"
FIXTURE_RUNS = (16, 21, 50, 58, 69, 77, 99, 108, 120, 128, 136)

QUALIFIERS = r"(?:\s*(?:const|noexcept(?:\s*\([^()]*\))?|override|final|mutable|&&|&|->\s*[\w:<>,\s*&]+|requires\b.*))*\s*$"
FUNCTION_TAIL = re.compile(r"\)" + QUALIFIERS)
CONSTRUCTOR_TAIL = re.compile(r"\)\s*(?:noexcept\s*)?:\s*[A-Za-z_]")
LAMBDA_TAIL = re.compile(r"\]\s*$")


def blank_literals(text):
    """Returns the text with string, char and raw-string literals and /* */ comments blanked,
    and // comments kept, so that lines and columns are unchanged.

    Example:
        blank_literals('x = "{";')  # -> 'x = "   ";' ... the brace no longer counts
    """
    out = []
    index = 0
    length = len(text)
    while index < length:
        char = text[index]
        if text.startswith("//", index):
            end = text.find("\n", index)
            end = length if end == -1 else end
            out.append(text[index:end])
            index = end
        elif text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = length if end == -1 else end + 2
            out.append(re.sub(r"[^\n]", " ", text[index:end]))
            index = end
        elif char == "R" and index + 1 < length and text[index + 1] == '"':
            opener = text.find("(", index + 2)
            delimiter = text[index + 2:opener]
            end = text.find(")" + delimiter + '"', opener)
            end = length if end == -1 else end + len(delimiter) + 2
            out.append('R"' + re.sub(r"[^\n]", " ", text[index + 2:end - 1]) + '"')
            index = end
        elif char in "\"'":
            end = index + 1
            while end < length and text[end] != char and text[end] != "\n":
                end += 2 if text[end] == "\\" else 1
            end = min(end + 1, length)
            out.append(char + " " * (end - index - 2) + char)
            index = end
        else:
            out.append(char)
            index += 1
    return "".join(out)


def classify(statement):
    """Says what the brace after this statement opens: "body", or "other" for a namespace,
    a class or an initializer, none of which is a body.

    Example:
        classify("int f(int x) const")  # -> "body"
        classify("namespace fixture")   # -> "other"
        classify("std::array t =")      # -> "other"
    """
    tail = statement.strip()
    # In an initializer list a brace after a member's name is that member's; the body follows
    # the last initializer's closing } or ).
    if CONSTRUCTOR_TAIL.search(tail) and tail.endswith(("}", ")")):
        return "body"
    if FUNCTION_TAIL.search(tail) or LAMBDA_TAIL.search(tail):
        return "body"
    return "other"


def in_body_lines(text):
    """Returns the set of line numbers, counted from 1, that sit inside a function body.

    Example:
        in_body_lines("int f()\\n{\\n\\treturn 0;\\n}\\n")  # -> {3}, the line inside the braces
    """
    code = blank_literals(text)
    # Comments are removed from what classifies a brace, so a // { cannot open one.
    code = re.sub(r"//[^\n]*", "", code)
    inside = set()
    stack = []
    statement = []
    line = 1
    for char in code:
        if char == "\n":
            if any(kind == "body" for kind in stack):
                inside.add(line + 1)
            line += 1
            statement.append(" ")
            continue
        if char == "{":
            kind = classify("".join(statement))
            stack.append(kind)
            # A body ends the statement; any other brace is part of it, as x{ 0 } is.
            if kind == "body":
                statement = []
            else:
                statement.append("{")
        elif char == "}":
            kind = stack.pop() if stack else "other"
            if kind == "body":
                statement = []
            else:
                statement.append("}")
        elif char == ";":
            statement = []
        else:
            statement.append(char)
    return inside


def long_runs(text):
    """Returns (first line, length) for every run of three or more // lines in a body.

    Example:
        long_runs(fixture_text)  # -> [(16, 3), (21, 3), ...]
    """
    body = in_body_lines(text)
    runs = []
    start = None
    count = 0
    for number, line in enumerate(text.splitlines() + [""], 1):
        if line.strip().startswith("//") and number in body:
            if start is None:
                start = number
            count += 1
            continue
        if start is not None and count > LONGEST_ALLOWED:
            runs.append((start, count))
        start = None
        count = 0
    return runs


def depth_runs(text):
    """Returns (first line, length) for every run of three or more // lines at brace depth 1 or
    more, the count this script replaces, so --compare can show what it rejects.

    Example:
        depth_runs("namespace n\\n{\\n// a\\n// b\\n// c\\nint f();\\n}\\n")  # -> [(3, 3)]
    """
    code = re.sub(r"//[^\n]*", "", blank_literals(text))
    depth_at = {}
    depth = 0
    line = 1
    for char in code:
        if char == "\n":
            line += 1
            depth_at[line] = depth
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
    runs = []
    start = None
    count = 0
    for number, source_line in enumerate(text.splitlines() + [""], 1):
        if source_line.strip().startswith("//") and depth_at.get(number, 0) >= 1:
            if start is None:
                start = number
            count += 1
            continue
        if start is not None and count > LONGEST_ALLOWED:
            runs.append((start, count))
        start = None
        count = 0
    return runs


def self_test():
    """Runs the fixture and returns 0 when it reports exactly FIXTURE_RUNS, 1 otherwise.

    Example:
        self_test()  # prints "self-test: 11 of 11 runs, none missed, none extra" -> 0
    """
    reported = {start for start, _ in long_runs(FIXTURE.read_text(encoding="utf-8"))}
    missed = sorted(set(FIXTURE_RUNS) - reported)
    extra = sorted(reported - set(FIXTURE_RUNS))
    if missed or extra:
        print(f"self-test FAILED: missed {missed}, extra {extra}")
        return 1
    print(f"self-test: {len(reported)} of {len(FIXTURE_RUNS)} runs, none missed, none extra")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--list", action="store_true", help="print every run as file:line")
    parser.add_argument("--file", action="append", default=[], help="read this file instead of src/")
    parser.add_argument("--compare", action="store_true", help="runs a brace-depth count would add")
    parser.add_argument("--self-test", action="store_true", help="check the fixture's exact runs")
    arguments = parser.parse_args()
    if arguments.self_test:
        return self_test()

    paths = [pathlib.Path(name) for name in arguments.file] or sorted(
        path for path in SOURCE.iterdir() if path.suffix in (".cpp", ".h"))
    found = {}
    rejected = []
    for path in paths:
        text = path.read_text(encoding="utf-8")
        runs = long_runs(text)
        if runs:
            found[path.name] = runs
        if arguments.compare:
            kept = {start for start, _ in runs}
            rejected += [(path.name, start, length) for start, length in depth_runs(text) if start not in kept]

    if arguments.list:
        for name, runs in sorted(found.items()):
            for start, length in runs:
                print(f"{name}:{start}  {length} lines")
    else:
        for name, runs in sorted(found.items(), key=lambda item: (-len(item[1]), item[0])):
            print(f"{len(runs):4}  {name}")
    total = sum(len(runs) for runs in found.values())
    print(f"{total} run{'s' if total != 1 else ''} in {len(found)} file{'s' if len(found) != 1 else ''}")

    if arguments.compare:
        print(f"{len(rejected)} runs a brace-depth count would add, none of them in a body:")
        for name, start, length in rejected[:3]:
            print(f"  {name}:{start}  {length} lines")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
