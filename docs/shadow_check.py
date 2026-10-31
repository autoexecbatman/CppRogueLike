"""Reports Clang -Wshadow warnings in src/, compiled the way the web build compiles them.

A rename that compiles can change behaviour only by name hiding: a renamed member now hides,
or is hidden by, a local or a parameter. This is the check that proves a rename, compared on
the touched files before and after. Each translation unit is compiled through
build-web-warn/compile_commands.json with the web build's own em++, flags and precompiled
header, plus -fsyntax-only -Wno-everything -Wshadow, so only shadow warnings print and nothing
is written. The tree is never reconfigured.

    python docs/shadow_check.py src/MenuSell.cpp        # those units only
    python docs/shadow_check.py --all                   # every src/*.cpp in the database
    python docs/shadow_check.py --self-test             # the planted shadow must give exactly 1

It prints each warning under src/ once, then the total, and exits 0: it reports and does not
gate, since the tree carries shadows nobody has fixed. --self-test exits 1 on a wrong count.
It needs a configured build-web-warn and the emsdk on PATH (source emsdk_env.sh first).

Example, the parameter 0274 removed, at the commit before it:

    $ python docs/shadow_check.py src/MenuSell.cpp
    E:/repo/C++RogueLike/src/MenuSell.cpp:101:38: warning: declaration shadows a field of 'MenuSell' [-Wshadow]
    1 -Wshadow warning under src/ across 1 unit

FALSIFIED, 2026-10-20. --self-test copies src/TileRefJson.cpp, which carries no shadow, to a
temporary directory with `{ const int record = 0; (void)record; }` added at the top of
parse_tile, whose parameter is `record`, and compiles the copy in the original's place. It
expects exactly that one warning. Its first run read 0, and the fault was in this script: it
kept only warnings whose path held /src/, and the copy lives elsewhere. A warning in the
substituted file is now kept by name.

FALSIFIED, 2026-10-31. The probe's site is found by PROBE_SIGNATURE with its indent stripped,
because an anchor written with the indent stopped matching once the namespace bodies were
indented. On the CppRL tree the self-test gives its 1 warning. With the probe changed to a
name that shadows nothing, it fails with "expected one warning at line 29, got []". With the
signature line deleted from TileRefJson.cpp, it fails naming the missing signature.
"""

import argparse
import json
import pathlib
import shlex
import subprocess
import sys
import tempfile
import typing

ROOT = pathlib.Path(__file__).resolve().parent.parent
DATABASE = ROOT / "build-web-warn" / "compile_commands.json"
EXTRA = ["-fsyntax-only", "-Wno-everything", "-Wshadow"]

# The self-test's planted shadow: a clean file, the function it goes into, and the probe.
PROBE_SOURCE = "src/TileRefJson.cpp"
PROBE_SIGNATURE = "TileRef parse_tile(const json& record)"
PROBE_LINE = "{ const int record = 0; (void)record; } // planted by shadow_check.py --self-test"


class PlantedProbe(typing.NamedTuple):
    """A copy of PROBE_SOURCE with PROBE_LINE in it, and the 1-based line it sits on."""

    text: str
    line: int


def planted_copy(original):
    """Returns original with PROBE_LINE as the first line of PROBE_SIGNATURE's body, or None.

    The signature is matched with its indent stripped, since the formatter owns the indent,
    and the probe takes the body brace's indent plus one tab. None means the signature is not
    on exactly one line, or the line after it is not the body's opening brace.

    Example:
        planted_copy("\\tTileRef parse_tile(const json& record)\\n\\t{\\n\\t}\\n")
        # -> PlantedProbe(text='\\tTileRef parse_tile(const json& record)\\n\\t{\\n
        #    \\t\\t{ const int record = 0; (void)record; } // planted by ...\\n\\t}\\n', line=3)
        planted_copy("int unrelated();\\n")   # -> None
    """
    newline = "\r\n" if "\r\n" in original else "\n"
    lines = original.split(newline)
    sites = [index for index, line in enumerate(lines) if line.strip() == PROBE_SIGNATURE]
    if len(sites) != 1 or sites[0] + 1 >= len(lines) or lines[sites[0] + 1].strip() != "{":
        return None
    brace = lines[sites[0] + 1]
    indent = brace[: len(brace) - len(brace.lstrip())]
    lines.insert(sites[0] + 2, indent + "\t" + PROBE_LINE)
    # The probe is the line after the brace; sites[0] is 0-based.
    return PlantedProbe(newline.join(lines), sites[0] + 3)


def normalise(path):
    """Returns a path with forward slashes and lower case, for comparing database entries.

    Example:
        normalise("E:\\\\repo\\\\Src\\\\A.cpp")   # -> "e:/repo/src/a.cpp"
    """
    return str(path).replace("\\", "/").lower()


def arguments_of(entry):
    """Returns a database entry's compiler arguments with its -o and output file removed.

    The command is split without POSIX rules, because its Windows paths carry backslashes that
    POSIX splitting reads as escapes.

    Example:
        arguments_of({"command": "em++ -c a.cpp -o a.o"})   # -> ["em++", "-c", "a.cpp"]
    """
    if "arguments" in entry:
        raw = list(entry["arguments"])
    else:
        raw = [token.strip('"') for token in shlex.split(entry["command"], posix=False)]
    kept = []
    skip_next = False
    for argument in raw:
        # The output path is the argument after -o, and a syntax-only run writes nothing.
        if skip_next:
            skip_next = False
            continue
        if argument == "-o":
            skip_next = True
            continue
        kept.append(argument)
    return kept


def shadow_warnings(entry, substitute):
    """Compiles one database entry with -Wshadow and returns its shadow warnings, slashes forward.

    With `substitute`, that file is compiled in the entry's place, with src/ on the quoted
    include path so its includes still resolve; its warnings are kept although it lies outside
    src/. Raises RuntimeError when the compile itself fails, since a unit that did not compile
    reported nothing.

    Example:
        shadow_warnings(entry_for("src/MenuSell.cpp"), None)
        # -> ["E:/repo/C++RogueLike/src/MenuSell.cpp:101:38: warning: declaration shadows ..."]
    """
    arguments = arguments_of(entry)
    if substitute is not None:
        arguments = [str(substitute) if normalise(argument) == normalise(entry["file"]) else argument for argument in arguments]
        arguments += ["-iquote", str(ROOT / "src")]
    result = subprocess.run(arguments + EXTRA, cwd=entry["directory"], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"compile failed ({result.returncode}) for {entry['file']}:\n{result.stderr[-2000:]}")

    kept = []
    for line in result.stderr.splitlines():
        if "warning:" not in line or "[-Wshadow" not in line:
            continue
        forward = line.replace("\\", "/")
        is_substitute = substitute is not None and normalise(substitute) in normalise(line)
        if "/src/" in forward or is_substitute:
            kept.append(forward)
    return kept


def entries_for(names, every):
    """Returns the database entries for the named src/ files, or for every src/*.cpp.

    Refuses a name the database does not hold, so a misspelt file cannot read as clean.

    Example:
        len(entries_for(["src/MenuSell.cpp"], False))   # -> 1
        entries_for(["src/Nope.cpp"], False)            # raises SystemExit: not in the database
    """
    entries = json.loads(DATABASE.read_text(encoding="utf-8"))
    if every:
        return [entry for entry in entries if "/src/" in normalise(entry["file"]) and normalise(entry["file"]).endswith(".cpp")]
    chosen = []
    for name in names:
        matches = [entry for entry in entries if normalise(entry["file"]) == normalise(ROOT / name)]
        if len(matches) != 1:
            raise SystemExit(f"{name}: {len(matches)} entries in {DATABASE}, expected 1")
        chosen.append(matches[0])
    return chosen


def report(entries):
    """Prints each warning once, then the total; a header shadow seen from several units counts once.

    Example:
        report(entries_for(["src/MenuSell.cpp"], False))
        # prints the MenuSell warning, then "1 -Wshadow warning under src/ across 1 unit"
    """
    seen = []
    for entry in entries:
        for warning in shadow_warnings(entry, None):
            if warning not in seen:
                seen.append(warning)
                print(warning)
    warning_word = "warning" if len(seen) == 1 else "warnings"
    unit_word = "unit" if len(entries) == 1 else "units"
    print(f"{len(seen)} -Wshadow {warning_word} under src/ across {len(entries)} {unit_word}")


def self_test():
    """Plants one shadow in a copy of PROBE_SOURCE and requires exactly that one warning.

    Example:
        self_test()   # prints "self-test: 1 warning, at the planted line" -> 0
    """
    original = (ROOT / PROBE_SOURCE).read_bytes().decode("utf-8")
    probe = planted_copy(original)
    if probe is None:
        print(f"self-test FAILED: {PROBE_SOURCE} has no single '{PROBE_SIGNATURE}' "
              "with its body's brace on the next line")
        return 1

    with tempfile.TemporaryDirectory() as scratch_directory:
        copy = pathlib.Path(scratch_directory) / "shadow_probe.cpp"
        copy.write_bytes(probe.text.encode("utf-8"))
        warnings = shadow_warnings(entries_for([PROBE_SOURCE], False)[0], copy)

    expected = f"shadow_probe.cpp:{probe.line}:"
    if len(warnings) != 1 or expected not in warnings[0]:
        print(f"self-test FAILED: expected one warning at line {probe.line}, got {warnings}")
        return 1
    print("self-test: 1 warning, at the planted line")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("files", nargs="*", help="src/ files to check, relative to the repository")
    parser.add_argument("--all", action="store_true", help="check every src/*.cpp in the database")
    parser.add_argument("--self-test", action="store_true", help="plant one shadow and require exactly it")
    options = parser.parse_args()

    if options.self_test:
        return self_test()
    if not options.all and not options.files:
        parser.error("name src/ files, or pass --all or --self-test")
    report(entries_for(options.files, options.all))
    return 0


if __name__ == "__main__":
    sys.exit(main())
