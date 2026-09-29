#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/disc-validation.XXXXXX")
echo "OUTPUT $OUT"
echo '82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d  disc/menu.bin' | sha256sum -c -
awk '/^s32 func_801E93A0\(/ {body=1} body {print} /^}/ {body=0}' src/menu/main/misc.c > "$OUT/disc_validation.inc"
if [[ ! -s "$OUT/disc_validation.inc" ]]; then
    # Existing generated stub behavior, for the pre-implementation RED witness.
    echo 's32 func_801E93A0(s32 disc) { (void)disc; return 0; }' > "$OUT/disc_validation.inc"
fi
BASE=(-std=gnu17 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -I"$OUT" -Ipc_port/src -Iinclude)
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${flags[@]}" pc_port/tests/disc_validation_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mode"
    timeout 30s "$OUT/$mode"
done
cp "$OUT/disc_validation.inc" "$OUT/good.inc"
for mutant in lid-bit ready-result seek-error header-magic disc-truncation table-size missing-sync debug-disc; do
    case "$mutant" in
        lid-bit) sed 's/\& 0x10/\& 0x20/g' "$OUT/good.inc" > "$OUT/disc_validation.inc" ;;
        ready-result) sed 's/ || result == 0//' "$OUT/good.inc" > "$OUT/disc_validation.inc" ;;
        seek-error) sed 's/\& 0x40/\& 0x20/' "$OUT/good.inc" > "$OUT/disc_validation.inc" ;;
        header-magic) sed 's/0x4E45585F/0x4E45585E/' "$OUT/good.inc" > "$OUT/disc_validation.inc" ;;
        disc-truncation) sed 's/(u32)disc + 0x30u/(u8)(disc + 0x30u)/' "$OUT/good.inc" > "$OUT/disc_validation.inc" ;;
        table-size) sed 's/0x8000/0x800/g' "$OUT/good.inc" > "$OUT/disc_validation.inc" ;;
        missing-sync) sed '/ArchiveCdDataSync(0);/d' "$OUT/good.inc" > "$OUT/disc_validation.inc" ;;
        debug-disc) sed 's/disc == 1/disc == 2/' "$OUT/good.inc" > "$OUT/disc_validation.inc" ;;
    esac
    ! cmp -s "$OUT/good.inc" "$OUT/disc_validation.inc"
    "${CC:-gcc}" "${BASE[@]}" -O2 pc_port/tests/disc_validation_test.c pc_port/src/battle_mips_adapter.c -o "$OUT/$mutant"
    if timeout 30s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
