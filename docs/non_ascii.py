"""Reports every character above 127 in the project's source and console text.

The project rule: source is ASCII. A character above 127 comes back as mojibake on a console
using the system code page, and the game's font has no glyph for it. Comments are not exempt.

    python docs/non_ascii.py                  # every hit as file:line:column, then the total
    python docs/non_ascii.py --file other.cpp # read that file instead of the project
    python docs/non_ascii.py --self-test      # the fixture must give exactly its three hits

It exits 1 on any hit and 0 on none. What it reads:

    src/*.cpp, src/*.h, src/json/*.json       the game and the tables it packs
    tests/**/*.cpp, *.h, *.ps1, *.json        tests, the probe script, the mutation plans
    docs/*.py                                 the scripts, whose output reaches a console
    data/**/*.json                            content the game prints

Example, on a file holding an em dash in a comment:

    $ python docs/non_ascii.py --file Thing.cpp
    Thing.cpp:12:30  U+2014  EM DASH
    1 character above 127 in 1 file

FALSIFIED, 2026-10-18. --self-test reads docs/non_ascii_fixture.cpp.txt, which holds a byte
order mark, an em dash in a comment and an arrow after code, and requires exactly those three
at their line and column. Run against a stub that exited 2, it failed; then an em dash put
into a src/ comment was reported at its line and column, and the file was restored.
Columns count characters from 1, and a byte order mark is the first character of line 1.
"""

import argparse
import pathlib
import sys
import unicodedata

ROOT = pathlib.Path(__file__).resolve().parent.parent
SCANNED = (
    ("src", ("*.cpp", "*.h")),
    ("src/json", ("*.json",)),
    ("tests", ("**/*.cpp", "**/*.h", "**/*.ps1", "**/*.json")),
    ("docs", ("*.py",)),
    ("data", ("**/*.json",)),
)

# The fixture's hits as (line, column, codepoint); any other set fails --self-test.
FIXTURE = pathlib.Path(__file__).resolve().parent / "non_ascii_fixture.cpp.txt"
FIXTURE_HITS = ((1, 1, 0xFEFF), (3, 15, 0x2014), (4, 24, 0x2192))


def hits_in(text):
    """Returns (line, column, codepoint) for every character above 127, both counted from 1.

    Example:
        hits_in("a\\n// x \\u2014 y")  # -> [(2, 6, 0x2014)]
    """
    found = []
    for number, line in enumerate(text.split("\n"), 1):
        for column, character in enumerate(line, 1):
            if ord(character) > 127:
                found.append((number, column, ord(character)))
    return found


def scanned_files():
    """Returns every file the rule covers, sorted, each once.

    Example:
        scanned_files()  # -> [PosixPath('E:/repo/C++RogueLike/data/content/items.json'), ...]
    """
    files = set()
    for folder, patterns in SCANNED:
        for pattern in patterns:
            files.update(path for path in (ROOT / folder).glob(pattern) if path.is_file())
    return sorted(files)


def self_test():
    """Reads the fixture and returns 0 when it gives exactly FIXTURE_HITS, 1 otherwise.

    Example:
        self_test()  # prints "self-test: 3 of 3 hits, none missed, none extra" -> 0
    """
    reported = set(hits_in(FIXTURE.read_bytes().decode("utf-8")))
    missed = sorted(set(FIXTURE_HITS) - reported)
    extra = sorted(reported - set(FIXTURE_HITS))
    if missed or extra:
        print(f"self-test FAILED: missed {missed}, extra {extra}")
        return 1
    print(f"self-test: {len(reported)} of {len(FIXTURE_HITS)} hits, none missed, none extra")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--file", action="append", default=[], help="read this file instead of the project")
    parser.add_argument("--self-test", action="store_true", help="check the fixture's exact hits")
    arguments = parser.parse_args()
    if arguments.self_test:
        return self_test()

    paths = [pathlib.Path(name) for name in arguments.file] or scanned_files()
    total = 0
    files = 0
    for path in paths:
        # Decoding as UTF-8 is the claim being checked, so a file that is not UTF-8 is a hit too.
        try:
            text = path.read_bytes().decode("utf-8")
        except UnicodeDecodeError as error:
            print(f"{path.as_posix()}: not UTF-8 ({error.reason} at byte {error.start})")
            total += 1
            files += 1
            continue
        found = hits_in(text)
        shown = path.relative_to(ROOT).as_posix() if path.is_absolute() and ROOT in path.parents else path.as_posix()
        for line, column, codepoint in found:
            print(f"{shown}:{line}:{column}  U+{codepoint:04X}  {unicodedata.name(chr(codepoint), 'UNNAMED')}")
        total += len(found)
        files += 1 if found else 0
    print(f"{total} character{'s' if total != 1 else ''} above 127 in {files} file{'s' if files != 1 else ''}")
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
