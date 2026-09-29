#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34N21_BUILD_DIR:-$ROOT/pc_port/build_native/w34n21_cd_status}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -ffunction-sections -fdata-sections)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
# The retained driver lane is warning-clean.  These are the two focused
# suppressions required to compile the rest of the legacy driver translation.
DRIVER_WARN=(-Wno-sign-conversion -Wno-unused-function)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
HELPER=pc_port/src/world_map_helper_96130.c
DRIVER=pc_port/src/world_map_frame_driver_712d0.c
TEST=pc_port/tests/w34n21_cd_status_prod_test.c

compile_and_run() {
    local name="$1"
    shift
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" \
        -DW34N21_TEST_HOOKS "$@" -c "$HELPER" \
        -o "$BUILD_DIR/$name.helper.o" || return
    "$CC" "${BASE[@]}" "${WARN[@]}" "${DRIVER_WARN[@]}" "${INC[@]}" \
        "$@" -c "$DRIVER" -o "$BUILD_DIR/$name.driver.o" || return
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" \
        "$@" -c "$TEST" -o "$BUILD_DIR/$name.test.o" || return
    "$CC" -no-pie -Wl,--gc-sections "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.helper.o" \
        "$BUILD_DIR/$name.driver.o" -o "$BUILD_DIR/$name" || return
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
}

assert_pass() {
    local name="$1"
    rg -q '^W34N21 CD STATUS CERTIFICATE PASS$' \
        "$BUILD_DIR/$name.stdout"
    ! rg -q '^ASSERTION ' "$BUILD_DIR/$name.stderr"
}

verify_provenance() {
    local name="$1"
    local symbol

    for symbol in wm_800968E0 wm_800967E4 wm_712d0_run_cd_sync_lane; do
        if ! nm -u "$BUILD_DIR/$name.test.o" | rg -q " U $symbol$"; then
            echo "test object does not leave $symbol undefined" >&2
            exit 1
        fi
        if [[ "$(nm --defined-only "$BUILD_DIR/$name" | \
                awk -v wanted="$symbol" '$3 == wanted { count++ } END { print count + 0 }')" != "1" ]]; then
            echo "linked binary does not contain exactly one $symbol" >&2
            exit 1
        fi
    done
    nm --defined-only "$BUILD_DIR/$name.helper.o" | \
        rg -q ' T wm_800968E0$'
    nm --defined-only "$BUILD_DIR/$name.helper.o" | \
        rg -q ' T wm_800967E4$'
    nm --defined-only "$BUILD_DIR/$name.driver.o" | \
        rg -q ' T wm_712d0_run_cd_sync_lane$'
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
verify_provenance O0

# Compile the helper once exactly as production does, without observation
# hooks.  The volatile guest accessors, not the hooks, must carry the reload
# and store-order semantics at O2.
"$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" -O2 -c "$HELPER" \
    -o "$BUILD_DIR/PROD_O2.helper.o"
if nm -u "$BUILD_DIR/PROD_O2.helper.o" | rg -q 'w34n21_test_'; then
    echo 'production O2 helper retained a certificate hook' >&2
    exit 1
fi
objdump -dr --disassemble=wm_800968E0 "$BUILD_DIR/PROD_O2.helper.o" \
    >"$BUILD_DIR/PROD_O2.wm_800968E0.objdump"
rg -q 'volatile const u8 \*p' "$HELPER"
rg -q 'volatile u8 \*p' "$HELPER"

for item in \
  'M1:W34N21_MUTANT_DISPATCH_IDLE_BUSY:dispatch.state0.exact_status' \
  'M2:W34N21_MUTANT_DISPATCH_SKIP_COUNTDOWN:dispatch.state4.countdown' \
  'M3:W34N21_MUTANT_DISPATCH_WRONG_TAIL:dispatch.state5.exact_tail_wrap' \
  'M4:W34N21_MUTANT_DROP_DISPATCH_STATUS:status.propagates.one' \
  'M5:W34N21_MUTANT_C624_STALE_TAIL:route.c624.reloads_callback_tail' \
  'M6:W34N21_MUTANT_DRIVER_SKIP_RETRY_VSYNC:driver.status3.retry_vsync_twice' \
  'M7:W34N21_MUTANT_DRIVER_NULL_CDSYNC_RESULT:driver.cdsync.exact_mode_and_result_pointer' \
  'M8:W34N21_MUTANT_DISPATCH_CACHED_STATE:dispatch.state4.reloads_live_state' \
  'M9:W34N21_MUTANT_STATE5_REORDER:dispatch.state5.exact_store_order'; do
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

echo 'PRODUCTION_O2_NO_TEST_HOOKS PASS'
echo 'W34N21 CERTIFICATE PASS O0/O2/UBSan; M1-M9 DETECTED; helper/test strict warnings clean; driver focused warnings clean'
