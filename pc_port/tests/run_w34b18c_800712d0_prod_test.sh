#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

OUT="${W34B18C_OUT:-pc_port/build_native/w34b18c-cert}"
mkdir -p "$OUT"

TEST_SOURCE="pc_port/tests/w34b18b_800712d0_prod_test.c"
FRAME_SOURCE="pc_port/src/world_map_frame_driver.c"
PROD_SOURCE="pc_port/src/world_map_init.c"

STRICT_FLAGS=(
    -std=gnu17 -g -Wall -Wextra -Wconversion -Wsign-conversion -Werror
    -DXENO_PC_PORT -fno-pie -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
)
PROD_FLAGS=(
    -std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY
    -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -include assert.h -w -g -m64 -fno-builtin -fno-pie
    -ffunction-sections -fdata-sections
    -Ipc_port/src -Ipc_port/include_shim -Iinclude
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
)

require_fixture() {
    local fixture="disc/world_map.bin"
    local expected_hash="4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70"
    local actual_hash

    if [[ ! -f "$fixture" ]]; then
        echo "ERROR: missing ignored fixture $fixture" >&2
        exit 1
    fi
    if [[ "$(stat -c '%s' "$fixture")" != "180422" ]]; then
        echo "ERROR: $fixture size is not 180422" >&2
        exit 1
    fi
    actual_hash="$(sha256sum "$fixture" | awk '{print $1}')"
    if [[ "$actual_hash" != "$expected_hash" ]]; then
        echo "ERROR: $fixture SHA-256 mismatch: $actual_hash" >&2
        exit 1
    fi
    actual_hash="$(dd if="$fixture" bs=1 skip=$((0x800967E4 - 0x8006FAF0)) \
        count=252 status=none | sha256sum | awk '{print $1}')"
    if [[ "$actual_hash" != "0d16c4f020e76808390b2ad94cdff89aa6c28b60bcd4beb2bd0890af938b1530" ]]; then
        echo "ERROR: retail 0x800967E4 slice SHA-256 mismatch: $actual_hash" >&2
        exit 1
    fi
}

make_mutant_source() {
    local mutant="$1"
    local destination="$2"

    cp "$PROD_SOURCE" "$destination"
    case "$mutant" in
        A)
            perl -0pi -e \
                's/if \(dbg0 != 0 && dbg1 != 0xFFFFFFFFu\) \{/if (dbg0 != 0 && dbg1 != 0u) {/' \
                "$destination"
            rg -q -F 'if (dbg0 != 0 && dbg1 != 0u) {' "$destination"
            ! rg -q -F 'if (dbg0 != 0 && dbg1 != 0xFFFFFFFFu) {' "$destination"
            ;;
        B)
            perl -0pi -e \
                's/if \(d788_record == 0\) \{\n        s_wm967e4_d788_null\+\+;\n        return 0;\n    \}/if (d788_record == 0) {\n        s_wm967e4_d788_null++;\n        goto c624_path;\n    }/' \
                "$destination"
            rg -q -U -F $'if (d788_record == 0) {\n        s_wm967e4_d788_null++;\n        goto c624_path;' \
                "$destination"
            ;;
        C)
            perl -0pi -e 's/return dispatch_result;/return 0;/' "$destination"
            rg -q -U -F $'if (dispatch_result != 0) {\n        if (dispatch_result == 1)' \
                "$destination"
            ! rg -q -F 'return dispatch_result;' "$destination"
            ;;
        *)
            echo "ERROR: unknown 967E4 mutant $mutant" >&2
            exit 1
            ;;
    esac
    if cmp -s "$PROD_SOURCE" "$destination"; then
        echo "ERROR: mutant $mutant did not alter copied production source" >&2
        exit 1
    fi
    diff --unified=0 "$PROD_SOURCE" "$destination" > "$OUT/m967_${mutant}.diff" || true
}

compile_certificate() {
    local label="$1"
    local optimization="$2"
    local production_source="$3"
    local frame_mutant="${4:-}"
    local sanitize="${5:-0}"
    local test_object="$OUT/${label}.test.o"
    local frame_object="$OUT/${label}.frame.o"
    local production_object="$OUT/${label}.world_map_init.o"
    local subject_object="$OUT/${label}.world_map_init.subject.o"
    local executable="$OUT/${label}"
    local sanitize_flags=()
    local frame_def=()

    if [[ "$sanitize" == "1" ]]; then
        sanitize_flags=(-fsanitize=undefined -fno-sanitize-recover=all)
    fi
    if [[ -n "$frame_mutant" ]]; then
        frame_def=("-DWM_712D0_MUTANT_M${frame_mutant}")
    fi

    gcc -c "$TEST_SOURCE" "${STRICT_FLAGS[@]}" "$optimization" \
        "${sanitize_flags[@]}" -o "$test_object"
    gcc -c "$FRAME_SOURCE" "${STRICT_FLAGS[@]}" "$optimization" \
        -DWM_712D0_TEST_TRACE -DWM_71490_CONTINUATION_DISABLED "${frame_def[@]}" \
        "${sanitize_flags[@]}" -o "$frame_object"
    gcc -c "$production_source" "${PROD_FLAGS[@]}" "$optimization" \
        "${sanitize_flags[@]}" -o "$production_object"

    objcopy \
        --weaken-symbol=wm_800968E0_dispatch_partial \
        --weaken-symbol=wm_8009699C_d788_processor \
        --weaken-symbol=wm_800966CC_c624_processor \
        "$production_object" "$subject_object"

    gcc -no-pie -Wl,--gc-sections "${sanitize_flags[@]}" \
        "$test_object" "$frame_object" "$subject_object" -o "$executable"

    nm "$test_object" > "$OUT/${label}.test.nm"
    nm "$production_object" > "$OUT/${label}.production.nm"
    nm "$executable" > "$OUT/${label}.executable.nm"
    if ! rg -q ' U wm_800967E4_dispatch_cd_work$' "$OUT/${label}.test.nm"; then
        echo "ERROR: $label test object does not leave 967E4 undefined" >&2
        exit 1
    fi
    if ! rg -q ' T wm_800967E4_dispatch_cd_work$' "$OUT/${label}.production.nm"; then
        echo "ERROR: $label production object does not own 967E4" >&2
        exit 1
    fi
    if [[ "$(nm --defined-only "$executable" | awk '$3 == "wm_800967E4_dispatch_cd_work" { count++ } END { print count + 0 }')" != "1" ]]; then
        echo "ERROR: $label final binary does not have exactly one 967E4 owner" >&2
        exit 1
    fi
    if rg -q 'wm_967e4_production' "$OUT/${label}.executable.nm"; then
        echo "ERROR: $label retained the obsolete duplicated 967E4 subject" >&2
        exit 1
    fi

    set +e
    "$executable" > "$OUT/${label}.stdout" 2> "$OUT/${label}.stderr"
    local rc=$?
    set -e
    echo "$rc" > "$OUT/${label}.rc"
}

require_pass() {
    local label="$1"
    if [[ "$(<"$OUT/${label}.rc")" != "0" ]]; then
        echo "ERROR: $label failed" >&2
        tail -40 "$OUT/${label}.stdout" >&2
        tail -40 "$OUT/${label}.stderr" >&2
        exit 1
    fi
    rg -q '^=== Results: 97/97 PASS ===$' "$OUT/${label}.stdout"
    if rg -qi 'runtime error|undefined behavior' "$OUT/${label}.stderr"; then
        echo "ERROR: $label emitted a sanitizer diagnostic" >&2
        exit 1
    fi
}

require_kill() {
    local label="$1"
    local named_assertion="$2"
    if [[ "$(<"$OUT/${label}.rc")" == "0" ]]; then
        echo "ERROR: $label survived" >&2
        exit 1
    fi
    if ! rg -q "^FAIL .*ASSERTION ${named_assertion}$" "$OUT/${label}.stdout"; then
        echo "ERROR: $label did not emit named assertion $named_assertion" >&2
        rg '^FAIL' "$OUT/${label}.stdout" >&2 || true
        exit 1
    fi
    printf '%s KILLED: %s\n' "$label" "$named_assertion"
}

require_fixture

compile_certificate O0 -O0 "$PROD_SOURCE"
require_pass O0
compile_certificate O2 -O2 "$PROD_SOURCE"
require_pass O2
compile_certificate UBSan_O2 -O2 "$PROD_SOURCE" "" 1
require_pass UBSan_O2

cmp "$OUT/O0.stdout" "$OUT/O2.stdout"
cmp "$OUT/O0.stdout" "$OUT/UBSan_O2.stdout"

for mutant in A B C; do
    mutant_source="$OUT/world_map_init.m967_${mutant}.c"
    make_mutant_source "$mutant" "$mutant_source"
    compile_certificate "M967_${mutant}" -O2 "$mutant_source"
done
require_kill M967_A M967-A-FFFFFFFF-sentinel-selects-D788
require_kill M967_B M967-B-D788-null-returns-without-C624
require_kill M967_C M967-C-dispatch-return-controls-frame-retry

for frame_mutant in $(seq 1 14); do
    compile_certificate "FRAME_M${frame_mutant}" -O2 "$PROD_SOURCE" "$frame_mutant"
    if [[ "$(<"$OUT/FRAME_M${frame_mutant}.rc")" == "0" ]]; then
        echo "ERROR: frame-driver mutant M${frame_mutant} survived" >&2
        exit 1
    fi
    frame_failure="$(rg -m1 '^FAIL' "$OUT/FRAME_M${frame_mutant}.stdout")"
    printf 'FRAME_M%s KILLED: %s\n' "$frame_mutant" "$frame_failure"
done

if rg -n 'wm_967e4_production|WM_967E4_MUTANT_[ABC]' "$TEST_SOURCE"; then
    echo "ERROR: certificate contains duplicated or test-local 967E4 mutant logic" >&2
    exit 1
fi

printf 'FOCUSED_REGIMES O0=97/97 O2=97/97 UBSan_O2=97/97\n'
printf 'REAL_967E4_MUTANTS 3/3 KILLED\n'
printf 'FRAME_DRIVER_MUTANTS 14/14 KILLED\n'
printf 'NORMALIZED_STDOUT IDENTICAL\n'
