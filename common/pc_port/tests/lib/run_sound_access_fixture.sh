#!/usr/bin/env bash
set -euo pipefail
fixture_file=${1:?expected the standalone sound fixture C file}
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
    "$cc" "${base[@]}" "${opts[@]}" "${instrument[@]}" -DBUILD_COMMANDS -c "$fixture_file" -o "$out/commands.o"
    "$cc" "${base[@]}" "${opts[@]}" "${fixture[@]}" "$fixture_file" "$out/commands.o" -o "$out/test"
    printf '%s ' "$mode"
    "$out/test"
done
