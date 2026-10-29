#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
source pc_port/tests/lib/sprite_d2_lib.sh
BUILD_DIR="${D2_BUILD_DIR:-$ROOT/pc_port/build_native/sprite_override_24730}"
mkdir -p "$BUILD_DIR"
# Same retail-differential test as temp1_anim_state_dispatch_24730, but the unit
# under test is the LINKED body: func_80024730 in pc_port/src/game_overrides.c
# (pc_port/port_owned_overrides.txt), not the temp1.c port arm that the link
# weakens.
TEST_SRC=pc_port/tests/temp1_anim_state_dispatch_24730_retail_test.c
UUT_SRC=pc_port/src/game_overrides.c
WEAKEN=(--weaken-symbol=func_80022DF4 --weaken-symbol=D_800592EC
        --weaken-symbol=D_8006F99C --weaken-symbol=D_8006F9AC
        --weaken-symbol=func_800BC158 --weaken-symbol=func_80025224)
TEST_ARGS=()
d2_check_disc
d2_regimes
echo "SPRITE OVERRIDE 24730 O0/O2/UBSAN PASS"
d2_mutant tail_mode0 '^void func_80024730(' 's/func_80025224(w + 0x1C, idx)/func_80025224(w + 0x1C, 0)/'
echo "SPRITE OVERRIDE 24730 mutant rejected: tail_mode0"
