#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

BUILD_DIR="${W34N53_BUILD_DIR:-$ROOT/pc_port/build_native/w34n53_capture_rows}"
CXX="${CXX:-g++}"
PROD="pc_port/extern/PsyCross/src/PsyX_main.cpp"
TEST="pc_port/tests/w34n53_capture_rows_prod_test.cpp"

mkdir -p "$BUILD_DIR"

BASE=(
    -std=gnu++17 -DUSE_EXTENDED_PRIM_POINTERS=0
    -include pc_port/src/port_compat.h
    -ffunction-sections -fdata-sections -fno-pie
    -Wall -Wextra -Werror
    -Wno-narrowing -Wno-format-security -Wno-unused-parameter
    -Wno-unused-variable -Wno-unused-function -Wno-sign-compare
    -Wno-write-strings -Wno-missing-field-initializers -Wno-parentheses
    -fpermissive
    -Ipc_port/extern/PsyCross/include
    -Ipc_port/extern/PsyCross/include/psx
    -Ipc_port/extern/PsyCross/src
    -I/usr/include/AL
)

read -r -a SDL_CFLAGS <<<"$(pkg-config --cflags sdl2)"

build_variant() {
    local name="$1"
    local optimization="$2"
    local sanitizer="$3"
    local define="${4:-}"
    local extra=()
    local prod_obj="$BUILD_DIR/$name-main.o"
    local test_obj="$BUILD_DIR/$name-test.o"
    local exe="$BUILD_DIR/$name"

    if [[ -n "$sanitizer" ]]; then
        extra+=("$sanitizer" -fno-sanitize-recover=all)
    fi
    if [[ -n "$define" ]]; then
        extra+=("-D$define")
    fi

    "$CXX" "${BASE[@]}" "${SDL_CFLAGS[@]}" "$optimization" "${extra[@]}" \
        -c "$PROD" -o "$prod_obj" || return 1
    "$CXX" -std=gnu++17 -ffunction-sections -fdata-sections -fno-pie \
        -Wall -Wextra -Werror "$optimization" "${extra[@]}" \
        -c "$TEST" -o "$test_obj" || return 1
    "$CXX" -no-pie -Wl,--gc-sections "${extra[@]}" \
        "$prod_obj" "$test_obj" -o "$exe" || return 1
    printf '%s\n' "$exe"
}

run_pass() {
    local name="$1"
    local optimization="$2"
    local sanitizer="${3:-}"
    local exe
    exe="$(build_variant "$name" "$optimization" "$sanitizer")"
    "$exe" >"$BUILD_DIR/$name.log" 2>&1
    rg -q '^W34N53 CAPTURE ROW CERTIFICATE PASS$' "$BUILD_DIR/$name.log"
    echo "W34N53 $name PASS"
}

run_mutant() {
    local name="$1"
    local define="$2"
    local assertion="$3"
    local exe
    local rc

    exe="$(build_variant "$name" -O0 '' "$define")"
    set +e
    "$exe" >"$BUILD_DIR/$name.log" 2>&1
    rc=$?
    set -e
    if [[ $rc -eq 0 ]]; then
        echo "W34N53 $name SURVIVED" >&2
        return 1
    fi
    rg -q "W34N53_ASSERT_FAIL:$assertion" "$BUILD_DIR/$name.log"
    echo "W34N53 $name DETECTED assertion=$assertion"
}

run_pass O0 -O0
run_pass O2 -O2
run_pass UBSan -O1 -fsanitize=undefined
run_mutant M1 W34N53_MUTANT_M1 even.outer_rows
run_mutant M2 W34N53_MUTANT_M2 even.interior_rows
run_mutant M3 W34N53_MUTANT_M3 even.pixel_stride

echo "W34N53 CAPTURE ROW CERTIFICATE PASS; O0/O2/nonrecovering UBSan; focused warnings clean; M1-M3 DETECTED"
