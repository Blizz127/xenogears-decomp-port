#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${WALK_ANIM_BUILD_DIR:-$ROOT/pc_port/build_native/walk_anim}"
CC="${CC:-gcc}"
MATCH=(rg)
if ! command -v rg >/dev/null; then MATCH=(grep -E); fi
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -fno-pie -no-pie -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -DUSE_EXTENDED_PRIM_POINTERS=0
      -ffunction-sections -fdata-sections -Wl,--gc-sections
      -include pc_port/tests/battle_sprite_abi_contract.h
      -fpermissive -w)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
SRC=(pc_port/tests/walk_anim_prod_test.c
     src/slus_006.64/system/temp1.c
     src/slus_006.64/system/animation_scripts.c
     pc_port/src/work_list_port.c)

compile_and_run() {
    local name="$1"
    local run_rc=0
    shift
    "$CC" "${BASE[@]}" "${INC[@]}" "$@" "${SRC[@]}" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr" || run_rc=$?
    return "$run_rc"
}

for regime in O0 O2 UBSan; do
    flags=(-O0 -g)
    if [[ "$regime" == O2 ]]; then flags=(-O2); fi
    if [[ "$regime" == UBSan ]]; then
        flags=(-O2 -g -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    compile_and_run "$regime" "${flags[@]}"
    "${MATCH[@]}" -q '^WALK ANIM certificate PASS$' "$BUILD_DIR/$regime.stdout"
    test ! -s "$BUILD_DIR/$regime.stderr"
    "${MATCH[@]}" -v '^\[xeno-port\] PSX RAM emulation:' "$BUILD_DIR/$regime.stdout" \
        >"$BUILD_DIR/$regime.normalized" || true
done
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/O2.normalized"
cmp "$BUILD_DIR/O0.normalized" "$BUILD_DIR/UBSan.normalized"
echo "WALK ANIM O0/O2/UBSAN PASS"

mutants=(
    'M1:WALK_ANIM_MUTANT_NO_SELECT:walk.selects.locomotion'
    'M2:WALK_ANIM_MUTANT_NO_TICK:walk.pose.advances'
)
for entry in "${mutants[@]}"; do
    label="${entry%%:*}"
    rest="${entry#*:}"
    define="${rest%%:*}"
    assertion="${rest#*:}"
    set +e
    compile_and_run "$label" -O0 -g -D"$define"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || \
       ! "${MATCH[@]}" -q "^ASSERTION ${assertion}$" "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED named mutant gate rc=$rc" >&2
        sed -n '1,80p' "$BUILD_DIR/$label.stderr" >&2 || true
        exit 1
    fi
    echo "$label DETECTED"
done
echo "WALK ANIM CERTIFICATE PASS; M1-M2 DETECTED"

# Negative control for the retail a0 contract: keep the call and signature,
# but deliberately discard the sprite argument in a scratch-only TU copy.
sed 's/func_800C11CC(pData);/func_800C11CC(NULL);/' \
    src/slus_006.64/system/temp1.c > "$BUILD_DIR/battle-null-argument.c"
SRC[1]="$BUILD_DIR/battle-null-argument.c"
set +e
compile_and_run battle-null-argument -O2
rc=$?
set -e
if [[ "$rc" == 0 ]] || ! "${MATCH[@]}" -q \
    '^ASSERTION battle.dispatch.preserves.sprite$' "$BUILD_DIR/battle-null-argument.stderr"; then
    echo 'BATTLE SPRITE ABI null-argument mutant survived or failed unexpectedly' >&2
    exit 1
fi
echo 'BATTLE SPRITE ABI null-argument mutant rejected'
