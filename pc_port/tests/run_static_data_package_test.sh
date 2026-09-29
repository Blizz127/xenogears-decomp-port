#!/usr/bin/env bash
# D5 packaging regression: resolve the user's data beside the executable when
# launched from an unrelated working directory and with no XENO_* path hints.
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
DATA_DIR="${XENO_PACKAGE_DATA_DIR:-$ROOT/disc}"
OUT="${XENO_PACKAGE_TEST_OUTDIR:-$ROOT/pc_port/build_native/static_package_$(date +%Y%m%d_%H%M%S)}"
SECONDS_TO_RUN="${XENO_PACKAGE_TEST_SECONDS:-90}"

[ -x "$BIN" ] || { echo "FAIL: no port binary at $BIN"; exit 1; }
[ -d "$DATA_DIR" ] || { echo "FAIL: no retail data directory at $DATA_DIR"; exit 1; }
[ ! -e "$OUT" ] || { echo "FAIL: output path already exists: $OUT"; exit 1; }

INSTALL="$OUT/install"
DISC="$INSTALL/disc"
CWD="$OUT/unrelated/cwd"
mkdir -p "$DISC" "$CWD"
cp "$BIN" "$INSTALL/xeno-port"

# Link the owner's local files into the package layout; never copy retail game
# bytes into the repository or its package test output.
for file in "$DATA_DIR"/*; do
    [ -e "$file" ] || continue
    ln -s "$file" "$DISC/$(basename "$file")"
done
[ -e "$DISC/SLUS_006.64" ] || { echo "FAIL: package is missing SLUS_006.64"; exit 1; }
[ -e "$DISC/disc1.bin" ] || { echo "FAIL: package is missing disc1.bin"; exit 1; }

cd "$CWD"
( sleep 25; echo 1 > bw.txt ) &
env -u XENO_DATA_DIR -u XENO_SLUS -u XENO_DISC -u XENO_BIOS \
    -u XENO_RETAIL_DATA_OPTIONAL \
    SDL_VIDEODRIVER=offscreen XENO_MODS_ENABLED=0 \
    XENO_FIELD_TEST=1 XENO_KERNEL_SEL=0 XENO_FIELD_MAP=1 \
    XENO_BATTLE_WARP_FILE=bw.txt \
    timeout "$SECONDS_TO_RUN" "$INSTALL/xeno-port" >"$OUT/run.log" 2>&1
rc=$?

fail=0
grep -Fq "main-exe static data loaded from $DISC/SLUS_006.64" "$OUT/run.log" || {
    echo "FAIL: main-exe data did not come from the executable's package directory"
    fail=1
}
grep -Fq "main-exe static data stays zero-filled" "$OUT/run.log" && {
    echo "FAIL: main-exe static data stayed zero-filled"
    fail=1
}
grep -Fq "enter retail battle.bin" "$OUT/run.log" || {
    echo "FAIL: battle was not entered"
    fail=1
}
grep -Eq "invalid/cyclic DMA chain|battle-mips\] stopped" "$OUT/run.log" && {
    echo "FAIL: battle stopped before the deadline"
    fail=1
}
[ "$rc" = 124 ] || { echo "FAIL: run ended rc=$rc before the deadline"; fail=1; }

if [ "$fail" -eq 0 ]; then
    echo "PASS: executable-relative package data loaded and battle ran for ${SECONDS_TO_RUN}s"
    echo "local evidence: $OUT/run.log"
fi
exit "$fail"
