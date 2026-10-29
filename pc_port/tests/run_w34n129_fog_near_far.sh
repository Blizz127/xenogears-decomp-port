#!/usr/bin/env bash
# W34N129: wm_80072BB0's SetFogNearFar arguments versus the retail fragment.
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d /tmp/w34n129.XXXXXX)
trap 'rm -rf "$OUT"' EXIT
build() {
    local name=$1; shift
    gcc -std=gnu17 -O2 "$@" -include assert.h -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -Wall -Wextra -Werror -Wno-unused-parameter \
        -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include \
        -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src \
        pc_port/tests/w34n129_fog_near_far_test.c pc_port/src/world_map_framebuffer_init.c \
        pc_port/src/battle_mips_adapter.c -o "$OUT/$name"
}
build pass
"$OUT/pass" 2>"$OUT/pass.err" | tee "$OUT/pass.log"
build mutant -DW34N129_MUTANT_SWAPPED
if "$OUT/mutant" > "$OUT/mutant.log" 2>&1; then
    echo "W34N129 FAIL: the swapped-branch mutant survived" >&2; exit 1
fi
grep -q "W34N129 FAIL d7cc" "$OUT/mutant.log"
echo "W34N129 negative control PASS: swapped near branch is rejected"
echo "W34N129 FOG NEAR/FAR PASS"
