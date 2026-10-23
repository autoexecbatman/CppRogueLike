"""Refuses lines a diff adds that break the 80-column rules.

The owner, 2026-10-22: "80 for code and comments; comments as files are
touched". The formatter holds code to 80; this holds what it cannot. Only
lines the diff adds are read, so the long comments already in the tree wait
for their files to be edited.

    python docs/comment_width_check.py --staged          # the staged diff
    python docs/comment_width_check.py --range A..B      # git diff A..B
    python docs/comment_width_check.py --self-test       # the fixture's lines

It exits 1 when it refuses a line, 0 when it refuses none, and 2 when git
fails. Lines are read from .cpp and .h files under src/ and tests/.

Three rules, each on an added line:

    comment   it holds a // comment and is wider than 80, a tab being 4
    padded    it is indented with spaces, after tabs or alone
    list      it ends in a comma inside a { it leaves open, the shape the
              formatter gives a wrapped braced list with no trailing comma

A code line with no comment is the formatter's business, so a long string
literal passes. The list rule reads formatted lines; an unformatted file is
for the formatter check to refuse first.

Example, over a range holding three planted lines and one clean one:

    $ python docs/comment_width_check.py --range BASE..HEAD
    src/StandIn.cpp:1  comment (88 columns)
    src/StandIn.cpp:2  padded (17 columns)
    src/StandIn.cpp:3  list (34 columns)
    3 refused
    $ python docs/comment_width_check.py --range 2b13f377..25197646
    0 refused
    $ python docs/comment_width_check.py --range nosuchref..HEAD
    git failed: git diff -U0 --no-color --no-ext-diff nosuchref..HEAD
    fatal: ambiguous argument 'nosuchref..HEAD': unknown revision ...

The functions, for a caller other than main: columns(text) measures a line,
added_lines(diff_text) yields what a diff adds, is_wide_comment, is_padded
and is_open_list are the rules, and findings(diff_text) applies them all.

WHERE IT RUNS. Before a commit, --staged. In CI, the added-lines job in
ci-tests.yml checks out the whole history and passes --range: a push's
previous tip to HEAD, a pull request's base to HEAD, and HEAD^ to HEAD for a
new branch or a manual run.

FALSIFIED, 2026-10-23, against docs/comment_width_check_fixture.diff.txt,
which --self-test reads twice: as checked out, and with every line ending
CRLF, since git stores the fixture LF whatever a line ended with. The red
step, a findings() that returned nothing, missed all 11 findings in both. The
fixture's silent lines are a comment at exactly 80, long code with no
comment, a // inside a string, a tab-only indent, a list with its trailing
comma, a one-line list as an argument, a { inside a string, removed and
context lines, and files outside src/ and tests/ or not C++. Every rule and
each part of the hunk reading is then broken by
docs/comment_width_check_mutations.json through mutate.py: 18 of 18 caught.
The CI job's script was run locally, as a stand-in for a CI run, over a
planted range in a scratch clone, under the variables of a push, a new
branch, a pull request and a manual run; the ranges holding the plants
refused all three lines, and the clean ones passed.

WHAT IT CANNOT SEE. The width of a /* */ block's lines; a list wrapped by
hand in an unformatted file; a raw string literal spanning lines, whose
inner lines read as code. A comment line moved without change shows in a
diff as added, and is refused: a touched line meets the rule.
"""

import argparse
import dataclasses
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from dispatch_audit import blank
from long_comments import blank_literals

LIMIT = 80
TAB = 4

FIXTURE = (pathlib.Path(__file__).resolve().parent
           / "comment_width_check_fixture.diff.txt")
FIXTURE_FINDINGS = {
    ("src/Fixture.cpp", 11, "comment"),
    ("src/Fixture.cpp", 40, "comment"),
    ("src/Fixture.cpp", 42, "comment"),
    ("src/Fixture.cpp", 44, "comment"),
    ("src/Fixture.cpp", 46, "padded"),
    ("src/Fixture.cpp", 48, "padded"),
    ("src/Fixture.cpp", 49, "list"),
    ("src/Fixture.cpp", 56, "list"),
    ("src/Fixture.cpp", 58, "comment"),
    ("tests/Fixture.h", 1, "comment"),
    ("src/Last.cpp", 6, "padded"),
}


@dataclasses.dataclass(frozen=True)
class Finding:
    """One refused line: where it is, which rule, and its width."""

    path: str
    line: int
    rule: str
    columns: int


CHECKED_PATH = re.compile(r"(?:src|tests)/.*\.(?:cpp|h)")
HUNK = re.compile(r"@@ -\d+(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")
PADDED = re.compile(r"(?:\t+ +| +)\S")


def columns(text):
    """The width of text with a tab advancing to the next multiple of 4.

    Example:
        columns("\\tab")     # -> 6
        columns("ab\\tc")    # -> 5, the tab fills columns 3 and 4
    """
    width = 0
    for character in text:
        if character == "\t":
            width += TAB - width % TAB
        else:
            width += 1
    return width


def added_lines(diff_text):
    """Yields (path, line, text) for each added line of a checked file.

    A hunk's header counts its lines, so a line added as "++x" is read as
    content and never as a file header. A trailing carriage return is dropped.

    Example:
        diff = "+++ b/src/A.cpp\\n@@ -0,0 +7,1 @@\\n+\\tint x;\\n"
        list(added_lines(diff))           # -> [('src/A.cpp', 7, '\\tint x;')]
        list(added_lines(diff.replace("src", "docs")))   # -> [], not read
    """
    path = None
    line = 0
    old_left = new_left = 0
    for raw in diff_text.split("\n"):
        text = raw.removesuffix("\r")
        # Inside a hunk, its counts decide what each line is.
        if old_left or new_left:
            if text.startswith("+"):
                new_left -= 1
                if path:
                    yield path, line, text[1:]
                line += 1
            elif text.startswith("-"):
                old_left -= 1
            elif text.startswith(" "):
                old_left -= 1
                new_left -= 1
                line += 1
            continue
        if text.startswith("+++ "):
            name = text[4:].removeprefix("b/")
            path = name if CHECKED_PATH.fullmatch(name) else None
            continue
        header = HUNK.match(text)
        if header:
            old_left = int(header.group(1) or 1)
            line = int(header.group(2))
            new_left = int(header.group(3) or 1)


def is_wide_comment(text):
    """Whether the line holds a // comment and is wider than 80 at tab 4.

    Example:
        is_wide_comment("\\t// " + "x" * 74)                 # -> True, 81
        is_wide_comment('\\ts = "http://' + "x" * 80 + '";')  # -> False
    """
    return "//" in blank_literals(text) and columns(text) > LIMIT


def is_padded(text):
    """Whether the line is indented with spaces, after tabs or alone.

    Example:
        is_padded("\\t\\t    value);")   # -> True
        is_padded("\\t\\tvalue);")       # -> False
    """
    return PADDED.match(text) is not None


def is_open_list(text):
    """Whether the line ends in a comma inside a { it leaves open.

    That is the shape the formatter gives a wrapped braced list with no
    trailing comma. Literals and comments are blanked first.

    Example:
        is_open_list("\\tconst Pair pair = Pair{ first,")   # -> True
        is_open_list("\\tcall(Pair{ first, second },")      # -> False
        is_open_list("\\tconst Pair pair = Pair{")          # -> False
    """
    code = blank(text).rstrip()
    if not code.endswith(","):
        return False
    # Braces closed on the line cancel out; a } closing an earlier line's {
    # leaves nothing open.
    depth = 0
    for character in code:
        if character == "{":
            depth += 1
        elif character == "}" and depth:
            depth -= 1
    return depth > 0


RULES = (("comment", is_wide_comment), ("padded", is_padded),
         ("list", is_open_list))


def findings(diff_text):
    """Every refused line in the diff text, in the order the diff lists them.

    A line breaking several rules is reported once per rule.

    Example:
        diff = "+++ b/src/A.cpp\\n@@ -0,0 +3,1 @@\\n+  value);\\n"
        findings(diff)
        # -> [Finding(path='src/A.cpp', line=3, rule='padded', columns=9)]
    """
    found = []
    for path, line, text in added_lines(diff_text):
        for rule, is_refused in RULES:
            if is_refused(text):
                found.append(Finding(path, line, rule, columns(text)))
    return found


def git_diff(arguments):
    """The -U0 diff the arguments name, as text; exits 2 when git fails.

    Git's output is decoded as UTF-8 whatever the console's code page is.

    Example:
        git_diff(argparse.Namespace(staged=True, range=None))
        # -> the text of git diff --cached -U0, "" when nothing is staged
    """
    command = ["git", "diff", "-U0", "--no-color", "--no-ext-diff"]
    command += ["--cached"] if arguments.staged else [arguments.range]
    finished = subprocess.run(command, capture_output=True, encoding="utf-8")
    if finished.returncode != 0:
        print(f"git failed: {' '.join(command)}\n{finished.stderr}")
        sys.exit(2)
    return finished.stdout


def self_test():
    """Returns 0 when the fixture gives exactly FIXTURE_FINDINGS, else 1.

    The fixture is read once with LF endings and once with CRLF, and both
    must give the same findings.

    Example:
        self_test()
        # prints "self-test: 11 of 11 findings with LF and with CRLF, none
        # missed, none extra" -> 0
    """
    text = FIXTURE.read_bytes().decode("utf-8").replace("\r\n", "\n")
    failed = False
    # A checkout may hold the fixture with either ending, so both are made.
    for ending in ("\n", "\r\n"):
        reported = {(finding.path, finding.line, finding.rule)
                    for finding in findings(text.replace("\n", ending))}
        missed = sorted(FIXTURE_FINDINGS - reported)
        extra = sorted(reported - FIXTURE_FINDINGS)
        if missed or extra:
            print(f"self-test FAILED with {ending!r} endings: "
                  f"missed {missed}, extra {extra}")
            failed = True
    if failed:
        return 1
    print(f"self-test: {len(FIXTURE_FINDINGS)} of {len(FIXTURE_FINDINGS)} "
          "findings with LF and with CRLF, none missed, none extra")
    return 0


def main():
    """Checks the diff the command line names and returns the exit code.

    Example:
        main()   # with --range 2b13f377..25197646 prints "0 refused" -> 0
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--staged", action="store_true",
                        help="check the staged diff")
    source.add_argument("--range", help="check git diff over A..B")
    source.add_argument("--self-test", action="store_true",
                        help="check the fixture's exact findings")
    arguments = parser.parse_args()
    if arguments.self_test:
        return self_test()
    found = findings(git_diff(arguments))
    for finding in found:
        print(f"{finding.path}:{finding.line}  {finding.rule} "
              f"({finding.columns} columns)")
    print(f"{len(found)} refused")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
