"""Finds lists kept in code: membership chains and repeated decisions.

The owner, 2026-10-22: long || chains are a "maintainance nightmare", and code
has to be extendible. Both shapes below hold a list that every site has to be
edited for when it grows, and either can usually become data: a set, a table,
a descriptor.

    python docs/dispatch_audit.py               # both reports, worst first
    python docs/dispatch_audit.py --file a.cpp  # these files instead of src/
    python docs/dispatch_audit.py --self-test   # the fixture's exact findings

It exits 1 when it finds anything and 0 when it finds nothing. It reports and
does not gate, because neither count starts at 0.

MEMBERSHIP CHAIN. One statement comparing a single subject with == to 4 or
more values joined by ||, or with != to 4 or more joined by &&, the negated
form. Statements are read across lines up to ; { or }, with comments and
literals blanked first. Comparisons of different subjects are a record
comparison and never a finding, however many there are:

    col == 0 || col == width || row == 0 || row == height   # no subject has 4

DISPATCH. A class deciding on one subject in 3 or more of its functions, the
house rule of three. Deciding means a switch on it, or an if-else ladder whose
arms compare it with == three or more times; two arms are an if and an else.
A function belongs to the class in Class::name, or to the class whose braces
hold its definition. A free function belongs to none and is never counted.

Example, on src/ at 2026-10-22 (first lines of each section):

    $ python docs/dispatch_audit.py
    MEMBERSHIP CHAINS: one subject against 4 or more values
       10  ItemClassification.h:124  itemClass == ItemClass::DAGGER, ...
        5  RoomEditor.cpp:324  symbol == '#', '.', ',', '+', '~'
    REPEATED DISPATCH: one subject decided in 3 or more functions of a class
        4  Player on get_creature_class(): calculate_thaco, ...
    6 membership chains in 4 files
    5 repeated dispatches in 5 classes

FALSIFIED, 2026-10-22. docs/dispatch_audit_fixture.cpp.txt marks each
finding it must give with an EXPECT comment, and --self-test fails on a
missing or an extra one. docs/dispatch_audit_mutations.json breaks each rule
in turn through mutate.py, and --self-test failed on all 14: joiners ignored,
subjects pooled, comments or literals left in, the subject running past the
if's parenthesis, statements cut per line, the != form dropped, either
threshold or the ladder length lowered, free functions counted, a function
in its class unowned, every else-if a new ladder, and a lambda inside a body
read as a function. The last one survived the first fixture, which had no
lambda in a member; one was added. On copies of real files,
ItemClassification's chain cut to 3 terms stopped reporting, and a fifth
class switch moved Player from 4 to 5.

WHAT IT CANNOT SEE. It reads text, not types, so x == A::B and y == A::B are
different subjects even when x and y hold the same value. A chain split
between a helper and its caller, a switch reached through a macro, and a
decision in a free function are invisible. Whether a finding should change
is not its business: a chain over four values can be the clearest code there
is. It names the site; reading decides.
"""

import argparse
import pathlib
import re
import sys

from long_comments import blank_literals

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE = ROOT / "src"
FIXTURE = (pathlib.Path(__file__).resolve().parent
           / "dispatch_audit_fixture.cpp.txt")
LEAST_CHAIN_TERMS = 4
LEAST_DISPATCH_FUNCTIONS = 3
LEAST_LADDER_ARMS = 3

OPERAND_CHARACTERS = set(
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.:->[]")
CONTROL_WORDS = {
    "if", "for", "while", "switch", "catch", "return", "sizeof", "decltype"}
FUNCTION_TAIL = re.compile(
    r"\)(?:\s*(?:const|noexcept|override|final|&&|&|->\s*[\w:<>,\s*&]+))*\s*$")
CONSTRUCTOR_TAIL = re.compile(r"\)\s*(?:noexcept\s*)?:\s*[A-Za-z_]")
FUNCTION_NAME = re.compile(r"((?:\w+::)*)(~?\w+)\s*\(")
CLASS_HEAD = re.compile(r"\b(?:class|struct)\s+(\w+)\b[^()]*$")


def blank(text):
    """The text with literals and both kinds of comment blanked, same layout.

    Example:
        blank('a == 1 // b == 2')   # -> 'a == 1          ', the comment spaces
    """
    without_literals = blank_literals(text)
    return re.sub(r"//[^\n]*", lambda match: " " * len(match.group(0)),
                  without_literals)


def line_of(text, offset):
    """The 1-based line holding offset.

    Example:
        line_of("a\\nb", 2)   # -> 2
    """
    return text.count("\n", 0, offset) + 1


def statements(text):
    """Each statement as (start, end), split at ; { and }, from its first
    non-blank character.

    Example:
        list(statements("a;\\n  b{"))   # -> [(0, 1), (5, 6)]
    """
    start = 0
    for match in re.finditer(r"[;{}]", text):
        segment = text[start:match.start()]
        leading = len(segment) - len(segment.lstrip())
        if segment.strip():
            yield start + leading, match.start()
        start = match.end()


def subject_before(text, end):
    """The operand ending at end: names, members, calls and indexing.

    The scan walks left, keeping parentheses balanced, and stops at an
    unmatched ( or a character no operand holds.

    Example:
        subject_before("if (grid.at(index) == 1", 19)   # -> "grid.at(index)"
    """
    index = end
    while index > 0 and text[index - 1].isspace():
        index -= 1
    stop = index
    depth = 0
    while index > 0:
        character = text[index - 1]
        if character == ")":
            depth += 1
        elif character == "(":
            if depth == 0:
                break
            depth -= 1
        elif depth == 0 and character not in OPERAND_CHARACTERS:
            break
        index -= 1
    return "".join(text[index:stop].split())


def value_after(text, original, start):
    """The operand starting at start, read from the original text so that a
    blanked literal keeps its value.

    Example:
        value_after("x == ' '", "x == '#'", 4)   # -> "'#'"
    """
    index = start
    while index < len(text) and text[index].isspace():
        index += 1
    begin = index
    if index < len(text) and text[index] in "'\"":
        closing = text.find(text[index], index + 1)
        index = len(text) if closing == -1 else closing + 1
        return original[begin:index]
    depth = 0
    while index < len(text):
        character = text[index]
        if character == "(":
            depth += 1
        elif character == ")":
            if depth == 0:
                break
            depth -= 1
        elif depth == 0 and character not in OPERAND_CHARACTERS:
            break
        index += 1
    return " ".join(original[begin:index].split())


def find_chains(name, text):
    """Every membership chain in one file, worst first.

    A statement comparing different subjects is a record comparison and
    reports nothing.

    Example:
        find_chains("a.cpp", "return k == A || k == B || k == C || k == D;")
        # -> [{"file": "a.cpp", "line": 1, "subject": "k", "terms": 4,
        #      "operator": "==", "values": ["A", "B", "C", "D"]}]
        find_chains("a.cpp", "return k == A && k == B && k == C && k == D;")
        # -> [], == joined by && is not a list
    """
    blanked = blank(text)
    found = []
    for start, end in statements(blanked):
        segment = blanked[start:end]
        original = text[start:end]
        for operator, joiner in (("==", "||"), ("!=", "&&")):
            # With the other joiner the comparisons are facts, not a list.
            if joiner not in segment:
                continue
            groups = {}
            for match in re.finditer(re.escape(operator), segment):
                subject = subject_before(segment, match.start())
                if subject:
                    value = value_after(segment, original, match.end())
                    groups.setdefault(subject, []).append(value)
            for subject, values in groups.items():
                if len(values) >= LEAST_CHAIN_TERMS:
                    found.append({
                        "file": name,
                        "line": line_of(blanked, start),
                        "subject": subject,
                        "terms": len(values),
                        "operator": operator,
                        "values": values})
    return sorted(found, key=chain_order)


def chain_order(chain):
    """Sorts chains by terms, most first, then by file and line.

    Example:
        chain_order({"terms": 5, "file": "a.cpp", "line": 3})   # -> (-5, ...)
    """
    return -chain["terms"], chain["file"], chain["line"]


def function_of(statement, scopes):
    """The (class, function) whose body a brace after statement opens.

    A body belongs to the class in Class::name, else to the class whose
    braces enclose it; a free function gives (None, name). Anything that is
    not a function body gives None.

    Example:
        function_of("int Shape::sides() const", [])   # -> ("Shape", "sides")
        function_of("if (ready)", [])                  # -> None
    """
    is_function = (FUNCTION_TAIL.search(statement)
                   or CONSTRUCTOR_TAIL.search(statement))
    if not is_function:
        return None
    match = FUNCTION_NAME.search(statement)
    if not match or match.group(2) in CONTROL_WORDS:
        return None
    qualifiers = [part for part in match.group(1).split("::") if part]
    if qualifiers:
        return qualifiers[-1], match.group(2)
    enclosing = scopes[-1] if scopes else None
    if enclosing and enclosing[0] == "class":
        return enclosing[1], match.group(2)
    return None, match.group(2)


def balanced_parentheses(text, opening):
    """The text inside the parentheses that open at opening.

    Example:
        balanced_parentheses("switch (at(1)) {", 7)   # -> "at(1)"
    """
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
            if depth == 0:
                return text[opening + 1:index]
    return text[opening + 1:]


def decisions(name, text):
    """Every decision a member function makes, in order.

    Yields ("switch", class, function, subject), and ("arm", class, function,
    subject, ladder) for each if in an if-else ladder, where ladder
    identifies the ladder the arm belongs to. A free function yields nothing.

    Example:
        list(decisions("a.cpp", "int A::f() { switch (k) { } }"))
        # -> [("switch", "A", "f", "k")]
    """
    blanked = blank(text)
    scopes = []
    ladders = {}
    ladder_count = 0
    statement_start = 0
    events = sorted(
        [(match.start(), "brace") for match in re.finditer(r"[{}]", blanked)]
        + [(match.start(), "end") for match in re.finditer(";", blanked)]
        + [(match.start(), "switch")
           for match in re.finditer(r"\bswitch\s*\(", blanked)]
        + [(match.start(), "if")
           for match in re.finditer(r"\bif\s*\(", blanked)])
    for position, kind in events:
        function = next((scope for scope in reversed(scopes)
                         if scope[0] == "function"), None)
        if kind == "end":
            statement_start = position + 1
        elif kind == "brace" and blanked[position] == "{":
            statement = blanked[statement_start:position]
            owner = function_of(statement, scopes)
            head = CLASS_HEAD.search(statement)
            # A lambda's body continues the function it sits in.
            if owner is not None and function is None:
                scopes.append(("function", owner[0], owner[1]))
            elif head and "enum" not in statement:
                scopes.append(("class", head.group(1)))
            else:
                scopes.append(("block",))
            statement_start = position + 1
        elif kind == "brace":
            if scopes:
                scopes.pop()
            statement_start = position + 1
        elif function is None or function[1] is None:
            continue
        elif kind == "switch":
            inside = balanced_parentheses(blanked, blanked.index("(", position))
            yield ("switch", function[1], function[2], "".join(inside.split()))
        else:
            inside = balanced_parentheses(blanked, blanked.index("(", position))
            comparison = inside.find("==")
            subject = ""
            if comparison != -1:
                subject = subject_before(inside, comparison)
            is_else = blanked[:position].rstrip().endswith("else")
            depth = len(scopes)
            if not is_else or depth not in ladders:
                ladder_count += 1
                ladders[depth] = (name, ladder_count)
            if subject:
                yield ("arm", function[1], function[2], subject, ladders[depth])


def find_dispatch(files):
    """Every class deciding on one subject in 3 or more functions, worst first.

    Example:
        find_dispatch({"shape.cpp": text})
        # -> [{"class": "Shape", "subject": "kind",
        #      "functions": ["name", "sides", "weight"],
        #      "files": ["shape.cpp"]}]
        find_dispatch({"pair.cpp": two_switches})   # -> [], under three
    """
    deciders = {}
    arm_counts = {}
    for name, text in files.items():
        for decision in decisions(name, text):
            if decision[0] == "switch":
                _, owner, function, subject = decision
                functions = deciders.setdefault((owner, subject), {})
                functions.setdefault(function, set()).add(name)
            else:
                _, owner, function, subject, ladder = decision
                key = (owner, subject, function, ladder)
                count, names = arm_counts.get(key, (0, set()))
                arm_counts[key] = (count + 1, names | {name})
    # A ladder decides on its subject from its third arm; two are if and else.
    for (owner, subject, function, _), (count, names) in arm_counts.items():
        if count >= LEAST_LADDER_ARMS:
            functions = deciders.setdefault((owner, subject), {})
            functions.setdefault(function, set()).update(names)
    found = []
    for (owner, subject), functions in deciders.items():
        if len(functions) >= LEAST_DISPATCH_FUNCTIONS:
            found.append({
                "class": owner,
                "subject": subject,
                "functions": sorted(functions),
                "files": sorted(set().union(*functions.values()))})
    return sorted(found, key=dispatch_order)


def dispatch_order(entry):
    """Sorts dispatch findings by functions, most first, then by class.

    Example:
        dispatch_order({"functions": ["f", "g", "h"], "class": "A",
                        "subject": "k"})   # -> (-3, "A", "k")
    """
    return -len(entry["functions"]), entry["class"], entry["subject"]


def expected_findings(text):
    """The fixture's EXPECT markers, as the findings it must report.

    Example:
        expected_findings("return a == 1 || // EXPECT CHAIN a 4")
        # -> ({(1, "a", 4)}, set())
    """
    chains = set()
    dispatch = set()
    for number, line in enumerate(text.splitlines(), start=1):
        chain = re.search(r"EXPECT CHAIN (\S+) (\d+)", line)
        if chain:
            chains.add((number, chain.group(1), int(chain.group(2))))
        repeated = re.search(r"EXPECT DISPATCH (\w+) (\S+) (\d+)", line)
        if repeated:
            dispatch.add((repeated.group(1), repeated.group(2),
                          int(repeated.group(3))))
    return chains, dispatch


def self_test():
    """Runs the fixture; 0 when it reports exactly its markers, else 1.

    Example:
        self_test()
        # prints "self-test: 4 chains and 1 dispatches, none missed,
        # none extra" -> 0
    """
    text = FIXTURE.read_text(encoding="utf-8")
    expected_chains, expected_dispatch = expected_findings(text)
    found_chains = {(chain["line"], chain["subject"], chain["terms"])
                    for chain in find_chains(FIXTURE.name, text)}
    found_dispatch = {(entry["class"], entry["subject"],
                       len(entry["functions"]))
                      for entry in find_dispatch({FIXTURE.name: text})}
    failures = []
    for label, expected, found in (
            ("chains", expected_chains, found_chains),
            ("dispatch", expected_dispatch, found_dispatch)):
        missed = sorted(expected - found)
        extra = sorted(found - expected)
        if missed or extra:
            failures.append(f"{label}: missed {missed}, extra {extra}")
    if failures:
        print("self-test FAILED")
        for failure in failures:
            print("  " + failure)
        return 1
    print(f"self-test: {len(found_chains)} chains and {len(found_dispatch)}"
          " dispatches, none missed, none extra")
    return 0


def main():
    """Prints both reports for src/, or for the files named, and the totals.

    Example:
        main()   # with --self-test -> self_test()'s result
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--file", action="append", default=[],
                        help="read this file instead of src/")
    parser.add_argument("--self-test", action="store_true",
                        help="check the fixture's exact findings")
    arguments = parser.parse_args()
    if arguments.self_test:
        return self_test()

    paths = [pathlib.Path(name) for name in arguments.file] or sorted(
        path for path in SOURCE.iterdir() if path.suffix in (".cpp", ".h"))
    files = {path.name: path.read_text(encoding="utf-8") for path in paths}
    chains = sorted((chain for name, text in files.items()
                     for chain in find_chains(name, text)), key=chain_order)
    dispatch = find_dispatch(files)

    print(f"MEMBERSHIP CHAINS: one subject against {LEAST_CHAIN_TERMS}"
          " or more values")
    for chain in chains:
        place = f"{chain['file']}:{chain['line']}"
        values = ", ".join(chain["values"])
        print(f"  {chain['terms']:3}  {place}  {chain['subject']}"
              f" {chain['operator']} {values}")
    print(f"REPEATED DISPATCH: one subject decided in"
          f" {LEAST_DISPATCH_FUNCTIONS} or more functions of a class")
    for entry in dispatch:
        functions = ", ".join(entry["functions"])
        where = ", ".join(entry["files"])
        print(f"  {len(entry['functions']):3}  {entry['class']} on"
              f" {entry['subject']}: {functions}  ({where})")
    chain_files = len({chain["file"] for chain in chains})
    classes = len({entry["class"] for entry in dispatch})
    print(f"{len(chains)} membership chains in {chain_files} files")
    print(f"{len(dispatch)} repeated dispatches in {classes} classes")
    return 1 if chains or dispatch else 0


if __name__ == "__main__":
    sys.exit(main())
