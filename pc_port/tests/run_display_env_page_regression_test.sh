#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

CC="${CC:-gcc}"
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/xeno-display-page.XXXXXX")"
trap 'rm -rf "$BUILD_DIR"' EXIT

check_production_selector() {
    local file="$1"
    local body

    body="$(sed -n '/int PsyX_PresentDisplayFromVRAM(void)/,/void PsyX_EndScene/p' "$file")"
    if ! grep -Fq 'GR_PresentVRAMDisplay(currentDispEnv.disp.x, currentDispEnv.disp.y,' <<<"$body" ||
       ! grep -Fq 'currentDispEnv.disp.w, currentDispEnv.disp.h,' <<<"$body" ||
       ! grep -Fq 'currentDispEnv.isrgb24, !g_GPUDisabledState);' <<<"$body"; then
        echo "FAIL: $file does not present the submitted currentDispEnv" >&2
        return 1
    fi
    if grep -Fq 'GR_PresentVRAMDisplay(activeDispEnv' <<<"$body"; then
        echo "FAIL: $file still presents the one-page-lagged activeDispEnv" >&2
        return 1
    fi
}

check_display_mask_adapter() {
    local main_file="$1"
    local render_file="$2"
    local body

    body="$(sed -n '/int PsyX_PresentDisplayFromVRAM(void)/,/void PsyX_EndScene/p' "$main_file")"
    if ! grep -Fq 'currentDispEnv.isrgb24, !g_GPUDisabledState);' <<<"$body"; then
        echo "FAIL: $main_file ignores SetDispMask(0) in the no-scene presenter" >&2
        return 1
    fi
    if ! grep -Fq 'int rgb24, int displayEnabled)' "$render_file" ||
       ! grep -Fq 'if (!displayEnabled)' "$render_file"; then
        echo "FAIL: $render_file lacks the non-destructive black scanout adapter" >&2
        return 1
    fi
}

check_production_selector pc_port/extern/PsyCross/src/PsyX_main.cpp
check_production_selector pc_port/patches/psycross_display_present.patch
check_display_mask_adapter pc_port/extern/PsyCross/src/PsyX_main.cpp \
    pc_port/extern/PsyCross/src/render/PsyX_render.cpp
check_display_mask_adapter pc_port/patches/psycross_display_present.patch \
    pc_port/patches/psycross_display_present.patch

"$CC" -std=gnu17 -Wall -Wextra -Werror -O2 \
    pc_port/tests/display_env_page_regression_test.c -o "$BUILD_DIR/display_page"
"$BUILD_DIR/display_page"

"$CC" -std=gnu17 -Wall -Wextra -Werror -O2 \
    -DXENO_ACTIVE_DISPENV_MUTANT \
    pc_port/tests/display_env_page_regression_test.c -o "$BUILD_DIR/active_mutant"
if "$BUILD_DIR/active_mutant" >"$BUILD_DIR/mutant.log" 2>&1; then
    echo "FAIL: activeDispEnv negative-control mutant survived" >&2
    cat "$BUILD_DIR/mutant.log" >&2
    exit 1
fi

echo "activeDispEnv one-page-lag mutant: rejected"

"$CC" -std=gnu17 -Wall -Wextra -Werror -O2 \
    -DXENO_IGNORE_DISP_MASK_MUTANT \
    pc_port/tests/display_env_page_regression_test.c -o "$BUILD_DIR/mask_mutant"
if "$BUILD_DIR/mask_mutant" >"$BUILD_DIR/mask-mutant.log" 2>&1; then
    echo "FAIL: SetDispMask-ignore negative-control mutant survived" >&2
    cat "$BUILD_DIR/mask-mutant.log" >&2
    exit 1
fi

echo "SetDispMask-ignore mutant: rejected"
