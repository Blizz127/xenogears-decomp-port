#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT="${W34B37_OUT:-pc_port/build_native/w34b37-cert}"
mkdir -p "$OUT"
FLAGS=(
    -std=gnu17 -g -Wall -Wextra -Wconversion -Wsign-conversion -Werror
    -fpermissive -fno-builtin -include assert.h -w
    -DXENO_PC_PORT -DWM_7369C_PROD_TEST -DSKIP_ASM -D_LANGUAGE_C
    -DUSE_EXTENDED_PRIM_POINTERS=0 -fno-pie -ffunction-sections
    -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
)

build_run() {
    local label="$1" opt="$2" san="${3:-}"
    gcc -c pc_port/tests/w34b37_7369c_ot_writer_prod_test.c "${FLAGS[@]}" \
        "$opt" $san -o "$OUT/$label.test.o"
    gcc -c pc_port/src/world_map_init.c "${FLAGS[@]}" "$opt" $san \
        -o "$OUT/$label.init.o"
    gcc -c pc_port/src/psx_memory.c "${FLAGS[@]}" "$opt" $san \
        -o "$OUT/$label.memory.o"
    gcc -no-pie -Wl,--gc-sections $san "$OUT/$label.test.o" \
        "$OUT/$label.init.o" "$OUT/$label.memory.o" -o "$OUT/$label"
    "$OUT/$label" > "$OUT/$label.stdout" 2> "$OUT/$label.stderr"
}

build_run O0 -O0
build_run O2 -O2
build_run UBSan_O2 -O2 "-fsanitize=undefined -fno-sanitize-recover=all"
for label in O0 O2 UBSan_O2; do
    rg -q '^W34B37 OT WRITER PASS first=0x80012340 second=0x8001e7a0$' \
        "$OUT/$label.stdout"
    ! rg -qi 'runtime error|undefined behavior' "$OUT/$label.stderr"
done
cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan_O2.stdout"
printf 'W34B37 TEST PASS O0/O2/UBSan=1/1/1 asymmetric guest mappings\n'
