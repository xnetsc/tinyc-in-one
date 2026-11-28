#!/usr/bin/env python3
"""Apply one unified-diff patch file to a directory, without `patch`.

The bundled-library builds (tools/build_default_libs.sh) used to call
the `patch` binary, which is not on PATH on the Windows runner
(MSYS2 MINGW64 ships no patch package) and is one more host tool the
build has to assume. This applier is the replacement: it needs only
python3, which the same script already requires for its config and
header edits, so it adds no new dependency on any runner.

Patch form: the libs-config/patches/*.patch form - a free-form header
(ignored), then one file per `--- a/<path>` / `+++ b/<path>` pair,
each with `@@ -old,start,count +new,start,count @@` hunks. Only
modification hunks are supported; a hunk is anchored at its stated
old line and its old lines (context plus `-` lines) must match the
file exactly there, allowing a small offset search around the anchor
(the way `patch` searches). This is deliberately stricter than GNU
patch's fuzz: a hunk whose context does not match the vendored file
fails loudly instead of silently applying at the wrong place. A
stray trailing line such as a lone `diff -ruN ...` with no hunks
after it is ignored, as `patch` ignores it.

Usage: apply_patch.py <patch-file> <directory>
"""
import re
import sys

HUNK_RE = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")


def parse_patch(text):
    """Return [(relpath, [(old_start, old_lines, new_lines), ...]), ...]."""
    files = []
    cur_path = None
    cur_hunks = []
    lines = text.split("\n")
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith("--- "):
            path = line[4:].strip()
            if path.startswith("a/"):
                path = path[2:]
            cur_path = path
            cur_hunks = []
            files.append((cur_path, cur_hunks))
            i += 1
            continue
        if line.startswith("+++ "):
            i += 1
            continue
        m = HUNK_RE.match(line)
        if m and cur_path is not None:
            old_start = int(m.group(1))
            old_count = int(m.group(2)) if m.group(2) is not None else 1
            new_count = int(m.group(4)) if m.group(4) is not None else 1
            old_lines, new_lines = [], []
            i += 1
            while i < len(lines) and (len(old_lines) < old_count
                                     or len(new_lines) < new_count):
                body = lines[i]
                if body.startswith("\\"):
                    sys.exit("apply_patch: unsupported marker line: "
                             f"{body!r}")
                if body == "":
                    # Some diff writers strip the space from an empty
                    # context line; read it as empty context.
                    old_lines.append("")
                    new_lines.append("")
                    i += 1
                    continue
                kind, text_line = body[0], body[1:]
                if kind == " ":
                    old_lines.append(text_line)
                    new_lines.append(text_line)
                elif kind == "-":
                    old_lines.append(text_line)
                elif kind == "+":
                    new_lines.append(text_line)
                else:
                    break
                i += 1
            if len(old_lines) != old_count or len(new_lines) != new_count:
                sys.exit(
                    f"apply_patch: hunk at {cur_path}:@{old_start} is "
                    f"short: read {len(old_lines)}/{old_count} old and "
                    f"{len(new_lines)}/{new_count} new lines")
            cur_hunks.append((old_start, old_lines, new_lines))
            continue
        i += 1
    return files


def apply_hunk(lines, old_start, old_lines, new_lines, relpath):
    """Replace old_lines with new_lines in lines (0-based list).

    The hunk is anchored at old_start (1-based); search a small
    window around the anchor for an exact match of the old lines.
    Returns the new list, or raises SystemExit on failure.
    """
    anchor = old_start - 1
    n = len(old_lines)
    if n == 0:
        pos = anchor
        return lines[:pos] + new_lines + lines[pos:]
    # Try the anchor first, then scan outward (patch-style offset).
    candidates = [anchor]
    for d in range(1, 51):
        candidates.append(anchor - d)
        candidates.append(anchor + d)
    for pos in candidates:
        if pos < 0 or pos + n > len(lines):
            continue
        if lines[pos:pos + n] == old_lines:
            return lines[:pos] + new_lines + lines[pos + n:]
    sys.exit(
        f"apply_patch: hunk at {relpath}:@{old_start} does not match "
        f"the file (context: {old_lines[0]!r} ...); refusing to apply")


def main(argv):
    if len(argv) != 3:
        sys.exit("usage: apply_patch.py <patch-file> <directory>")
    patch_file, directory = argv[1], argv[2]
    with open(patch_file) as f:
        text = f.read()
    for relpath, hunks in parse_patch(text):
        if not hunks:
            continue
        if relpath == "/dev/null":
            sys.exit("apply_patch: new-file hunks are not supported: "
                     f"{patch_file}")
        path = f"{directory}/{relpath}"
        with open(path) as f:
            content = f.read()
        trailing_nl = content.endswith("\n")
        lines = content.split("\n")
        if trailing_nl:
            lines.pop()
        offset = 0
        for old_start, old_lines, new_lines in hunks:
            lines = apply_hunk(lines, old_start + offset, old_lines,
                               new_lines, relpath)
            offset += len(new_lines) - len(old_lines)
        with open(path, "w") as f:
            f.write("\n".join(lines))
            if trailing_nl:
                f.write("\n")


if __name__ == "__main__":
    main(sys.argv)
