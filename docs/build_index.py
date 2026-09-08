"""Index every file in src/ by the documentation block it already carries.

The convention here is that a file opens with a comment block saying what it is and how
to use it. That block is good and it is invisible: reading it means opening 69 files.
This collects them into one page, so the map exists without a second copy of the prose
that could drift from it.

It is also a coverage check. A file with no block is a finding and the script exits
non-zero, the way docs/analyze.ps1 and docs/check_headers.ps1 do. What it can measure is
presence - whether a block exists, whether it shows usage. Whether the block is any good
is a thing only reading it settles, and no tool here claims otherwise.

The output is generated, so it is not tracked: regenerate it rather than reading a copy
that stopped being true three commits ago.

Usage:

    python docs/build_index.py                  # writes docs/src_index.html
    python docs/build_index.py --out other.html # elsewhere
    python docs/build_index.py --check          # report coverage, write nothing

Exit codes: 0 every file carries a block, 1 at least one does not.
"""

import argparse
import html
import sys
from pathlib import Path

import doc_page

# Lines that may precede a file's documentation block without ending the search for it.
# Anything else - a declaration, a namespace, a using - means the file has no block.
SKIPPABLE_PREFIXES = ("#pragma", "#include", "#ifndef", "#define", "#endif")


def is_skippable(line):
    """Whether a line may sit above the documentation block without being code.

    Example:

        is_skippable("#pragma once")     # True
        is_skippable("")                 # True  - blank lines separate the block
        is_skippable("class Foo")        # False - a declaration ends the search

    Args:
        line: one source line, without its newline.
    """
    stripped = line.strip()
    # A blank line separates the includes from the block and is not code.
    if not stripped:
        return True
    return stripped.startswith(SKIPPABLE_PREFIXES)


def strip_marker(line):
    """One comment line with its `//` removed and its indentation kept.

    The single space after the marker is the separator, not indentation - stripping the
    whole leading run instead would flatten every example into the prose.

    Example:

        strip_marker("// What it is.")       # 'What it is.'
        strip_marker("//     call();")       # '    call();'

    Args:
        line: a source line beginning with `//`.
    """
    body = line[2:]
    if body.startswith(" "):
        body = body[1:]
    return body.rstrip()


def extract_block(text):
    """Return the file's documentation block as a list of comment lines, or an empty list.

    The block is the first run of column-zero `//` lines that appears before any
    declaration. Indented comments are ignored: those are step comments inside a body,
    and treating one as a file header would put a sentence about one branch at the top of
    the page.

    Example:

        extract_block("#pragma once\\n\\n// What this is.\\n// More.\\nclass Foo;")
        # ['What this is.', 'More.']

        extract_block("class Foo;\\n// too late to be a header\\n")
        # []

    Args:
        text: the whole file, already decoded.
    """
    block = []
    for line in text.splitlines():
        # Inside the block: keep taking comment lines until one is not.
        if block:
            if line.startswith("//"):
                block.append(strip_marker(line))
                continue
            break
        # Before the block: a column-zero comment opens it.
        if line.startswith("//"):
            block.append(strip_marker(line))
            continue
        # Anything that is not a comment and not skippable means there is no block.
        if not is_skippable(line):
            break
    return block


def first_paragraph(block):
    """The block's opening paragraph, joined into one line.

    Example:

        first_paragraph(["What it is.", "", "Usage:", "    call()"])
        # 'What it is.'

    Args:
        block: the comment lines returned by extract_block.
    """
    paragraph = []
    for line in block:
        # A blank comment line ends the paragraph.
        if not line:
            break
        paragraph.append(line)
    return " ".join(paragraph)


def has_usage(block):
    """Whether the block shows a caller how to use the file.

    Presence only. A block whose usage section is one stale line still counts here, which
    is the limit of what any documentation tool can check.

    Example:

        has_usage(["What it is.", "", "Usage:", "    Thing thing(1);"])   # True
        has_usage(["What it is.", "", "Usage, from mcts.cpp:", "    f();"])  # True
        has_usage(["What it is."])                                       # False

    Args:
        block: the comment lines returned by extract_block.
    """
    for line in block:
        # The colon is not part of the convention: "Usage, from az_trainer.cpp:" and
        # "Usage - starts from scratch:" are both in use, and matching "Usage:" alone
        # missed five of the twelve blocks that carry one.
        if line.startswith("Usage") or line.startswith("Example"):
            return True
    return False


def collect(source_directory):
    """Every source file paired with what its documentation block says.

    Walks recursively: this repository nests src/ by subsystem, so a non-recursive glob
    finds three files and reports full coverage over them. Build output under src/Debug
    and unbuilt .cpp.test.cpp files are skipped - neither is source.

    Sorted by name within each group so the page is stable between runs and a diff of two
    runs shows a real change rather than a reordering.

    Example:

        entries = collect(Path("src"))
        entries[0]["name"]        # 'az_coverage.cpp'
        entries[0]["documented"]  # True

    Args:
        source_directory: the directory to walk, recursively.
    """
    entries = []
    for path in sorted(source_directory.rglob("*.h")) + sorted(source_directory.rglob("*.cpp")):
        # src/Debug holds Visual Studio build output, not source.
        if "Debug" in path.parts:
            continue
        # A .cpp.test.cpp is in no CMakeLists and is not built.
        if path.name.endswith(".test.cpp"):
            continue
        text = path.read_text(encoding="utf-8-sig")
        block = extract_block(text)
        entries.append(
            {
                "name": path.name,
                "lines": len(text.splitlines()),
                "block": block,
                "summary": first_paragraph(block),
                "usage": has_usage(block),
                "documented": bool(block),
                "is_test": path.name.endswith("_test.cpp"),
            }
        )
    return entries


def pair_up(entries):
    """Group the files into modules, standalone programs and tests.

    A module is a header and the .cpp that implements it. Listing the two in separate
    sections put mcts.h twenty rows away from mcts.cpp, which are one thing read two
    ways - the header says what the module promises and the source says how it keeps
    the promise, and a reader wants both at once.

    A .cpp with no matching header is a program: one main(), nothing to declare.

    Example:

        modules, programs, tests = pair_up(collect(Path("src")))
        modules[0]["stem"]                    # 'az_network'
        modules[0]["header"]["name"]          # 'az_network.h'
        modules[0]["source"]["name"]          # 'az_network.cpp'
        programs[0]["name"]                   # 'az_coverage.cpp'

    Args:
        entries: records from collect.
    """
    by_name = {entry["name"]: entry for entry in entries}
    tests = [entry for entry in entries if entry["is_test"]]

    modules = []
    paired_sources = set()
    for entry in entries:
        if not entry["name"].endswith(".h"):
            continue
        stem = entry["name"][:-2]
        source = by_name.get(stem + ".cpp")
        if source is not None:
            paired_sources.add(source["name"])
        modules.append({"stem": stem, "header": entry, "source": source})

    # Whatever is left: a .cpp that implements no header of its own, which in this
    # repository means a program with a main().
    programs = [
        entry
        for entry in entries
        if entry["name"].endswith(".cpp")
        and not entry["is_test"]
        and entry["name"] not in paired_sources
    ]
    return modules, programs, tests


def side(entry, role):
    """One half of a module: its file name, what it says, and how long it is.

    A header with no implementation still gets both halves rendered, with the missing
    side saying so - an absent panel would read as a layout bug rather than as a
    header-only module, which several here legitimately are.

    Example:

        side({"name": "evaluator.h", "summary": "What the search needs.",
              "lines": 44, "usage": False}, "header")
        # '<div class="side"><div class="role">header</div>...'

    Args:
        entry: one record from collect, or None where the file does not exist.
        role: what to label this half, "header" or "implementation".
    """
    if entry is None:
        return (
            f'<div class="side empty"><div class="role">{role}</div>'
            '<p class="missing">none - the header stands alone</p></div>'
        )
    # Linked only where the declarations page has a section: a .cpp never does, and
    # neither does a header clang could not parse.
    name_cell = (
        f'<a href="src_api.html#{html.escape(entry["name"])}">'
        f'<code>{html.escape(entry["name"])}</code></a>'
        if doc_page.anchor_exists(entry["name"])
        else f'<code>{html.escape(entry["name"])}</code>'
    )
    summary = doc_page.render_comment(
        doc_page.segments_from_lines(entry["block"]), "no documentation block"
    )
    usage = ' <span class="tag">usage</span>' if entry["usage"] else ""
    # Why a name is not a link, said out loud. Silence here reads as a broken link:
    # the reader cannot tell a header the declarations page could not parse from one
    # the generator forgot.
    return (
        f'<div class="side"><div class="role">{role}</div>'
        f'<div class="filename">{name_cell}'
        f'<span class="num">{entry["lines"]} lines</span>{usage}</div>'
        f"<p>{summary}</p></div>"
    )


def render(entries, source_label):
    """The whole page, as one self-contained HTML string.

    Example:

        page = render(collect(Path("src")), "src")
        page.startswith("<!DOCTYPE html>")   # True

    Args:
        entries: records from collect.
        source_label: what to call the directory in the page heading.
    """
    documented = sum(1 for entry in entries if entry["documented"])
    with_usage = sum(1 for entry in entries if entry["usage"])
    parts = [
        doc_page.head(f"{source_label} index", "files", doc_page.source_stamp(Path("src"))),
        f"<h1>{html.escape(source_label)}</h1>",
        f'<p class="sub">{len(entries)} files. {documented} carry a documentation block, '
        f"{with_usage} show usage. Generated - regenerate rather than edit.</p>",
    ]
    modules, programs, tests = pair_up(entries)

    parts.append(f"<h2>Modules <span class='meta'>({len(modules)})</span></h2>")
    parts.append(
        '<p class="meta">Header and implementation together: what the module promises, '
        "and how it keeps the promise. Every header name links to its declarations.</p>"
    )
    for module in modules:
        parts.append('<div class="pair">')
        parts.append(side(module["header"], "header"))
        parts.append(side(module["source"], "implementation"))
        parts.append("</div>")

    for title, rows in (("Programs", programs), ("Tests", tests)):
        if not rows:
            continue
        parts.append(f"<h2>{title} <span class='meta'>({len(rows)})</span></h2>")
        parts.append('<div class="scroll"><table><thead><tr>')
        parts.append("<th>File</th><th>What it is</th><th>Lines</th><th>Usage</th>")
        parts.append("</tr></thead><tbody>")
        for entry in rows:
            summary = doc_page.render_comment(
                doc_page.segments_from_lines(entry["block"]), "no documentation block"
            )
            parts.append(
                f'<tr><td><code>{html.escape(entry["name"])}</code></td>'
                f"<td>{summary}</td>"
                f'<td class="num">{entry["lines"]}</td>'
                f'<td class="num">{"yes" if entry["usage"] else "-"}</td></tr>'
            )
        parts.append("</tbody></table></div>")
    parts.append(
        doc_page.foot(
            "Presence is all this measures. Whether a block is any good is settled by "
            "reading it. Header names link to what they declare."
        )
    )
    return "\n".join(parts)


def main():
    """Build the index, or just report coverage, and exit non-zero on an undocumented file."""
    parser = argparse.ArgumentParser(description="Index src/ by its documentation blocks.")
    parser.add_argument("--source", default="src", help="directory to index")
    parser.add_argument("--out", default="docs/src_index.html", help="page to write")
    parser.add_argument("--check", action="store_true", help="report only, write nothing")
    arguments = parser.parse_args()

    source_directory = Path(arguments.source)
    if not source_directory.is_dir():
        print(f"no such directory: {source_directory}")
        return 1

    entries = collect(source_directory)
    if not entries:
        print(f"no sources found in {source_directory}")
        return 1

    # The finding, printed before the summary so it is not scrolled past.
    undocumented = [entry["name"] for entry in entries if not entry["documented"]]
    for name in undocumented:
        print(f"  {name:<32} no documentation block")

    if not arguments.check:
        out_path = Path(arguments.out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        # CRLF, as everything in this repository is.
        out_path.write_text(render(entries, arguments.source), encoding="utf-8", newline="\r\n")
        print(f"wrote {out_path}")

    documented = len(entries) - len(undocumented)
    with_usage = sum(1 for entry in entries if entry["usage"])
    print(f"{documented} of {len(entries)} documented, {with_usage} show usage")
    return 1 if undocumented else 0


if __name__ == "__main__":
    sys.exit(main())
