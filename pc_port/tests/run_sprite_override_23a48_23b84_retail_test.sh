#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
source pc_port/tests/lib/sprite_d2_lib.sh
# Unit under test: the LINKED bodies in pc_port/src/game_overrides.c
# (func_80023A48 and func_80023B84 are strong port-owned overrides; the temp1.c
# port arms are weakened by the link).
TEST_SRC=pc_port/tests/sprite_child_chain_retail_test.c
UUT_SRC=pc_port/src/game_overrides.c
TEST_ARGS=()
d2_check_disc

STUBBED=(func_80023958 func_800239F4 func_800233A4 func_80023440 func_80023468
         func_80023A48 func_80024730 func_8001EE74 func_80023950)

# suite_setup SUITE UUT_FN: stub (weaken) every callee, keep UUT_FN strong.
suite_setup() {
    local suite="$1" uut="$2" f
    BUILD_DIR="${D2_BUILD_DIR:-$ROOT/pc_port/build_native/sprite_override_$suite}"
    mkdir -p "$BUILD_DIR"
    D2_EXTRA=(-DSUITE_$suite)
    WEAKEN=()
    for f in "${STUBBED[@]}"; do
        if [[ "$f" != "$uut" ]]; then WEAKEN+=(--weaken-symbol=$f); fi
    done
}

suite_setup 23A48 func_80023A48
d2_regimes
echo "SPRITE OVERRIDE 23A48 O0/O2/UBSAN PASS"
d2_mutant a48_mode2_size '^void\* func_80023A48(' 's/extra = 0x54;/extra = 0x50;/'
d2_mutant a48_type6_pkg   '^void\* func_80023A48(' 's/pkg = D_8005A474;/pkg = D_8006BE10;/'
echo "SPRITE OVERRIDE 23A48 mutants rejected"

suite_setup 23B84 func_80023B84
d2_regimes
echo "SPRITE OVERRIDE 23B84 O0/O2/UBSAN PASS"
d2_mutant b84_copy_ac '^void\* func_80023B84(' 's/(\*(u32\*)(pSrc + 0xAC) \& 0x7FF80)/(*(u32*)(pSrc + 0xAC) \& 0x7FF00)/'
d2_mutant b84_parent_link '^void\* func_80023B84(' 's/\*(u32\*)(pDst + 0x70) = (u32)(uintptr_t)pSrc;/*(u32*)(pDst + 0x70) = 0;/'
echo "SPRITE OVERRIDE 23B84 mutants rejected"
