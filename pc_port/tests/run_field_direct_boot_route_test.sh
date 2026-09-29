#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${FIELD_DIRECT_BOOT_BUILD_DIR:-$ROOT/pc_port/build_native/field_direct_boot_route}"
CC_BIN="${CC:-cc}"

mkdir -p "$BUILD_DIR"
read -r selector_sha _ < <(dd if="$ROOT/disc/SLUS_006.64" bs=1 \
    skip=$((0x800198c0-0x8000f800)) count=$((0x18)) status=none | sha256sum)
test "$selector_sha" = 29ee614fc352b8704ea1eb96ff8247947619ef368a95196073a03f2300c0422d
rg -q 'D_8004FE45 = PcPort_SelectBootMovie\(ArchiveGetDiscNumber\(\)\);' "$ROOT/pc_port/src/port_main.c"
"$CC_BIN" -std=c17 -Wall -Wextra -Werror -Wconversion -Wsign-conversion \
    -I"$ROOT/pc_port/src" \
    "$ROOT/pc_port/tests/field_direct_boot_route_test.c" \
    -o "$BUILD_DIR/test"
"$BUILD_DIR/test"
mkdir -p "$BUILD_DIR/old-mapping"
sed 's/return discNumber == 1 ? 16u : 7u;/return (unsigned char)discNumber;/' \
    "$ROOT/pc_port/src/field_direct_boot_route.h" > "$BUILD_DIR/old-mapping/field_direct_boot_route.h"
"$CC_BIN" -std=c17 -Wall -Wextra -Werror -Wconversion -Wsign-conversion \
    -I"$BUILD_DIR/old-mapping" "$ROOT/pc_port/tests/field_direct_boot_route_test.c" \
    -o "$BUILD_DIR/old-mapping/test"
if "$BUILD_DIR/old-mapping/test" > "$BUILD_DIR/old-mapping/result.log" 2>&1; then
    echo 'BOOT MOVIE OLD MAPPING SURVIVED' >&2
    exit 1
fi
rg -q 'retail disc 1 boot must select movie 16' "$BUILD_DIR/old-mapping/result.log"
echo 'BOOT MOVIE SELECTOR NEGATIVE CONTROL PASS'
