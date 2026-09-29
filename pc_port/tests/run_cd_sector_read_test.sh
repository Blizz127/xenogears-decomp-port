#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/cd-sector-read.XXXXXX")
echo "OUTPUT $OUT"
SOURCE=pc_port/extern/PsyCross/src/psx/LIBCD.C
sed -n '/^typedef struct _VFILE/,/^enum ReadMode/{ /^enum ReadMode/!p; }' "$SOURCE" > "$OUT/vfile.inc"
sed -n '/^int CdRead(int sectors/,/^int CdSetDebug(/{ /^int CdSetDebug(/!p; }' "$SOURCE" > "$OUT/sector_read.inc"
for mode in O0 O2 UBSan ASan CXX; do
    flags=(-"$mode");compiler="${CC:-gcc}";language=(-std=gnu17)
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    if [[ "$mode" == ASan ]]; then flags=(-O1 -fsanitize=address -fno-omit-frame-pointer); fi
    if [[ "$mode" == CXX ]]; then flags=(-O2);compiler="${CXX:-g++}";language=(-x c++ -std=gnu++17); fi
    "$compiler" "${language[@]}" "${flags[@]}" -I"$OUT" -Ipc_port/extern/PsyCross/src pc_port/tests/cd_sector_read_test.c -o "$OUT/$mode"
    timeout 40s "$OUT/$mode"
done
cp "$OUT/vfile.inc" "$OUT/good_vfile.inc"
cp "$OUT/sector_read.inc" "$OUT/good_sector.inc"
for mutant in false-element-count ignored-data-short ignored-audio-short false-success; do
    cp "$OUT/good_vfile.inc" "$OUT/vfile.inc"
    cp "$OUT/good_sector.inc" "$OUT/sector_read.inc"
    case "$mutant" in
        false-element-count) sed 's/return bytes \/ _ElementSize;/return _ElementCount;/' "$OUT/good_vfile.inc" > "$OUT/vfile.inc" ;;
        ignored-data-short) sed 's/sizeof(Sector), 1, \&g_imageFile) != 1/sizeof(Sector), 1, \&g_imageFile) == SIZE_MAX/' "$OUT/good_sector.inc" > "$OUT/sector_read.inc" ;;
        ignored-audio-short) sed 's/sizeof(AudioSector), 1, \&g_imageFile) != 1/sizeof(AudioSector), 1, \&g_imageFile) == SIZE_MAX/' "$OUT/good_sector.inc" > "$OUT/sector_read.inc" ;;
        false-success) sed 's/return -1;/return 0;/' "$OUT/good_sector.inc" > "$OUT/sector_read.inc" ;;
    esac
    if cmp -s "$OUT/good_vfile.inc" "$OUT/vfile.inc" && cmp -s "$OUT/good_sector.inc" "$OUT/sector_read.inc"; then
        echo "FAIL mutant did not change source: $mutant"; exit 1
    fi
    "${CC:-gcc}" -std=gnu17 -O2 -I"$OUT" -Ipc_port/extern/PsyCross/src pc_port/tests/cd_sector_read_test.c -o "$OUT/$mutant"
    if timeout 40s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
