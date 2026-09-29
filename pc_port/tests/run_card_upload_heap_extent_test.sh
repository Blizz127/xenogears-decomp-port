#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/card-upload-heap-extent.XXXXXX")
echo "OUTPUT $OUT"
awk '/^void\* HeapAlloc\(/ {body=1} body {print} /^}/ {body=0}' src/slus_006.64/system/memory.c > "$OUT/allocator.inc"
awk '/^void GR_CopyVRAM\(/ {body=1} body {print} /^}/ {body=0}' pc_port/extern/PsyCross/src/render/PsyX_render.cpp > "$OUT/copy.inc"
test -s "$OUT/allocator.inc"
test -s "$OUT/copy.inc"
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/card_upload_heap_extent_test.c -o "$OUT/$mode"
    echo "RUN $mode heap/upload diagnostic"
    timeout 10s "$OUT/$mode"
done
