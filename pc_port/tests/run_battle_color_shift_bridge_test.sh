#!/usr/bin/env bash
# Battle bridge binding of retail 0x80025D4C to func_80025D4C
# (pc_port/tests/battle_color_shift_bridge_test.c).  It needs a hybrid port
# build: the generated map, the production runtime source and objects.
#
# The hybrid binary must export func_80025D4C, so the map row binds.  The
# runtime source, linked against the port body's production object and the
# PsyCross GTE, must then handle a call at the retail site natively, with
# caller hosts both off and on.  Every other symbol the runtime reaches gets a
# placeholder.  Output goes to BATTLE_COLOR_SHIFT_BRIDGE_OUT (a scratch
# directory, never /tmp).
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=${BATTLE_COLOR_SHIFT_BRIDGE_OUT:?set BATTLE_COLOR_SHIFT_BRIDGE_OUT to a scratch directory}
BUILD=pc_port/build_native
OBJ=$BUILD/obj
BIN=$BUILD/xeno-port
PSX=pc_port/extern/PsyCross
mkdir -p "$OUT"
for f in "$BIN" "$BUILD/battle_bridge_map.inc" "$OBJ/retail_leaf_adapters.c.o" "$OBJ/psx_memory.c.o"; do
    [ -f "$f" ] || { echo "COLOR SHIFT BRIDGE FAIL missing $f (build the port first)"; exit 1; }
done
grep -q '{ 0x80025d4cu, 0u, 1, "func_80025D4C" }' "$BUILD/battle_bridge_map.inc" ||
    { echo "COLOR SHIFT BRIDGE FAIL no map row for 0x80025d4c"; exit 1; }
nm -D --defined-only "$BIN" | awk '$3 == "func_80025D4C" && $2 == "T" { ok = 1 } END { exit !ok }' ||
    { echo "COLOR SHIFT BRIDGE FAIL $BIN does not export func_80025D4C"; exit 1; }
echo "ok   $BIN exports func_80025D4C; map row { 0x80025d4cu, 0u, 1, \"func_80025D4C\" }"

COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -D_GNU_SOURCE= -w -O0 -g
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$BUILD"
    -I"$PSX/include" -I"$PSX/include/psx")
gcc "${COMMON[@]}" -DBATTLE_RUNTIME_SOURCE="\"$PWD/pc_port/src/battle_mips_runtime.c\"" \
    -c pc_port/tests/battle_color_shift_bridge_test.c -o "$OUT/test.o"
gcc "${COMMON[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/cpu.o"
for source in psx/INLINE_C.C gte/PsyX_GTE.cpp; do
    g++ -std=c++17 -fno-pie -O0 -fpermissive -w -DUSE_EXTENDED_PRIM_POINTERS=0 \
        -I"$PSX/include" -I"$PSX/include/psx" -include pc_port/src/port_compat.h \
        -c "$PSX/src/$source" -o "$OUT/${source##*/}.o"
done
LINK=("$OUT/test.o" "$OUT/cpu.o" "$OBJ/retail_leaf_adapters.c.o" "$OBJ/psx_memory.c.o"
    "$OUT/INLINE_C.C.o" "$OUT/PsyX_GTE.cpp.o")
if ! g++ -no-pie -rdynamic "${LINK[@]}" -ldl -lm -o "$OUT/test" 2>"$OUT/link.err"; then
    undefined=$(grep -oE "undefined reference to \`[A-Za-z0-9_.]+'" "$OUT/link.err" \
        | sed "s/.*\`//; s/'//" | sort -u)
    [ -n "$undefined" ] || { cat "$OUT/link.err"; echo "COLOR SHIFT BRIDGE FAIL link"; exit 1; }
    {
        echo "/* Generated: placeholders for symbols this link lacks. */"
        for s in $undefined; do
            case "$s" in
                D_*|g_*|jtbl_*|jpt_*|s_*) echo "unsigned char $s[0x10000];" ;;
                *) echo "long $s(void) { return 0; }" ;;
            esac
        done
    } > "$OUT/placeholders.c"
    gcc -c -w "$OUT/placeholders.c" -o "$OUT/placeholders.o"
    g++ -no-pie -rdynamic "${LINK[@]}" "$OUT/placeholders.o" -ldl -lm -o "$OUT/test"
    echo "     ($(echo "$undefined" | wc -l) placeholders)"
fi
status=0
"$OUT/test" | tee "$OUT/off.log" | tail -2 || status=1
"$OUT/test" hosts | tee "$OUT/on.log" | tail -1 || status=1
grep -q '^COLOR SHIFT BRIDGE PASS .*caller hosts off' "$OUT/off.log" || status=1
grep -q '^COLOR SHIFT BRIDGE PASS .*caller hosts on' "$OUT/on.log" || status=1
exit $status
