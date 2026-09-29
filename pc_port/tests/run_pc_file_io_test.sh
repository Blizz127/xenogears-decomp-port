#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
out=$(mktemp -d)
trap 'rm -rf -- "$out"' EXIT
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [[ "$opt" == UBSan ]]; then
        flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    "${CC:-gcc}" -std=gnu17 -Wall -Wextra -Werror "${flags[@]}" -Iinclude \
        pc_port/src/pc_file_io.c pc_port/tests/pc_file_io_test.c -o "$out/$opt"
    "$out/$opt"
done
