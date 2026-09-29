#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${W34N5_OUT:-$ROOT/pc_port/build_native/w34n5_wds_lifecycle}"
CC="${CC:-clang}"
PROD_CC="${PROD_CC:-gcc}"
PROD="$ROOT/pc_port/src/world_map_init.c"
TEST="$ROOT/pc_port/tests/w34n5_wds_lifecycle_prod_test.c"
mkdir -p "$OUT"
cd "$ROOT"

STRICT=(-std=gnu17 -Wall -Wextra -Wconversion -Wsign-conversion -Werror
        -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -fno-pie
        -ffunction-sections -fdata-sections
        -Ipc_port/include_shim -Iinclude -Ipc_port/src
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
PROD_FLAGS=(-std=gnu17 -fpermissive -DXENO_PC_PORT
            -DXENO_FIELD_OBJECT_OVERLAY -DSKIP_ASM -D_LANGUAGE_C
            -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h -w -m64
            -fno-builtin -fno-inline -fkeep-static-functions -fno-pie
            -ffunction-sections -fdata-sections
            -Ipc_port/src -Ipc_port/include_shim -Iinclude
            -Ipc_port/extern/PsyCross/include
            -Ipc_port/extern/PsyCross/include/psx)

# The field bank owner must be published for retail cleanup (func_8001B66C)
# to release. Since the sector-pump rewrite (field_wds_stream_retail_test),
# the publisher is retail's streaming loader, not the retired host stager.
rg -q -F 'g_GameCurLoadedWDS = func_800380D0(D_800C3A1C, 0x2000, 0);' \
    src/field/main/misc8.c

compile_run() {
    local label="$1"
    local opt="$2"
    local source="$3"
    local sanitize="${4:-0}"
    local san=()
    if [[ "$sanitize" == 1 ]]; then
        san=(-fsanitize=undefined -fno-sanitize-recover=all)
    fi
    "$CC" -c "$TEST" "${STRICT[@]}" "$opt" "${san[@]}" \
        -o "$OUT/$label.test.o"
    "$PROD_CC" -c "$source" "${PROD_FLAGS[@]}" "$opt" "${san[@]}" \
        -o "$OUT/$label.prod.local.o"
    objcopy --globalize-symbol=wm_fresh_session_wds_cleanup \
            --globalize-symbol=wm_first_wds_consumer \
            "$OUT/$label.prod.local.o" "$OUT/$label.prod.o"
    "$CC" -no-pie -Wl,--gc-sections "${san[@]}" \
        "$OUT/$label.test.o" "$OUT/$label.prod.o" -o "$OUT/$label"
    set +e
    "$OUT/$label" >"$OUT/$label.stdout" 2>"$OUT/$label.stderr"
    local rc=$?
    set -e
    printf '%d\n' "$rc" >"$OUT/$label.rc"
}

require_pass() {
    local label="$1"
    test "$(<"$OUT/$label.rc")" = 0
    rg -q '^W34N5 WDS LIFECYCLE FOCUSED CERTIFICATE PASS$' \
        "$OUT/$label.stdout"
    ! rg -qi 'runtime error|undefined behavior' "$OUT/$label.stderr"
}

mutant() {
    local name="$1"
    local expression="$2"
    local replacement="$3"
    local file="$OUT/$name.c"
    cp "$PROD" "$file"
    perl -0pi -e "s/\\Q$expression\\E/$replacement/" "$file"
    ! cmp -s "$PROD" "$file"
    printf '%s\n' "$file"
}

compile_run O0 -O0 "$PROD"
compile_run O2 -O2 "$PROD"
compile_run UBSan -O2 "$PROD" 1
for regime in O0 O2 UBSan; do require_pass "$regime"; done

m1="$(mutant M1 'WM_U32(WM_FLAG_C894_ABS) == 0u' \
              'WM_U32(WM_FLAG_C894_ABS) != 0u')"
m2="$(mutant M2 'g_GameCurLoadedWDS = result;' \
              'g_GameCurLoadedWDS = NULL;')"
m3="$(mutant M3 'WM_U32(WM_CONSUMER_RESULT) = (u32)(uintptr_t)result;' \
              'WM_U32(WM_CONSUMER_RESULT) = 0u;')"

compile_run M1 -O2 "$m1"
compile_run M2 -O2 "$m2"
compile_run M3 -O2 "$m3"

test "$(<"$OUT/M1.rc")" != 0
rg -q '^ASSERTION cleanup_fresh_session_C894_zero$' "$OUT/M1.stderr"
test "$(<"$OUT/M2.rc")" != 0
rg -q '^ASSERTION world_native_authority_published$' "$OUT/M2.stderr"
test "$(<"$OUT/M3.rc")" != 0
rg -q '^ASSERTION world_guest_authority_published$' "$OUT/M3.stderr"

echo 'W34N5 CERTIFICATE O0/O2/nonrecovering-UBSan PASS; strict test warnings clean'
echo 'M1-M3 DETECTED by named assertions'
