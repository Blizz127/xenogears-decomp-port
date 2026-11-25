#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
cc=${CC:-cc}
base=(-std=gnu17 -fPIE -Wall -Wextra -Werror)
fragment=()
if [[ $# -ge 1 ]]; then fragment=("-DTRANSFER_FRAGMENT=\"$(realpath -- "$1")\""); fi
for mode in O0 O2 UBSan access; do
    opts=(-O2)
    observe=()
    checker=()
    case "$mode" in
        O0) opts=(-O0) ;;
        UBSan) opts+=(-fsanitize=undefined -fno-sanitize-recover=all) ;;
        access)
            observe=(-fsanitize=thread -finstrument-functions -fno-inline)
            if [[ $("$cc" --version) == *clang* ]]; then
                observe+=(-mllvm -tsan-distinguish-volatile=1)
            else
                observe+=(--param tsan-distinguish-volatile=1)
            fi
            checker=(-DOBSERVE_ACCESSES)
            ;;
    esac
    "$cc" "${base[@]}" "${opts[@]}" "${observe[@]}" "${fragment[@]}" -DBUILD_COMMANDS \
        -c "$here/host.c" -o "$tmp/commands.o"
    "$cc" "${base[@]}" "${opts[@]}" "${checker[@]}" -pie "$here/host.c" "$tmp/commands.o" -o "$tmp/test"
    printf '%s ' "$mode"
    "$tmp/test"
    if [[ $# -ge 2 ]]; then "$tmp/test" "$2"; fi
done
