#!/usr/bin/env bash
# Both numeric fixtures execute the PSX instructions from the named owned images.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/../../.."

check_image() {
    local file=$1 expected=$2 digest
    digest=$(sha256sum < "$file")
    if [[ ${digest%% *} != "$expected" ]]; then
        printf 'Unexpected SHA-256 for %s\n' "$file" >&2
        return 1
    fi
}

run_numeric_test() {
    local source=$1
    shift
    local build
    build=$(mktemp -d "${TMPDIR:-/tmp}/xeno-guest-numeric.XXXXXXXX")
    (
        ulimit -c 0
        trap 'rm -rf "$build"' EXIT
        "${CC:-cc}" -std=c11 -D_POSIX_C_SOURCE=200809L -O2 -Wall -Wextra -Werror -Ipc_port/src \
            "$source" pc_port/src/battle_mips_adapter.c -o "$build/test"
        env -u XENO_DISABLE_CPU_INTERPRETER "$build/test" "$@"
    )
}

overlay=${XENO_BATTLE_OVERLAY:-disc/battle.bin}
check_image "$overlay" 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
