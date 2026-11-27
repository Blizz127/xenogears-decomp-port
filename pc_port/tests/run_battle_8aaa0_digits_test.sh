#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/lib/battle_guest_numeric.sh"
run_numeric_test pc_port/tests/battle_8aaa0_digits_test.c "$overlay"
