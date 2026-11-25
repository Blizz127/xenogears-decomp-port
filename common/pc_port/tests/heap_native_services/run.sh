#!/usr/bin/env bash
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
for mode in O0 O2 UBSan; do
    opts=(-O2)
    case "$mode" in
        O0) opts=(-O0);;
        UBSan) opts+=(-fsanitize=undefined -fno-sanitize-recover=all);;
    esac
    "${CC:-cc}" -std=gnu11 -UNDEBUG -Wall -Wextra -Werror "${opts[@]}" "$here/host.c" -o "$tmp/heap-test"
    "$tmp/heap-test"
done
