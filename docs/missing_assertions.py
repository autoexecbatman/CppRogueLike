"""Finds assumptions the code makes that nothing states.

The owner's priority, 2026-10-01: "Assertion should not be verified if there is no
need. More important is to write more assertions that are unwritten." An assertion
that was never written cannot fail at all, which is a larger hole than an existing one
nobody has watched fire.

Three checks, each a different shape of the same gap. They report and never gate.

    python docs/missing_assertions.py                   # all three
    python docs/missing_assertions.py --owned           # loops over owned elements
    python docs/missing_assertions.py --documented      # preconditions in prose only
    python docs/missing_assertions.py --pointers        # pointer parameters

CHECK ONE - owned element. A range-based loop that reaches an element of a container
the code owns without asserting it first. This is the rule CLAUDE.md states outright
and the shape issues/0136 closed across seven passes. Three forms are accepted as
satisfying it: an assert on the element, an assert over the whole container just above
the loop, and a guard - because a guard is a decision somebody made, and
pair_symmetry.py --guards is what reports a thing guarded in one place and asserted in
another. This check is for an element reached with neither.

CHECK TWO - documented precondition. A function whose own comment says a caller must
do something first, and whose body then asserts nothing. The comment is the evidence
that the precondition is real, so this needs no judgment about whether it matters:
somebody already decided it did, wrote it in English, and left it unenforced. In this
project the contract sits on the declaration in the header while the body lives in the
sibling .cpp, so this follows Class::name across the two - a version that read only
definitions reported a clean zero while missing every contract in the tree.

CHECK THREE - pointer parameter. A function taking a raw pointer that reaches through
it without asserting and without checking. A raw pointer in a signature here means
either "this may be absent", which wants a check, or "a reference would not do", which
wants an assert. Neither being present is the finding.

WHAT IT CANNOT DO. It reads text, not types, so it cannot tell a pointer from an
iterator, and it cannot see an assertion inside a helper the function calls. An
invariant established at construction is invisible to it: EquippedItem's readers
dereference bare and are correct, because the constructor asserts. Every count is a
floor, and it finds only shapes somebody thought to pattern - a precondition nobody
wrote down is not here. See feedback_check_for_disagreement_not_absence in the memory
store for why absence is the hard thing to detect.

FALSIFICATIONS, 2026-10-01, before any count here was trusted. Each check was driven
from absent to present and back; the edits are recorded in the issue that introduced
this file. The first run found two of its own bugs - a multiplying star read as a
dereference, and a guard it never looked for - and check two's first zero was inert.
"""

import argparse
import pathlib
import re
import sys

# A range-based loop, binding by reference or by value.
RANGE_FOR = re.compile(r"\bfor\s*\(\s*(?:const\s+)?auto\s*(?:&|\*)?\s*&?\s*(\w+)\s*:\s*([^)]+)\)")

# A function definition's opening line: no semicolon, so declarations are not matched.
DEFINITION = re.compile(
    r"^[A-Za-z_][\w:<>,&*\s\[\]]*?\b([A-Za-z_]\w*)\s*\(([^;{]*)\)\s*"
    r"(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?\{?\s*$")

# A member declaration in a header: the same shape, ending in a semicolon. Contracts
# live on these and the bodies live in the sibling .cpp.
DECLARATION = re.compile(
    r"^\s*(?:\[\[nodiscard\]\]\s*)?[\w:<>,&*\s\[\]]*?\b(\w+)\s*\(([^;{]*)\)\s*"
    r"(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?;\s*$")

# A raw pointer parameter: a type, a star, a name. Smart pointers are excluded by the
# caller, which skips any argument list naming one.
POINTER_PARAM = re.compile(r"(?:^|,)\s*(?:const\s+)?([A-Za-z_]\w*(?:::\w+)*)\s*\*\s*(\w+)\s*(?:,|$)")

# Prose stating a precondition. Deliberately short: a loose set matched ordinary
# description, and a checker that cries wolf gets ignored.
PRECONDITION = re.compile(
    r"\b(must be called|must have|must be set|must already|caller must|requires that|"
    r"only valid|only after|before any|has to be called|expects a|expects the)\b", re.I)

# A dereferencing star opens an expression; a multiplying star follows an operand.
# "-e.vel_y * e.turbulence" matched the first draft and was reported as a dereference,
# which is how two of this checker's first five findings turned out to be its own bugs.
STAR_OPENS = r"(?:^|[=,(\[;{}&|!?:<>+\-*/%]|\breturn\b)\s*"

LOOK_BACK = 6
MAX_BODY = 160
KEYWORDS = {"if", "for", "while", "switch", "return", "else", "catch", "do", "sizeof"}


def source_files(root):
    """Every C++ source and header directly in src/, which is flat by design."""
    return sorted(root.glob("*.cpp")) + sorted(root.glob("*.h"))


def read_lines(path):
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def strip_comment(line):
    """The code half of a line. A loop inside a usage example is prose, not code."""
    cut = line.find("//")
    return line if cut < 0 else line[:cut]


def body_range(lines, start):
    """The half-open line range of the brace-delimited body opening at or after start."""
    depth = 0
    opened = False
    for index in range(start, min(start + MAX_BODY, len(lines))):
        code = strip_comment(lines[index])
        depth += code.count("{") - code.count("}")
        if "{" in code:
            opened = True
        if opened and depth <= 0:
            return start, index + 1
    return start, min(start + MAX_BODY, len(lines))


def body_text(lines, start, stop):
    return "\n".join(strip_comment(lines[offset]) for offset in range(start, stop))


def dereferences(code, name):
    """Whether this line reaches through name, by arrow, by star, or through a member."""
    escaped = re.escape(name)
    return bool(
        re.search(r"\b" + escaped + r"\s*->", code)
        or re.search(r"\b" + escaped + r"\.\w+\s*->", code)
        or re.search(STAR_OPENS + r"\*\s*" + escaped + r"\b", code)
        or re.search(STAR_OPENS + r"\*\s*" + escaped + r"\.\w+", code))


def asserts_on(code, name):
    return bool(re.search(r"(?<![A-Za-z_])assert\s*\([^)]*\b" + re.escape(name) + r"\b", code))


def guards(code, name):
    escaped = re.escape(name)
    return bool(re.search(r"\bif\s*\(\s*!?\s*" + escaped + r"(\.\w+)?\s*(\)|&&|\|\|)", code))


def owned_element_loops(files):
    """Loops reaching an owned element with neither an assert nor a check."""
    findings = []
    for path in files:
        lines = read_lines(path)
        for index, line in enumerate(lines):
            match = RANGE_FOR.search(strip_comment(line))
            if not match:
                continue
            name, container = match.group(1), match.group(2).strip()

            # An assert over the whole container just above the loop satisfies the rule.
            window = "\n".join(lines[max(0, index - LOOK_BACK):index])
            bare = container.lstrip("*").strip()
            tail = bare.split("->")[-1].split(".")[-1]
            if "assert" in window and (bare in window or (len(tail) > 2 and tail in window)):
                continue

            start, stop = body_range(lines, index)
            if guards(body_text(lines, start, stop), name):
                continue
            for offset in range(start + 1, stop):
                inner = strip_comment(lines[offset])
                if asserts_on(inner, name):
                    break
                if dereferences(inner, name):
                    findings.append((path.name, offset + 1, name, container, inner.strip()[:78]))
                    break
    return findings


def comment_block_above(lines, index):
    """The run of // lines immediately above index, read top to bottom."""
    said = []
    for back in range(index - 1, max(-1, index - 24), -1):
        stripped = lines[back].strip()
        if not stripped.startswith("//"):
            break
        said.append(stripped.lstrip("/ ").rstrip())
    return " ".join(reversed(said))


def body_in_sibling(path, name):
    """The body of Class::name in the .cpp beside this header.

    src/ is flat and one class per file, so the header's stem is the class name.
    """
    sibling = path.with_suffix(".cpp")
    if not sibling.exists():
        return None
    lines = read_lines(sibling)
    opener = re.compile(
        r"^[A-Za-z_][\w:<>,&*\s\[\]]*?\b" + re.escape(path.stem) + r"::" + re.escape(name) + r"\s*\(")
    for index, line in enumerate(lines):
        if opener.match(line.rstrip()):
            start, stop = body_range(lines, index)
            return body_text(lines, start, stop)
    return None


def documented_preconditions(files):
    """Functions whose comment states a precondition and whose body asserts nothing."""
    findings = []
    for path in files:
        lines = read_lines(path)
        for index, line in enumerate(lines):
            stripped = line.rstrip()
            if line.lstrip().startswith("//"):
                continue
            declared = stripped.endswith(";")
            match = DECLARATION.match(stripped) if declared else DEFINITION.match(stripped)
            if not match or match.group(1) in KEYWORDS:
                continue
            name = match.group(1)

            block = comment_block_above(lines, index)
            if not block or not PRECONDITION.search(block):
                continue

            if declared:
                body = body_in_sibling(path, name)
                # No body found means it is defined elsewhere or the class does not
                # match the filename; say nothing rather than guess.
                if body is None or "assert" in body:
                    continue
            else:
                start, stop = body_range(lines, index)
                if "assert" in body_text(lines, start, stop) or stop - start < 3:
                    continue
            findings.append((path.name, index + 1, name, block[:100]))

    # A contract repeated on the declaration and the definition is one defect. Keep the
    # header's, which is the one a caller reads.
    seen = {}
    for filename, number, name, block in findings:
        key = (pathlib.Path(filename).stem, name)
        if key not in seen or filename.endswith(".h"):
            seen[key] = (filename, number, name, block)
    return sorted(seen.values())


def pointer_parameters(files):
    """Pointer parameters a function reaches through without asserting or checking."""
    findings = []
    for path in files:
        lines = read_lines(path)
        for index, line in enumerate(lines):
            if line.lstrip().startswith("//"):
                continue
            match = DEFINITION.match(line.rstrip())
            if not match or match.group(1) in KEYWORDS:
                continue
            name, arguments = match.group(1), match.group(2)
            if "unique_ptr" in arguments or "shared_ptr" in arguments:
                continue

            start, stop = body_range(lines, index)
            if stop - start < 3:
                continue
            body = body_text(lines, start, stop)

            for _, parameter in POINTER_PARAM.findall(arguments):
                if asserts_on(body, parameter) or guards(body, parameter):
                    continue
                if not any(dereferences(strip_comment(lines[offset]), parameter)
                           for offset in range(start, stop)):
                    continue
                findings.append((path.name, index + 1, name, parameter))
    return findings


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--owned", action="store_true", help="only the owned-element check")
    parser.add_argument("--documented", action="store_true", help="only the documented-precondition check")
    parser.add_argument("--pointers", action="store_true", help="only the pointer-parameter check")
    parser.add_argument("--root", default="src", help="directory to read (default src)")
    arguments = parser.parse_args()

    root = pathlib.Path(arguments.root)
    if not root.is_dir():
        print("no such directory: " + str(root), file=sys.stderr)
        return 2

    files = source_files(root)
    chosen = arguments.owned or arguments.documented or arguments.pointers
    print(f"missing assertions over {len(files)} files in {root}/")

    if arguments.owned or not chosen:
        findings = owned_element_loops(files)
        print(f"\nOWNED ELEMENT REACHED WITH NEITHER ASSERT NOR CHECK: {len(findings)}")
        for filename, number, name, container, text in findings:
            print(f"  {filename}:{number}  {name} of {container}")
            print(f"      {text}")

    if arguments.documented or not chosen:
        findings = documented_preconditions(files)
        print(f"\nPRECONDITION STATED IN PROSE AND NOT ASSERTED: {len(findings)}")
        for filename, number, name, block in findings:
            print(f"  {filename}:{number}  {name}")
            print(f"      \"{block}\"")

    if arguments.pointers or not chosen:
        findings = pointer_parameters(files)
        print(f"\nPOINTER PARAMETER REACHED WITHOUT ASSERT OR CHECK: {len(findings)}")
        for filename, number, name, parameter in findings:
            print(f"  {filename}:{number}  {name}  reaches through  {parameter}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
