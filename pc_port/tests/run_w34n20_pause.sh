#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34N20_BUILD_DIR:-$ROOT/pc_port/build_native/w34n20_pause}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -ffunction-sections -fdata-sections)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
DRIVER_WARN=(-Wno-sign-conversion -Wno-unused-function)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
PAUSE=pc_port/src/world_map_pause.c
DRIVER=pc_port/src/world_map_frame_driver_712d0.c
TEST=pc_port/tests/w34n20_pause_prod_test.c

compile_and_run() {
    local name="$1"
    shift
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" \
        "$@" -c "$PAUSE" -o "$BUILD_DIR/$name.pause.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${DRIVER_WARN[@]}" "${INC[@]}" \
        "$@" -c "$DRIVER" -o "$BUILD_DIR/$name.driver.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" \
        "$@" -c "$TEST" -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie -Wl,--gc-sections "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.pause.o" \
        "$BUILD_DIR/$name.driver.o" -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
}

assert_pass() {
    local name="$1"
    rg -q '^W34N20 PAUSE CERTIFICATE PASS$' "$BUILD_DIR/$name.stdout"
    ! rg -q '^ASSERTION ' "$BUILD_DIR/$name.stderr"
}

compile_and_run O0 -O0 -g
compile_and_run O2 -O2
compile_and_run UBSan -O2 -g -fsanitize=undefined \
    -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    assert_pass "$regime"
done
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"

for item in \
  'M1:W34N20_MUTANT_GRAPHICS_NATIVE_TWIN:graphics.guest_static_source' \
  'M2:W34N20_MUTANT_GRAPHICS_WRONG_UPLOAD_ARGS:graphics.retail_upload_args' \
  'M3:W34N20_MUTANT_GRAPHICS_SKIP_FREE:graphics.call_order_and_free' \
  'M4:W34N20_MUTANT_MUTE_SKIP_FLAG:mute.control_flag' \
  'M5:W34N20_MUTANT_MUTE_23_VOICES:mute.all_24_voices' \
  'M6:W34N20_MUTANT_MUTE_ZERO_ADSR1:mute.adsr1_preserves_low_byte' \
  'M7:W34N20_MUTANT_WAIT_SKIP_CLEAR:wait.start_clear_each_round' \
  'M8:W34N20_MUTANT_WAIT_DROP_RELEASED_SOURCE:wait.release_source_accumulates' \
  'M9:W34N20_MUTANT_WAIT_WRONG_ENTRY_COPY:wait.entry_copy_to_zero' \
  'M10:W34N20_MUTANT_WAIT_WRONG_START_MASK:wait.start_mask_and_round_count' \
  'M11:W34N20_MUTANT_WAIT_SKIP_ENABLE:wait.sound_reenabled' \
  'M12:W34N20_MUTANT_WAIT_WRONG_ENV_STRIDE:wait.final_env_reloads_and_stride' \
  'M13:W34N20_MUTANT_DRIVER_WRONG_D804_GUARD:driver.start_guard_calls_modal' \
  'M14:W34N20_MUTANT_DRIVER_SKIP_DISCONNECT_WAIT:driver.disconnect_guard_calls_modal'; do
    IFS=: read -r label define assertion <<<"$item"
    set +e
    compile_and_run "$label" -O0 -g -D"$define"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || \
       ! rg -q "^ASSERTION $assertion([[:space:]]|$)" \
           "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED rc=$rc expected=$assertion" >&2
        tail -80 "$BUILD_DIR/$label.stderr" >&2
        exit 1
    fi
    echo "$label DETECTED assertion=$assertion"
done

rg -Fq 'PSX_ADDR(UINT32_C(0x8004FBD8))' "$PAUSE"
rg -q 'voice < 24u' "$PAUSE"
rg -q 'wm_712d0_run_pause_lanes' "$DRIVER"
rg -q 'wm_8007634C' "$DRIVER"
rg -q 'wm_80076594' "$DRIVER"

echo 'W34N20 CERTIFICATE PASS O0/O2/UBSan; M1-M14 DETECTED; pause/test strict warnings clean; driver focused warnings clean'
