#!/usr/bin/env bash
# Differential test of the native func_800B1720 port body (src/battle/main77.c)
# against the retail MIPS bytes (battle.bin + retail SetShadeTex/SetSemiTrans
# from SLUS_006.64) on the MIPS adapter.  Native side links the real PsyCross
# SetShadeTex/SetSemiTrans.  Env: DISC_DIR (default $ROOT/disc),
# PSYCROSS_DIR (default $ROOT/pc_port/extern/PsyCross).
set -euo pipefail
if [[ ${B1720_BOUNDED:-0} != 1 ]]; then
    exec nice -n 19 timeout -k 5s 120s env B1720_BOUNDED=1 "$0" "$@"
fi
ulimit -c 0
ulimit -v 12582912
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
DISC_DIR="${DISC_DIR:-$ROOT/disc}"
PSYCROSS="${PSYCROSS_DIR:-$ROOT/pc_port/extern/PsyCross}"
BUILD_DIR="${B1720_BUILD_DIR:-$ROOT/pc_port/build_native/battle_b1720_port}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"

[[ "$(sha256sum "$DISC_DIR/battle.bin" | cut -d' ' -f1)" == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]] \
    || { echo "ERROR: battle.bin SHA-256 mismatch" >&2; exit 1; }
[[ "$(sha256sum "$DISC_DIR/SLUS_006.64" | cut -d' ' -f1)" == dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119 ]] \
    || { echo "ERROR: SLUS_006.64 SHA-256 mismatch" >&2; exit 1; }
export B1720_BATTLE_BIN="$DISC_DIR/battle.bin" B1720_SLUS="$DISC_DIR/SLUS_006.64"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -include stdint.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -I"$PSYCROSS/include" -I"$PSYCROSS/include/psx" -Ipc_port/src)

# Real PsyCross SetSemiTrans/SetShadeTex (sed-extracted, as the popup runner does).
sed -n -e '/^void SetSemiTrans(/,/^}/p' -e '/^void SetShadeTex(/,/^}/p' \
    "$PSYCROSS/src/psx/LIBGPU.C" > "$BUILD_DIR/gpu.c"
grep -q setShadeTex "$BUILD_DIR/gpu.c" && grep -q setSemiTrans "$BUILD_DIR/gpu.c"

run() {
    local name="$1" main77="$2"; shift 2
    local flags=("$@")
    "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" -c "$main77" -o "$BUILD_DIR/$name.main77.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" -include common.h -include psyq/libgpu.h -include psyq/libgte.h -include psx_memory.h \
        -c "$BUILD_DIR/gpu.c" -o "$BUILD_DIR/$name.gpu.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/src/battle_mips_adapter.c -o "$BUILD_DIR/$name.cpu.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror -Wno-unused-parameter "${flags[@]}" \
        -c pc_port/tests/battle_b1720_port_test.c -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie "${flags[@]}" "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.cpu.o" \
        "$BUILD_DIR/$name.main77.o" "$BUILD_DIR/$name.gpu.o" -Wl,--gc-sections \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"
}

if [[ "${B1720_MUTANT_FILE:-}" != "" ]]; then
    # B1720_MUTANT_FILE=<hand-edited main77.c copy>: run only O0; must FAIL.
    run mutant "$B1720_MUTANT_FILE" -O0 -g
    exit 0
fi
if [[ "${B1720_MUTANT:-}" != "" ]]; then
    # B1720_MUTANT="<sed-expr>": run only O0 on a mutated copy; must FAIL.
    sed "$B1720_MUTANT" src/battle/main77.c > "$BUILD_DIR/mutant_main77.c"
    if cmp -s src/battle/main77.c "$BUILD_DIR/mutant_main77.c"; then
        echo "MUTANT PATTERN DID NOT APPLY" >&2; exit 2
    fi
    run mutant "$BUILD_DIR/mutant_main77.c" -O0 -g
    exit 0
fi

run O0 src/battle/main77.c -O0 -g
run O2 src/battle/main77.c -O2
echo "BATTLE B1720 PORT O0/O2 PASS"
