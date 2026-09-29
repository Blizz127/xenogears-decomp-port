#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34B23_I1_BUILD_DIR:-$ROOT/pc_port/build_native/w34b23_i1}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DWM_8C040_TEST_TRACE -DWM_93534_TEST_TRACE)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/src -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
SRC=(pc_port/tests/w34b23_i1_8c040_prod_test.c
     pc_port/src/psx_memory.c
     pc_port/src/world_map_helper_93534.c
     pc_port/src/world_map_helper_8c040.c)

# PsyCross lib
PSYLIB=$(find pc_port/build -name libpsycross.a 2>/dev/null | head -1)
if [ -z "$PSYLIB" ]; then
  PSYLIB=$(find pc_port/build_native -name libpsycross.a 2>/dev/null | head -1)
fi
LIBS=()
if [ -n "$PSYLIB" ]; then
  LIBS=("$PSYLIB" -lSDL2 -lopenal -lGL -lm -ldl -lpthread)
else
  LIBS=(-lSDL2 -lopenal -lGL -lm -ldl -lpthread)
fi

require_retail() {
  local f="disc/world_map.bin"
  local exp="4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70"
  local slice="39c1312b824b45d331207d8a0b13809e19e9c52f09a7c099669414bf55c6fd2e"
  [ -f "$f" ] || { echo "missing $f" >&2; exit 1; }
  [ "$(wc -c < "$f")" = "180422" ] || { echo "size mismatch" >&2; exit 1; }
  local a=$(sha256sum "$f" | awk '{print $1}')
  [ "$a" = "$exp" ] || { echo "SHA mismatch $a" >&2; exit 1; }
  local b=$(dd if="$f" bs=1 skip=$((0x8008C040-0x8006FAF0)) count=412 status=none | sha256sum | awk '{print $1}')
  [ "$b" = "$slice" ] || { echo "slice SHA mismatch $b" >&2; exit 1; }
}

build_and_run() {
  local name=$1; shift
  gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" "${SRC[@]}" "${LIBS[@]}" -o "$BUILD_DIR/$name"
  "$BUILD_DIR/$name" >"$BUILD_DIR/$name.raw" 2>"$BUILD_DIR/$name.stderr"
  rg -v 'PSX RAM emulation' "$BUILD_DIR/$name.raw" >"$BUILD_DIR/$name.stdout" || true
  cat "$BUILD_DIR/$name.stdout"
}

require_retail

echo "== focused production regimes =="
build_and_run focused_O0 -O0 -g
build_and_run focused_O2 -O2
build_and_run focused_ubsan -O2 -g -fsanitize=undefined -fno-sanitize-recover=all

for r in focused_O0 focused_O2 focused_ubsan; do
  grep -q "W34B23-I1 0x8008C040 focused oracle PASS" "$BUILD_DIR/$r.stdout"
  test ! -s "$BUILD_DIR/$r.stderr"
done
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_O2.stdout"
cmp "$BUILD_DIR/focused_O0.stdout" "$BUILD_DIR/focused_ubsan.stdout"
echo "FOCUSED O0/O2/UBSAN PASS"

mutants=(SKIP_WRAP WRONG_WRAP_ARG WRONG_DISTANCE_X WRONG_DISTANCE_Z UNSIGNED_DELTA WRONG_SQUARE_ROOT_INPUT SKIP_SQUARE_ROOT BOUNDARY_LT_VS_LE FIRST_MATCH_VS_LAST_MATCH WRONG_ITERATION_STRIDE WRONG_SENTINEL WRONG_RETURN OFF_BY_ONE_COUNT)
killed=0
for m in "${mutants[@]}"; do
  name="mutant_$m"
  gcc "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O0 -DWM_8C040_MUTANT_"$m" "${SRC[@]}" "${LIBS[@]}" -o "$BUILD_DIR/$name"
  set +e
  "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" 2>"$BUILD_DIR/$name.stderr"
  rc=$?
  set -e
  if [ $rc -eq 0 ] && ! grep -q "ASSERTION" "$BUILD_DIR/$name.stderr" && grep -q "PASS" "$BUILD_DIR/$name.stdout"; then
    echo "$m FAILED to kill (rc=$rc)" >&2
    cat "$BUILD_DIR/$name.stderr" >&2
    exit 1
  fi
  if [ $rc -eq 0 ]; then
    # If it passed but should have failed, check if oracle mismatch expected
    if grep -q "PASS" "$BUILD_DIR/$name.stdout"; then
      echo "$m FAILED mutant gate (should have failed) rc=$rc" >&2
      exit 1
    fi
  fi
  echo "$m KILLED rc=$rc"
  killed=$((killed+1))
done

echo "MUTANTS $killed/${#mutants[@]} KILLED"
echo "W34B23-I1 0x8008C040 focused certificate PASS"
