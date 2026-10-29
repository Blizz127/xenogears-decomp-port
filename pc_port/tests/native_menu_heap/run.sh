#!/usr/bin/env bash
set -euo pipefail
if [[ ${XENO_HEAP_PREVIEW_BOUNDED:-0} != 1 ]]; then
    exec timeout -k 5s 60s env XENO_HEAP_PREVIEW_BOUNDED=1 "$0" "$@"
fi
cd "$(dirname "$0")/../../.."
ulimit -c 0
ulimit -v 12582912
OUT=$(mktemp -d "${TMPDIR:-/tmp}/xeno-menu-heap.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
read -r -a PORT_FLAGS < pc_port/build_native_only/port-source.cflags
gcc "${PORT_FLAGS[@]}" -fno-pie -ffunction-sections -fdata-sections -c src/slus_006.64/system/memory.c -o "$OUT/memory.o"
gcc -std=c17 -O1 -Wall -Wextra -Werror -fno-pie -no-pie -Wl,--gc-sections pc_port/tests/native_menu_heap/preview-test.c "$OUT/memory.o" -o "$OUT/test"
"$OUT/test"
