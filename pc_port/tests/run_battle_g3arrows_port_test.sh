#!/usr/bin/env bash
# Differential test of the native func_80077698 port body (src/battle/main7_port.c)
# against the retail MIPS bytes (battle.bin + retail bzero/SetPolyG3 from
# SLUS_006.64) on the MIPS adapter.  Native side links the port's
# SetPolyG3 (psyq_compat_leaf.inc) and libc bzero.  Env: DISC_DIR (default $ROOT/disc),
# PSYCROSS_DIR (default $ROOT/pc_port/extern/PsyCross).
set -euo pipefail
if [[ ${G3ARROWS_BOUNDED:-0} != 1 ]]; then
    exec nice -n 19 timeout -k 5s 120s env G3ARROWS_BOUNDED=1 "$0" "$@"
fi
ulimit -c 0
ulimit -v 12582912
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
DISC_DIR="${DISC_DIR:-$ROOT/disc}"
PSYCROSS="${PSYCROSS_DIR:-$ROOT/pc_port/extern/PsyCross}"
BUILD_DIR="${G3ARROWS_BUILD_DIR:-$ROOT/pc_port/build_native/battle_g3arrows_port}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"

[[ "$(sha256sum "$DISC_DIR/battle.bin" | cut -d' ' -f1)" == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]] \
    || { echo "ERROR: battle.bin SHA-256 mismatch" >&2; exit 1; }
[[ "$(sha256sum "$DISC_DIR/SLUS_006.64" | cut -d' ' -f1)" == dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]] \
    || { echo "ERROR: SLUS_006.64 SHA-256 mismatch" >&2; exit 1; }
export G3_BATTLE_BIN="$DISC_DIR/battle.bin" G3_SLUS="$DISC_DIR/SLUS_006.64"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -include stdint.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -I"$PSYCROSS/include" -I"$PSYCROSS/include/psx" -Ipc_port/src)

# The port's SetPolyG3 (PsyCross only declares it; the definition is the
# one-line compat body in pc_port/src/psyq_compat_leaf.inc, sed-extracted).
sed -n -e '/^void SetPolyG3(/p' pc_port/src/psyq_compat_leaf.inc > "$BUILD_DIR/gpu.c"
grep -q 'setcode(primitive, 0x30)' "$BUILD_DIR/gpu.c"

run() {
    local name="$1" port="$2"; shift 2
    local flags=("$@")
    "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" -c "$port" -o "$BUILD_DIR/$name.port.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" -include common.h -include psyq/libgpu.h -include psyq/libgte.h -include psx_memory.h \
        -c "$BUILD_DIR/gpu.c" -o "$BUILD_DIR/$name.gpu.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/src/battle_mips_adapter.c -o "$BUILD_DIR/$name.cpu.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror -Wno-unused-parameter "${flags[@]}" \
        -c pc_port/tests/battle_g3arrows_port_test.c -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie "${flags[@]}" "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.cpu.o" \
        "$BUILD_DIR/$name.port.o" "$BUILD_DIR/$name.gpu.o" -Wl,--gc-sections \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"
}

if [[ "${G3ARROWS_MUTANT:-}" != "" ]]; then
    # G3ARROWS_MUTANT="<sed-expr>": run only O0 on a mutated copy; must FAIL.
    sed "$G3ARROWS_MUTANT" src/battle/main7_port.c > "$BUILD_DIR/mutant_main7_port.c"
    if cmp -s src/battle/main7_port.c "$BUILD_DIR/mutant_main7_port.c"; then
        echo "MUTANT PATTERN DID NOT APPLY" >&2; exit 2
    fi
    run mutant "$BUILD_DIR/mutant_main7_port.c" -O0 -g
    exit 0
fi

run O0 src/battle/main7_port.c -O0 -g
run O2 src/battle/main7_port.c -O2
echo "BATTLE G3ARROWS PORT O0/O2 PASS"
