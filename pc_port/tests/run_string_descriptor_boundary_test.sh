#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/string-descriptor-boundary.XXXXXX")
echo "OUTPUT $OUT"
awk '/^static unsigned char s_StringRenderDescriptor/ {body=1} body && /^#else/ {exit} body {print}' src/slus_006.64/system/system.c > "$OUT/pointers.inc"
test -s "$OUT/pointers.inc"
awk '/^s32 SystemRenderStringEntry\(/ {body=1} body {print} /^}/ {body=0}' src/slus_006.64/system/system.c > "$OUT/render.inc"
test -s "$OUT/render.inc"
BASE=(-std=gnu17 -fno-pie -no-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -I"$OUT" -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/string_descriptor_boundary_test.c -o "$OUT/$mode"
    echo "RUN $mode pointer boundary contract"
    timeout 10s "$OUT/$mode"
done
