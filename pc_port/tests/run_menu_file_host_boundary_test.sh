#!/usr/bin/env bash
# Diagnostic witness of an unresolved backend, never a card acceptance gate.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/menu-file-host-boundary.XXXXXX")
echo "OUTPUT $OUT"
objdump -d --disassemble=func_801C9038 pc_port/build_native/xeno-port > "$OUT/production-bindings.txt"
# The host file access goes through the native memory-card layer
# (pc_port/src/memcard_port.c), not raw libc open/read/close.
for call in open read close; do
    if ! grep -q "<PcPortCard_$call>" "$OUT/production-bindings.txt"; then
        echo "MENU FILE HOST BOUNDARY FAIL: func_801C9038 does not call PcPortCard_$call" >&2
        exit 1
    fi
done
awk '/^s32 func_801C9038\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/card_file.inc"
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" -std=gnu17 -Wall -Wextra -Werror "${flags[@]}" -I"$OUT" pc_port/tests/menu_file_host_boundary_test.c -o "$OUT/$mode"
    "$OUT/$mode"
done
