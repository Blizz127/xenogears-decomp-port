#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/pc_port/build_tests/w34n112_mode17_particles"
CC_BIN="${CC:-cc}"
COMMON=(
  -std=gnu17 -DXENO_PC_PORT -D_LANGUAGE_C
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion
  -I"$ROOT/pc_port/include_shim" -I"$ROOT/include"
  -I"$ROOT/pc_port/extern/PsyCross/include"
  -I"$ROOT/pc_port/extern/PsyCross/include/psx"
  -I"$ROOT/pc_port/src"
  "$ROOT/pc_port/tests/w34n112_mode17_particles_prod_test.c"
  "$ROOT/pc_port/src/world_map_callback_83214.c"
  "$ROOT/pc_port/src/world_map_scheduler.c"
)

mkdir -p "$OUT"

check_object_pointer_publication() {
  local source="$1"
  local block

  block="$(sed -n '/func_8002CB54(model_host,/,/Reloc table lookup:/p' \
    "$source")"
  if ! printf '%s\n' "$block" | \
      rg -q -F '*(u32*)(e + 72) = host_ptr_to_psx_u32(out1);' || \
     ! printf '%s\n' "$block" | \
      rg -q -F '*(u32*)(e + 76) = host_ptr_to_psx_u32(out2);'; then
    echo "ASSERTION producer.guest_buffer_publication FAILED" >&2
    return 1
  fi
}

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

check_slice $((0x80082F64)) $((0x80083108)) \
  f385fea48beb367407abdf3ef2ecd981a50f142299a269365aa6d36e5717de07
check_slice $((0x80083108)) $((0x800831D8)) \
  4db38536cc83aad77b9390a751cf6f4502e75abd22d717788844482ce00e2beb
check_slice $((0x800831D8)) $((0x80083214)) \
  3d05362719183b5ac038455cdd3a3886ed3ea5776ce763145f580c9c6cd7ab6c
check_slice $((0x80083214)) $((0x80083264)) \
  a620e27bb61dede1c6b36de9bc3859bc5bf09908682677f27f2cc67d976b42e5
check_slice $((0x80083264)) $((0x800834D0)) \
  c79fa95b7b4e764e000663dc6b7e099a076da07e228b716bc5eb473ec7ce35be
check_slice $((0x80082F64)) $((0x800834D0)) \
  ab14db3e4a0a4e416db0aefd5223fb41b020258db87b21317a1c954bcbdc2eaa

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
    sed -n '1,320p' "$OUT/$name.log" >&2
    exit 1
  fi
  echo "$name DETECTED ($assertion)"
}

run_regime o0 -O0
run_regime o2 -O2
run_regime ubsan -O2 -fsanitize=undefined -fno-sanitize-recover=undefined

run_mutant m1 -DW34N112_MUTANT_WRONG_OBJECT_STRIDE init.object_address
run_mutant m2 -DW34N112_MUTANT_WRONG_PRIMITIVE_CODE init.primitive_layout
run_mutant m3 -DW34N112_MUTANT_DROP_LAST_PRIMITIVE init.primitive_count
run_mutant m4 -DW34N112_MUTANT_SKIP_BUFFER_MIRROR init.buffer_mirror
run_mutant m5 -DW34N112_MUTANT_STATE2_USES_STATE1_TABLE state.table_selection
run_mutant m6 -DW34N112_MUTANT_SKIP_ACTIVE_FLAG state.active
run_mutant m7 -DW34N112_MUTANT_SHORT_PARAMETER_COPY state.parameter_copy
run_mutant m8 -DW34N112_MUTANT_WRONG_RED_LOWER_CLAMP oscillator.clamp
run_mutant m9 -DW34N112_MUTANT_WRONG_ANGLE_MASK motion.angle
run_mutant m10 -DW34N112_MUTANT_SKIP_MATRIX_COPY transform.matrix_copy
run_mutant m11 -DW34N112_MUTANT_WRONG_BUFFER_SIDE color.buffer_side

check_object_pointer_publication "$ROOT/pc_port/src/world_map_init.c"
cp "$ROOT/pc_port/src/world_map_init.c" "$OUT/m12_world_map_init.c"
sed -i \
  '/host_ptr_to_psx_u32(out1);/d; /host_ptr_to_psx_u32(out2);/d' \
  "$OUT/m12_world_map_init.c"
if check_object_pointer_publication "$OUT/m12_world_map_init.c" \
    >"$OUT/m12.log" 2>&1; then
  echo "ERROR: m12 survived" >&2
  exit 1
fi
if ! rg -q 'ASSERTION producer.guest_buffer_publication FAILED' \
    "$OUT/m12.log"; then
  echo "ERROR: m12 did not fail named assertion producer.guest_buffer_publication" >&2
  exit 1
fi
echo "m12 DETECTED (producer.guest_buffer_publication)"

echo "W34N112 MODE17 PARTICLE CERTIFICATE PASS: O0/O2/UBSan; M1-M12 detected; strict warnings clean"
