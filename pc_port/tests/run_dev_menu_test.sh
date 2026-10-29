#!/usr/bin/env bash
# Dev menu controller / input-capture test (pc_port/tests/dev_menu_test.cpp):
# SDL virtual pads through the patched PsyX_pad.cpp and port_dev_menu.c.
# Needs the patched PsyCross tree (pc_port/build_port.sh); run in xenogears-dev.
set -euo pipefail
cd "$(dirname "$0")/../.."
PSX=pc_port/extern/PsyCross
grep -q 'PcPort_DevMenuPad' "$PSX/src/pad/PsyX_pad.cpp" || {
    echo "FAIL: $PSX/src/pad/PsyX_pad.cpp lacks psycross_pad_select.patch (run pc_port/build_port.sh)"; exit 1; }
grep -q '_xeno_dev_menu_values' "$PSX/src/pad/PsyX_pad.cpp" || {
    echo "FAIL: $PSX/src/pad/PsyX_pad.cpp lacks psycross_dev_menu_values.patch (run pc_port/build_port.sh)"; exit 1; }
# The toolbar strip is gone: nothing reserves or subtracts window pixels.
if grep -rq 'PC_PORT_HOST_TOOLBAR_HEIGHT' pc_port/src pc_port/patches; then
    echo "FAIL: PC_PORT_HOST_TOOLBAR_HEIGHT is still referenced"; exit 1; fi
grep -q 'extern "C" int PsyX_HostToolbarReservedHeight(void) { return 0; }' pc_port/src/psycross_host_toolbar.inl
if grep -q 'ANALOG: ON' "$PSX/src/pad/PsyX_pad.cpp"; then
    echo "FAIL: PsyCross Select+Start analog toggle still present"; exit 1; fi

OUT=$(mktemp -d "${TMPDIR:-/tmp}/dev-menu.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
# Whitespace-separated compiler and SDL overrides, without shell evaluation.
# Explicit empty SDL flags/libs skip pkg-config as well.
read -r -a task_dev_cc <<< "${CC:-gcc}"
read -r -a task_dev_cxx <<< "${CXX:-g++}"
if [[ ! ${SDL_CFLAGS+x} ]]; then SDL_CFLAGS=$(pkg-config --cflags sdl2); fi
if [[ ! ${SDL_LIBS+x} ]]; then SDL_LIBS=$(pkg-config --libs sdl2); fi
read -r -a task_dev_sdl_flags <<< "$SDL_CFLAGS"
read -r -a task_dev_sdl_libs <<< "$SDL_LIBS"
INC=(-include pc_port/src/port_compat.h -I"$PSX/include" -I"$PSX/include/psx" -I"$PSX/src" -Ipc_port/src)
"${task_dev_cxx[@]}" -std=gnu++17 -O1 -g -Wall -Wno-narrowing -fpermissive "${INC[@]}" "${task_dev_sdl_flags[@]}" -UNDEBUG \
    -c "$PSX/src/pad/PsyX_pad.cpp" -o "$OUT/PsyX_pad.o"
"${task_dev_cxx[@]}" -std=gnu++17 -O1 -g -Wall "${INC[@]}" "${task_dev_sdl_flags[@]}" -UNDEBUG \
    -c pc_port/tests/dev_menu_test.cpp -o "$OUT/test.o"
"${task_dev_cc[@]}" -std=gnu17 -O1 -g -Wall -Wextra -UNDEBUG -Ipc_port/src -c pc_port/src/port_dev_menu.c -o "$OUT/menu.o"
"${task_dev_cc[@]}" -std=gnu17 -O1 -g -Wall -Wextra -UNDEBUG -Ipc_port/src -c pc_port/src/port_dev_menu_warps.c -o "$OUT/warps.o"
"${task_dev_cc[@]}" -std=gnu17 -O1 -g -Wall -Wextra -UNDEBUG -Ipc_port/src -c pc_port/src/port_dev_menu_steps.c -o "$OUT/steps.o"
"${task_dev_cxx[@]}" "$OUT/test.o" "$OUT/PsyX_pad.o" "$OUT/menu.o" "$OUT/warps.o" "$OUT/steps.o" \
    "${task_dev_sdl_libs[@]}" -o "$OUT/dev_menu_test"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 60s "$OUT/dev_menu_test"
