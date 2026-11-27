#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib/battle_guest_numeric.sh"
slus=${XENO_SLUS_EXE:-disc/SLUS_006.64}
check_image "$slus" dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119
run_numeric_test pc_port/tests/battle_728b8_strip_test.c "$overlay" "$slus"
