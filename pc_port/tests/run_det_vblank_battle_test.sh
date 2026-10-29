#!/usr/bin/env bash
# XENO_DETERMINISTIC_VBLANK_BATTLE: production-linked unit test of the battle
# test clock (det_vblank_battle_test.c).  Runs the switch on and unset.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
PSYX="pc_port/extern/PsyCross"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xeno-det-battle.XXXXXX")"
[[ -n "${KEEP_BUILD:-}" ]] || trap 'rm -rf "$BUILD_DIR"' EXIT
INC=(-Ipc_port/include_shim -Iinclude -I"$PSYX/include" -I"$PSYX/include/psx")
GFLAGS=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY
        -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
        -include assert.h -w -O0 -g -m64 -fno-builtin
        -ffunction-sections -fdata-sections)
"$CC" -c pc_port/src/psyq_compat.c "${GFLAGS[@]}" "${INC[@]}" -o "$BUILD_DIR/psyq_compat.o"
read -r -a SDL_CFLAGS <<< "$(pkg-config --cflags sdl2)"
"${CXX:-g++}" -std=c++17 -O0 -g -fpermissive -w -ffunction-sections -fdata-sections \
    "${SDL_CFLAGS[@]}" "${INC[@]}" -c "$PSYX/src/psx/LIBAPI.C" -o "$BUILD_DIR/libapi.o"
"$CC" -c src/slus_006.64/system/controller2.c "${GFLAGS[@]}" "${INC[@]}" -Ipc_port/src \
    -o "$BUILD_DIR/controller2.o"
"$CC" "${GFLAGS[@]}" "${INC[@]}" -Wl,--gc-sections \
    pc_port/tests/det_vblank_battle_test.c pc_port/src/controller_vblank_service.c \
    "$BUILD_DIR/controller2.o" "$BUILD_DIR/psyq_compat.o" "$BUILD_DIR/libapi.o" \
    -o "$BUILD_DIR/test_det_battle"
clean=(-u XENO_FIELD_STEP_PAD_INPUT -u XENO_MENU_FORCE -u XENO_KERNEL_SEL -u XENO_FIELD_TEST
       -u XENO_PAD_TEST_INPUT -u XENO_PAD_ON_CONTROL -u XENO_SPEED)
env "${clean[@]}" XENO_DETERMINISTIC_VBLANK=2 XENO_DETERMINISTIC_VBLANK_FROM_FIELD=490 \
    XENO_DETERMINISTIC_VBLANK_BATTLE=1 "$BUILD_DIR/test_det_battle" on 2> "$BUILD_DIR/on.err" ||
    { cat "$BUILD_DIR/on.err" >&2; exit 1; }
grep '^\[det-vblank\] battle' "$BUILD_DIR/on.err"
for want in 'battle 1 enter: deterministic' 'battle 1 live:' 'battle 1 stall guard' \
            'battle 1 return: restored live=0'; do
    grep -q "\[det-vblank\] $want" "$BUILD_DIR/on.err" || { echo "missing log: $want" >&2; exit 1; }
done
env "${clean[@]}" -u XENO_DETERMINISTIC_VBLANK_BATTLE XENO_DETERMINISTIC_VBLANK=2 \
    XENO_DETERMINISTIC_VBLANK_FROM_FIELD=490 "$BUILD_DIR/test_det_battle" off 2> "$BUILD_DIR/off.err" ||
    { cat "$BUILD_DIR/off.err" >&2; exit 1; }
if grep -q '\[det-vblank\] battle' "$BUILD_DIR/off.err"; then
    echo "switch unset but battle log lines present" >&2; exit 1
fi
# Negative control: the "on" assertions must reject a host-paced battle.
if env "${clean[@]}" -u XENO_DETERMINISTIC_VBLANK_BATTLE XENO_DETERMINISTIC_VBLANK=2 \
    XENO_DETERMINISTIC_VBLANK_FROM_FIELD=490 "$BUILD_DIR/test_det_battle" on >/dev/null 2>&1; then
    echo "negative control: on-assertions passed without the switch" >&2; exit 1
fi
echo "negative control: on-assertions reject the host-paced battle"
echo "DET VBLANK BATTLE TEST PASS"
