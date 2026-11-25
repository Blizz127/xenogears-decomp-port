#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
build=$(mktemp -d)
trap 'rm -rf -- "$build"' EXIT
for mode in unoptimized optimized sanitized; do
    case "$mode" in
        unoptimized) flags=(-O0) ;;
        optimized) flags=(-O2) ;;
        sanitized) flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all) ;;
    esac
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror "${flags[@]}" "$here/host.c" -o "$build/test"
    "$build/test"
done
