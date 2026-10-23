"""Reports what an 80-column limit would do to src/ and tests/, before it lands.

    python docs/wrap_report.py                # classes and the comment backlog
    python docs/wrap_report.py --list         # every statement, every file
    python docs/wrap_report.py --without X    # one proposed option left out
    python docs/wrap_report.py --diff src/A.cpp   # one file, today -> proposed
    python docs/wrap_report.py --self-test    # the fixture's exact findings

It formats every file twice, with today's .clang-format and with PROPOSED
applied to it, and changes no file. It reports and does not gate.

The owner, 2026-10-22: "80 for code and comments; comments as files are
touched". So the code is what a reformat moves, and the comment lines over
80 are a backlog listed by file, rewrapped by hand when a file is next
edited.

PROPOSED, and what each option is for. Each was falsified by running the
report without it, 2026-10-23, on src/ and tests/ at 2901573d, where the
full set gives 3226 statements and no padded line:

    ColumnLimit 80
        the limit; 393 statements without it
    AlignAfterOpenBracket false
        no continuation lined up under its bracket; 119 padded lines in
        28 files without it
    BreakAfterOpenBracketFunction true
        a call or signature that wraps breaks after (; 1806 statements
        without it
    BreakAfterOpenBracketIf true
        the same for a condition; 57 lines in 9 files change without it
    BreakAfterOpenBracketLoop true
        the same for a loop header; 73 padded lines in 10 files without it
    BreakAfterOpenBracketSwitch true
        the same for a switch; no switch here wraps, so nothing changes
        without it, and a planted long switch condition shows it acting
    AllowAllArgumentsOnNextLine false
        one argument per line; 2586 statements without it
    AllowAllParametersOfDeclarationOnNextLine false
        one parameter per line; 2814 statements without it
    ReflowComments false
        without it, examples are merged into the next line: the guard
        stops on AbilityAllocation.cpp
    BreakStringLiterals false
        without it, "a" "b" splits literals a grep looks for: the guard
        stops on AiMimic.cpp
    BreakConstructorInitializers AfterColon
        initializers start on the indent; 82 padded lines in 33 files
        without it
    PackConstructorInitializers Never
        one initializer per line; 171 lines in 38 files change without it
    BreakInheritanceList AfterColon
        base classes start on the indent; 4 padded lines in 4 files
        without it
    BreakBeforeTernaryOperators false
        a wrapped ternary breaks after ? and :; 61 padded lines in 16
        files without it
    BreakTemplateDeclarations Yes
        a template header keeps its own line; without it 8 headers join
        what they declare, 24 lines in 7 files

HOW A STATEMENT IS FOLLOWED. The two formats differ only in whitespace, so
the text without whitespace is identical on both sides, and a character
before maps to the same character after. The report checks that and stops
on a file where it fails rather than guess. A statement ends at a ; outside
parentheses or at a block's brace; an initializer's braces, a preprocessor
line and an access label end nothing. One counts when it had 1 or 2 lines
and has 3 or more, and its line is given in the checkout file.

Example, at 2901573d (the head of each section):

    $ python docs/wrap_report.py
    7 of 533 files differ from today's own format; their lines below are
    mapped back to the checkout
    STATEMENTS SPREAD OVER 3 OR MORE LINES: 3226 in 374 of 533 files
        982   30.4%  call
        572   17.7%  test assertion
    COMMENT LINES OVER 80, rewrapped as their files are touched: 5358 in
    449 files, 525 of them example lines

FALSIFIED, 2026-10-22. docs/wrap_report_fixture.cpp.txt plants one
statement of each class and two long comment lines, each named by a marker,
and --self-test fails on a missing or an extra finding. Its red run, before
the analysis existed, missed all of them. docs/wrap_report_mutations.json
breaks the rules in turn through mutate.py, and --self-test failed on all
19: the fixture formatted against today's file instead of the pre-limit
one, a ; in a for header ending a statement, an initializer read as a block,
a label or a preprocessor line heading a statement, the trailing-comment
rule off, two-line wraps counted, a comment line counted into a statement,
each class's rule off, a comment just over the limit or an example line
missed, and the limit moved. The fixture's for header was added for the
first of these, which no case could see before. The stream guard is
falsified by the two option runs above, where it stopped both times.

WHAT IT CANNOT SEE. The classes come from a statement's first words and
shape, not a parse, so a macro reads as a call. A /* */ comment inside a
statement counts as part of it. Whether a statement should be rewritten
rather than wrapped is the reader's call; the class names the usual fix.
"""

import argparse
import collections
import difflib
import bisect
import pathlib
import re
import subprocess
import sys
import tempfile

from dispatch_audit import blank
from long_comments import blank_literals

ROOT = pathlib.Path(__file__).resolve().parent.parent
FIXTURE = (pathlib.Path(__file__).resolve().parent
           / "wrap_report_fixture.cpp.txt")
CLANG_FORMAT = ("C:/Program Files/Microsoft Visual Studio/18/Community/VC/"
                "Tools/Llvm/x64/bin/clang-format.exe")
LIMIT = 80
TAB_WIDTH = 4
PROPOSED = {
    "ColumnLimit": "80",
    "AlignAfterOpenBracket": "false",
    "BreakAfterOpenBracketFunction": "true",
    "BreakAfterOpenBracketIf": "true",
    "BreakAfterOpenBracketLoop": "true",
    "BreakAfterOpenBracketSwitch": "true",
    "AllowAllArgumentsOnNextLine": "false",
    "AllowAllParametersOfDeclarationOnNextLine": "false",
    "ReflowComments": "false",
    "BreakStringLiterals": "false",
    "BreakConstructorInitializers": "AfterColon",
    "PackConstructorInitializers": "Never",
    "BreakInheritanceList": "AfterColon",
    "BreakBeforeTernaryOperators": "false",
    "BreakTemplateDeclarations": "Yes",
}
# The two values today's file held before the limit; every other PROPOSED
# key was absent, so the style's default applied.
PRE_LIMIT = {
    "ColumnLimit": "0",
    "AlignAfterOpenBracket": "DontAlign",
}


def proposed_config(today, without):
    """Today's .clang-format text with PROPOSED applied, less any named in
    without."""
    kept = {key: value for key, value in PROPOSED.items() if key not in without}
    lines = [line for line in today.splitlines()
             if line.split(":")[0].strip() not in PROPOSED]
    lines += [f"{key}: {value}" for key, value in kept.items()]
    return "\n".join(lines) + "\n"


def pre_limit_config(today):
    """Today's .clang-format text as it was before the 80-column limit:
    every PROPOSED key taken out and PRE_LIMIT put back."""
    lines = [line for line in today.splitlines()
             if line.split(":")[0].strip() not in PROPOSED]
    lines += [f"{key}: {value}" for key, value in PRE_LIMIT.items()]
    return "\n".join(lines) + "\n"


def formatted(path, config_path, source_name):
    """The file as clang-format prints it under the given config."""
    result = subprocess.run(
        [CLANG_FORMAT, f"--style=file:{config_path}",
         f"--assume-filename={source_name}", str(path)],
        capture_output=True, text=True, encoding="utf-8", check=True)
    return result.stdout


BLOCK_HEAD = re.compile(
    r"(\)\s*(?:const|noexcept|override|final|mutable|->\s*[\w:<>*&\s]+|\s)*"
    r"|\]|\b(?:else|do|try))\s*$")
SCOPE_HEAD = re.compile(r"^(?:template\s*<[^;]*?>\s*)?"
                        r"(?:class|struct|union|enum|namespace)\b[^=]*$")
CONSTRUCTOR_HEAD = re.compile(r"\)\s*:\s")
DECLARATION = re.compile(
    r"^(?:template\s*<[^;]*?>\s*)?(?:\[\[[\w:]+\]\]\s*)*"
    r"(?:(?:static|virtual|inline|constexpr|consteval|explicit|friend|extern)"
    r"\s+)*(?:const\s+)?[\w:]+(?:\s*<[^()]*>)?[\s*&]+~?[\w:]+\s*\(.*\)"
    r"(?:\s*(?:const|noexcept|override|final|=\s*0|=\s*default|=\s*delete))*$")
ASSIGNMENT = re.compile(r"(?<![=!<>+\-*/%&|^])=(?!=)")


def statements(mask):
    """Each statement in the code mask as (start, end, delimiter).

    A statement ends at a ; outside parentheses, or at a block's { or }. An
    initializer's braces are not a block, so `Point point{ x, y };` stays one
    statement, and `for (a; b; c)` is not cut at its semicolons.

    Example:
        list(statements("f(a);\\nif (b)\\n{\\n}"))
        # -> [(0, 4, ";"), (5, 13, "{")], from just after the ;
    """
    found = []
    frames = [{"kind": "block", "parens": 0}]
    start = 0
    for index, character in enumerate(mask):
        frame = frames[-1]
        if character == "(":
            frame["parens"] += 1
        elif character == ")":
            frame["parens"] = max(0, frame["parens"] - 1)
        elif character == ";":
            if frame["kind"] == "block" and frame["parens"] == 0:
                found.append((start, index, ";"))
                start = index + 1
        elif character == "{":
            head = " ".join(mask[start:index].split())
            is_block = frame["kind"] == "block" and (
                head == "" or BLOCK_HEAD.search(head)
                or SCOPE_HEAD.search(head) or head.endswith(":")
                or (head.endswith("}") and CONSTRUCTOR_HEAD.search(head)))
            if is_block:
                found.append((start, index, "{"))
                start = index + 1
            frames.append({"kind": "block" if is_block else "initializer",
                           "parens": 0})
        elif character == "}":
            closed = frames.pop() if len(frames) > 1 else frame
            if closed["kind"] == "block":
                found.append((start, index, "}"))
                start = index + 1
    return [(begin, end, delimiter) for begin, end, delimiter in found
            if mask[begin:end].strip()]


def classify(flat, delimiter):
    """The kind of statement, from its code with whitespace collapsed.

    Example:
        classify('assert(x && " ")', ";")   # -> "assert"
        classify("void f(int a, int b)", "{")   # -> "signature"
    """
    if re.match(r"(?:static_)?assert\s*\(", flat):
        return "assert"
    if re.match(r"(?:EXPECT|ASSERT)_\w+\s*\(", flat):
        return "test assertion"
    if re.match(r"(?:else\s+)?(?:if|while|for|switch)\b", flat):
        if re.search(r"&&|\|\|", flat):
            return "condition"
        return "control statement"
    if re.match(r"return\b", flat):
        return "return"
    if "<<" in flat:
        return "stream chain"
    before_call = flat.split("(")[0]
    is_expression = re.match(r"(?:throw|new|delete)\b", flat)
    is_definition = (delimiter == "{" and "(" in flat and "](" not in flat
                     and not ASSIGNMENT.search(before_call))
    if not is_expression and (is_definition or DECLARATION.match(flat)):
        return "signature"
    # A brace inside a call's arguments is an argument; the call is the shape.
    first_brace = flat.find("{")
    first_paren = flat.find("(")
    if first_brace != -1 and (first_paren == -1 or first_brace < first_paren):
        return "braced initializer"
    if ASSIGNMENT.search(flat):
        return "assignment"
    if "(" in flat:
        return "call"
    return "other"


def line_starts(text):
    """The offset each line of text starts at, for bisecting an offset."""
    return [0] + [index + 1 for index, character in enumerate(text)
                  if character == "\n"]


def find_wraps(name, before, after):
    """Every statement the proposed config spreads from 1 or 2 lines to 3 or
    more, with its class.

    The formatter changes only whitespace, so the text without whitespace is
    the same on both sides; a position before maps to the same character
    after. A file where it differs stops the report rather than guess.

    Example:
        find_wraps("a.cpp", "f(first, second, third);\\n",
                   "f(\\n\\tfirst,\\n\\tsecond,\\n\\tthird);\\n")
        # -> [{"file": "a.cpp", "line": 1, "class": "call",
        #      "before": 1, "after": 4}]
    """
    before_stream = [index for index, character in enumerate(before)
                     if not character.isspace()]
    after_stream = [index for index, character in enumerate(after)
                    if not character.isspace()]
    same_text = (len(before_stream) == len(after_stream) and all(
        before[left] == after[right]
        for left, right in zip(before_stream, after_stream)))
    if not same_text:
        raise SystemExit(f"{name}: the formatter changed more than whitespace")
    rank = {position: order for order, position in enumerate(before_stream)}
    # A preprocessor line or an access label ends no statement, so each is
    # blanked like a comment.
    mask = re.sub(r"(?m)^[ \t]*(?:#.*|(?:public|private|protected)\s*:)",
                  lambda match: " " * len(match.group(0)), blank(before))
    mask_lines = mask.splitlines()
    before_text_lines = before.splitlines()
    kept_comments = blank_literals(before)
    before_lines = line_starts(before)
    after_lines = line_starts(after)
    found = []
    for start, end, delimiter in statements(mask):
        # Code and literal characters count; a // comment beside them does not.
        positions = [position for position in range(start, end)
                     if not before[position].isspace()
                     and not (mask[position] == " "
                              and kept_comments[position] != " ")]
        if not positions:
            continue
        old = {bisect.bisect_right(before_lines, position)
               for position in positions}
        new = {bisect.bisect_right(after_lines, after_stream[rank[position]])
               for position in positions}
        if len(old) <= 2 and len(new) >= 3:
            flat = " ".join(mask[start:end].split())
            kind = classify(flat, delimiter)
            # One line that fits without its comment is broken by the comment.
            if len(old) == 1:
                line = min(old) - 1
                code_width = width(mask_lines[line].rstrip())
                if code_width <= LIMIT < width(before_text_lines[line]):
                    kind = "trailing comment"
            found.append({"file": name, "line": min(old),
                          "class": kind,
                          "before": len(old), "after": len(new)})
    return found


def width(line):
    """The columns a line takes with tabs at TAB_WIDTH."""
    return len(line.expandtabs(TAB_WIDTH))


def long_comments(name, text):
    """Every comment line over LIMIT, marking indented example lines.

    Example:
        long_comments("a.cpp", "// " + "x" * 90)
        # -> [{"file": "a.cpp", "line": 1, "width": 93, "is_example": False}]
    """
    found = []
    for number, line in enumerate(text.splitlines(), start=1):
        if line.strip().startswith("//") and width(line) > LIMIT:
            found.append({"file": name, "line": number, "width": width(line),
                          "is_example": bool(re.match(r"\s*//\s{2,}\S",
                                                      line))})
    return found


def long_code(name, text):
    """Every line over LIMIT that is not a comment line.

    Example:
        long_code("a.cpp", "f();\\n")   # -> []
    """
    return [{"file": name, "line": number, "width": width(line)}
            for number, line in enumerate(text.splitlines(), start=1)
            if width(line) > LIMIT and not line.strip().startswith("//")]


def expected_findings(text):
    """The fixture's markers: (line, class) statements and comment lines."""
    lines = text.splitlines()
    statements = set()
    comments = set()
    for number, line in enumerate(lines, start=1):
        marker = line.strip()
        if marker.startswith("// EXPECT "):
            statements.add((number + 1, marker[len("// EXPECT "):]))
        if "LONG-COMMENT" in line:
            comments.add((number, False))
        if "LONG-EXAMPLE" in line:
            comments.add((number, True))
    return statements, comments


def self_test():
    """Formats the fixture before the limit and with PROPOSED; 0 when it
    reports exactly its markers. The baseline is the pre-limit file, because
    once the limit lands today's file formats the fixture as PROPOSED does."""
    today = (ROOT / ".clang-format").read_text(encoding="utf-8")
    text = FIXTURE.read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory() as folder:
        today_path = pathlib.Path(folder) / "today.clang-format"
        proposed_path = pathlib.Path(folder) / "proposed.clang-format"
        today_path.write_text(pre_limit_config(today), encoding="utf-8")
        proposed_path.write_text(proposed_config(today, []), encoding="utf-8")
        before = formatted(FIXTURE, today_path, "fixture.cpp")
        after = formatted(FIXTURE, proposed_path, "fixture.cpp")
    expected_statements, expected_comments = expected_findings(before)
    found_statements = {(wrap["line"], wrap["class"])
                        for wrap in find_wraps(FIXTURE.name, before, after)}
    found_comments = {(comment["line"], comment["is_example"])
                      for comment in long_comments(FIXTURE.name, before)}
    failures = []
    for label, expected, found in (
            ("statements", expected_statements, found_statements),
            ("comments", expected_comments, found_comments)):
        missed = sorted(expected - found)
        extra = sorted(found - expected)
        if missed or extra:
            failures.append(f"{label}: missed {missed}, extra {extra}")
    if failures:
        print("self-test FAILED")
        for failure in failures:
            print("  " + failure)
        return 1
    print(f"self-test: {len(found_statements)} statements and"
          f" {len(found_comments)} comment lines, none missed, none extra")
    return 0


def main():
    """Parses the arguments and runs the report or the self-test."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--self-test", action="store_true",
                        help="check the fixture's exact findings")
    parser.add_argument("--list", action="store_true",
                        help="every statement and every file's comments")
    parser.add_argument("--without", action="append", default=[],
                        choices=sorted(PROPOSED),
                        help="leave one proposed option out")
    parser.add_argument("--diff", help="print one file's change, as a path")
    arguments = parser.parse_args()
    if arguments.self_test:
        return self_test()

    today = (ROOT / ".clang-format").read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory() as folder:
        today_path = pathlib.Path(folder) / "today.clang-format"
        proposed_path = pathlib.Path(folder) / "proposed.clang-format"
        today_path.write_text(today, encoding="utf-8")
        proposed_path.write_text(proposed_config(today, arguments.without),
                                 encoding="utf-8")
        if arguments.diff:
            path = ROOT / arguments.diff
            before = formatted(path, today_path, path.name)
            after = formatted(path, proposed_path, path.name)
            sys.stdout.writelines(difflib.unified_diff(
                before.splitlines(keepends=True),
                after.splitlines(keepends=True), "today", "proposed"))
            return 0
        sources = source_files()
        pairs = {path.relative_to(ROOT).as_posix():
                 (path.read_text(encoding="utf-8"),
                  formatted(path, today_path, path.name),
                  formatted(path, proposed_path, path.name))
                 for path in sources}
    report(pairs, arguments.list)
    return 0


def source_files():
    """Every .cpp and .h under src/ and tests/, the json data left out."""
    return sorted(path for folder in ("src", "tests")
                  for path in (ROOT / folder).rglob("*")
                  if path.suffix in (".cpp", ".h") and "json" not in path.parts)


def checkout_line(formatted_text, raw_text, line):
    """The line of the checkout file that a line of its formatted copy came
    from: exact where the two agree, the start of the block where they do not.

    Example:
        checkout_line("a\nb\nc", "a\n\nb\nc", 3)   # -> 4
    """
    matcher = difflib.SequenceMatcher(a=formatted_text.splitlines(),
                                      b=raw_text.splitlines(), autojunk=False)
    for _, a_start, a_end, b_start, b_end in matcher.get_opcodes():
        if a_start < line <= a_end:
            # Past the end of the checkout side of a block, take its last line.
            last = max(b_end - b_start - 1, 0)
            return b_start + min(line - 1 - a_start, last) + 1
    return line


def report(pairs, is_listing):
    """Prints the statements by class, the comment backlog and what stays
    long. Each file comes as its checkout text and its text formatted today
    and as proposed; a statement's line is given in the checkout file."""
    wraps = []
    for name, (raw, before, after) in pairs.items():
        for wrap in find_wraps(name, before, after):
            if before != raw:
                wrap["line"] = checkout_line(before, raw, wrap["line"])
            wraps.append(wrap)
    unformatted = sum(raw != before for raw, before, _ in pairs.values())
    comments = [comment for name, (raw, _, _) in pairs.items()
                for comment in long_comments(name, raw)]
    code = [line for name, (_, _, after) in pairs.items()
            for line in long_code(name, after)]
    padded = [(name, number) for name, (_, _, after) in pairs.items()
              for number, line in enumerate(after.splitlines(), start=1)
              if re.match(r"\t+ +\S", line)]
    print(f"{unformatted} of {len(pairs)} files differ from today's own"
          " format; their lines below are mapped back to the checkout")

    files = len({wrap["file"] for wrap in wraps})
    print(f"STATEMENTS SPREAD OVER 3 OR MORE LINES: {len(wraps)} in {files}"
          f" of {len(pairs)} files")
    classes = collections.Counter(wrap["class"] for wrap in wraps)
    for name, count in classes.most_common():
        share = 100 * count / max(len(wraps), 1)
        print(f"  {count:5}  {share:5.1f}%  {name}")
    if is_listing:
        for wrap in sorted(wraps, key=lambda item: (item["file"],
                                                    item["line"])):
            print(f"  {wrap['file']}:{wrap['line']}  {wrap['class']}"
                  f"  {wrap['before']} -> {wrap['after']} lines")

    examples = sum(comment["is_example"] for comment in comments)
    by_file = collections.Counter(comment["file"] for comment in comments)
    print(f"COMMENT LINES OVER {LIMIT}, rewrapped as their files are touched:"
          f" {len(comments)} in {len(by_file)} files, {examples} of them"
          " example lines")
    shown = by_file.most_common() if is_listing else by_file.most_common(10)
    for name, count in shown:
        file_examples = sum(comment["is_example"] for comment in comments
                            if comment["file"] == name)
        print(f"  {count:5}  {name}  ({file_examples} examples)")

    print(f"CODE LINES STILL OVER {LIMIT} AFTER IT: {len(code)}")
    for line in code[:20]:
        print(f"  {line['file']}:{line['line']}  {line['width']} columns")
    print(f"LINES INDENTED WITH TABS THEN SPACES AFTER IT: {len(padded)}"
          f" in {len({name for name, _ in padded})} files")
    for name, number in padded[:5]:
        print(f"  {name}:{number}")


if __name__ == "__main__":
    sys.exit(main())
