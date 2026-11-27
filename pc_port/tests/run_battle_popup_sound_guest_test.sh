#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../.." && pwd)
media=${XENO_GUEST_TEST_DISC:-"$root/disc"}
(
    cd "$media"
    sha256sum -c <<'SUMS'
dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119  SLUS_006.64
1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291  battle.bin
SUMS
)
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
"${CC:-cc}" -std=gnu11 -O1 -Wall -Wextra -Werror -I"$root/pc_port/src" \
    "$root/pc_port/tests/battle_popup_sound_guest_test.c" \
    "$root/pc_port/src/battle_mips_adapter.c" -o "$out/check"
env -u XENO_DISABLE_CPU_INTERPRETER "$out/check" "$media/SLUS_006.64" "$media/battle.bin"
