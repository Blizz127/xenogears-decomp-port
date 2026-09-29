#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

fail() {
    echo "BATTLE overlay bootstrap FAIL: $*" >&2
    exit 1
}

test -f config/battle.yaml || fail "missing config/battle.yaml"
test -f config/symbol_addrs.battle.txt || fail "missing battle symbol file"
test -f src/battle/main.c || fail "missing generated battle source"

rg -q '^sha1: 124703d415f570f9cd8d7dad89c6a4eddfd7e6f0$' \
    config/battle.yaml || fail "config SHA-1 is not the extracted retail overlay"
rg -q 'basename: battle\.bin' config/battle.yaml || fail "wrong overlay basename"
rg -q 'target_path: disc/battle\.bin' config/battle.yaml || fail "wrong overlay target"
rg -q 'vram: 0x8006faf0' config/battle.yaml || fail "wrong overlay load address"
# The pinned splat (0.33.2) has no generate_asm_macros_files option (the key
# makes it reject the config), so it never writes include/include_asm.h; the
# port-aware header is safe as long as nobody turns the option on.
if rg -q 'generate_asm_macros_files:\s*True' config/battle.yaml; then
    fail "battle split may overwrite the port-aware include_asm header"
fi
rg -q '0x133C, (asm|c)' config/battle.yaml || \
    fail "first retail code split boundary is missing"
rg -q '0x1450, c, main' config/battle.yaml || \
    fail "state-2 entry split boundary is missing"
rg -q 'vram: 0x800c3a6c, type: bss' config/battle.yaml || \
    fail "state-2 BSS boundary is missing"
rg -q '^func_80070F40 = 0x80070F40; // type:func$' \
    config/symbol_addrs.battle.txt || fail "retail battle entry symbol is missing"
for helper in func_80070E2C func_80070EB0 func_80070EDC; do
    rg -q "^${helper} = 0x${helper#func_}; // type:func$" \
        config/symbol_addrs.battle.txt || \
        fail "pre-entry retail helper symbol is missing: $helper"
    rg -q "glabel ${helper}$" asm/battle/133C.s || \
        fail "generated assembly omits pre-entry retail helper: $helper"
done
rg -q 'INCLUDE_ASM\("asm/battle/nonmatchings/main", func_80070F40\);' \
    src/battle/main.c || fail "generated source does not retain the retail entry body"

# A split and a C symbol are evidence infrastructure, not a native battle
# implementation.  Keep state 2 fail-closed until the entry dependency closure
# has real C owners and the port advertises that explicit readiness contract.
if rg -n 'g_MainGameStates\[2\]\.pFnMain\s*=\s*PcPort_Battle' \
    pc_port/src/game_overrides.c; then
    rg -q 'PC_PORT_BATTLE_ENTRY_READY' pc_port/src/game_overrides.c || \
        fail "state 2 is rebound without the explicit real-entry readiness gate"
fi

echo "BATTLE overlay bootstrap PASS (retail metadata + fail-closed bind gate)"
