#!/usr/bin/env bash
# Differential test of the native step driver func_800B9508 (src/battle/
# mainc108_port.c) with func_800B9284 (mainc108_port.c) and func_800BF0C4
# (src/battle/mainc126.c port arm) against the retail MIPS bytes (battle.bin) on
# the MIPS adapter; every other callee is a recording spy on both sides.
# Env: DISC_DIR (default $ROOT/disc), PSYCROSS_DIR (default
# $ROOT/pc_port/extern/PsyCross), B9508_BUILD_DIR.
#   B9508_MUTANT_TU=<file> B9508_MUTANT="<sed-expr>"
#       run only O0 with that port TU (mainc108_port.c / mainc126.c) sed-mutated;
#       must FAIL
set -euo pipefail
if [[ ${B9508_BOUNDED:-0} != 1 ]]; then
    exec nice -n 19 timeout -k 5s 120s env B9508_BOUNDED=1 "$0" "$@"
fi
ulimit -c 0
ulimit -v 12582912
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
DISC_DIR="${DISC_DIR:-$ROOT/disc}"
PSYCROSS="${PSYCROSS_DIR:-$ROOT/pc_port/extern/PsyCross}"
BUILD_DIR="${B9508_BUILD_DIR:-$ROOT/pc_port/build_native/battle_b9508_port}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"

[[ "$(sha256sum "$DISC_DIR/battle.bin" | cut -d' ' -f1)" == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]] \
    || { echo "ERROR: battle.bin SHA-256 mismatch" >&2; exit 1; }
export B9508_BATTLE_BIN="$DISC_DIR/battle.bin"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DXENO_BATTLE_OVERLAY_HOST_BODIES -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -include stdint.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -I"$PSYCROSS/include" -I"$PSYCROSS/include/psx" -Ipc_port/src)
TUS=(mainc108_port mainc126)

run() {
    local name="$1" mtu="$2" mfile="$3"; shift 3
    local flags=("$@") objs=() tu src extra
    for tu in "${TUS[@]}"; do
        src="src/battle/$tu.c"
        if [[ "$tu.c" == "$mtu" ]]; then src="$mfile"; fi
        extra=()
        # mainc126.c also defines func_800BF354 (a spy here, not a batch member);
        # its other port bodies are discarded by --gc-sections.
        if [[ "$tu" == mainc126 ]]; then extra=(-Dfunc_800BF354=b9508_unused_func_800BF354); fi
        "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" "${extra[@]}" -c "$src" -o "$BUILD_DIR/$name.$tu.o"
        objs+=("$BUILD_DIR/$name.$tu.o")
    done
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/src/battle_mips_adapter.c -o "$BUILD_DIR/$name.cpu.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" \
        -c pc_port/tests/battle_b9508_port_test.c -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie "${flags[@]}" "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.cpu.o" \
        "${objs[@]}" -Wl,--gc-sections \
        -Wl,--defsym=D_80059464=g_PsxRam+0x59464 \
        -Wl,--defsym=D_8005A474=g_PsxRam+0x5A474 -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"
}

if [[ "${B9508_MUTANT:-}" != "" ]]; then
    mtu="${B9508_MUTANT_TU:?B9508_MUTANT needs B9508_MUTANT_TU}"
    sed "$B9508_MUTANT" "src/battle/$mtu" > "$BUILD_DIR/mutant_$mtu"
    if cmp -s "src/battle/$mtu" "$BUILD_DIR/mutant_$mtu"; then
        echo "MUTANT PATTERN DID NOT APPLY" >&2; exit 2
    fi
    run mutant "$mtu" "$BUILD_DIR/mutant_$mtu" -O0 -g
    exit 0
fi

run O0 "" "" -O0 -g
run O2 "" "" -O2
echo "BATTLE B9508 PORT O0/O2 PASS"
