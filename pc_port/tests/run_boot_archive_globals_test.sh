#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
CC="${CC:-gcc}"
TMPDIR_TEST="$(mktemp -d "${TMPDIR:-/tmp}/xeno-boot-archive-test.XXXXXX")"
trap 'rm -rf "$TMPDIR_TEST"' EXIT

INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include \
     -Ipc_port/extern/PsyCross/include/psx)
CFLAGS=(-std=gnu17 -fpermissive -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -include assert.h -w -O0 -g -ffunction-sections -fdata-sections)

"$CC" -c pc_port/src/data_boot_globals.c "${CFLAGS[@]}" "${INC[@]}" \
    -o "$TMPDIR_TEST/data_boot_globals.o"
"$CC" -c pc_port/src/archive_port.c "${CFLAGS[@]}" "${INC[@]}" \
    -o "$TMPDIR_TEST/archive_port.o"

readelf -sW "$TMPDIR_TEST/data_boot_globals.o" > "$TMPDIR_TEST/data.syms"
for symbol in D_8005A470 D_80062514; do
    awk -v wanted="$symbol" '$NF == wanted { if ($4 != "OBJECT") exit 1; if ($7 == "UND") exit 1; found=1 } END { exit(found ? 0 : 1) }' \
        "$TMPDIR_TEST/data.syms"
done

"$CC" "${CFLAGS[@]}" "${INC[@]}" -c pc_port/tests/boot_archive_globals_test.c \
    -o "$TMPDIR_TEST/harness.o"
"$CC" -Wl,--gc-sections "$TMPDIR_TEST/harness.o" \
    "$TMPDIR_TEST/archive_port.o" "$TMPDIR_TEST/data_boot_globals.o" \
    -o "$TMPDIR_TEST/test_boot_archive_globals"
"$TMPDIR_TEST/test_boot_archive_globals"
