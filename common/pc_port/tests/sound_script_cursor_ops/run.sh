#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
cc=${CC:-cc}
common=(-std=c11 -Wall -Wextra -Werror)
fragment=()
if [[ $# -eq 1 ]]; then
    fragment=("-DCURSOR_FRAGMENT=\"$(realpath -- "$1")\"")
fi
for mode in O0 O2 UBSan access; do
    opt=(-O2)
    instrument=()
    checker=()
    case "$mode" in
        O0) opt=(-O0) ;;
        UBSan) opt+=(-fsanitize=undefined -fno-sanitize-recover=all) ;;
        access)
            instrument=(-fsanitize=thread)
            if [[ $("$cc" --version) == *clang* ]]; then
                instrument+=(-mllvm -tsan-distinguish-volatile=1)
            else
                instrument+=(--param tsan-distinguish-volatile=1)
            fi
            checker=(-DOBSERVE_ACCESSES)
            ;;
    esac
    "$cc" "${common[@]}" "${opt[@]}" "${instrument[@]}" "${fragment[@]}" -DBUILD_COMMANDS \
        -c "$here/host.c" -o "$tmp/commands.o"
    "$cc" "${common[@]}" "${opt[@]}" "${checker[@]}" "$here/host.c" "$tmp/commands.o" -o "$tmp/check"
    printf '%s ' "$mode"
    "$tmp/check"
done
