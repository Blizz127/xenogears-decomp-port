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
# The isolated link allows unresolved builders. Check that every builder the
# supplied descriptor table requires has a concrete source-level binding.
echo "--- source pins ---"
python3 - "$OVERRIDES" "$RETAIL" <<'PYBLOCK'
import re, struct, sys
from pathlib import Path
text = Path(sys.argv[1]).read_text()
owned = Path(sys.argv[2]).read_bytes()
start = 0x8004FE50 - 0x8000F800
required = {struct.unpack_from("<I", owned, start + row * 40 + 24)[0] for row in range(17)}
body = text.split("static ModelPrimBuildProc ModelPrimBuilder(", 1)[1].split("\n}", 1)[0]
pairs = re.findall(r"case 0x([0-9A-F]+)u: return ModelBuild([0-9A-F]+);", body)
bound = {int(address, 16) for address, name in pairs if address == name}
if required != bound:
    raise SystemExit("PRIM TABLE SOURCE PIN FAIL: unresolved builder mapping")
print(f"  {len(required)} distinct native builder bindings verified")
PYBLOCK

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

# Change the populated result of one live hydration iteration for each fault.
for row in 7 14; do
    mutate "drop_row_$row" "
needle = '        ModelPrimDesc *desc = &D_8004FE50[i];'
assert src.count(needle) == 1
src = src.replace(needle, needle + '\n        if (i == $row) continue;')
"
done
mutate bad_stride_07 '
needle = "        desc->outputStride = rows[i][9];"
assert src.count(needle) == 1
src = src.replace(needle, needle + "\n        if (i == 7) desc->outputStride ^= 4u;")
'
mutate null_proc0_0E '
needle = "        desc->buildProc = ModelPrimBuilder(rows[i][6]);"
assert src.count(needle) == 1
src = src.replace(needle, needle + "\n        if (i == 14) desc->proc[0] = NULL;")
'
echo
echo "PRIM TABLE CONTROLS REJECTED ${CONTROLS_REJECTED}/${CONTROLS_TOTAL}"
if [ "$CONTROLS_REJECTED" -ne "$CONTROLS_TOTAL" ]; then
    echo "PRIM TABLE RESULT FAIL (a control slipped through)"
    exit 1
fi
echo "PRIM TABLE RESULT PASS"
