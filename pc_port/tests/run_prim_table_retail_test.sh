#!/usr/bin/env bash
#
# Regression certificate for the D_8004FE50 model-prim dispatch table.
#
# Links the real production pc_port/src/game_overrides.c (the table's owner)
# against a driver that re-reads the retail table out of disc/SLUS_006.64 and
# compares strides, buildProcs and the populated-row set.  Then proves the test
# actually detects damage by rebuilding it against deliberately mutated copies
# of game_overrides.c that MUST fail.
#
# Undefined references from the production TU (everything not yet ported) are
# tolerated with --unresolved-symbols=ignore-all; the driver only reads the
# table's data words, it never calls a walker.
set -euo pipefail
cd "$(dirname "$0")/../.."
ulimit -c 0

OUT=${PRIM_TABLE_OUT:-$(mktemp -d /tmp/xeno-prim-table.XXXXXXXX)}
mkdir -p "$OUT"
echo "PRIM TABLE OUTPUT $OUT"
cleanup() { [ -n "${PRIM_TABLE_OUT:-}" ] || rm -rf "$OUT"; }
trap cleanup EXIT

OVERRIDES=pc_port/src/game_overrides.c
DRIVER=pc_port/tests/prim_table_retail_prod_test.c
RETAIL=disc/SLUS_006.64

if [ ! -f "$RETAIL" ]; then
    echo "PRIM TABLE SKIP: $RETAIL not present"
    exit 0
fi

# Same flags the port build uses for this TU (pc_port/build_port.sh).
CFLAGS=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY
        -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
        -include assert.h -w -O0 -g -m64 -fno-builtin
        -Ipc_port/include_shim -Iinclude -Ipc_port/build_native -Ipc_port/src
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
LDFLAGS=(-m64 -no-pie -fno-pie -Wl,--unresolved-symbols=ignore-all)

build_and_run() {
    # $1 = label, $2 = game_overrides.c to compile, $3 = expect ("pass"/"fail")
    local label=$1 src=$2 expect=$3
    local bin="$OUT/$label"
    local log="$OUT/$label.log"
    local rc=0

    # A control MUST be caught by the test's assertions at run time, not by
    # failing to compile.  Every mutation below is still valid C (dropping a
    # designated-initialiser row or changing a constant), so a build failure
    # here means the environment broke (commonly a full /tmp), NOT that the
    # test detected anything.  Treating it as a pass would make the controls
    # blind, so it is a hard error.
    if ! gcc "${CFLAGS[@]}" "$src" "$DRIVER" "${LDFLAGS[@]}" -o "$bin" \
            >"$OUT/$label.build.log" 2>&1; then
        echo "PRIM TABLE BUILD FAILED for $label (environment, not a rejection)"
        tail -20 "$OUT/$label.build.log"
        return 1
    fi

    "$bin" "$RETAIL" >"$log" 2>&1 || rc=$?
    if [ "$expect" = "pass" ]; then
        if [ "$rc" -ne 0 ]; then
            echo "PRIM TABLE PRODUCTION RUN FAILED (rc=$rc)"
            cat "$log"
            return 1
        fi
        grep -E "^PRIM TABLE (CASES|RESULT)" "$log"
    else
        if [ "$rc" -eq 0 ]; then
            echo "PRIM TABLE CONTROL $label WAS NOT REJECTED -- test is blind"
            cat "$log"
            return 1
        fi
        echo "  control $label: correctly rejected ($(grep -c '^FAIL' "$log") assertion failures)"
    fi
    return 0
}

# ---------------------------------------------------------------------------
# Source-level pin: every row present in the initialiser must declare a
# buildProc and all three strides.  This cannot be checked at run time because
# the buildProcs are not-yet-ported externs that the ignore-all link resolves
# to 0, so it is verified against the source text here.
# ---------------------------------------------------------------------------
echo "--- source pins ---"
python3 - "$OVERRIDES" <<'PY'
import re, sys
src = open(sys.argv[1]).read()

table = re.search(r'ModelPrimDesc D_8004FE50\[17\] = \{(.*?)\n\};', src, re.S)
assert table, "game_overrides.c: could not locate the D_8004FE50 initialiser"
body = table.group(1)

rows = re.findall(r'\[(0x[0-9A-Fa-f]{2})\] = \{(.*?)\n    \},', body, re.S)
assert rows, "game_overrides.c: no rows parsed out of D_8004FE50"

missing = []
for label, row in rows:
    for field in ('.buildProc', '.cmdStride', '.packetStride', '.outputStride'):
        if field not in row:
            missing.append(f"{label} lacks {field}")
if missing:
    raise SystemExit("PRIM TABLE SOURCE PIN FAIL: " + "; ".join(missing))

print(f"  {len(rows)} populated rows, each with buildProc + 3 strides: "
      + ", ".join(label for label, _ in rows))
PY

echo "--- production ---"
build_and_run production "$OVERRIDES" pass

# ---------------------------------------------------------------------------
# Controls.  Each mutates the production table the way a real regression would
# and must be caught.
# ---------------------------------------------------------------------------
echo "--- controls ---"
CONTROLS_REJECTED=0
CONTROLS_TOTAL=0

mutate() {
    # $1 = label, $2 = python expression body operating on `src`
    local label=$1
    local script=$2
    local dst="$OUT/$label.c"
    CONTROLS_TOTAL=$((CONTROLS_TOTAL + 1))
    python3 - "$OVERRIDES" "$dst" <<PY
import sys
src = open(sys.argv[1]).read()
$script
open(sys.argv[2], "w").write(src)
PY
    if build_and_run "$label" "$dst" fail; then
        CONTROLS_REJECTED=$((CONTROLS_REJECTED + 1))
    fi
}

# 1. Drop row 0x07 entirely -- reproduces the absent/zero-filled row bug.
mutate drop_row_07 '
import re
m = re.search(r"\n    \[0x07\] = \{.*?\n    \},", src, re.S)
assert m, "could not find row 0x07 to drop"
src = src[:m.start()] + src[m.end():]
'

# 2. Drop row 0x0E entirely -- same class, the other row filled this pass.
mutate drop_row_0E '
import re
m = re.search(r"\n    \[0x0E\] = \{.*?\n    \},", src, re.S)
assert m, "could not find row 0x0E to drop"
src = src[:m.start()] + src[m.end():]
'

# 3. Corrupt row 0x07's outputStride away from the retail 0x28.
mutate bad_stride_07 '
import re
m = re.search(r"(\[0x07\] = \{.*?\.outputStride = )0x28", src, re.S)
assert m, "could not find row 0x07 outputStride"
src = src[:m.end(1)] + "0x20" + src[m.end():]
'

# 4. NULL out row 0x0E proc[0] while leaving the row populated -- the exact
#    "build side links packets the walker cannot parse" shape.
mutate null_proc0_0E '
import re
m = re.search(r"(\[0x0E\] = \{.*?\.proc = \{ )ModelPrimQuadG4Variant0", src, re.S)
assert m, "could not find row 0x0E proc[0]"
src = src[:m.end(1)] + "NULL" + src[m.end():]
'

echo
echo "PRIM TABLE CONTROLS REJECTED ${CONTROLS_REJECTED}/${CONTROLS_TOTAL}"
if [ "$CONTROLS_REJECTED" -ne "$CONTROLS_TOTAL" ]; then
    echo "PRIM TABLE RESULT FAIL (a control slipped through)"
    exit 1
fi
echo "PRIM TABLE RESULT PASS"
