"""The chrome both generated documentation pages share: styling, navigation, footer.

There are two pages and they are two views of one thing - build_index.py lists the files
and what each is for, api_surface.py lists what each header declares. They looked like
one product only by coincidence, because the same stylesheet had been pasted into both
generators, and a change to one would have drifted silently from the other.

Nothing here knows what either page contains. It supplies the opening tags, the palette
and the navigation strip; each generator supplies its own body.

Usage:

    import doc_page

    parts = [doc_page.head("src index", "files", stamp)]   # active tab: files | api
    parts.append("<p>whatever the page is</p>")
    parts.append(doc_page.foot())
    page = "".join(parts)

The active tab is the page being written, so the strip renders it as current rather than
as a link to itself.
"""

import html

# The two pages, in the order the strip shows them. The file names are the contract
# between the generators: build_index.py writes the first, api_surface.py the second, and
# each links to the other by these names.
PAGES = (
    ("files", "src_index.html", "Files"),
    ("api", "src_api.html", "Declarations"),
)

def anchor_exists(header_name):
    """Whether the declarations page has a section for this header to link to.

    Every header is parsed now. libclang reads the LibTorch and raylib ones too, which
    the JSON dump could not - so the exclusion list this used to hold is gone rather
    than shortened. A .cpp still has no section: only headers are parsed.

    Example:

        anchor_exists("mcts.h")         # True
        anchor_exists("az_network.h")   # True - libclang parses it in 9.4 seconds
        anchor_exists("mcts.cpp")       # False - only headers are parsed

    Args:
        header_name: a file name, with its extension.
    """
    return header_name.endswith(".h")


STYLE = """
details.internal { margin: 18px 0 26px; padding-left: 14px; border-left: 2px solid var(--rule); }
details.internal > summary { font-size: 13px; color: var(--dim); cursor: pointer;
    list-style: none; padding: 4px 0; user-select: none; }
details.internal > summary::-webkit-details-marker { display: none; }
details.internal > summary::before { content: ""; display: inline-block;
    width: 0; height: 0; margin-right: 8px; vertical-align: middle;
    border-left: 5px solid var(--dim); border-top: 4px solid transparent;
    border-bottom: 4px solid transparent; transition: transform .12s; }
details.internal[open] > summary::before { transform: rotate(90deg); }
details.internal > summary:hover { color: var(--ink); }
details.internal > .scroll { margin-top: 8px; }
details.file { margin: 2.25rem 0 0; border-top: 1px solid var(--rule);
    scroll-margin-top: 4rem; }
details.file > summary { font-size: 1.1rem; font-weight: 600; color: var(--ink);
    cursor: pointer; list-style: none; padding: .6rem 0 .4rem; user-select: none; }
details.file > summary::-webkit-details-marker { display: none; }
details.file > summary::before { content: ""; display: inline-block;
    width: 0; height: 0; margin-right: 8px; vertical-align: middle;
    border-left: 5px solid var(--dim); border-top: 4px solid transparent;
    border-bottom: 4px solid transparent; transition: transform .12s; }
details.file[open] > summary::before { transform: rotate(90deg); }
details.file > summary:hover { color: var(--link); }
details.file > summary .back { font-size: .75rem; font-weight: 400; margin-left: .6rem;
    color: var(--dim); text-decoration: none; }
details.file > summary .back:hover { text-decoration: underline; }
details > summary .tag { margin-left: 8px; font-size: 11px; color: var(--dim);
    border: 1px solid var(--rule); border-radius: 3px; padding: 1px 5px; }
.fileblock { margin: 0 0 22px; padding: 14px 18px; background: var(--panel);
    border-radius: 6px; font-size: 13px; line-height: 1.55; }
.fileblock > *:first-child { margin-top: 0; }
.fileblock > *:last-child { margin-bottom: 0; }
.stamp { color: #7c8798; font-size: 11px; margin: 0 0 18px; font-family: ui-monospace, monospace; }
:root{--ink:#1a1c1e;--dim:#5b6167;--rule:#d8dce0;--panel:#f5f6f8;--bad:#9a3324;
--link:#1f5c8b;--bg:#ffffff;
--code-bg:#1b1f24;--code-ink:#e3e8ee;--code-rule:#2c3239}
@media(prefers-color-scheme:dark){:root{--ink:#e6e8ea;--dim:#a0a6ac;--rule:#33383d;
--panel:#1c1f22;--bad:#e08475;--link:#79b3dd;--bg:#131517}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);
font:16px/1.5 -apple-system,"Segoe UI",Roboto,Arial,sans-serif}
main{margin:0 auto;padding:1.5rem 1.25rem 5rem;max-width:84rem}
a{color:var(--link)}
nav{position:sticky;top:0;z-index:10;background:var(--bg);
border-bottom:1px solid var(--rule)}
nav .strip{margin:0 auto;padding:0 1.25rem;max-width:84rem;display:flex;
align-items:center;gap:.25rem;min-height:3rem;flex-wrap:wrap}
nav .brand{font-weight:600;margin-right:.75rem}
nav a,nav span.current{display:inline-block;padding:.35rem .7rem;border-radius:.3rem;
text-decoration:none;font-size:.92rem}
nav a{color:var(--dim)}
nav a:hover{background:var(--panel);color:var(--ink)}
nav span.current{background:var(--panel);color:var(--ink);font-weight:600}
h1{font-size:1.7rem;margin:1.25rem 0 .25rem}
h2{font-size:1.1rem;margin:2.25rem 0 .4rem;padding-top:.6rem;
border-top:1px solid var(--rule);scroll-margin-top:4rem}
h2 a{text-decoration:none;color:inherit}
h2 .back{font-size:.75rem;font-weight:400;margin-left:.6rem;color:var(--dim)}
.sub{color:var(--dim);margin:0 0 1.5rem}
.scroll{overflow-x:auto;margin:.6rem 0}
table{border-collapse:collapse;width:100%;font-size:.87rem}
th,td{text-align:left;padding:.38rem .6rem;border-bottom:1px solid var(--rule);
vertical-align:top}
thead th{border-bottom:2px solid var(--rule)}
td.num{text-align:right;color:var(--dim);white-space:nowrap}
tbody tr:hover{background:var(--panel)}
code{font-family:"Cascadia Mono",Consolas,monospace;font-size:.9em}
.missing{color:var(--bad);font-style:italic}
.meta{color:var(--dim);font-size:.85rem}
.owner{color:var(--dim)}
.pair{display:grid;grid-template-columns:1fr 1fr;gap:1px;margin:1rem 0;
background:var(--rule);border:1px solid var(--rule);border-radius:.35rem;
overflow:hidden}
@media(max-width:44rem){.pair{grid-template-columns:1fr}}
.side{background:var(--bg);padding:.7rem .9rem}
.side.empty{background:var(--panel)}
.side .role{font-size:.7rem;text-transform:uppercase;letter-spacing:.06em;
color:var(--dim);margin-bottom:.3rem}
.side .filename{display:flex;align-items:baseline;gap:.5rem;flex-wrap:wrap;
margin-bottom:.35rem}
.side .num{color:var(--dim);font-size:.78rem}
.side .tag{font-size:.7rem;color:var(--dim);border:1px solid var(--rule);
border-radius:.2rem;padding:0 .3rem}
.side p{margin:0;font-size:.87rem}
.decls{table-layout:fixed}
.decls td:nth-child(1),.decls th:nth-child(1){width:22%}
.decls td:nth-child(2),.decls th:nth-child(2){width:20%}
.decls td code{overflow-wrap:anywhere}
td p,.side p{margin:0 0 .5rem}
td p:last-child,.side p:last-child{margin-bottom:0}
td pre,.side pre,.fileblock pre{margin:.5rem 0;padding:.6rem .85rem .6rem 2.15rem;
text-indent:-1.3rem;background:var(--code-bg);border:1px solid var(--code-rule);
border-radius:.3rem;max-width:100%}
td pre code,.side pre code,.fileblock pre code{font-size:.68rem;line-height:1.5;color:var(--code-ink);
white-space:pre-wrap;overflow-wrap:break-word;tab-size:2}
.jump{margin:1rem 0;padding:.75rem 1rem;background:var(--panel);border-radius:.35rem}
.jump a{display:inline-block;margin:.15rem .5rem .15rem 0;font-size:.85rem;
font-family:"Cascadia Mono",Consolas,monospace}
"""


def segments_from_lines(lines):
    """Comment lines split into prose paragraphs and code blocks, in order.

    An indented run inside a comment is an example, and flattening it into the prose
    turns a usage block into a run-on sentence. Indentation is the only marker this
    codebase uses for one, so it is what the split reads. Callers strip the comment
    markers first; this sees text with its indentation intact.

    Example:

        segments_from_lines(["What it is.", "", "    call();", "    more();"])
        # [('prose', 'What it is.'), ('code', 'call();\nmore();')]

        segments_from_lines([])   # []

    Args:
        lines: comment lines, markers already removed, indentation preserved.
    """
    segments = []
    buffer = []
    mode = None

    def flush():
        # Prose joins into one line; code keeps its own, with trailing blanks dropped.
        nonlocal buffer, mode
        while buffer and not buffer[-1].strip():
            buffer.pop()
        if buffer:
            joiner = "\n" if mode == "code" else " "
            segments.append((mode, joiner.join(buffer)))
        buffer = []

    for line in lines:
        if not line.strip():
            # A blank line inside an example belongs to it; between paragraphs it ends one.
            if mode == "code":
                buffer.append("")
            else:
                flush()
            continue
        kind = "code" if line.startswith("    ") else "prose"
        if mode is None:
            mode = kind
        if kind != mode:
            flush()
            mode = kind
        buffer.append(line[4:] if kind == "code" else line.strip())
    flush()
    return segments


def render_comment(segments, empty_note):
    """A comment as HTML: paragraphs of prose, examples in their own block.

    Example:

        render_comment([('prose', 'What it is.'), ('code', 'call();')], "none")
        # '<p>What it is.</p><pre><code>call();</code></pre>'

        render_comment([], "no comment")
        # '<span class="missing">no comment</span>'

        render_comment([], "")   # '' - the caller says it another way

    Args:
        segments: what segments_from_lines returned.
        empty_note: what to say where there is nothing to show.
    """
    if not segments:
        # An empty note means the caller has something else to say about the absence,
        # so nothing is rendered rather than an empty highlight.
        if not empty_note:
            return ""
        return f'<span class="missing">{html.escape(empty_note)}</span>'
    parts = []
    for kind, body in segments:
        if kind == "code":
            parts.append(f"<pre><code>{html.escape(body)}</code></pre>")
        else:
            parts.append(f"<p>{html.escape(body)}</p>")
    return "".join(parts)


def navigation(active):
    """The strip of links to every generated page, with the current one not a link.

    Example:

        navigation("files")
        # '<nav><div class="strip"><span class="brand">C++RogueLike</span>
        #  <span class="current">Files</span>
        #  <a href="src_api.html">Declarations</a></div></nav>'

    Args:
        active: the key of the page being written, "files" or "api".
    """
    items = []
    for key, filename, label in PAGES:
        # The page being written links to everything except itself: a self-link reads as
        # navigation and does nothing, which is worse than no link at all.
        if key == active:
            items.append(f'<span class="current">{label}</span>')
        else:
            items.append(f'<a href="{filename}">{label}</a>')
    return (
        '<nav><div class="strip"><span class="brand">C++RogueLike</span>' + "".join(items) + "</div></nav>"
    )


def source_stamp(source_directory):
    """A short hash of every source file, with the time it was taken.

    A generated page carries no mark of what it was built from, so one three edits
    behind looks exactly as authoritative as a current one. Printing the digest on the
    page turns "is this current" into a comparison anyone can make: regenerate, and if
    the digest did not change, the sources did not either.

    The hash covers file names, sizes and modification times rather than contents -
    enough to catch an edit, and it does not read a hundred files to build a header.

    Example:

        source_stamp(Path("src"))
        # 'a41c9f02 - 2026-08-21 21:58'

    Args:
        source_directory: the directory whose files the page was generated from.
    """
    import hashlib
    from datetime import datetime

    digest = hashlib.sha256()
    # Sorted, so the digest does not depend on the order the filesystem returns.
    for entry in sorted(source_directory.glob("*.*")):
        stat = entry.stat()
        digest.update(f"{entry.name}:{stat.st_size}:{int(stat.st_mtime)}".encode("utf-8"))
    return f"{digest.hexdigest()[:8]} - {datetime.now().strftime('%Y-%m-%d %H:%M')}"


def head(title, active, stamp):
    """Everything above a page's own content, ending with the opening main tag.

    Example:

        head("src index", "files", "a41c9f02 - 2026-08-21 21:58").endswith("<main>")   # True

    Args:
        title: what the browser tab says.
        active: which navigation entry to render as current.
        stamp: the source digest to print, or empty to print none.
    """
    banner = f'<p class="stamp">source {html.escape(stamp)}</p>' if stamp else ""
    return (
        "<!DOCTYPE html>"
        '<html lang="en"><head><meta charset="utf-8">'
        '<meta name="viewport" content="width=device-width, initial-scale=1">'
        f"<title>{html.escape(title)}</title>"
        f"<style>{STYLE}</style></head><body>"
        f"{navigation(active)}"
        "<main>"
        f"{banner}"
    )


def foot(note):
    """The closing tags, under one line of small print.

    Example:

        foot("Presence is all this measures.").endswith("</html>")   # True

    Args:
        note: the small print, already plain text.
    """
    return f'<p class="meta">{html.escape(note)}</p></main></body></html>'
