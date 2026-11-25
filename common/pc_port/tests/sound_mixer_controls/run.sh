#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
out=$(mktemp -d)
trap 'rm -rf -- "$out"' EXIT
cc=${CC:-cc}
base=(-std=c11 -Wall -Wextra -Werror)
for mode in O0 O2 UBSan access; do
    opts=(-O2)
    instrument=()
    fixture=()
    case "$mode" in
        O0) opts=(-O0) ;;
        UBSan) opts+=(-fsanitize=undefined -fno-sanitize-recover=all) ;;
        access)
            instrument=(-fsanitize=thread)
            if [[ $("$cc" --version) == *clang* ]]; then
                instrument+=(-mllvm -tsan-distinguish-volatile=1)
            else
                instrument+=(--param tsan-distinguish-volatile=1)
            fi
            fixture=(-DOBSERVE_ACCESSES)
            ;;
    esac
    "$cc" "${base[@]}" "${opts[@]}" "${instrument[@]}" -DBUILD_COMMANDS -c "$here/host.c" -o "$out/commands.o"
    "$cc" "${base[@]}" "${opts[@]}" "${fixture[@]}" "$here/host.c" "$out/commands.o" -o "$out/test"
    printf '%s ' "$mode"
    "$out/test"
done
