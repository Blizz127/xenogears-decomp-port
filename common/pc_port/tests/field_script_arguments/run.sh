#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")"
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
for mode in O0 O2 UBSan; do
    flags=("-$mode")
    if [[ $mode == UBSan ]]; then
        flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    "${CC:-cc}" -std=c17 -Wall -Wextra -Werror "${flags[@]}" host.c -o "$tmp/test"
    "$tmp/test"
done
