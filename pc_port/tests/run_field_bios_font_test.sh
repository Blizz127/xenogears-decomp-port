#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/field-bios-font.XXXXXX")
echo "OUTPUT $OUT"
echo '11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef  disc/scph5500.bin' | sha256sum -c -
export XENO_BIOS=disc/scph5500.bin
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -ffunction-sections -fdata-sections -Ipc_port/include_shim -Iinclude -Ipc_port/src
      -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${BASE[@]}" "${flags[@]}" -include assert.h -w -c src/field/main/misc9.c -o "$OUT/field.o"
    gcc "${BASE[@]}" "${flags[@]}" pc_port/tests/field_bios_font_test.c "$OUT/field.o" \
        pc_port/src/krom_rom.c pc_port/src/krom_mapping.c $(pkg-config --cflags --libs libcrypto) \
        -pthread -Wl,--gc-sections -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
