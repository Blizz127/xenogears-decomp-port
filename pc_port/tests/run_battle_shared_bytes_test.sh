#!/usr/bin/env bash
# Shared single-byte battle bindings (pc_port/tests/battle_shared_bytes_test.c):
# the production hybrid binary must export every shared native byte (else
# the runtime's dlsym would miss it), and the production
# runtime source, built against the production objects that define them,
# must resolve each guest byte to its native global, carry a guest store
# there, and drive misc4.c's D_8005942C == 1 field-entry branch.
#
# Needs a hybrid port build (pc_port/build_native).  Output goes to
# BATTLE_SHARED_BYTES_OUT (a scratch directory, never /tmp).
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=${BATTLE_SHARED_BYTES_OUT:?set BATTLE_SHARED_BYTES_OUT to a scratch directory}
BUILD=pc_port/build_native
OBJ=$BUILD/obj
BIN=$BUILD/xeno-port
mkdir -p "$OUT"
NAMES="D_800594D4 D_800594D5 D_800594D6 D_8004FE44 D_8004FE45 D_8004FE46 D_8004FE47 D_8005942C D_80062514"

for f in "$BIN" "$BUILD/battle_bridge_map.inc" "$OBJ/stubs.o"; do
    [ -f "$f" ] || { echo "BATTLE SHARED BYTES FAIL missing $f (build the port first)"; exit 1; }
done
missing=0
dyn=$(nm -D --defined-only "$BIN")
for n in $NAMES; do
    if ! echo "$dyn" | awk -v n="$n" '$3 == n && $2 ~ /^[BDRV]$/ { ok = 1 } END { exit !ok }'; then
        echo "FAIL $BIN does not export $n as data (the runtime's dlsym would miss it)"
        missing=1
    fi
done
[ "$missing" = 0 ] && echo "ok   $BIN exports all shared bytes in .dynsym: $NAMES"

PSX=pc_port/extern/PsyCross
COMMON=(-std=gnu17 -fno-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -D_GNU_SOURCE= -w -O0 -g
    -Ipc_port/include_shim -Iinclude -Ipc_port/src -I"$BUILD"
    -I"$PSX/include" -I"$PSX/include/psx")
gcc "${COMMON[@]}" -DBATTLE_RUNTIME_SOURCE="\"$PWD/pc_port/src/battle_mips_runtime.c\"" \
    -c pc_port/tests/battle_shared_bytes_test.c -o "$OUT/test.o"
gcc "${COMMON[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/cpu.o"

# The production objects that define the shared bytes, and misc4.c's field
# entry; every other symbol they reach gets a placeholder returning 0.
LINK=("$OUT/test.o" "$OUT/cpu.o" "$OBJ/stubs.o" "$OBJ/psx_memory.c.o"
    "$OBJ/src_field_main_misc4.c.o" "$OBJ/src_slus_006.64_system_temp3.c.o"
    "$OBJ/data_slus_sdata.c.o" "$OBJ/data_boot_globals.c.o")
if ! gcc -no-pie -rdynamic "${LINK[@]}" -ldl -lm -lstdc++ -o "$OUT/test" 2>"$OUT/link.err"; then
    undefined=$(grep -oE "undefined reference to \`[A-Za-z0-9_.]+'" "$OUT/link.err" \
        | sed "s/.*\`//; s/'//" | sort -u)
    [ -n "$undefined" ] || { cat "$OUT/link.err"; echo "BATTLE SHARED BYTES FAIL link"; exit 1; }
    {
        echo "/* Generated: placeholders for symbols this link lacks. */"
        for s in $undefined; do
            case "$s" in
                D_*|g_*|jtbl_*|jpt_*|s_*|gteRegs) echo "unsigned char $s[0x10000];" ;;
                *) echo "long $s(void) { return 0; }" ;;
            esac
        done
    } > "$OUT/placeholders.c"
    gcc -c -w "$OUT/placeholders.c" -o "$OUT/placeholders.o"
    gcc -no-pie -rdynamic "${LINK[@]}" "$OUT/placeholders.o" -ldl -lm -lstdc++ -o "$OUT/test"
    echo "     ($(echo "$undefined" | wc -l) placeholders)"
fi
status=0
"$OUT/test" || status=1
[ "$missing" = 0 ] || status=1
exit $status
