#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"
ubsan_preflight

cd "$(dirname "$0")/../.."
printf '%s  %s\n' 4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70 \
    disc/world_map.bin | sha256sum --check -
if [[ -n ${W34CB1_90A84_BUILD_DIR:-} ]]; then
    out=$W34CB1_90A84_BUILD_DIR
    mkdir -p "$out"
else
    out=$(mktemp -d "${TMPDIR:-/tmp}/w34cb1.XXXXXX")
    trap 'rm -rf "$out"' EXIT
fi

run_case() {
    local name=$1 status=0
    shift
    gcc -std=gnu17 -Wall -Wextra -Wconversion -Wsign-conversion -Werror \
        -Ipc_port/src "$@" pc_port/tests/w34cb1_90a84_prod_test.c \
        pc_port/src/battle_mips_adapter.c -o "$out/$name"
    "$out/$name"
    "$out/$name" --drop-store > "$out/$name.control.log" 2>&1 || status=$?
    if [[ $status != 1 ]] ||
       ! grep -Fxq 'W34CB1 FAIL: delay-slot store absent before second call' "$out/$name.control.log"; then
        echo "W34CB1 FAIL: $name lost-store control did not trigger its assertion" >&2
        return 1
    fi
    echo "W34CB1 $name detected the deliberately dropped store"
}

run_case O0 -O0 -g
run_case O2 -O2
run_case UBSan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all
