#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/cd-stream-pause.XXXXXX")
echo "OUTPUT $OUT"
SOURCE=pc_port/extern/PsyCross/src/psx/LIBCD.C
sed -n '/^typedef struct _VFILE/,/^enum ReadMode/{ /^enum ReadMode/!p; }' "$SOURCE" > "$OUT/vfile.inc"
sed -n '/^int _eCdSpoolerFunc()/,/^}/p' "$SOURCE" > "$OUT/stream.inc"
test -s "$OUT/stream.inc"
for mode in O0 O2 UBSan ASan CXX; do
    flags=(-"$mode");compiler="${CC:-gcc}";language=(-std=gnu17)
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    if [[ "$mode" == ASan ]]; then flags=(-O1 -fsanitize=address -fno-omit-frame-pointer); fi
    if [[ "$mode" == CXX ]]; then flags=(-O2);compiler="${CXX:-g++}";language=(-x c++ -std=gnu++17); fi
    "$compiler" "${language[@]}" "${flags[@]}" -I"$OUT" -Ipc_port/extern/PsyCross/src pc_port/tests/cd_stream_pause_test.c -o "$OUT/$mode"
    timeout 20s "$OUT/$mode"
done
cp "$OUT/stream.inc" "$OUT/good.inc"
for mutant in fallthrough locked-return lost-stop; do
    case "$mutant" in
        fallthrough) sed '/_xeno_cd_pause_boundary/{n;d;}' "$OUT/good.inc" | sed '/_xeno_cd_pause_boundary/{n;d;}' > "$OUT/stream.inc" ;;
        locked-return) sed '/_xeno_cd_pause_boundary/{n;d;}' "$OUT/good.inc" > "$OUT/stream.inc" ;;
        lost-stop) sed 's/g_cdReadDoneFlag = 1;/g_cdReadDoneFlag = 0;/' "$OUT/good.inc" > "$OUT/stream.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/stream.inc"
    "${CC:-gcc}" -std=gnu17 -O2 -I"$OUT" -Ipc_port/extern/PsyCross/src pc_port/tests/cd_stream_pause_test.c -o "$OUT/$mutant"
    if timeout 20s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
