#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${W34B26_OUT:-pc_port/build_native/w34b26-cert}"
mkdir -p "$OUT"

FLAGS=(
    -std=gnu17 -g -Wall -Wextra -Wconversion -Wsign-conversion -Werror
    -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
)
TEST="pc_port/tests/w34b26_80071490_prod_test.c"
FRAME="pc_port/src/world_map_frame_driver.c"

build_run() {
    local label="$1"
    local opt="$2"
    local san="${3:-}"
    gcc -c "$TEST" "${FLAGS[@]}" "$opt" $san -o "$OUT/$label.test.o"
    gcc -c "$FRAME" "${FLAGS[@]}" "$opt" -DWM_712D0_TEST_TRACE $san \
        -DWM_7169C_CONTINUATION_DISABLED -o "$OUT/$label.frame.o"
    gcc -c pc_port/src/world_map_image_transfer_25044.c -std=gnu17 -g -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src "$opt" $san -o "$OUT/$label.xfer.o"
    gcc -c pc_port/src/world_map_upload_pump_74f2c.c -std=gnu17 -g -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src "$opt" $san -o "$OUT/$label.pump.o"
    gcc -c pc_port/src/world_map_ot_adapter.c -std=gnu17 -g -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src "$opt" $san -o "$OUT/$label.ot.o"
    gcc -no-pie -Wl,--gc-sections $san "$OUT/$label.test.o" \
        "$OUT/$label.frame.o" "$OUT/$label.xfer.o" "$OUT/$label.pump.o" "$OUT/$label.ot.o" -o "$OUT/$label"
    set +e
    "$OUT/$label" > "$OUT/$label.stdout" 2> "$OUT/$label.stderr"
    local rc=$?
    set -e
    echo "$rc" > "$OUT/$label.rc"
}

build_run O0 -O0
build_run O2 -O2
build_run UBSan_O2 -O2 "-fsanitize=undefined -fno-sanitize-recover=all"

for label in O0 O2 UBSan_O2; do
    test "$(<"$OUT/$label.rc")" = 0
    rg -q '^=== Results: 12/12 PASS ===$' "$OUT/$label.stdout"
    ! rg -qi 'runtime error|undefined behavior' "$OUT/$label.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan_O2.stdout"
printf 'W34B26 TEST PASS O0=12/12 O2=12/12 UBSan_O2=12/12\n'
