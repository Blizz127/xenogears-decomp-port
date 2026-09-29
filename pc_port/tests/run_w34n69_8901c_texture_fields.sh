#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"

OUT=pc_port/build_tests/w34n69_8901c_texture_fields
CC="${CC:-cc}"
mkdir -p "$OUT"

TEST=pc_port/tests/w34b5c_8008901c_prod_test.c
PROD=pc_port/src/world_map_common_tail.c
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
  -Ipc_port/include -Iinclude -Ipc_port/src
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
)

build_and_run() {
  local name=$1
  shift
  "$CC" "${COMMON[@]}" "$@" "$TEST" "$PROD" -o "$OUT/$name"
  "$OUT/$name" >"$OUT/$name.stdout" 2>"$OUT/$name.stderr"
  grep -q 'record0 packet clut is GetClut' "$OUT/$name.stdout"
  grep -q 'record0 packet tpage is GetTPage' "$OUT/$name.stdout"
  grep -q 'Results: 56/56 PASS' "$OUT/$name.stdout"
  printf '%s PASS\n' "$name"
}

build_and_run O0 -O0 -g
build_and_run O2 -O2
build_and_run UBSan -O1 -g -fsanitize=undefined -fno-sanitize-recover=all

"$CC" "${COMMON[@]}" -O0 -g -DWM_8901C_MUTANT_SWAPPED_TEXTURE_FIELDS \
  "$TEST" "$PROD" -o "$OUT/M1_swapped_texture_fields"
set +e
"$OUT/M1_swapped_texture_fields" \
  >"$OUT/M1_swapped_texture_fields.stdout" \
  2>"$OUT/M1_swapped_texture_fields.stderr"
mutant_rc=$?
set -e
if [[ $mutant_rc -eq 0 ]]; then
  echo 'M1 failure: swapped texture fields survived' >&2
  exit 1
fi
grep -q 'FAIL: record0 packet clut is GetClut' \
  "$OUT/M1_swapped_texture_fields.stdout"
grep -q 'FAIL: record0 packet tpage is GetTPage' \
  "$OUT/M1_swapped_texture_fields.stdout"
echo 'M1 swapped texture fields DETECTED'

echo 'W34N69 0x8008901C texture-field certificate PASS; O0/O2/UBSan; strict warnings clean; M1 DETECTED'
