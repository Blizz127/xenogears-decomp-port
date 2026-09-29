#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/krom-font-nobios.XXXXXX")
echo "OUTPUT $OUT"
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" -std=gnu17 -Wall -Wextra -Werror "${flags[@]}" -Ipc_port/src \
        pc_port/tests/krom_font_nobios_test.c pc_port/src/krom_mapping.c pc_port/src/krom_rom.c \
        $(pkg-config --cflags --libs libcrypto) -pthread -o "$OUT/$mode"
    env -u XENO_BIOS timeout 30s "$OUT/$mode"
done
