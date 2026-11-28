#!/bin/bash
# Generate libs-config/<family>/pcre2/pcre2.h from the vendored
# pcre2.h.in. The .in file's only placeholders are the four
# version fields. The Windows family's header additionally
# predefines PCRE2_STATIC (see libs-config/README.md).
set -e
cd "$(dirname "$0")/.."
ROOT=$PWD
IN=$ROOT/vendor-pcre2/src/pcre2.h.in

for FAMILY in linux-gnu android darwin windows; do
    OUT=$ROOT/libs-config/$FAMILY/pcre2/pcre2.h
    mkdir -p "$(dirname "$OUT")"
    sed -e 's/@PCRE2_MAJOR@/10/' \
        -e 's/@PCRE2_MINOR@/45/' \
        -e 's/@PCRE2_PRERELEASE@//' \
        -e 's/@PCRE2_DATE@/2025-02-05/' \
        "$IN" > "$OUT"
    if [ "$FAMILY" = windows ]; then
        python3 - "$OUT" <<'PYEOF'
import sys
p = sys.argv[1]
t = open(p).read()
anchor = "#ifndef PCRE2_H_IDEMPOTENT_GUARD\n"
add = ("/* The bundled PCRE2 library is static-only, so declare its\n"
       "   API without the DLL import decoration. */\n"
       "#define PCRE2_STATIC 1\n")
assert anchor in t
t = t.replace(anchor, add + anchor, 1)
open(p, "w").write(t)
PYEOF
    fi
    echo "generated: $OUT"
done
