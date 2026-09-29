#!/usr/bin/env bash
# Retail ControllerInit (0x80036288) installs the USA button layout. The
# production body (src/slus_006.64/system/controller2.c, one body for both
# builds; ControllerResetState comes from system/controller.c) is compared with
# an independent interpreter over the retail bytes, at O0/O2/UBSan, then two
# defective copies of the body must be rejected. Finally, if a port binary is
# present, XENO_BUTTON_LAYOUT must reject an unknown value.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=${CONTROLLER_INIT_OUT:-$(mktemp -d "${TMPDIR:-/tmp}/xeno-controller-init.XXXXXXXX")}
mkdir -p "$OUT"
python3 pc_port/tests/controller_init_layout_oracle.py disc/SLUS_006.64 \
    "$OUT/controller_init_layout_expected.h"

GAME=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h
      -ffunction-sections -fdata-sections -fno-builtin
      -Ipc_port/include_shim -Iinclude -Ipc_port/src
      -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
build_and_run() {  # $1=label $2=controller2 source $3..=flags
    local label="$1" src="$2"; shift 2
    gcc "${GAME[@]}" "$@" -w -c "$src" -o "$OUT/$label.controller2.o"
    gcc "${GAME[@]}" "$@" -w -c src/slus_006.64/system/controller.c -o "$OUT/$label.controller.o"
    gcc -std=gnu17 "$@" -Wall -Wextra -Werror -I"$OUT" \
        -c pc_port/tests/controller_init_layout_test.c -o "$OUT/$label.test.o"
    gcc "$@" -Wl,--gc-sections "$OUT/$label.controller2.o" "$OUT/$label.controller.o" \
        "$OUT/$label.test.o" -o "$OUT/$label"
    "$OUT/$label"
}
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    echo "CONTROLLER INIT production TU $mode"
    build_and_run "$mode" src/slus_006.64/system/controller2.c "${flags[@]}"
done

# Negative controls: mechanical copies of the production body, each must fail.
body_start='^void ControllerInit(void) {'
declare -A mutants=(
    [no-usa-swap]='/g_ControllerButtonMappings\[0\] = 1;/d; /g_ControllerButtonMappings\[1\] = 0;/d'
    [actuator-not-copied]='s/    p\[1\] = p\[0\];/    ;/'
)
for name in "${!mutants[@]}"; do
    sed "${mutants[$name]}" src/slus_006.64/system/controller2.c > "$OUT/$name.c"
    if cmp -s "$OUT/$name.c" src/slus_006.64/system/controller2.c; then
        echo "FAIL: mutant $name did not change the source"; exit 1
    fi
    if build_and_run "mut-$name" "$OUT/$name.c" -O2 > "$OUT/$name.log" 2>&1; then
        cat "$OUT/$name.log"; echo "FAIL: negative control $name was NOT rejected"; exit 1
    fi
    echo "control $name rejected: $(grep -m1 'FAIL' "$OUT/$name.log")"
done
grep -q "$body_start" src/slus_006.64/system/controller2.c

BIN=pc_port/build_native/xeno-port
if [ -x "$BIN" ]; then
    set +e
    XENO_BUTTON_LAYOUT=bogus SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}" \
        timeout 20s "$BIN" > "$OUT/bogus.log" 2>&1
    rc=$?
    set -e
    if [ "$rc" != 1 ] || ! grep -q 'XENO_BUTTON_LAYOUT=bogus: expected usa or jp' "$OUT/bogus.log"; then
        tail -5 "$OUT/bogus.log"; echo "FAIL: port accepted XENO_BUTTON_LAYOUT=bogus (rc=$rc)"; exit 1
    fi
    echo "port rejects XENO_BUTTON_LAYOUT=bogus"
else
    echo "NOTE: $BIN absent; XENO_BUTTON_LAYOUT binary check NOT RUN"
    exit 1
fi
echo "PASS ControllerInit USA layout (O0/O2/UBSan + 2 negative controls + layout switch)"
