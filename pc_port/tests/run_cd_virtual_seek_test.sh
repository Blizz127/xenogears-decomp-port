#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/cd-virtual-seek.XXXXXX")
echo "OUTPUT $OUT"
sed -n '/^typedef struct _VFILE/,/^enum ReadMode/{ /^enum ReadMode/!p; }' pc_port/extern/PsyCross/src/psx/LIBCD.C > "$OUT/vfile.inc"
for mode in O0 O2 UBSan ASan CXX; do
    flags=(-"$mode");compiler="${CC:-gcc}";language=(-std=gnu17)
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    if [[ "$mode" == ASan ]]; then flags=(-O1 -fsanitize=address -fno-omit-frame-pointer); fi
    if [[ "$mode" == CXX ]]; then flags=(-O2);compiler="${CXX:-g++}";language=(-x c++ -std=gnu++17); fi
    "$compiler" "${language[@]}" "${flags[@]}" -I"$OUT" pc_port/tests/cd_virtual_seek_test.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/vfile.inc" "$OUT/good.inc"
for mutant in rejects-end lost-current invalid-origin narrowed-tell; do
    case "$mutant" in
        rejects-end) sed 's/(uint64_t)target > _Stream->size/(uint64_t)target >= _Stream->size/' "$OUT/good.inc" > "$OUT/vfile.inc" ;;
        lost-current) sed 's/origin = (int64_t)(current - base)/origin = 0/' "$OUT/good.inc" > "$OUT/vfile.inc" ;;
        invalid-origin) sed 's/else return -1;/else origin = 0;/' "$OUT/good.inc" > "$OUT/vfile.inc" ;;
        narrowed-tell) sed 's/return position < 0 || position > INT_MAX ? -1 : (int)position;/return (int)position;/' "$OUT/good.inc" > "$OUT/vfile.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/vfile.inc"
    "${CC:-gcc}" -std=gnu17 -O2 -I"$OUT" pc_port/tests/cd_virtual_seek_test.c -o "$OUT/$mutant"
    if timeout 30s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
