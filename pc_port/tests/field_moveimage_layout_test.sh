#!/usr/bin/env bash
# The field MoveImage ring uses four globals whose retail BSS addresses are
# adjacent at exact byte offsets.  Keeping those offsets on the host matters:
# D_800AF5E8 is an array of 32 eight-byte RECT records, followed by two arrays
# of 32 s16 values and the ring index at +0x180.  Independent host stubs make
# the RECT writes overlap the following arrays and corrupt the source Y.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"

test -x "$BINARY"

symbol_address() {
    local symbol="$1"
    nm -n "$BINARY" | awk -v wanted="$symbol" \
        '$3 == wanted { print "0x" $1; found=1 } END { if (!found) exit 1 }'
}

rect=$(($(symbol_address D_800AF5E8)))
source_x=$(($(symbol_address D_800AF6E8)))
source_y=$(($(symbol_address D_800AF728)))
ring_index=$(($(symbol_address D_800AF768)))

test "$((source_x - rect))" -eq 256
test "$((source_y - source_x))" -eq 64
test "$((ring_index - source_y))" -eq 64

echo 'field MoveImage BSS layout: PASS'
