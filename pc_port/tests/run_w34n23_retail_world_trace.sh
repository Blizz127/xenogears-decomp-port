#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
APP="${XENO_PCSX_REDUX:-/home/blizz/apps/pcsx-redux/build293-3e10093a/PCSX-Redux-build293-3e10093a.AppImage}"
DISC="${XENO_RETAIL_DISC:-$ROOT/disc/disc1.bin}"
BIOS="${XENO_RETAIL_BIOS:-$ROOT/disc/scph5500.bin}"
STATE="${XENO_RETAIL_WORLD_STATE:-$ROOT/scratchpad/retail_world_session_3e10093a.state}"
CONFIG="${XENO_PCSX_CONFIG:-${XDG_CONFIG_HOME:-$HOME/.config}/pcsx-redux/pcsx.json}"
PROFILE="${XENO_RETAIL_TRACE_PROFILE:-frames}"
case "$PROFILE" in
    frames)
        LUA="$ROOT/pc_port/tests/w34n23_retail_world_trace.lua"
        PREFIX=W34N23_RETAIL
        ;;
    session_exit)
        LUA="$ROOT/pc_port/tests/w34n24_retail_session_exit.lua"
        PREFIX=W34N24_RETAIL
        ;;
    terminal_exit)
        LUA="$ROOT/pc_port/tests/w34n27_retail_terminal_exit.lua"
        PREFIX=W34N27_RETAIL
        ;;
    transition_exit)
        LUA="$ROOT/pc_port/tests/w34n29_retail_transition_exit.lua"
        PREFIX=W34N29_RETAIL
        ;;
    default_exit)
        LUA="$ROOT/pc_port/tests/w34n31_retail_default_exit.lua"
        PREFIX=W34N31_RETAIL
        ;;
    *)
        printf 'unknown XENO_RETAIL_TRACE_PROFILE=%s\n' "$PROFILE" >&2
        exit 2
        ;;
esac
OUT="${W34N23_OUT:-$ROOT/pc_port/build_native/w34n23_retail_world_trace}"
FRAME_LIMIT="${XENO_RETAIL_TRACE_FRAMES:-5}"
WATCHDOG="${XENO_RETAIL_TRACE_TIMEOUT:-120}"

APP_SHA=b27a564e6c32333453433c950ac26e652f40af32d5e236441f36e8123c9c651b
DISC_SHA=39c547a9afc6da15d847ef81a2c6cea1a6516bdfa562cf13b0999b04e8598bda
BIOS_SHA=11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef
STATE_SHA=6ad6c512a85b78bdde866240ababa43a8534ad218d546a979cbe3f37e9925781

require_sha() {
    local label="$1"
    local file="$2"
    local expected="$3"
    local actual

    if [[ ! -f "$file" ]]; then
        printf 'W34N23 MISSING %s=%s\n' "$label" "$file" >&2
        exit 2
    fi
    actual="$(sha256sum "$file" | awk '{print $1}')"
    if [[ "$actual" != "$expected" ]]; then
        printf 'W34N23 SHA_MISMATCH %s expected=%s actual=%s file=%s\n' \
            "$label" "$expected" "$actual" "$file" >&2
        exit 2
    fi
    printf 'W34N23 SHA_OK %s=%s\n' "$label" "$actual"
}

if [[ ! "$FRAME_LIMIT" =~ ^[1-9][0-9]*$ ]]; then
    echo 'XENO_RETAIL_TRACE_FRAMES must be a positive integer' >&2
    exit 2
fi
if [[ ! "$WATCHDOG" =~ ^[1-9][0-9]*$ ]]; then
    echo 'XENO_RETAIL_TRACE_TIMEOUT must be a positive integer' >&2
    exit 2
fi
command -v xvfb-run >/dev/null
command -v jq >/dev/null
command -v setsid >/dev/null
require_sha app "$APP" "$APP_SHA"
require_sha disc "$DISC" "$DISC_SHA"
require_sha bios "$BIOS" "$BIOS_SHA"
require_sha state "$STATE" "$STATE_SHA"
if [[ ! -f "$CONFIG" ]]; then
    printf 'W34N23 MISSING pcsx_config=%s\n' "$CONFIG" >&2
    exit 2
fi

mkdir -p "$OUT"
run_tmp="$(mktemp -d /tmp/w34n23-retail-world.XXXXXX)"
active_pid=""
active_watchdog=""
cleanup() {
    if [[ -n "$active_watchdog" ]]; then
        kill "$active_watchdog" 2>/dev/null || true
    fi
    if [[ -n "$active_pid" ]] && kill -0 -- "-$active_pid" 2>/dev/null; then
        kill -TERM -- "-$active_pid" 2>/dev/null || true
        sleep 1
        kill -KILL -- "-$active_pid" 2>/dev/null || true
    fi
    rm -rf -- "$run_tmp"
}
trap cleanup EXIT

export XENO_RETAIL_WORLD_STATE="$STATE"
export XENO_RETAIL_TRACE_FRAMES="$FRAME_LIMIT"

run_once() {
    local label="$1"
    local run_dir="$run_tmp/$label"
    local config_dir="$run_dir/config/pcsx-redux"
    local log="$OUT/$label.log"
    local normalized="$OUT/$label.normalized.log"
    local trace="$OUT/$label.trace.txt"
    local live="$OUT/$label.live.log"
    local run_pid watchdog_pid rc

    mkdir -p "$config_dir" "$run_dir/cache"
    jq '.emulator.Debug.Debug = true |
        .emulator.Debug.GdbServer = false |
        .emulator.Dynarec = false' "$CONFIG" >"$config_dir/pcsx.json"
    jq -e '.emulator.Debug.Debug == true and
           .emulator.Debug.GdbServer == false and
           .emulator.Dynarec == false' "$config_dir/pcsx.json" >/dev/null

    XDG_CONFIG_HOME="$run_dir/config" XDG_CACHE_HOME="$run_dir/cache" \
        XENO_RETAIL_TRACE_LIVE="$live" \
        setsid xvfb-run -a "$APP" \
            -interpreter -run --bios "$BIOS" -iso "$DISC" \
            -memcard1 "$run_dir/memcard1.mcd" \
            -memcard2 "$run_dir/memcard2.mcd" \
            -dofile "$LUA" -stdout -lua_stdout -no-gui-log \
            >"$log" 2>&1 &
    run_pid=$!
    active_pid="$run_pid"

    (
        sleep "$WATCHDOG"
        if kill -0 "$run_pid" 2>/dev/null; then
            printf 'W34N23 WATCHDOG label=%s seconds=%s\n' \
                "$label" "$WATCHDOG" >&2
            kill -TERM -- "-$run_pid" 2>/dev/null || true
            sleep 10
            kill -KILL -- "-$run_pid" 2>/dev/null || true
        fi
    ) &
    watchdog_pid=$!
    active_watchdog="$watchdog_pid"

    set +e
    wait "$run_pid"
    rc=$?
    set -e
    kill "$watchdog_pid" 2>/dev/null || true
    wait "$watchdog_pid" 2>/dev/null || true
    active_watchdog=""
    if kill -0 -- "-$run_pid" 2>/dev/null; then
        kill -TERM -- "-$run_pid" 2>/dev/null || true
    fi
    active_pid=""

    if [[ "$rc" -ne 0 ]]; then
        tail -120 "$log" >&2
        printf 'W34N23 RETAIL_RUN_FAILED label=%s rc=%d\n' \
            "$label" "$rc" >&2
        return 1
    fi

    sed -E 's/[[:space:]]+$//' "$log" >"$normalized"
    if rg -q "^${PREFIX} FAIL " "$normalized"; then
        rg "^${PREFIX} (FAIL|PASS)" "$normalized" >&2
        return 1
    fi
    rg -q '^CPU type: Interpreted$' "$normalized"
    rg -q "^${PREFIX} STATE_LOADED$" "$normalized"
    rg -q "^${PREFIX} SLOT1_CALL target=80072238$" "$normalized"
    test "$(rg -c "^${PREFIX} FRAME_HEAD " "$normalized")" = \
        "$FRAME_LIMIT"
    test "$(rg -c "^${PREFIX} FRAME_BRANCH " "$normalized")" = \
        "$FRAME_LIMIT"

    if [[ "$PROFILE" == frames ]]; then
        rg -q '^W34N23_RETAIL SLOT1_RETURN D7CC=2$' "$normalized"
        rg -q '^W34N23_RETAIL DRIVER_ENTRY D554=0 D7CC=2$' "$normalized"
        rg -q "^W34N23_RETAIL PASS frames=${FRAME_LIMIT} slot1=1 driver=1 slot2=0$" \
            "$normalized"
    elif [[ "$PROFILE" == session_exit ]]; then
        rg -q "^W34N24_RETAIL SEED_D554 frame=${FRAME_LIMIT} before=1 after=0$" \
            "$normalized"
        rg -q "^W34N24_RETAIL FRAME_BRANCH frame=${FRAME_LIMIT} D554=0 taken=0$" \
            "$normalized"
        rg -q '^W34N24_RETAIL DRIVER_EXIT D554=0 D7CC=2$' "$normalized"
        rg -q '^W34N24_RETAIL SLOT2_CALL target=8007299c D7CC=2$' "$normalized"
        rg -q '^W34N24_RETAIL SLOT2_RETURN D7CC=2$' "$normalized"
        rg -q '^W34N24_RETAIL SESSION_DECISION D7CC=2 repeat=1$' "$normalized"
        rg -q '^W34N24_RETAIL NEXT_SESSION_HEAD D7CC=2$' "$normalized"
        rg -q "^W34N24_RETAIL PASS frames=${FRAME_LIMIT} slot1=1 slot2=1 repeat=1$" \
            "$normalized"
    elif [[ "$PROFILE" == terminal_exit ]]; then
        rg -q "^W34N27_RETAIL SEED_D554 frame=${FRAME_LIMIT} before=1 after=0$" \
            "$normalized"
        rg -q "^W34N27_RETAIL FRAME_BRANCH frame=${FRAME_LIMIT} D554=0 taken=0$" \
            "$normalized"
        rg -q '^W34N27_RETAIL SLOT2_CALL target=8007299c D7CC=0$' \
            "$normalized"
        rg -q '^W34N27_RETAIL SEED_TERMINAL D7CC_BEFORE=2 D7CC_AFTER=0 D7D8=ffffffff BBC4_BEFORE=0 BBC4_AFTER=1$' \
            "$normalized"
        rg -q '^W34N27_RETAIL SLOT2_RETURN D7CC=0 D7D8=' "$normalized"
        rg -q '^W34N27_RETAIL TERMINAL_DECISION D7CC=0 lane=zero$' \
            "$normalized"
        rg -q '^W34N27_RETAIL LOAD_OVERLAY call=1 a0=1$' "$normalized"
        rg -q '^W34N27_RETAIL CHANGE_STATE call=1 a0=1$' "$normalized"
        rg -q '^W34N27_RETAIL REGION_GUARD BBC4=1 D7D8=ffffffff TYPE=SKIPPED helper_expected=0$' \
            "$normalized"
        rg -q '^W34N27_RETAIL REGION_OUTPUTS writes=0 ' "$normalized"
        rg -q '^W34N27_RETAIL EF68 ' "$normalized"
        rg -q '^W34N27_RETAIL COMMON_EPILOGUE ' "$normalized"
        rg -q '^W34N27_RETAIL SYNC_762FC call=1 BYTE591AE=00$' "$normalized"
        rg -q '^W34N27_RETAIL MAIN_LOOP a0=0 BYTE591AE=00 ' "$normalized"
        rg -q "^W34N27_RETAIL PASS frames=${FRAME_LIMIT} slot1=1 slot2=1 terminal=0 helper_calls=[01]$" \
            "$normalized"
    elif [[ "$PROFILE" == transition_exit ]]; then
        rg -q "^W34N29_RETAIL SEED_D554 frame=${FRAME_LIMIT} before=1 after=0$" \
            "$normalized"
        rg -q '^W34N29_RETAIL SEED_D7CC before=2 after=1$' "$normalized"
        rg -q '^W34N29_RETAIL SLOT2_CALL target=8007299c D7CC=1$' \
            "$normalized"
        rg -q '^W34N29_RETAIL SLOT2_RETURN D7CC=1 ' "$normalized"
        rg -q '^W34N29_RETAIL TERMINAL_DECISION D7CC=1 lane=transition$' \
            "$normalized"
        rg -q '^W34N29_RETAIL LOAD_OVERLAY call=1 a0=2$' "$normalized"
        rg -q '^W34N29_RETAIL CHANGE_STATE call=1 a0=2$' "$normalized"
        rg -q '^W34N29_RETAIL SOUND_CLEANUP call=1 BYTE594F8=00 ' \
            "$normalized"
        rg -q '^W34N29_RETAIL ALIGNED_SIZE call=1 ' "$normalized"
        rg -q '^W34N29_RETAIL COPY call=1 dst=80062648 ' "$normalized"
        rg -q '^W34N29_RETAIL MANAGER_CREATE call=1 a0=80062648 ' \
            "$normalized"
        rg -q '^W34N29_RETAIL MANAGER_CONFIGURE call=1 ' "$normalized"
        rg -q '^W34N29_RETAIL SYNC_762FC call=1 BYTE591AE=00$' "$normalized"
        rg -q "^W34N29_RETAIL PASS frames=${FRAME_LIMIT} slot1=1 slot2=1 transition=1$" \
            "$normalized"
    else
        rg -q "^W34N31_RETAIL SEED_D554 frame=${FRAME_LIMIT} before=1 after=0$" \
            "$normalized"
        rg -q '^W34N31_RETAIL SEED_D7CC before=2 after=-1$' "$normalized"
        rg -q '^W34N31_RETAIL SLOT2_CALL target=8007299c D7CC=-1$' \
            "$normalized"
        rg -q '^W34N31_RETAIL SLOT2_RETURN D7CC=-1$' "$normalized"
        rg -q '^W34N31_RETAIL TERMINAL_DECISION D7CC=-1 lane=default$' \
            "$normalized"
        rg -q '^W34N31_RETAIL CHANGE_STATE call=1 a0=0$' "$normalized"
        rg -q '^W34N31_RETAIL CLEAR_IMAGE call=1 rect=0,0,319,431 rgb=0,0,64$' \
            "$normalized"
        rg -q '^W34N31_RETAIL DRAW_SYNC call=1 a0=0$' "$normalized"
        rg -q '^W34N31_RETAIL SYNC_762FC call=1 BYTE591AE=00$' "$normalized"
        rg -q "^W34N31_RETAIL PASS frames=${FRAME_LIMIT} slot1=1 slot2=1 default=1$" \
            "$normalized"
    fi

    rg "^${PREFIX} " \
        "$normalized" >"$trace"
}

run_once run1
run_once run2
cmp "$OUT/run1.trace.txt" "$OUT/run2.trace.txt"
cp "$OUT/run1.trace.txt" "$OUT/trace.txt"
cat "$OUT/trace.txt"
echo 'W34N23 REPEAT_TRACE_IDENTICAL=YES'
printf 'W34N23 RETAIL WORLD TRACE PASS profile=%s\n' "$PROFILE"
