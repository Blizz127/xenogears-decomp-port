#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n111_mode17_command"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n111_mode17_command_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_827c8.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

check_slice() {
  local lo="$1"
  local hi="$2"
  local expected="$3"
  local actual
  actual="$(dd if="$ROOT/disc/world_map.bin" bs=1 \
    skip=$((lo - 0x8006FAF0)) count=$((hi - lo)) status=none | \
    sha256sum | awk '{print $1}')"
  if [[ "$actual" != "$expected" ]]; then
    printf 'ERROR: retail slice 0x%08X..0x%08X SHA mismatch: %s\n' \
      "$lo" "$hi" "$actual" >&2
    exit 1
  fi
}

check_slice $((0x800827C8)) $((0x800827EC)) \
  97dc80eafdca7f8ab4ff87f912647188fee1e76de91b8c775e2e44117223dbbf
check_slice $((0x80076B34)) $((0x80076BC4)) \
  69ec010d238d8012c84864a98d0691d538fdb131eeae068ec9ab931e04a8a3df
check_slice $((0x80076BC4)) $((0x80076DA4)) \
  f0470ef0465fbe7a9c1ee0b608a83b7159ac8fd7155c0b75a242d5836195659d
check_slice $((0x8009A3C0)) $((0x8009A3F0)) \
  c131683500b8887634be850125d2c76fe3ae98e6ac8908c4fdb90d25d7912c3e

run_regime() {
  local name="$1"
  shift
  "$CC_BIN" "${COMMON[@]}" "$@" -o "$OUT/$name"
  "$OUT/$name"
}

run_mutant() {
  local name="$1"
  local define="$2"
  local assertion="$3"
  "$CC_BIN" "${COMMON[@]}" -O2 "$define" -o "$OUT/$name"
  if "$OUT/$name" >"$OUT/$name.log" 2>&1; then
    echo "ERROR: $name survived" >&2
    exit 1
  fi
  if ! grep -q "ASSERTION $assertion FAILED" "$OUT/$name.log"; then
    echo "ERROR: $name did not fail named assertion $assertion" >&2
    sed -n '1,300p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N111_MUTANT_WRONG_SCRIPT_POINTER init.script_pointer
run_mutant m2 -DW34N111_MUTANT_WRONG_ADVANCE_SCALE update.advance_scale
run_mutant m3 -DW34N111_MUTANT_TIMER_EXPIRES_LATE timer.expiry
run_mutant m4 -DW34N111_MUTANT_WRONG_POSITION_Z command3.position
run_mutant m5 -DW34N111_MUTANT_SWAP_ANGLE_YZ command4.angles
run_mutant m6 -DW34N111_MUTANT_WRONG_ANGLE_SOURCE command5.angle_source
run_mutant m7 -DW34N111_MUTANT_SOUND_BANK_ZERO command9.sound_bank
run_mutant m8 -DW34N111_MUTANT_SWAP_SOUND_CONTROL_ARGS command10.sound_control
run_mutant m9 -DW34N111_MUTANT_SWAP_STATE_GLOBALS command11.state
run_mutant m10 -DW34N111_MUTANT_SKIP_EXIT command0.exit

echo "W34N111 MODE17 COMMAND CERTIFICATE PASS: O0/O2/UBSan; M1-M10 detected; strict warnings clean"
