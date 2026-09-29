#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(mktemp -d -t field-party-gear.XXXXXX)"
trap 'rm -rf -- "$OUT"' EXIT
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0 -fno-pie -no-pie
      -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src
     -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)

for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    gcc "${BASE[@]}" "${INC[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/src/field_party_gear.c -o "$OUT/$opt.native.o"
    gcc "${BASE[@]}" "${INC[@]}" "${flags[@]}" -Wall -Wextra -Werror \
        -c pc_port/tests/field_party_gear_test.c -o "$OUT/$opt.test.o"
    gcc -no-pie "${flags[@]}" -Wl,--gc-sections \
        "$OUT/$opt.native.o" "$OUT/$opt.test.o" -o "$OUT/$opt.test"
    "$OUT/$opt.test" >"$OUT/$opt.log"
done

cmp "$OUT/O0.log" "$OUT/O2.log"
cmp "$OUT/O0.log" "$OUT/UBSan.log"
cat "$OUT/O0.log"
