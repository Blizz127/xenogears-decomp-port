#!/usr/bin/env bash
# Differential test of the native command-menu state-5 handler func_80081B58 (src/battle/
# mainasm_12068_port.c) against the retail MIPS bytes (battle.bin) on the MIPS
# adapter; every callee is a recording spy on both sides.
# Env: DISC_DIR (default $ROOT/disc), PSYCROSS_DIR (default
# $ROOT/pc_port/extern/PsyCross), B81B58_BUILD_DIR, B81B58_CASES.
#   B81B58_MUTANT_PERL=<perl -0pe expression>
#       run only O0 with src/battle/mainasm_12068_port.c perl-mutated; must FAIL
set -euo pipefail
if [[ ${B81B58_BOUNDED:-0} != 1 ]]; then
    exec nice -n 19 timeout -k 5s 120s env B81B58_BOUNDED=1 "$0" "$@"
fi
ulimit -c 0
ulimit -v 12582912
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
DISC_DIR="${DISC_DIR:-$ROOT/disc}"
PSYCROSS="${PSYCROSS_DIR:-$ROOT/pc_port/extern/PsyCross}"
BUILD_DIR="${B81B58_BUILD_DIR:-$ROOT/pc_port/build_native/battle_81b58_port}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"

[[ "$(sha256sum "$DISC_DIR/battle.bin" | cut -d' ' -f1)" == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]] \
    || { echo "ERROR: battle.bin SHA-256 mismatch" >&2; exit 1; }
export B81B58_BATTLE_BIN="$DISC_DIR/battle.bin"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DXENO_BATTLE_OVERLAY_HOST_BODIES -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -include stdint.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -I"$PSYCROSS/include" -I"$PSYCROSS/include/psx" -Ipc_port/src)

run() {
    local name="$1" src="$2"; shift 2
    local flags=("$@")
    "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" -c "$src" -o "$BUILD_DIR/$name.tu.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/src/battle_mips_adapter.c -o "$BUILD_DIR/$name.cpu.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" \
        -c pc_port/tests/battle_81b58_port_test.c -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie "${flags[@]}" "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.cpu.o" \
        "$BUILD_DIR/$name.tu.o" -Wl,--gc-sections -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"
}

if [[ "${B81B58_MUTANT_PERL:-}" != "" ]]; then
    perl -0pe "$B81B58_MUTANT_PERL" src/battle/mainasm_12068_port.c > "$BUILD_DIR/mutant_mainasm_12068_port.c"
    if cmp -s src/battle/mainasm_12068_port.c "$BUILD_DIR/mutant_mainasm_12068_port.c"; then
        echo "MUTANT PATTERN DID NOT APPLY" >&2; exit 2
    fi
    run mutant "$BUILD_DIR/mutant_mainasm_12068_port.c" -O0 -g
    exit 0
fi

run O0 src/battle/mainasm_12068_port.c -O0 -g
run O2 src/battle/mainasm_12068_port.c -O2
echo "BATTLE 81B58 PORT O0/O2 PASS"
