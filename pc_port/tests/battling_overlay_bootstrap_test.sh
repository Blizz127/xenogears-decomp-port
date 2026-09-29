#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

fail() {
    echo "BATTLING overlay bootstrap FAIL: $*" >&2
    exit 1
}

test -f config/battling.yaml || fail "missing config/battling.yaml"
test -f config/symbol_addrs.battling.txt || fail "missing battling symbol file"
test -f src/battling/main.c || fail "missing generated battling source"

rg -q '^sha1: 9cfebaf201a0dfd352c11007a63ad7a955533b17$' \
    config/battling.yaml || fail "config SHA-1 is not the extracted retail overlay"
rg -q 'basename: battling\.bin' config/battling.yaml || fail "wrong overlay basename"
rg -q 'target_path: disc/battling\.bin' config/battling.yaml || fail "wrong overlay target"
rg -q 'vram: 0x8006faf0' config/battling.yaml || fail "wrong overlay load address"
rg -q '0x193A0, c, main' config/battling.yaml || \
    fail "state-4 entry split boundary is missing"
rg -q '^BattlingMain = 0x80088E90; // type:func$' \
    config/symbol_addrs.battling.txt || fail "retail state-4 entry symbol is missing"
rg -q 'INCLUDE_ASM\("asm/battling/nonmatchings/main", BattlingMain\);' \
    src/battling/main.c || fail "generated source does not retain the retail entry body"

# State 4 remains fail-closed in the PC port.  A symbol declaration and a
# generated INCLUDE_ASM body are not a host implementation; binding it here
# would route the runtime to an unavailable overlay.  Permit a future bind
# only when the source has a real entry definition and the port explicitly
# supplies the integration marker.
if rg -n 'g_MainGameStates\[4\]\.pFnMain\s*=' pc_port/src/game_overrides.c; then
    if ! rg -q 'PC_PORT_BATTLING_ENTRY_READY' pc_port/src/game_overrides.c; then
        fail "state 4 is bound without the explicit real-entry readiness gate"
    fi
fi

echo "BATTLING overlay bootstrap PASS (retail metadata + fail-closed bind gate)"
