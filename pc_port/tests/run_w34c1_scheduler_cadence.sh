#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${W34C1_CADENCE_BUILD_DIR:-$ROOT/pc_port/build_native/w34c1_cadence}"
CC="${CC:-clang}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C
      -ffunction-sections -fdata-sections)
WARN=(-Wall -Wextra -Wconversion -Wsign-conversion -Werror)
DRIVER_WARN=(-Wno-sign-conversion -Wno-unused-function)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)
DRIVER=pc_port/src/world_map_frame_driver_712d0.c
MAIN=pc_port/src/world_map_main_loop_71034.c
TERMINAL=pc_port/src/world_map_terminal_zero_710e4.c
CAPTURE=pc_port/src/world_map_capture.c
SYNC_HELPER=pc_port/src/world_map_helper_762fc.c
PRESENCE_HELPER=pc_port/src/world_map_helper_75d4c.c
MODE_STUBS=pc_port/tests/world_map_mode_lifecycle_stubs.c
TEST=pc_port/tests/w34c1_scheduler_cadence_prod_test.c

compile_and_run() {
    local name="$1"
    local driver_source="$2"
    local main_source="$3"
    local run_rc=0
    shift 3
    "$CC" "${BASE[@]}" "${WARN[@]}" "${DRIVER_WARN[@]}" "${INC[@]}" \
        "$@" -c "$driver_source" -o "$BUILD_DIR/$name.driver.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" \
        -c "$main_source" -o "$BUILD_DIR/$name.main.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" \
        -c "$TERMINAL" -o "$BUILD_DIR/$name.terminal.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" \
        -c "$CAPTURE" -o "$BUILD_DIR/$name.capture.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" \
        -c "$SYNC_HELPER" -o "$BUILD_DIR/$name.sync-helper.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" \
        -c "$PRESENCE_HELPER" -o "$BUILD_DIR/$name.presence-helper.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" \
        -c "$TEST" -o "$BUILD_DIR/$name.test.o"
    "$CC" "${BASE[@]}" "${WARN[@]}" "${INC[@]}" "$@" \
        -c "$MODE_STUBS" -o "$BUILD_DIR/$name.mode-stubs.o"
    "$CC" -no-pie -Wl,--gc-sections "$@" \
        "$BUILD_DIR/$name.test.o" "$BUILD_DIR/$name.main.o" \
        "$BUILD_DIR/$name.terminal.o" \
        "$BUILD_DIR/$name.driver.o" "$BUILD_DIR/$name.capture.o" \
        "$BUILD_DIR/$name.sync-helper.o" \
        "$BUILD_DIR/$name.presence-helper.o" \
        "$BUILD_DIR/$name.mode-stubs.o" \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" \
        2>"$BUILD_DIR/$name.stderr" || run_rc=$?
    return "$run_rc"
}

assert_clean_pass() {
    local name="$1"
    rg -q '^W34C1 scheduler cadence certificate PASS$' \
        "$BUILD_DIR/$name.stdout"
    if rg -q '^(ASSERTION|.*ERROR)' "$BUILD_DIR/$name.stderr"; then
        cat "$BUILD_DIR/$name.stderr" >&2
        exit 1
    fi
}

compile_and_run O0 "$DRIVER" "$MAIN" -O0 -g
compile_and_run O2 "$DRIVER" "$MAIN" -O2
compile_and_run UBSan "$DRIVER" "$MAIN" -O2 -g \
    -fsanitize=undefined -fno-sanitize-recover=all
for regime in O0 O2 UBSan; do
    assert_clean_pass "$regime"
done
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"

clear_line="$(rg -n 'wm_ot_clear_r_guest\(ot_ptr, 0x400u\);' "$DRIVER" | cut -d: -f1)"
inner_line="$(rg -n 'wm_712d0_run_second_scheduler\(\);' "$DRIVER" | cut -d: -f1)"
draw_line="$(rg -n 'wm_ot_draw_otag_guest\(guest_ot \+ 0xFFCu\);' "$DRIVER" | cut -d: -f1)"
test "$clear_line" -lt "$inner_line"
test "$inner_line" -lt "$draw_line"
if rg -q 'XENO_WORLD_OPEN_LOOP' pc_port/src/world_map_init.c; then
    echo 'XENO_WORLD_OPEN_LOOP still controls production world init' >&2
    exit 1
fi
rg -q 'g_MainGameStates\[3\]\.pFnMain = PcPort_WorldMapInitMain;' \
    pc_port/src/game_overrides.c
if rg -q 'PcPort_WorldMapInitEnabled' pc_port/src/game_overrides.c; then
    echo 'XENO_WORLD_INIT still controls game-state 3 selection' >&2
    exit 1
fi
echo "ORDER CLEAR_OT -> INNER_SCHEDULER -> DRAW_OT -> 0x719C8 LIMIT PASS"
echo "OPEN_LOOP SELECTOR RETIRED PASS"
echo "WORLD STATE 3 SELECTOR RETIRED PASS"

make_mutant() {
    local label="$1"
    cp "$DRIVER" "$BUILD_DIR/$label.driver.c"
    cp "$MAIN" "$BUILD_DIR/$label.main.c"
}

make_mutant M1
perl -0pi -e 's/(for \(;;\) \{\n        frame =)/for (;;) {\n        wm_80097800();\n        frame =/' \
    "$BUILD_DIR/M1.driver.c"
rg -q '^        wm_80097800\(\);$' "$BUILD_DIR/M1.driver.c"

make_mutant M2
perl -0pi -e 's/#elif defined\(WM_712D0_MUTANT_DOUBLE_SECOND_SCHEDULER\)/#elif 1/' \
    "$BUILD_DIR/M2.driver.c"
perl -0pi -e 's/#if defined\(WM_712D0_MUTANT_NO_SECOND_SCHEDULER\)/#if 1/' \
    "$BUILD_DIR/M2.driver.c"

make_mutant M3
perl -0pi -e 's/#elif defined\(WM_712D0_MUTANT_DOUBLE_SECOND_SCHEDULER\)/#elif 1/' \
    "$BUILD_DIR/M3.driver.c"

make_mutant M4
perl -0pi -e 's/(run->displayed_frames >= run->frame_limit)(\)\n            return WM_712D0_RUN_BOUNDED_EXIT;)/$1 - 1$2/' \
    "$BUILD_DIR/M4.driver.c"
rg -q 'run->displayed_frames >= run->frame_limit - 1' \
    "$BUILD_DIR/M4.driver.c"

make_mutant M5
perl -0pi -e 's/(run->displayed_frames >= run->frame_limit)(\)\n            return WM_712D0_RUN_BOUNDED_EXIT;)/$1 + 1$2/' \
    "$BUILD_DIR/M5.driver.c"
rg -q 'run->displayed_frames >= run->frame_limit \+ 1' \
    "$BUILD_DIR/M5.driver.c"

make_mutant M6
perl -0pi -e 's/\(frame % 60\) != 0/((frame + 1) % 60) != 0/' \
    "$BUILD_DIR/M6.main.c"

make_mutant M7
perl -0pi -e 's/(for \(;;\) \{\n        frame =)/for (;;) {\n        fd_sw(D_8009BE3C, 0x8009BC40u);\n        fd_sw(D_8009D7F0, 1u);\n        frame =/' \
    "$BUILD_DIR/M7.driver.c"

make_mutant M8
perl -0pi -e 's/return WM_712D0_RUN_BOUNDED_EXIT;/break;/' \
    "$BUILD_DIR/M8.driver.c"

make_mutant M9
perl -0pi -e 's/if \(slot1_addr != 0 &&/if (0 \&\& slot1_addr != 0 \&\&/' \
    "$BUILD_DIR/M9.main.c"

make_mutant M10
perl -0pi -e 's/        func_800250E0\(\(int\)fd_lw\(D_8009D7F0\)\);/        \/\* omitted 0x800250E0 \*\//' \
    "$BUILD_DIR/M10.driver.c"

make_mutant M11
perl -0pi -e 's/        wm_80025044_guest_safe\(\);/        \/\* omitted 0x80025044 \*\//' \
    "$BUILD_DIR/M11.driver.c"

make_mutant M12
perl -0pi -e 's/        \(void\)wm_80074F2C\(\);/        \/\* omitted 0x80074F2C \*\//' \
    "$BUILD_DIR/M12.driver.c"

make_mutant M13
perl -0pi -e 's/        \(void\)wm_80075104\(\);/        \/\* omitted 0x80075104 \*\//' \
    "$BUILD_DIR/M13.driver.c"

make_mutant M14
perl -0pi -e 's/        \(void\)wm_80074F2C\(\);\n        \(void\)wm_80075104\(\);/        (void)wm_80075104();\n        (void)wm_80074F2C();/' \
    "$BUILD_DIR/M14.driver.c"

make_mutant M15
perl -0pi -e 's/        func_8001D468\(\);/        \/\* omitted 0x8001D468 \*\//' \
    "$BUILD_DIR/M15.driver.c"

for entry in \
    'M1:cadence.outer_scheduler.exactly_once' \
    'M2:cadence.inner_scheduler.once_per_displayed_frame' \
    'M3:cadence.inner_scheduler.once_per_displayed_frame' \
    'M4:frame_limit.displayed_frames.exactly_120' \
    'M5:frame_limit.displayed_frames.exactly_120' \
    'M6:capture.frame60.request_equals_present60' \
    'M7:driver.entry.once.ot_buffers_alternate' \
    'M8:bounded_exit.skips_terminal_lane' \
    'M9:session.slot1.exactly_once' \
    'M10:frame_seams.250e0.once_per_frame' \
    'M11:frame_seams.25044.once_per_frame' \
    'M12:frame_seams.74f2c.once_per_frame' \
    'M13:frame_seams.75104.once_per_frame' \
    'M14:frame_seams.retail_order' \
    'M15:frame_seams.1d468.once_per_frame'; do
    label="${entry%%:*}"
    assertion="${entry#*:}"
    set +e
    compile_and_run "$label" "$BUILD_DIR/$label.driver.c" \
        "$BUILD_DIR/$label.main.c" -O0 -g
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || \
       ! rg -q "^ASSERTION $assertion([[:space:]]|$)" \
           "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED mutant gate rc=$rc expected=$assertion" >&2
        tail -80 "$BUILD_DIR/$label.stderr" >&2
        exit 1
    fi
    echo "$label DETECTED assertion=$assertion"
done

for entry in \
    'M16:W34N18_LANE_MUTANT_SKIP_SELECTOR_CALL:transition.lane.calls.selector' \
    'M17:W34N18_LANE_MUTANT_WRONG_D80C_GUARD:transition.lane.calls.selector' \
    'M18:W34N18_LANE_MUTANT_SKIP_SESSION_EXIT:transition.lane.success.session.exit' \
    'M19:W34N18_LANE_MUTANT_WRONG_PARTY_PUBLISH:transition.lane.success.party.publish' \
    'M20:W34N18_LANE_MUTANT_SKIP_D80C_CLEAR:transition.lane.tail.clears.d80c' \
    'M21:W34N18_LANE_MUTANT_WRONG_TOGGLE_MASK:transition.lane.tail.toggles.bit100'; do
    IFS=: read -r label define assertion <<<"$entry"
    set +e
    compile_and_run "$label" "$DRIVER" "$MAIN" -O0 -g -D"$define"
    rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || \
       ! rg -q "^ASSERTION $assertion([[:space:]]|$)" \
           "$BUILD_DIR/$label.stderr"; then
        echo "$label FAILED mutant gate rc=$rc expected=$assertion" >&2
        tail -80 "$BUILD_DIR/$label.stderr" >&2
        exit 1
    fi
    echo "$label DETECTED assertion=$assertion"
done

echo "W34C1 CADENCE CERTIFICATE PASS O0/O2/UBSan; M1-M21 DETECTED; focused warnings clean"
