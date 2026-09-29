#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34N19_BUILD_DIR:-$ROOT/pc_port/build_native/w34n19_frame_authority}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -ffunction-sections -fdata-sections)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
PROD_WARN=(-Wno-sign-conversion -Wno-unused-function)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
DRIVER=pc_port/src/world_map_frame_driver_712d0.c
TAIL=pc_port/src/world_map_common_tail.c
TEST=pc_port/tests/w34n19_frame_authority_prod_test.c

compile_and_run() {
    local name="$1"
    shift
    "$CC" "${BASE[@]}" "${WARN[@]}" "${PROD_WARN[@]}" "${INC[@]}" \
        "$@" -c "$DRIVER" -o "$BUILD_DIR/$name.driver.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${PROD_WARN[@]}" "${INC[@]}" \
        "$@" -c "$TAIL" -o "$BUILD_DIR/$name.tail.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" \
        "$@" -c "$TEST" -o "$BUILD_DIR/$name.test.o"
    "$CC" -no-pie -Wl,--gc-sections "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.driver.o" \
        "$BUILD_DIR/$name.tail.o" -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr"
}

assert_pass() {
    local name="$1"
    rg -q '^W34N19 FRAME AUTHORITY CERTIFICATE PASS$' \
        "$BUILD_DIR/$name.stdout"
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
  'M1:W34N19_MUTANT_COMMON_TAIL_GUEST_TWIN:common_tail.native_authority' \
  'M2:W34N19_MUTANT_PARTY_GUARD_GUEST_TWIN:party.guard.native_authority' \
  'M3:W34N19_MUTANT_PARTY_CLEAR_ONLY_ON_GUARD:party.reconvergence.always_clears_bd34' \
  'M4:W34N19_MUTANT_PARTY_INVERT_PRESENCE_BRANCH:party.primary_selectors_and_a4_stride' \
  'M5:W34N19_MUTANT_PARTY_WRONG_SELECTOR:party.primary_selectors_and_a4_stride' \
  'M6:W34N19_MUTANT_PARTY_WRONG_STRIDE:party.primary_selectors_and_a4_stride' \
  'M7:W34N19_MUTANT_PARTY_SKIP_RECONCILE:party.reconcile.called_once' \
  'M8:W34N19_MUTANT_MENU_GUEST_TWINS:menu.native_authorities' \
  'M9:W34N19_MUTANT_TRANSITION_WRONG_PAGE:transition.guest_6ee68_authority'; do
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

rg -q 'D_80059179 == 0u' "$DRIVER"
rg -q 'D_80059460 = 0u' "$DRIVER"
rg -q 'g_MenuDebugEnabled = 0u' "$DRIVER"
rg -q 'D_80059171 = 1u' "$DRIVER"
rg -q '#define D_8006EE68  0x8006EE68u' "$DRIVER"
if rg -q '#define D_800(69179|69178|69171|69460|7EE68)' "$DRIVER"; then
    echo 'wrong-page frame-driver macro remains' >&2
    exit 1
fi

echo 'W34N19 CERTIFICATE PASS O0/O2/UBSan; M1-M9 DETECTED; focused warnings clean'
