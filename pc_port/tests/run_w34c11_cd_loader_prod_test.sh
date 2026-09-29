#!/usr/bin/env bash
# W34C11 — CD record-list loader + 967E4 wiring certificate, O0/O2/UBSan + mutants.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"; BUILD_DIR="${W34C11_BUILD_DIR:-$ROOT/pc_port/build_native}"; mkdir -p "$BUILD_DIR"; cd "$ROOT"
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C)
gcc "${BASE[@]}" "${INC[@]}" -w -O0 -c pc_port/src/world_map_helper_9623c.c -o "$BUILD_DIR/w34c11_9623c.o"
gcc "${BASE[@]}" "${INC[@]}" -w -O0 -c pc_port/src/world_map_helper_96328.c -o "$BUILD_DIR/w34c11_96328.o"
SRC=(pc_port/tests/w34c11_cd_loader_prod_test.c pc_port/src/psx_memory.c pc_port/src/world_map_helper_96130.c "$BUILD_DIR/w34c11_9623c.o" "$BUILD_DIR/w34c11_96328.o")
# 966cc.c also holds the PC-file loader (PCopen etc.); only 9699C is under test — compile it with -w and
# link the PC loader symbols from the test's stub by excluding 966CC via a wrapper object.
gcc "${BASE[@]}" "${INC[@]}" -w -O0 -c pc_port/src/world_map_helper_966cc.c -o "$BUILD_DIR/w34c11_966cc.o"
objcopy --weaken-symbol=wm_800966CC "$BUILD_DIR/w34c11_966cc.o"
bar() { local n="$1"; shift; gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" "$BUILD_DIR/w34c11_966cc.o" -lm -o "$BUILD_DIR/$n"; "$BUILD_DIR/$n"; }
echo "== focused production regimes =="; bar w34c11_O0 -O0 -g; bar w34c11_O2 -O2; bar w34c11_ubsan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
echo "== mutants =="
for m in "M1 WM_967E4_MUTANT_M1" "M2 WM_967E4_MUTANT_M2" "M3 WM_9699C_MUTANT_M3" "M4 WM_9699C_MUTANT_M4" "M5 WM_9699C_MUTANT_M5" "M6 WM_963E4_MUTANT_M6" "M7 WM_963E4_MUTANT_M7" "M8 WM_963E4_MUTANT_M8" "M9 WM_96328_MUTANT_M9"; do
    set -- $m; name=$1; def=$2
    gcc "${BASE[@]}" "${INC[@]}" -w -D$def -O0 -c pc_port/src/world_map_helper_966cc.c -o "$BUILD_DIR/w34c11_966cc_$name.o"; objcopy --weaken-symbol=wm_800966CC "$BUILD_DIR/w34c11_966cc_$name.o"
    gcc "${BASE[@]}" "${INC[@]}" -w -D$def -O0 -c pc_port/src/world_map_helper_9623c.c -o "$BUILD_DIR/w34c11_9623c_$name.o"
    gcc "${BASE[@]}" "${INC[@]}" -w -D$def -O0 -c pc_port/src/world_map_helper_96328.c -o "$BUILD_DIR/w34c11_96328_$name.o"
    gcc "${BASE[@]}" "${INC[@]}" -w -D$def -O0 pc_port/tests/w34c11_cd_loader_prod_test.c pc_port/src/psx_memory.c pc_port/src/world_map_helper_96130.c "$BUILD_DIR/w34c11_9623c_$name.o" "$BUILD_DIR/w34c11_96328_$name.o" "$BUILD_DIR/w34c11_966cc_$name.o" -lm -o "$BUILD_DIR/w34c11_mutant_$name"
    set +e; "$BUILD_DIR/w34c11_mutant_$name" >"$BUILD_DIR/w34c11_mutant_$name.log" 2>&1; rc=$?; set -e
    if [ "$rc" -eq 0 ] || ! grep -q 'ASSERTION ' "$BUILD_DIR/w34c11_mutant_$name.log"; then echo "$name FAILED mutant gate (rc=$rc)" >&2; exit 1; fi
    echo "$name: DETECTED $(grep -o 'ASSERTION [a-z_0-9]* FAILED' "$BUILD_DIR/w34c11_mutant_$name.log" | head -1)"
done
echo "W34C11 CD loader focused certificate PASS"
