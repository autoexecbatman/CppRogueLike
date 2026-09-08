"""List every class and function a header in src/ declares, with the comment above it.

Clang parses the headers, so this reads real declarations rather than guessing at them
with a regular expression - templates, defaulted members, deleted operators and
multi-line signatures all arrive resolved, and access and deletedness are answered by the
compiler rather than inferred.

It uses libclang's cursor traversal, which is lazy. The JSON route was tried first in
the project this was ported from and abandoned on a measurement: `clang -ast-dump=json`
on a header pulling in a large dependency emitted 6.35 GB in 68 seconds, which no Python
process can load. The same header through a cursor took 9.4 seconds at constant memory.
Do not go back to the JSON dump.

`-fparse-all-comments` is what makes clang attach the plain `//` blocks this repository
writes. Doxygen ignores every one of them, which is why it is not used here.

Coverage is reported per file, and a declaration with no comment is a finding. Presence
is the whole of what it measures. Whether a comment says anything worth reading is
settled by reading it.

Usage:

    python docs/api_surface.py                     # writes docs/src_api.html
    python docs/api_surface.py --check             # report only, write nothing
    python docs/api_surface.py --only Creature.h   # one file, printed to the terminal

Exit codes: 0 every required declaration carries a comment, 1 at least one does not,
2 libclang is missing or a header could not be parsed at all.
"""

import argparse
import io
import html
import sys
from pathlib import Path

import build_index
import doc_page

try:
    import clang.cindex as cindex
except ImportError:  # reported by main rather than raised at import
    cindex = None

# The libclang bundled with the Python binding is older than this MSVC standard
# library accepts: every parse dies on `error STL1000: Unexpected compiler version,
# expected Clang 20 or newer`, and the translation unit never loads. Visual Studio
# ships a current one, so point the binding at that. Checked 2026-09-08: clang
# 22.1.3 parses Creature.h with zero fatal diagnostics where the bundled build
# produced twenty and then failed to build a translation unit at all.
VISUAL_STUDIO_LIBCLANG = (
    "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/libclang.dll"
)
if cindex is not None and Path(VISUAL_STUDIO_LIBCLANG).exists():
    cindex.Config.set_library_file(VISUAL_STUDIO_LIBCLANG)


def tolerate_unknown_enum_ids():
    """Stop the Python binding rejecting enum values its libclang is newer than.

    The binding is libclang 18.1.1 - the newest on PyPI - and the DLL above is
    22.1.3. The pairing is forced: an 18-era DLL cannot parse this MSVC standard
    library at all, dying on `error STL1000: Unexpected compiler version, expected
    Clang 20 or newer` before a translation unit exists. So the DLL reports kinds
    the binding's tables do not list, and BaseEnumeration.from_id raises rather
    than returning one.

    The raise is on BaseEnumeration, shared by every enum, and its message is
    hardcoded to "Unknown template argument kind" whatever the class - so the text
    names the wrong enum. The real one here is CursorKind: newer clang emits
    cursor kinds 18 has no entry for.

    An unknown id becomes a synthesized member of the right class, named
    UNKNOWN_<id>. Traversal continues, and because the name matches nothing in
    LISTED_KINDS the declaration is skipped rather than misreported.

    Example:

        tolerate_unknown_enum_ids()
        cindex.CursorKind.from_id(155).name    # -> 'UNKNOWN_155'
        cindex.CursorKind.from_id(5).name      # -> 'STRUCT_DECL'
    """
    base = cindex.BaseEnumeration
    original = base.from_id.__func__

    def from_id(cls, id):
        try:
            return original(cls, id)
        except ValueError:
            # Build the member directly: __init__ would refuse a second load, and
            # _name_map is rebuilt lazily from the class dict, so name it there.
            member = cls.__new__(cls)
            member.value = id
            while len(cls._kinds) <= id:
                cls._kinds.append(None)
            cls._kinds[id] = member
            setattr(cls, f"UNKNOWN_{id}", member)
            cls._name_map = None
            return member

    base.from_id = classmethod(from_id)


if cindex is not None:
    tolerate_unknown_enum_ids()

# Where the project's headers and its dependencies live. raylib and nlohmann_json come
# from vcpkg for the native build; the path is repeated here because clang is being run
# directly rather than through the build. The Emscripten build fetches its own copies
# into build-web-warn/_deps, which this does not read.
INCLUDE_PATHS = (
    "src",
    "E:/dev/vcpkg/installed/x64-windows/include",
)

# What appears on the page. A typedef or a friend declaration is not something a caller
# documents, and listing it would dilute the coverage number.
LISTED_KINDS = frozenset(
    {
        "STRUCT_DECL",
        "CLASS_DECL",
        "ENUM_DECL",
        "FUNCTION_DECL",
        "CXX_METHOD",
        "CONSTRUCTOR",
        "FIELD_DECL",
        "VAR_DECL",
    }
)

# What a missing comment is a finding on: everything listed. A constant carries a
# meaning its name and value do not give - which colour is the head and which the tail -
# so a group commented once leaves the rest of the group unexplained.
REQUIRES_COMMENT = LISTED_KINDS

# Where a definition can have linkage of its own. Anything declared inside a function
# body is a local: it has no linkage, so it is not this page's business.
SCOPES_WITH_LINKAGE = frozenset(
    {"TRANSLATION_UNIT", "NAMESPACE", "STRUCT_DECL", "CLASS_DECL"}
)

# The kinds that can be = delete.
METHOD_KINDS = frozenset({"CXX_METHOD", "CONSTRUCTOR"})


def signature_of(cursor):
    """The declaration's type, with constexpr restored when the source said so.

    A constexpr variable has a const type, so clang reports `constexpr int CELL = 22`
    as `const int` and the page cannot tell a compile-time constant from a runtime one.
    The keyword is recovered from the declaration's own tokens.

    Example:

        signature_of(cursor_for("constexpr int CELL = 22;"))
        # 'constexpr int'

    Args:
        cursor: any declaration cursor.
    """
    # A cursor with no type at all - a namespace, say - has no signature to show.
    if cursor.type is None:
        return ""
    spelling = cursor.type.spelling
    # Only a variable or field can be constexpr in a way the type hides.
    if cursor.kind.name not in {"VAR_DECL", "FIELD_DECL"}:
        return spelling
    # Read the declaration's own tokens; the keyword precedes the type.
    for token in cursor.get_tokens():
        if token.spelling == "constexpr":
            return "constexpr " + spelling[len("const "):] if spelling.startswith("const ") else "constexpr " + spelling
        # Stop at the name: anything after it belongs to the initialiser.
        if token.spelling == cursor.spelling:
            break
    return spelling

def clang_arguments():
    """The command line clang is given for every header.

    Warnings are off. C++23 rather than 20: this project uses std::expected and
    deducing this, and parsing it as C++20 misreads what the headers declare. The
    libclang being driven is Visual Studio's, not the one bundled with the Python
    binding - see VISUAL_STUDIO_LIBCLANG above for why.

    Example:

        "-std=c++23" in clang_arguments()          # True
        any(a.startswith("-Isrc") for a in clang_arguments())   # True
    """
    arguments = ["-std=c++23", "-x", "c++", "-fparse-all-comments", "-w"]
    arguments.extend(f"-I{path}" for path in INCLUDE_PATHS)
    return arguments


def comment_segments(raw):
    """A raw comment split into prose paragraphs and code blocks, in order.

    Only the marker stripping is here: clang hands back raw_comment with its `//` intact,
    and what to do with the stripped lines is doc_page's, because the file index renders
    the same blocks.

    Example:

        comment_segments("// What it is." + chr(10) + "//" + chr(10) + "//     call();")
        # [('prose', 'What it is.'), ('code', 'call();')]

        comment_segments(None)   # []

    Args:
        raw: cursor.raw_comment, which is None where no comment attaches.
    """
    if not raw:
        return []
    lines = []
    for line in raw.splitlines():
        stripped = line.lstrip()
        # Every marker this codebase, or a stray Doxygen block, might use.
        for marker in ("///", "//!", "//", "/**", "/*", "*/"):
            if stripped.startswith(marker):
                stripped = stripped[len(marker) :]
                break
        else:
            if stripped.startswith("*"):
                stripped = stripped[1:]
        # One space after the marker is the separator, not indentation.
        if stripped.startswith(" "):
            stripped = stripped[1:]
        lines.append(stripped.rstrip())
    return doc_page.segments_from_lines(lines)


def flatten(segments):
    """The prose of a comment as one line, for a terminal report and an emptiness test.

    Example:

        flatten([('prose', 'What it is.'), ('code', 'call();')])   # 'What it is.'
        flatten([])                                                # ''

    Args:
        segments: what comment_segments returned.
    """
    return " ".join(body for kind, body in segments if kind == "prose").strip()


def declarations_in(header, index):
    """Every listed declaration the header itself introduces, with its comment.

    Only public declarations are kept. A private or protected member is implementation,
    and a private nested type's own members are public by the struct default, so without
    the parent check the walk would climb back out of the hiding and list them.

    Example:

        found = declarations_in(Path("src/value_range.h"), cindex.Index.create())
        [entry["name"] for entry in found]
        # ['ValueRange', 'observe', 'normalize', 'isEstablished']

    Args:
        header: path to the header to parse.
        index: a clang Index, reused across headers so the library loads once.
    """
    unit = index.parse(str(header), args=clang_arguments())
    if unit is None:
        raise RuntimeError(f"libclang could not parse {header}")

    target = header.name
    found = []
    for cursor in unit.cursor.walk_preorder():
        location = cursor.location.file
        # A declaration from an include is not this header's surface.
        if location is None or Path(location.name).name != target:
            continue
        kind = cursor.kind.name
        if kind not in LISTED_KINDS or not cursor.spelling:
            continue
        # INVALID is what clang reports at namespace scope, where access does not apply.
        if cursor.access_specifier.name not in ("PUBLIC", "INVALID"):
            continue
        parent = cursor.semantic_parent
        # A local inside an inline function body is not the header's surface.
        if parent is None or parent.kind.name not in SCOPES_WITH_LINKAGE:
            continue
        # A hidden enclosing type takes its members with it.
        if parent.access_specifier.name in ("PRIVATE", "PROTECTED"):
            continue
        # A deleted member is one of a block written under a single comment - "this type
        # has no value semantics" is said once, not five times.
        deleted = kind in METHOD_KINDS and cursor.is_deleted_method()
        enclosing = ""
        if parent is not None and parent.kind.name in ("STRUCT_DECL", "CLASS_DECL"):
            enclosing = parent.spelling
        found.append(
            {
                "kind": kind,
                "name": cursor.spelling,
                "signature": signature_of(cursor),
                "segments": comment_segments(cursor.raw_comment),
                "comment": flatten(comment_segments(cursor.raw_comment)),
                "line": cursor.location.line,
                "required": kind in REQUIRES_COMMENT and not deleted,
                "deleted": deleted,
                "enclosing": enclosing,
            }
        )
    return found


def is_internal(cursor):
    """Whether this definition is visible only inside its own translation unit.

    Two ways a definition gets internal linkage, and both are asked of clang rather than
    of the text: the `static` keyword, and sitting inside an unnamed namespace, which
    clang reports as a namespace whose spelling is empty.

    Example:

        is_internal(cursor_for("namespace { void helper() {} }"))   # True
        is_internal(cursor_for("void drawBoard() {}"))              # False

    Args:
        cursor: any definition cursor.
    """
    # The static keyword, on a free function or a variable.
    if cursor.storage_class is not None and cursor.storage_class.name == "STATIC":
        return True
    # Otherwise walk out through the enclosing scopes looking for an unnamed namespace.
    parent = cursor.semantic_parent
    while parent is not None:
        if parent.kind.name == "NAMESPACE" and not parent.spelling:
            return True
        parent = parent.semantic_parent
    return False


def internal_definitions_in(source, index):
    """Every internal-linkage definition a .cpp file introduces, with its comment.

    These are not API surface - nothing outside the file can call them - so they are
    listed apart from the headers. They are still required to carry a comment: a
    file-local helper's contract exists nowhere but in its own body, which is the one
    place a caller reading the header will never look.

    Example:

        found = internal_definitions_in(Path("src/board_render.cpp"), cindex.Index.create())
        [entry["name"] for entry in found]
        # ['loadOne', 'unloadOne', 'cellRect']

    Args:
        source: path to the .cpp file to parse.
        index: a clang Index, reused across files so the library loads once.
    """
    unit = index.parse(str(source), args=clang_arguments())
    if unit is None:
        raise RuntimeError(f"libclang could not parse {source}")

    target = source.name
    found = []
    for cursor in unit.cursor.walk_preorder():
        location = cursor.location.file
        # A definition pulled in from a header belongs to that header's section.
        if location is None or Path(location.name).name != target:
            continue
        kind = cursor.kind.name
        if kind not in LISTED_KINDS or not cursor.spelling:
            continue
        # A declaration with no body is the header's business, not this file's.
        if not cursor.is_definition():
            continue
        if not is_internal(cursor):
            continue
        parent = cursor.semantic_parent
        # A local inside a function body has no linkage at all, and the walk out of it
        # would find the enclosing unnamed namespace and call it internal.
        if parent is None or parent.kind.name not in SCOPES_WITH_LINKAGE:
            continue
        enclosing = ""
        if parent is not None and parent.kind.name in ("STRUCT_DECL", "CLASS_DECL"):
            enclosing = parent.spelling
        found.append(
            {
                "kind": kind,
                "name": cursor.spelling,
                "signature": signature_of(cursor),
                "segments": comment_segments(cursor.raw_comment),
                "comment": flatten(comment_segments(cursor.raw_comment)),
                "line": cursor.location.line,
                "required": kind in REQUIRES_COMMENT,
                "deleted": False,
                "enclosing": enclosing,
            }
        )
    return found


# Where parsed results are kept between runs. Not committed: it is derived, and a stale
# one is invisible in a diff.
CACHE_PATH = Path("build/api_surface_cache.json")


def cache_key(source):
    """What identifies one file's parse, including the parser that produced it.

    Size and modification time catch an edit to the source. The digest of this module
    is in the key as well, so changing what the parser looks for invalidates every
    entry - a cache that outlived its parser would serve findings the current code
    would no longer report, which is the one failure a cache must not have.

    Example:

        cache_key(Path("src/Actor/Creature.h"))
        # '5f2a1c...:18422:1755800000'

    Args:
        source: the header or .cpp being parsed.
    """
    import hashlib

    stat = source.stat()
    parser = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()[:12]
    return f"{parser}:{stat.st_size}:{int(stat.st_mtime)}"


def load_cache():
    """The stored parses, or an empty mapping when there is no usable cache.

    A corrupt or unreadable cache is not an error worth stopping for - it means the
    next run reparses everything, which is what would have happened without it.

    Example:

        load_cache()          # {'Creature.h': {'key': '5f2a...', 'declarations': [...]}}

    Args:
        none.
    """
    import json

    try:
        return json.loads(CACHE_PATH.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def save_cache(cache):
    """Writes the parses back, creating the directory if it is not there.

    Example:

        save_cache({"Creature.h": {"key": "5f2a...", "declarations": []}})

    Args:
        cache: the mapping load_cache would read.
    """
    import json

    CACHE_PATH.parent.mkdir(parents=True, exist_ok=True)
    CACHE_PATH.write_text(json.dumps(cache), encoding="utf-8")


def parse_with_cache(source, index, cache, read):
    """One file's declarations, from the cache when its key still matches.

    Example:

        parse_with_cache(Path("src/Actor/Creature.h"), index, {}, declarations_in)[0]["name"]
        # 'MonteCarloSearch'

    Args:
        source: the file to parse.
        index: a clang Index, created only when something must actually be parsed.
        cache: the mapping from load_cache, updated in place.
        read: declarations_in for a header, internal_definitions_in for a .cpp.
    """
    key = cache_key(source)
    stored = cache.get(source.name)
    if stored is not None and stored["key"] == key:
        # Segments come back from JSON as lists; the renderer unpacks pairs either way.
        return stored["declarations"]
    found = read(source, index())
    cache[source.name] = {"key": key, "declarations": found}
    return found


def file_block(path):
    """The file's opening documentation block, as renderable segments.

    Empty for a file that opens with no block - which the index page already reports as a
    finding, so this stays silent rather than counting the same gap twice.

    Example:

        file_block(Path("src/az_coverage.cpp"))[0][0]
        # 'prose'

    Args:
        path: the header or .cpp to read.
    """
    # utf-8-sig, as build_index reads them: several files here carry a BOM, and it
    # lands on the first line so the leading "//" no longer starts the line.
    source = path.read_text(encoding="utf-8-sig")
    lines = build_index.extract_block(source)
    if not lines:
        return []
    # extract_block has already removed the markers; stripping again eats two
    # characters of the first word.
    return doc_page.segments_from_lines(lines)


def collect(source_directory):
    """Every header paired with the declarations it introduces.

    Example:

        parsed = collect(Path("src"))
        parsed[0]["file"]                # 'az_network.h'
        len(parsed[0]["declarations"])   # 22

    Args:
        source_directory: directory of headers and sources to parse, non-recursively.
    """
    cache = load_cache()
    # Created on first use, so a fully cached run never loads libclang at all.
    made = []

    def index():
        if not made:
            made.append(cindex.Index.create())
        return made[0]

    # Recursive: this repository nests src/ by subsystem. src/Debug is Visual Studio
    # build output and .cpp.test.cpp files are in no CMakeLists, so neither is source.
    def sources(pattern):
        return [
            path for path in sorted(source_directory.rglob(pattern))
            if "Debug" not in path.parts and not path.name.endswith(".test.cpp")
        ]

    parsed = [
        {"file": header.name, "internal": False, "block": file_block(header),
         "declarations": parse_with_cache(header, index, cache, declarations_in)}
        for header in sources("*.h")
    ]
    # A .cpp contributes only what nothing outside it can reach.
    parsed.extend(
        {"file": source.name, "internal": True, "block": file_block(source),
         "declarations": parse_with_cache(source, index, cache, internal_definitions_in)}
        for source in sources("*.cpp")
    )
    save_cache(cache)
    # A file's internals belong beside its own header, not in a block of sources at the
    # end - reading one unit should not mean scrolling past every other.
    return sorted(parsed, key=lambda entry: (Path(entry["file"]).stem, entry["internal"]))


def render(parsed):
    """The whole page, as one self-contained HTML string.

    Example:

        page = render(collect(Path("src")))
        page.startswith("<!DOCTYPE html>")   # True

    Args:
        parsed: records from collect.
    """
    total = sum(len(entry["declarations"]) for entry in parsed)
    required = [
        declaration
        for entry in parsed
        for declaration in entry["declarations"]
        if declaration["required"]
    ]
    documented = sum(1 for declaration in required if declaration["comment"])
    parts = [
        doc_page.head("src api surface", "api", doc_page.source_stamp(Path("src"))),
        "<h1>src, declaration by declaration</h1>",
        f'<p class="sub">{total} declarations across {len(parsed)} files. '
        f"{documented} of {len(required)} carry a comment. Headers give the API surface; "
        "a .cpp gives only its internal-linkage definitions, which nothing outside the "
        "file can call and whose contract is nowhere else. Parsed by libclang, not by "
        "pattern matching. Generated - regenerate rather than edit.</p>",
    ]
    # A file earns a section by having either declarations or a block. A program
    # whose only function is main has no declarations and still has to appear.
    listed = [entry for entry in parsed if entry["declarations"] or entry["block"]]
    parts.append('<div class="jump">')
    parts.extend(
        f'<a href="#{html.escape(entry["file"])}">{html.escape(entry["file"])}</a>'
        for entry in listed
        # The same rule the headings use: a folded section sits under its own header and
        # listing it here would double the list.
        if not (entry["internal"] and entry["declarations"])
    )
    parts.append("</div>")
    for entry in listed:
        # The id is the file name, which is unique and is what a reader would type. The
        # back link goes to the other page rather than up this one.
        # Folded only when there is something to fold: a .cpp with no file-local
        # declarations has nothing behind the summary, so it gets a plain heading.
        folded = entry["internal"] and entry["declarations"]
        if folded:
            # Folded shut. These are subordinate to the header above them, and open by
            # default they bury it - a reader looking for the module's surface would
            # scroll past every private helper to reach the next one.
            count = len(entry["declarations"])
            parts.append(
                f'<details class="internal" id="{html.escape(entry["file"])}">'
                f"<summary>{html.escape(entry['file'])}"
                f'<span class="tag">{count} file-local</span></summary>'
            )
        else:
            # The whole section folds, heading included. Shut by default, so the page
            # opens as an index of files and a reader opens the one they came for
            # rather than scrolling past 800 declarations to reach it.
            count = len(entry["declarations"])
            tag = (
                f'<span class="tag">{count} declaration{"" if count == 1 else "s"}</span>'
                if count
                else ""
            )
            parts.append(
                f'<details class="file" id="{html.escape(entry["file"])}">'
                f'<summary>{html.escape(entry["file"])}{tag}'
                f'<a class="back" href="src_index.html">what it is for</a></summary>'
            )
        # What the file is, above the list of what it declares.
        if entry["block"]:
            parts.append('<div class="fileblock">')
            parts.append(doc_page.render_comment(entry["block"], ""))
            parts.append("</div>")
        # A file with only a block has no table to draw, but its fold still closes.
        if not entry["declarations"]:
            parts.append("</details>")
            continue
        # Fixed layout: an example inside a cell would otherwise size the column to its
        # longest line and push the prose column off the page.
        parts.append('<div class="scroll"><table class="decls"><thead><tr>')
        parts.append("<th>Declaration</th><th>Type</th><th>What it is</th>")
        parts.append("</tr></thead><tbody>")
        for declaration in entry["declarations"]:
            owner = (
                f'<span class="owner">{html.escape(declaration["enclosing"])}::</span>'
                if declaration["enclosing"]
                else ""
            )
            # A deleted member is not undocumented, it is removed from the interface.
            # Saying "no comment" of one reads as an omission somebody should fix.
            if declaration["deleted"]:
                empty_note = ""
            else:
                empty_note = "no comment"
            said = doc_page.render_comment(declaration["segments"], empty_note)
            if declaration["deleted"]:
                said = '<span class="tag">deleted</span>' + (said if said else "")
            parts.append(
                f"<tr><td><code>{owner}{html.escape(declaration['name'])}</code></td>"
                f'<td><code>{html.escape(declaration["signature"])}</code></td>'
                f"<td>{said}</td></tr>"
            )
        parts.append("</tbody></table></div>")
        parts.append("</details>")
    parts.append(
        doc_page.foot(
            "Presence is all this measures. Whether a comment says anything worth "
            "reading is settled by reading it."
        )
    )
    return "\n".join(parts)


def main():
    """Parse the headers, report comment coverage, and write the page unless checking."""
    parser = argparse.ArgumentParser(description="List src/ declarations and their comments.")
    parser.add_argument("--source", default="src", help="directory of headers")
    parser.add_argument("--out", default="docs/src_api.html", help="page to write")
    parser.add_argument("--check", action="store_true", help="report only, write nothing")
    parser.add_argument("--only", default="", help="parse one header or .cpp and print it")
    arguments = parser.parse_args()

    if cindex is None:
        print("libclang is not installed: pip install libclang")
        return 2

    source_directory = Path(arguments.source)
    try:
        # One header at a time when asked, so a parse failure is readable.
        if arguments.only:
            # src/ is nested here, so a bare name is searched for rather than
            # joined: --only Creature.h finds src/Actor/Creature.h.
            direct = source_directory / arguments.only
            if direct.exists():
                chosen = direct
            else:
                found = [p for p in source_directory.rglob(arguments.only)
                         if "Debug" not in p.parts]
                if not found:
                    print(f"no file named {arguments.only} under {source_directory}")
                    return 2
                chosen = found[0]
            read = internal_definitions_in if chosen.suffix == ".cpp" else declarations_in
            for declaration in read(chosen, cindex.Index.create()):
                missing = declaration["required"] and not declaration["comment"]
                owner = f'{declaration["enclosing"]}::' if declaration["enclosing"] else ""
                mark = "! " if missing else "  "
                print(f'{mark}{owner}{declaration["name"]:<34} {declaration["signature"]}')
            return 0
        parsed = collect(source_directory)
    except (RuntimeError, OSError) as error:
        print(error)
        return 2

    # The findings first, so they are not below a summary line that reads as a pass.
    undocumented = [
        (entry["file"], declaration)
        for entry in parsed
        for declaration in entry["declarations"]
        if declaration["required"] and not declaration["comment"]
    ]
    for file_name, declaration in undocumented:
        owner = f'{declaration["enclosing"]}::' if declaration["enclosing"] else ""
        print(f'  {file_name:<26} {owner}{declaration["name"]:<30} no comment')

    if not arguments.check:
        out_path = Path(arguments.out)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        # CRLF, as everything in this repository is.
        out_path.write_text(render(parsed), encoding="utf-8", newline="\r\n")
        print(f"wrote {out_path}")

    total = len(
        [
            declaration
            for entry in parsed
            for declaration in entry["declarations"]
            if declaration["required"]
        ]
    )
    print(f"{total - len(undocumented)} of {total} required declarations documented, ", end="")
    print(f"{len(parsed)} files parsed")
    return 1 if undocumented else 0


if __name__ == "__main__":
    sys.exit(main())
