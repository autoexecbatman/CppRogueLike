"""Finds classes whose access sections are out of the house order.

The house order is private, then protected, then public (CLAUDE.md, C++ rules). This
reads every class and struct body in the files it is given and reports each one whose
access sections ever step back down that order: a `private:` after a `public:`, or a
`protected:` after a `public:`. Repeating a label is not a finding, so a class may open
`private:` twice. A struct's members before its first label are public, so they count as
an implicit `public` section at that position.

    python docs/section_order.py              # every .h and .cpp directly in src/
    python docs/section_order.py src/Trap.h   # just the files named
    python docs/section_order.py --check      # the same report, exit 1 on a finding

Each finding is one line: the label that steps back, at its file:line, the class it is
in, and that class's labels in order.

    src/GameLoopCoordinator.h:24  GameLoopCoordinator  public -> private
    src/ItemEnhancements.h:131  ItemEnhancement  (public) -> private

An implicit section is shown in parentheses.

HOW IT READS. Comments, string literals and character literals are blanked first, line
breaks kept, so line numbers stay true. Then a brace stack: a `{` opens a class body
when the text since the previous `;`, `{` or `}` ends in `class Name` or `struct Name`
(with anything after the name, such as a base list) and that keyword does not follow
`enum`. Every other `{` is an ordinary block. A label counts only at the top of its own
class body, so a nested class keeps its own order and a function body holds none.

WHAT IT CANNOT SEE. A class's members before its first label are private, the lowest
rank, so they can never be out of order and are not read. Macros that expand to labels
or class heads are invisible, and so is code the preprocessor removes. It reads text,
never a build.

FALSIFICATIONS, run 2026-10-17 before the count was trusted:
1. GameLoopCoordinator.h as it stood at 68314610 (public then private) was reported at
   its `private:` line; after the sections were swapped the same file reports nothing.
2. A scratch file holding an `enum class` with a colon-typed base, a template with
   `class T` in its parameter list, a nested struct whose own order is right inside an
   outer class whose order is wrong, a label inside a string literal and one inside a
   comment: exactly one finding, the outer class's, at the right line.
3. Run 2026-10-19, when the implicit public section of a struct was added: with
   ItemEnhancements.h as it stood at 1164f488 (28 lines of members, then `private:`),
   every file in src/ gave exactly one finding, ItemEnhancement at its `private:` line.
   Before the rule it reported 0. After the private functions moved above an explicit
   `public:`, the tree reports 0 again.
4. The same day, a scratch header of six structs and classes: members then an explicit
   `public:`, an empty struct, a template with `class T`, a nested enum before `private:`,
   a class with leading members, and a struct opening `private:`. Exactly two findings,
   the template and the nested enum, each at its `private:` line.
"""

import argparse
import pathlib
import re
import sys

RANK = {"private": 0, "protected": 1, "public": 2}
LABEL = re.compile(r"\b(public|protected|private)\s*:(?!:)")
CLASS_HEAD = re.compile(r"\b(enum\s+)?(class|struct)\s+([A-Za-z_]\w*)")
# A struct's members before its first label are public, shown as an implicit section.
IMPLICIT_PUBLIC = "(public)"


def blank_comments_and_literals(text):
    """Returns text with comments and literals replaced by spaces, newlines kept.

    Example:
        blank_comments_and_literals('a // b\\n"c";')   # -> 'a     \\n   ;'
    """
    out = []
    index = 0
    length = len(text)
    while index < length:
        pair = text[index:index + 2]
        char = text[index]
        # A line comment runs to the line's end, which is kept.
        if pair == "//":
            end = text.find("\n", index)
            end = length if end == -1 else end
            out.append(" " * (end - index))
            index = end
            continue
        # A block comment may span lines; its line breaks are kept.
        if pair == "/*":
            end = text.find("*/", index + 2)
            end = length if end == -1 else end + 2
            out.append(re.sub(r"[^\n]", " ", text[index:end]))
            index = end
            continue
        # A string or character literal, with escapes skipped over.
        if char in "\"'":
            end = index + 1
            while end < length and text[end] != char:
                end += 2 if text[end] == "\\" else 1
            end = min(end + 1, length)
            out.append(re.sub(r"[^\n]", " ", text[index:end]))
            index = end
            continue
        out.append(char)
        index += 1
    return "".join(out)


def class_name_opened(statement):
    """Returns (name, keyword) for the class a `{` after this text opens, or None for a block.

    Example:
        class_name_opened("class Trap : public TileFeature\\n")   # -> ('Trap', 'class')
        class_name_opened("struct Box\\n")                        # -> ('Box', 'struct')
        class_name_opened("enum class TrapType\\n")               # -> None
        class_name_opened("void Trap::save(json& j)\\n")          # -> None
    """
    heads = list(CLASS_HEAD.finditer(statement))
    if not heads:
        return None
    # The last head is the one the brace belongs to: "template <class T> struct Box".
    last = heads[-1]
    if last.group(1):
        return None
    # A parenthesis after the name means a function returning or taking a class.
    if "(" in statement[last.end():]:
        return None
    return last.group(3), last.group(2)


def findings_in(path):
    """Returns (line, class, labels) for each class in the file out of house order.

    Example, on GameLoopCoordinator.h with public before private:
        findings_in(Path("src/GameLoopCoordinator.h"))
        # -> [(24, 'GameLoopCoordinator', ['public', 'private'])]
    """
    text = blank_comments_and_literals(path.read_text(encoding="utf-8", errors="replace"))
    # Each frame is (class head or None, [(label, line), ...]); a head is (name, keyword).
    stack = []
    statement_start = 0
    found = []
    index = 0
    line = 1
    while index < len(text):
        char = text[index]
        if char == "\n":
            line += 1
        elif char == "{":
            stack.append((class_name_opened(text[statement_start:index]), []))
            statement_start = index + 1
        elif char == "}":
            if stack:
                head, labels = stack.pop()
                found.extend(out_of_order(head[0] if head else None, labels))
            statement_start = index + 1
        elif char == ";":
            statement_start = index + 1
        elif stack and stack[-1][0] is not None:
            # A label only counts at the top of its own class body.
            match = LABEL.match(text, index)
            if match and (index == 0 or not (text[index - 1].isalnum() or text[index - 1] == "_")):
                stack[-1][1].append((match.group(1), line))
                index = match.end()
                continue
            # The first member of a struct, before any label, opens its implicit public section.
            if stack[-1][0][1] == "struct" and not stack[-1][1] and not char.isspace():
                stack[-1][1].append((IMPLICIT_PUBLIC, line))
        index += 1
    return found


def out_of_order(name, labels):
    """Returns the first step down the house order in one class, or nothing.

    Example:
        out_of_order("Box", [("public", 3), ("private", 9)])   # -> [(9, 'Box', ['public', 'private'])]
        out_of_order("Box", [("private", 3), ("public", 9)])   # -> []
        out_of_order("Box", [("(public)", 2), ("private", 9)]) # -> [(9, 'Box', ['(public)', 'private'])]
    """
    if name is None:
        return []
    highest = -1
    for label, line in labels:
        rank = RANK["public"] if label == IMPLICIT_PUBLIC else RANK[label]
        if rank < highest:
            return [(line, name, [each for each, _ in labels])]
        highest = max(highest, rank)
    return []


def main():
    parser = argparse.ArgumentParser(description="Classes whose access sections are out of the house order.")
    parser.add_argument("paths", nargs="*", help="files to read; every .h and .cpp in src/ when none are named")
    parser.add_argument("--check", action="store_true", help="exit 1 when anything is reported")
    arguments = parser.parse_args()

    root = pathlib.Path(__file__).resolve().parent.parent
    paths = [pathlib.Path(each) for each in arguments.paths] or sorted(
        list((root / "src").glob("*.h")) + list((root / "src").glob("*.cpp")))

    total = 0
    for path in paths:
        for line, name, labels in findings_in(path):
            shown = path.relative_to(root) if path.is_absolute() and root in path.parents else path
            print(f"{shown.as_posix()}:{line}  {name}  {' -> '.join(labels)}")
            total += 1
    print(f"{total} classes out of order in {len(paths)} files")
    return 1 if arguments.check and total else 0


if __name__ == "__main__":
    sys.exit(main())
