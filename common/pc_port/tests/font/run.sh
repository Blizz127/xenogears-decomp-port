#!/bin/sh
set -eu
here=$(CDPATH= cd "$(dirname "$0")" && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/font-test.XXXXXX")
trap 'rm -rf "$work"' EXIT
cc=${CC:-cc}
for mode in O0 O2 UBSan; do
    case "$mode" in
        UBSan) set -- -O2 -fsanitize=undefined -fno-sanitize-recover=all ;;
        *) set -- "-$mode" ;;
    esac
    "$cc" -std=c11 -Wall -Wextra -Werror -UNDEBUG "$@" "$here/host.c" -o "$work/test"
    "$work/test"
    printf '%s %s passed\n' "$cc" "$mode"
done
