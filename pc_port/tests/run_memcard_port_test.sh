#!/usr/bin/env bash
# Native memory-card layer (pc_port/src/memcard_port.c): card-image format and
# BIOS file-API semantics, O0/O2/UBSan+ASan.
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/memcard-port.XXXXXX")
INC=(-Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 San; do
    flags=(-"$mode")
    if [ "$mode" = San ]; then flags=(-O1 -fsanitize=undefined,address -fno-sanitize-recover=all); fi
    gcc -std=gnu17 -Wall -Wextra -Werror "${flags[@]}" "${INC[@]}" pc_port/tests/memcard_port_test.c pc_port/src/memcard_port.c -o "$OUT/$mode"
    rm -rf "$OUT/cards.$mode"
    "$OUT/$mode" "$OUT/cards.$mode"
done
echo "PASS memcard_port (O0/O2/San)"
