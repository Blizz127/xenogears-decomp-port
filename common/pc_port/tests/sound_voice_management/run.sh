#!/usr/bin/env bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
cc=${CC:-cc}
common=(-std=gnu17 -Wall -Wextra -Werror)
part=()
if [[ $# -ge 1 ]]; then part=("-DVOICE_FRAGMENT=\"$(realpath -- "$1")\""); fi
for mode in O0 O2 UBSan access; do
    opt=(-O2)
    watch=()
    checker=()
    case "$mode" in
        O0) opt=(-O0) ;;
        UBSan) opt+=(-fsanitize=undefined -fno-sanitize-recover=all) ;;
        access)
            watch=(-fsanitize=thread)
            if [[ $("$cc" --version) == *clang* ]]; then
                watch+=(-mllvm -tsan-distinguish-volatile=1)
            else
                watch+=(--param tsan-distinguish-volatile=1)
            fi
            checker=(-DOBSERVE_ACCESSES)
            ;;
    esac
    "$cc" "${common[@]}" "${opt[@]}" "${watch[@]}" "${part[@]}" -fPIC -fno-inline \
        -fsemantic-interposition -DBUILD_COMMANDS -c "$here/host.c" -o "$tmp/leaf.o"
    # Preserve the same implementation text while allowing the group checker
    # to observe/transparently mutate the per-channel provider's inputs.
    objcopy --weaken-symbol=SoundAbortVoiceOnChannel "$tmp/leaf.o" "$tmp/group.o"
    printf '%s ' "$mode"
    "$cc" "${common[@]}" "${opt[@]}" "${checker[@]}" "$here/host.c" "$tmp/leaf.o" -o "$tmp/test-leaves"
    "$tmp/test-leaves"
    "$cc" "${common[@]}" "${opt[@]}" "${checker[@]}" -DCLEAR_GROUPS "$here/host.c" "$tmp/group.o" -o "$tmp/test-groups"
    "$tmp/test-groups"
done
