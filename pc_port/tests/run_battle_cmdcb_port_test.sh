#!/usr/bin/env bash
# Differential test of the native command-callback callee port bodies
#   func_800B905C (src/battle/mainc107_port.c)   func_800BFA9C (mainc132_port.c)
#   func_800B8EBC (main106_port.c)               func_800B7E94 (mainc100_port.c)
#   func_800BF4F0 (mainc127_port.c)              func_800B9B54 (mainl109_port.c)
#   func_800B9284 (mainc108_port.c)
# against the retail MIPS bytes (battle.bin) on the MIPS adapter; every other
# callee is a recording spy on both sides.
# Env: DISC_DIR (default $ROOT/disc), PSYCROSS_DIR (default
# $ROOT/pc_port/extern/PsyCross), CMDCB_BUILD_DIR.
#   CMDCB_MUTANT_TU=<file> CMDCB_MUTANT="<sed-expr>"
#       run only O0 with that port TU (e.g. mainc107_port.c) sed-mutated; must FAIL
set -euo pipefail
if [[ ${CMDCB_BOUNDED:-0} != 1 ]]; then
    exec nice -n 19 timeout -k 5s 120s env CMDCB_BOUNDED=1 "$0" "$@"
fi
ulimit -c 0
ulimit -v 12582912
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
DISC_DIR="${DISC_DIR:-$ROOT/disc}"
PSYCROSS="${PSYCROSS_DIR:-$ROOT/pc_port/extern/PsyCross}"
BUILD_DIR="${CMDCB_BUILD_DIR:-$ROOT/pc_port/build_native/battle_cmdcb_port}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"

[[ "$(sha256sum "$DISC_DIR/battle.bin" | cut -d' ' -f1)" == 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291 ]] \
    || { echo "ERROR: battle.bin SHA-256 mismatch" >&2; exit 1; }
export CMDCB_BATTLE_BIN="$DISC_DIR/battle.bin"

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DXENO_BATTLE_OVERLAY_HOST_BODIES -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -include stdint.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -I"$PSYCROSS/include" -I"$PSYCROSS/include/psx" -Ipc_port/src)
TUS=(mainc107_port mainc132_port main106_port mainc100_port mainc127_port mainl109_port mainc108_port)

run() {
    local name="$1" mtu="$2" mfile="$3"; shift 3
    local flags=("$@") objs=() tu src extra
    for tu in "${TUS[@]}"; do
        src="src/battle/$tu.c"
        if [[ "$tu.c" == "$mtu" ]]; then src="$mfile"; fi
        extra=()
        # func_800BF3E8 is a spy here (not a batch member); keep its port body
        # out of the link.
        if [[ "$tu" == mainc127_port ]]; then extra=(-Dfunc_800BF3E8=cmdcb_unused_func_800BF3E8); fi
        # func_800B9508 (also in mainc108_port.c) is a spy here, not a batch member.
        if [[ "$tu" == mainc108_port ]]; then extra=(-Dfunc_800B9508=cmdcb_unused_func_800B9508); fi
        "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" "${extra[@]}" -c "$src" -o "$BUILD_DIR/$name.$tu.o"
        objs+=("$BUILD_DIR/$name.$tu.o")
    done
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/src/battle_mips_adapter.c -o "$BUILD_DIR/$name.cpu.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -w "${flags[@]}" \
        -c pc_port/tests/battle_cmdcb_port_test.c -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie "${flags[@]}" "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.cpu.o" \
        "${objs[@]}" -Wl,--gc-sections \
        -Wl,--defsym=D_80059464=g_PsxRam+0x59464 \
        -Wl,--defsym=D_8005A474=g_PsxRam+0x5A474 -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name"
}

if [[ "${CMDCB_MUTANT:-}" != "" ]]; then
    mtu="${CMDCB_MUTANT_TU:?CMDCB_MUTANT needs CMDCB_MUTANT_TU}"
    sed "$CMDCB_MUTANT" "src/battle/$mtu" > "$BUILD_DIR/mutant_$mtu"
    if cmp -s "src/battle/$mtu" "$BUILD_DIR/mutant_$mtu"; then
        echo "MUTANT PATTERN DID NOT APPLY" >&2; exit 2
    fi
    run mutant "$mtu" "$BUILD_DIR/mutant_$mtu" -O0 -g
    exit 0
fi

run O0 "" "" -O0 -g
run O2 "" "" -O2
echo "BATTLE CMDCB PORT O0/O2 PASS"
