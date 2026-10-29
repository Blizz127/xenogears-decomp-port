#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
source pc_port/tests/lib/sprite_d2_lib.sh
BUILD_DIR="${D2_BUILD_DIR:-$ROOT/pc_port/build_native/sprite_bind_23538_full}"
mkdir -p "$BUILD_DIR"
TEST_SRC=pc_port/tests/sprite_bind_23538_full_retail_test.c
UUT_SRC=src/slus_006.64/system/temp1.c
WEAKEN=(--weaken-symbol=SpriteSetScale --weaken-symbol=SpriteComputeTransformMatrix
        --weaken-symbol=func_800234AC)
TEST_ARGS=()
d2_check_disc
d2_regimes
echo "SPRITE BIND 23538 FULL O0/O2/UBSAN PASS"
d2_mutant scale_shift11 '^void func_80023538(' 's/signedValue << 10/signedValue << 11/'
echo "SPRITE BIND 23538 FULL mutant rejected: scale_shift11"
