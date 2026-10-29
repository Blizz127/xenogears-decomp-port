#!/usr/bin/env bash
# Differential test of the native Attack-accept target chain port bodies
# (func_80084A7C -> func_800841E0 -> func_80083FF4, src/battle/main35_port.c)
# against the retail MIPS bytes (battle.bin) on the MIPS adapter; the chain
# has no outside callee, so both sides run real code and compare the return
# value, all of guest RAM, and the callee-saved registers.
# Env: DISC_DIR (default $ROOT/disc), PSYCROSS_DIR (default
# $ROOT/pc_port/extern/PsyCross), TLIST_BUILD_DIR.
#   TLIST_MUTANT="<sed-expr>"      run only O0 on a sed-mutated copy of the
#                                  port TU; must FAIL
#   TLIST_MUTANT_FILE=<file>       same with a hand-edited copy
set -euo pipefail
if [[ ${TLIST_BOUNDED:-0} != 1 ]]; then
    exec nice -n 19 timeout -k 5s 120s env TLIST_BOUNDED=1 "$0" "$@"
fi
ulimit -c 0
ulimit -v 12582912
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
DISC_DIR="${DISC_DIR:-$ROOT/disc}"
PSYCROSS="${PSYCROSS_DIR:-$ROOT/pc_port/extern/PsyCross}"
BUILD_DIR="${TLIST_BUILD_DIR:-$ROOT/pc_port/build_native/battle_target_list_port}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"

[[ "$(sha256sum "$DISC_DIR/battle.bin" | cut -d' ' -f1)" == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]] \
    || { echo "ERROR: battle.bin SHA-256 mismatch" >&2; exit 1; }
export TLIST_BATTLE_BIN="$DISC_DIR/battle.bin"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -include stdint.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -I"$PSYCROSS/include" -I"$PSYCROSS/include/psx" -Ipc_port/src)

run() {
    local name="$1" port="$2"; shift 2
    local flags=("$@")
    "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" -c "$port" -o "$BUILD_DIR/$name.port.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/src/battle_mips_adapter.c -o "$BUILD_DIR/$name.cpu.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror -Wno-unused-parameter "${flags[@]}" \
        -c pc_port/tests/battle_target_list_port_test.c -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie "${flags[@]}" "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.cpu.o" \
        "$BUILD_DIR/$name.port.o" -Wl,--gc-sections -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"
}

PORT=src/battle/main35_port.c
if [[ "${TLIST_MUTANT_FILE:-}" != "" ]]; then
    run mutant "$TLIST_MUTANT_FILE" -O0 -g
    exit 0
fi
if [[ "${TLIST_MUTANT:-}" != "" ]]; then
    sed "$TLIST_MUTANT" "$PORT" > "$BUILD_DIR/mutant_port.c"
    if cmp -s "$PORT" "$BUILD_DIR/mutant_port.c"; then
        echo "MUTANT PATTERN DID NOT APPLY" >&2; exit 2
    fi
    run mutant "$BUILD_DIR/mutant_port.c" -O0 -g
    exit 0
fi

run O0 "$PORT" -O0 -g
run O2 "$PORT" -O2
echo "BATTLE TARGET LIST PORT O0/O2 PASS"
