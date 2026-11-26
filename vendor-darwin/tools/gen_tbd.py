#!/usr/bin/env python3
"""Generate the Darwin link stubs in vendor-darwin/lib/.

The stubs are text (.tbd) files in the format TCC's Mach-O
linker reads: an install name plus flat symbol lists.  Every
stub names /usr/lib/libSystem.B.dylib as its install name, so a
program linked against a stub loads the real system library at
run time; the stub itself carries no code, only the symbol
surface the bundled headers declare.

The symbol surface is extracted mechanically from the headers in
vendor-darwin/include/ (the headers are written one declaration
per statement, which keeps this extraction exact), plus a short
hand list of symbols the headers use but do not declare (the
__fpclassify functions behind math.h's fpclassify macro).  A
declaration carrying __DARWIN_INODE64(sym) contributes both the
plain name (arm64) and the sym$INODE64 variant (x86_64).  Files:

  libc.tbd       every declared function and variable
  libSystem.tbd  the same surface (so -lSystem resolves too)
  libm.tbd       the math.h surface plus the __fpclassify set
  libpthread.tbd the pthread.h surface plus pthread_sigmask
  libdl.tbd      the dlfcn.h surface

Run with no arguments to regenerate vendor-darwin/lib/ in place.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
VENDOR = os.path.dirname(HERE)
INCLUDE = os.path.join(VENDOR, "include")
LIB = os.path.join(VENDOR, "lib")

INSTALL_NAME = "/usr/lib/libSystem.B.dylib"

# Symbols the headers rely on without declaring them: math.h's
# fpclassify macro dispatches to these per-type functions.
EXTRAS = ["__fpclassifyf", "__fpclassifyd", "__fpclassifyl"]


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


def statements(text):
    """Split preprocessed header text into top-level statements.

    Preprocessor lines are dropped; a statement ends at a ';' at
    brace and paren depth zero.
    """
    # Join backslash-continued lines first: a multi-line macro is
    # one logical preprocessor line and is dropped whole.  Dropping
    # only its first physical line would leak the continuation
    # (with unbalanced parens) into the statement stream.
    logical = []
    pending = ""
    for line in text.splitlines():
        if line.endswith("\\"):
            pending += line[:-1] + " "
            continue
        logical.append(pending + line)
        pending = ""
    if pending:
        logical.append(pending)
    lines = [l for l in logical if not l.lstrip().startswith("#")]
    text = "\n".join(lines)
    out, cur = [], []
    bdepth = pdepth = 0
    for ch in text:
        cur.append(ch)
        if ch == "{":
            bdepth += 1
        elif ch == "}":
            bdepth -= 1
        elif ch == "(":
            pdepth += 1
        elif ch == ")":
            pdepth -= 1
        elif ch == ";" and bdepth == 0 and pdepth == 0:
            out.append("".join(cur))
            cur = []
    return [s.strip() for s in out if s.strip()]


_IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def extract_from_header(path):
    """Return (functions, variables) declared by one header."""
    with open(path) as f:
        text = strip_comments(f.read())
    funcs, vars_ = set(), set()
    for st in statements(text):
        # Skip typedefs only: a typedef of a function-pointer type
        # would otherwise be harvested as a function below.  Do NOT
        # skip statements starting with struct/union/enum: a
        # declaration whose return type is a struct (e.g.
        # "struct tm *gmtime_r(...)") starts with "struct" but is a
        # real function declaration, and a struct definition is
        # already skipped by the brace check below.
        if st.startswith("typedef"):
            continue
        if "{" in st or "}" in st:
            continue  # a definition body, not a declaration
        m = re.search(r"\(\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(", st)
        if m and "(" not in st[: m.start()]:
            funcs.add(m.group(1))  # type (*name(args))(args)
            continue
        if "(" in st:
            head = st[: st.index("(")]
            if "=" in head:
                continue
            ids = _IDENT.findall(head)
            if ids:
                funcs.add(ids[-1])
            # A __DARWIN_INODE64(sym) suffix renames the symbol on
            # x86_64 to sym$INODE64 (the 64-bit-inode variant in
            # libSystem); the stub is shared by both targets, so
            # both the plain name (arm64) and the suffixed name
            # (x86_64) are listed.
            m2 = re.search(r"__DARWIN_INODE64\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)", st)
            if m2:
                funcs.add(m2.group(1) + "$INODE64")
            continue
        if st.startswith("extern"):
            body = st[len("extern"):]
            for declarator in body.rstrip(";").split(","):
                declarator = declarator.split("=")[0]
                declarator = re.sub(r"\[.*?\]", "", declarator)
                ids = _IDENT.findall(declarator)
                if ids:
                    vars_.add(ids[-1])
    return funcs, vars_


def extract_all():
    """Map header name -> (functions, variables); plus totals."""
    per_header = {}
    all_funcs, all_vars = set(), set()
    for root, _dirs, files in os.walk(INCLUDE):
        for name in sorted(files):
            if not name.endswith(".h"):
                continue
            path = os.path.join(root, name)
            rel = os.path.relpath(path, INCLUDE)
            funcs, vars_ = extract_from_header(path)
            per_header[rel] = (funcs, vars_)
            all_funcs |= funcs
            all_vars |= vars_
    return per_header, all_funcs, all_vars


def write_tbd(path, symbols):
    syms = sorted("_" + s for s in symbols)
    with open(path, "w") as f:
        f.write("--- !tapi-tbd\n")
        f.write("tbd-version: 4\n")
        f.write("targets: [ x86_64-macos, arm64-macos ]\n")
        f.write(f"install-name: {INSTALL_NAME}\n")
        f.write("exports:\n")
        # TCC's .tbd reader scans for repeated "symbols: [ ... ]"
        # lists; keep each list modest in size.
        for i in range(0, len(syms), 64):
            chunk = ", ".join(syms[i : i + 64])
            f.write(f"  - symbols: [ {chunk} ]\n")
    return len(syms)


def main():
    per_header, all_funcs, all_vars = extract_all()
    surface = all_funcs | all_vars | set(EXTRAS)
    math_funcs, _ = per_header["math.h"]
    pthread_funcs, _ = per_header["pthread.h"]
    dl_funcs, _ = per_header["dlfcn.h"]
    jobs = [
        ("libc.tbd", surface),
        ("libSystem.tbd", surface),
        ("libm.tbd", set(math_funcs) | set(EXTRAS)),
        ("libpthread.tbd", set(pthread_funcs) | {"pthread_sigmask"}),
        ("libdl.tbd", set(dl_funcs)),
    ]
    os.makedirs(LIB, exist_ok=True)
    for name, syms in jobs:
        n = write_tbd(os.path.join(LIB, name), syms)
        print(f"{name}: {n} symbols")
    print(f"headers: {len(per_header)}, functions: {len(all_funcs)}, "
          f"variables: {len(all_vars)}, extras: {len(EXTRAS)}")


if __name__ == "__main__":
    sys.exit(main())
