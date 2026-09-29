#!/usr/bin/env bash
# The retail globals D_8005A418 and D_8005A41C are the second and third
# entries of the three-pointer party skin buffer table rooted at 0x8005A414.
# On the 64-bit host each pointer occupies eight bytes, so the symbolic host
# aliases must use host pointer strides while preserving the retail names.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"

test -x "$BINARY"

symbol_address() {
    local symbol="$1"
    nm -n "$BINARY" | awk -v wanted="$symbol" '$3 == wanted { print "0x" $1; found=1 } END { if (!found) exit 1 }'
}

base=$(( $(symbol_address g_PartyDataBuffers) ))
slot1=$(( $(symbol_address D_8005A418) ))
slot2=$(( $(symbol_address D_8005A41C) ))

test "$slot1" -eq $((base + 8))
test "$slot2" -eq $((base + 16))

echo 'field party buffer aliases: PASS'
