#!/usr/bin/env bash
# Host game-controller selection / mapping test (pc_port/tests/pad_select_test.cpp).
# No hardware: SDL virtual joysticks.  Needs the patched PsyCross tree
# (pc_port/build_port.sh applies pc_port/patches/psycross_pad_select.patch)
# and SDL2 >= 2.24 development files; run inside xenogears-dev.
set -euo pipefail
cd "$(dirname "$0")/../.."
PSX=pc_port/extern/PsyCross

grep -q '_xeno_pad_select' "$PSX/src/pad/PsyX_pad.cpp" || {
    echo "FAIL: $PSX/src/pad/PsyX_pad.cpp lacks psycross_pad_select.patch (run pc_port/build_port.sh)"
    exit 1
}
grep -q 'psycross_pad_select.patch" "_xeno_pad_select"' pc_port/build_port.sh

# The test's mapping table must be PsyCross's own defaults (retail layout).
for want in 'gc_square = SDL_CONTROLLER_BUTTON_X' 'gc_circle = SDL_CONTROLLER_BUTTON_B' \
            'gc_triangle = SDL_CONTROLLER_BUTTON_Y' 'gc_cross = SDL_CONTROLLER_BUTTON_A' \
            'gc_l1 = SDL_CONTROLLER_BUTTON_LEFTSHOULDER' \
            'gc_l2 = SDL_CONTROLLER_AXIS_TRIGGERLEFT | CONTROLLER_MAP_FLAG_AXIS' \
            'gc_r1 = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER' \
            'gc_r2 = SDL_CONTROLLER_AXIS_TRIGGERRIGHT | CONTROLLER_MAP_FLAG_AXIS' \
            'gc_l3 = SDL_CONTROLLER_BUTTON_LEFTSTICK' 'gc_r3 = SDL_CONTROLLER_BUTTON_RIGHTSTICK' \
            'gc_select = SDL_CONTROLLER_BUTTON_BACK' 'gc_start = SDL_CONTROLLER_BUTTON_START'; do
    grep -qF "g_cfg_controllerMapping.$want;" "$PSX/src/PsyX_main.cpp" || {
        echo "FAIL: PsyCross default mapping changed: $want"; exit 1; }
done

OUT=$(mktemp -d "${TMPDIR:-/tmp}/pad-select.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
SDL_CFLAGS=$(pkg-config --cflags sdl2)
SDL_LIBS=$(pkg-config --libs sdl2)
INC=(-include pc_port/src/port_compat.h -I"$PSX/include" -I"$PSX/include/psx" -I"$PSX/src")
g++ -std=gnu++17 -O1 -g -Wall -Wno-narrowing -fpermissive "${INC[@]}" $SDL_CFLAGS \
    -c "$PSX/src/pad/PsyX_pad.cpp" -o "$OUT/PsyX_pad.o"
g++ -std=gnu++17 -O1 -g -Wall "${INC[@]}" $SDL_CFLAGS \
    -c pc_port/tests/pad_select_test.cpp -o "$OUT/pad_select_test.o"
# xg_plat_input_set_pad_face_swap from the real backend file; its SDK
# routing wrappers are dropped by section GC.
gcc -std=gnu17 -O1 -g -w -ffunction-sections -fdata-sections -I"$PSX/include" \
    -c pc_port/src/plat/xg_plat_psycross.c -o "$OUT/xg_plat_psycross.o"
g++ -Wl,--gc-sections "$OUT/pad_select_test.o" "$OUT/PsyX_pad.o" "$OUT/xg_plat_psycross.o" \
    $SDL_LIBS -o "$OUT/pad_select_test"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 60s "$OUT/pad_select_test"
