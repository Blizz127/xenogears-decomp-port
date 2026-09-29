#!/usr/bin/env bash
set -euo pipefail

binary="${XENO_PORT_BINARY:-pc_port/build_native/xeno-port}"
logs=()
cleanup() {
    if [ "${#logs[@]}" -gt 0 ]; then
        rm -f "${logs[@]}"
    fi
}
trap cleanup EXIT

run_case() {
    local name="$1"
    local map="$2"
    local entrance="$3"
    local seconds="$4"
    local log
    local rc
    local video="${SDL_VIDEODRIVER:-x11}"
    local -a runner=()

    log="$(mktemp "/tmp/xeno-${name,,}-smoke.XXXXXX.log")"
    logs+=("$log")

    # SDL's dummy driver is not sufficient for this OpenGL path. In a display-
    # less CI shell, provide a real X11 framebuffer through Xvfb by default;
    # callers that set SDL_VIDEODRIVER explicitly retain that choice.
    if [ -z "${DISPLAY:-}" ] && [ -z "${SDL_VIDEODRIVER:-}" ]; then
        if ! command -v xvfb-run >/dev/null 2>&1; then
            echo "$name smoke: FAIL (DISPLAY is unset and xvfb-run is unavailable)" >&2
            exit 1
        fi
        runner=(xvfb-run -a)
        video=x11
    fi

    set +e
    "${runner[@]}" env \
        XENO_FIELD_TEST=1 \
        XENO_KERNEL_SEL=0 \
        XENO_FIELD_MAP="$map" \
        XENO_FIELD_ENTRANCE="$entrance" \
        XENO_FIELD_DIAG=1 \
        SDL_VIDEODRIVER="$video" \
        timeout --signal=TERM --kill-after=3 "${seconds}s" "$binary" >"$log" 2>&1
    rc=$?
    set -e

    case "$rc" in
        0|124|137|143) ;;
        *)
            sed -n '1,260p' "$log"
            echo "$name smoke: FAIL (runtime rc=$rc)" >&2
            exit 1
            ;;
    esac

    # The field loop intentionally remains alive under the smoke harness.
    # Require a real OT submission after the four-frame fade gate, plus an
    # actor entering the sprite renderer. The second assertion catches a boot-
    # global guard that leaves the background path alive while hiding actors.
    if ! rg -q '\[field-diag\] frame=[0-9]+ .*primSubmits=[1-9][0-9]* DrawOTag=1' "$log"; then
        sed -n '1,260p' "$log"
        echo "$name smoke: FAIL (no nonzero primitive submission)" >&2
        exit 1
    fi

    if ! rg -q '\[field-diag\] func_80075B44 frame=[0-9]+ active=[1-9][0-9]* .*globalSkip=0' "$log"; then
        sed -n '1,260p' "$log"
        echo "$name smoke: FAIL (actor renderer globally skipped)" >&2
        exit 1
    fi

    # Village maps submit 2D NPC sprites; the opening room / forest maps are
    # model- and object-heavy and may legitimately report plain=0.
    if [ "$name" = "Map0" ] || [ "$name" = "Map1" ]; then
        if ! rg -q '\[field-diag\] func_80075B44 frame=[0-9]+ active=[1-9][0-9]* plain=[1-9]' "$log"; then
            sed -n '1,260p' "$log"
            echo "$name smoke: FAIL (no plain actor draw)" >&2
            exit 1
        fi
    fi

    # Zeboim sky bridge (Id-fight map): pin the map identity (FieldLoad 383),
    # its texture archive drain, and the party actor sprite on top of the
    # generic OT/actor gates above.
    if [ "$name" = "Map383" ]; then
        if ! rg -q '\[field-diag\] FieldLoad begin field=383' "$log"; then
            sed -n '1,260p' "$log"
            echo "$name smoke: FAIL (field 383 never began loading)" >&2
            exit 1
        fi
        if ! rg -q 'retail decoder: [0-9]+ sectors fed, sectionsLeft=0 stripsLeft=0 done=1' "$log"; then
            sed -n '1,260p' "$log"
            echo "$name smoke: FAIL (field 383 texture archive did not drain)" >&2
            exit 1
        fi
        if ! rg -q '\[field-diag\] frame=[0-9]+ .*DrawOTag=1' "$log"; then
            sed -n '1,260p' "$log"
            echo "$name smoke: FAIL (no presented OT frame)" >&2
            exit 1
        fi
        if ! rg -q '\[field-diag\] func_80075B44 frame=[0-9]+ active=[1-9][0-9]* plain=[1-9]' "$log"; then
            sed -n '1,260p' "$log"
            echo "$name smoke: FAIL (no plain actor draw)" >&2
            exit 1
        fi
    fi

    if [ "$name" = "Map16" ]; then
        if ! rg -q '\[field-diag\] models frame=[0-9]+ .*emitted=[1-9]' "$log" &&
           ! rg -q 'objects=[1-9][0-9]*' "$log"; then
            sed -n '1,260p' "$log"
            echo "$name smoke: FAIL (no forest model/object activity)" >&2
            exit 1
        fi
        if rg -q '\[field-diag\] actor3 ip=32767 ' "$log"; then
            sed -n '1,260p' "$log"
            echo "$name smoke: FAIL (actor-3 IP frozen on object-loader stub)" >&2
            exit 1
        fi
    fi

    echo "$name smoke: PASS (runtime rc=$rc; nonzero OT submission and actor draw observed)"
}

run_case Map0 0 0 12
run_case Map1 1 6 25
run_case Map14 14 0 16
run_case Map15 15 0 16
run_case Map16 16 0 16
run_case Map383 383 0 25
