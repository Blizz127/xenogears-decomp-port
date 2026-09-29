#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/cd-image-open.XXXXXX")
echo "OUTPUT $OUT"
ARGS=()
if [[ -n "${XENO_IMAGE_OPEN_TEST_DISC:-}" ]]; then ARGS=("$XENO_IMAGE_OPEN_TEST_DISC"); fi
SOURCE=pc_port/extern/PsyCross/src/psx/LIBCD.C
sed -n '/^typedef struct _VFILE/,/^enum ReadMode/{ /^enum ReadMode/!p; }' "$SOURCE" > "$OUT/vfile.inc"
sed -n '/^char g_cdImageBinaryFileName/,/^\/\/ utility function/{ /^\/\/ utility function/!p; }' "$SOURCE" > "$OUT/image_config.inc"
sed -n '/^int PsyX_CD_CheckImageAvailable()/,/^CdlFILE\* CdSearchFile/{ /^CdlFILE\* CdSearchFile/!p; }' "$SOURCE" > "$OUT/image_available.inc"
sed -n '/^void PsyX_CD_Shutdown(void)/,/^}/p' "$SOURCE" > "$OUT/image_shutdown.inc"
sed -n '/^int OpenBinaryImageFile()/,/^int CdLastCom(/{ /^int CdLastCom(/!p; }' "$SOURCE" > "$OUT/image_open.inc"
for mode in O0 O2 UBSan ASan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    if [[ "$mode" == ASan ]]; then flags=(-O1 -fsanitize=address -fno-omit-frame-pointer); fi
    "${CC:-gcc}" -std=gnu17 -I"$OUT" "${flags[@]}" pc_port/tests/cd_image_open_test.c -o "$OUT/$mode"
    (cd "$OUT"; timeout 20s "$OUT/$mode" "${ARGS[@]}")
done
# PsyCross's uppercase .C is compiled as C++ in production, not as C.
"${CXX:-g++}" -x c++ -std=gnu++17 -fpermissive -I"$OUT" -O2 pc_port/tests/cd_image_open_test.c -o "$OUT/CXX"
(cd "$OUT"; timeout 20s "$OUT/CXX" "${ARGS[@]}")
cp "$OUT/image_open.inc" "$OUT/good.inc"
cp "$OUT/image_config.inc" "$OUT/good_config.inc"
cp "$OUT/image_shutdown.inc" "$OUT/good_shutdown.inc"
for mutant in false-success repeated-open lost-close short-image missing-index ignored-read-error wrong-mode raw-availability stale-init-ready stale-shutdown-ready; do
    cp "$OUT/good.inc" "$OUT/image_open.inc"
    cp "$OUT/good_config.inc" "$OUT/image_config.inc"
    cp "$OUT/good_shutdown.inc" "$OUT/image_shutdown.inc"
    case "$mutant" in
        false-success) sed 's/return OpenBinaryImageFile();/OpenBinaryImageFile(); return 1;/' "$OUT/good.inc" > "$OUT/image_open.inc" ;;
        repeated-open) sed 's/!image.basePtr \&\& !image.fp/!image.basePtr/' "$OUT/good.inc" > "$OUT/image_open.inc" ;;
        lost-close) sed 's/fclose(image.fp);/(void)image.fp;/' "$OUT/good.inc" > "$OUT/image_open.inc" ;;
        short-image) sed 's/length < g_cdSectorSize/length < 0/' "$OUT/good.inc" > "$OUT/image_open.inc" ;;
        missing-index) sed 's/stage != 3/stage < 2/' "$OUT/good.inc" > "$OUT/image_open.inc" ;;
        ignored-read-error) sed 's/if (ferror(cueFp))/if (0)/' "$OUT/good.inc" > "$OUT/image_open.inc" ;;
        wrong-mode) sed 's/strcmp(second, "MODE2\/2352")/0/' "$OUT/good.inc" > "$OUT/image_open.inc" ;;
        raw-availability) sed 's/return g_xeno_cdImageReady \&\&/return 1 \&\&/' "$OUT/good_config.inc" > "$OUT/image_config.inc" ;;
        stale-init-ready) sed '/g_xeno_cdImageReady = 0;/d' "$OUT/good.inc" > "$OUT/image_open.inc" ;;
        stale-shutdown-ready) sed '/g_xeno_cdImageReady = 0;/d' "$OUT/good_shutdown.inc" > "$OUT/image_shutdown.inc" ;;
    esac
    if cmp -s "$OUT/good.inc" "$OUT/image_open.inc" && cmp -s "$OUT/good_config.inc" "$OUT/image_config.inc" && cmp -s "$OUT/good_shutdown.inc" "$OUT/image_shutdown.inc"; then
        echo "FAIL mutant did not change source: $mutant"; exit 1
    fi
    "${CC:-gcc}" -std=gnu17 -I"$OUT" -O2 pc_port/tests/cd_image_open_test.c -o "$OUT/$mutant"
    if (cd "$OUT"; timeout 20s "$OUT/$mutant") > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
