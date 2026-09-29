#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$root"
out="pc_port/build_tests/w34n22_7565c"
mkdir -p "$out"

fixture=disc/world_map.bin
expected_world=4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70
expected_slice=e535f68765934333b427762b0d22dfbdeeb06003c800956f999bcf6e33a1becd
test "$(sha256sum "$fixture" | awk '{print $1}')" = "$expected_world"
test "$(dd if="$fixture" bs=1 skip=$((0x5b6c)) count=$((0x264)) status=none | sha256sum | awk '{print $1}')" = "$expected_slice"

base=(-DXENO_PC_PORT -DWM_7565C_TEST_HOOKS -std=c11 -Wall -Wextra -Werror -Wconversion
      -Wsign-conversion -Iinclude -Ipc_port/src)
src=(pc_port/tests/w34n22_7565c_restore_prod_test.c
     pc_port/src/psx_memory.c pc_port/src/world_map_helper_7565c.c)

for regime in O0 O2 UBSan; do
    flags=(-O0)
    [[ "$regime" == O2 ]] && flags=(-O2)
    [[ "$regime" == UBSan ]] && \
        flags=(-O2 -fsanitize=undefined -fno-sanitize-recover=all)
    gcc "${base[@]}" "${flags[@]}" "${src[@]}" -o "$out/$regime"
    "$out/$regime"
done

mutants=(
  "M1:W34N22_MUTANT_GUEST_SOURCE:source.native.authority"
  "M2:W34N22_MUTANT_SHORT_POOL_COPY:pool.full.0x2000"
  "M3:W34N22_MUTANT_SKIP_POSE_COPY:runtime.record.dual.publish"
  "M4:W34N22_MUTANT_SKIP_TIMERS:timers.full.0x20"
  "M5:W34N22_MUTANT_SHORT_RING_COPY:ring.full.0x280"
  "M6:W34N22_MUTANT_SKIP_CAMERA_POSITION:camera.position.four.words"
  "M7:W34N22_MUTANT_SWAP_FINAL_STORES:restore.write.order"
)
for spec in "${mutants[@]}"; do
    IFS=: read -r name define assertion <<<"$spec"
    exe="$out/$name"
    log="$out/$name.log"
    gcc "${base[@]}" -O2 -D"$define" "${src[@]}" -o "$exe"
    set +e
    "$exe" >"$log" 2>&1
    rc=$?
    set -e
    if [[ $rc -eq 0 ]] || ! rg -q "ASSERTION $assertion FAILED" "$log"; then
        echo "$name failed mutant gate rc=$rc expected=$assertion" >&2
        cat "$log" >&2
        exit 1
    fi
    echo "$name DETECTED by ASSERTION $assertion"
done

bash pc_port/tests/run_w34n7_slot2_teardown.sh
bash pc_port/tests/run_w34n9_slot1_owner.sh

rg -q 'pc_port/src/world_map_helper_7565c.c' pc_port/build_port.sh
integration_block="$(sed -n '/initial_c894 = wm_72238_lw(WM_72238_C894)/,+100p' \
    pc_port/src/world_map_session_setup_72238.c)"
for token in 'func_80039CC4();' 'func_800399D4(D_80062528);' \
             'wm_8007565C();' 'wm_80075D4C();'; do
    rg -Fq "$token" <<<"$integration_block"
done

echo 'W34N22 7565C O0/O2/UBSan PASS; strict warnings clean; M1-M7 DETECTED; round-trip and slot-1 integration PASS'
