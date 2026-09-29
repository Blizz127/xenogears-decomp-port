#!/usr/bin/env bash
# Computed VectorNormal reciprocal-sqrt table == retail table (read from disc).
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${XENO_TEST_TMP:-$HOME/.cache/xeno/port}/gte_normalize_table"
mkdir -p "$(dirname "$out")"
gcc -std=c99 -O2 -Wall -Wextra -Werror pc_port/tests/gte_normalize_table_test.c -o "$out"
"$out" disc/SLUS_006.64
