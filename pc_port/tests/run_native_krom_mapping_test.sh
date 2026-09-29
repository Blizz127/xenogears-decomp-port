#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/native-krom-mapping.XXXXXX")
echo "OUTPUT $OUT"
echo '11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef  disc/scph5500.bin' | sha256sum -c -
echo '82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d  disc/menu.bin' | sha256sum -c -
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" -std=gnu17 "${flags[@]}" -DTEST_NATIVE_KROM -Ipc_port/src pc_port/tests/bios_krom_mapping_test.c pc_port/src/battle_mips_adapter.c "${KROM_MAPPING_SOURCE:-pc_port/src/krom_mapping.c}" -o "$OUT/$mode"
    timeout 60s "$OUT/$mode"
done
cp "${KROM_MAPPING_SOURCE:-pc_port/src/krom_mapping.c}" "$OUT/good.c"
for mutant in assumed-selector stale-span wrong-rebase short-bound overflow-bound; do
    case "$mutant" in
        assumed-selector) sed 's/if (!selector)/if (0)/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        stale-span) sed '/\*span = NULL;/d' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        wrong-rebase) sed 's/\*span = bios + (address - 0xBFC00000)/\*span = bios + (address - 0xBFC00000) + 1/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        short-bound) sed 's/length > 0xBFC80000 - address/length >= 0xBFC80000 - address/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        overflow-bound) sed 's/length > 0xBFC80000 - address/address + length > 0xBFC80000/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
    esac
    ! cmp -s "$OUT/good.c" "$OUT/$mutant.c"
    "${CC:-gcc}" -std=gnu17 -O2 -DTEST_NATIVE_KROM -Ipc_port/src pc_port/tests/bios_krom_mapping_test.c pc_port/src/battle_mips_adapter.c "$OUT/$mutant.c" -o "$OUT/$mutant"
    if timeout 60s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
for mutant in lost-scratch wrong-stride wrong-trail wrong-base wrong-bound lost-mask; do
    case "$mutant" in
        lost-scratch) sed 's/record = scratch_selector/record = 0/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        wrong-stride) sed 's/index \* 30/index * 32/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        wrong-trail) sed 's/trail >= 0x7F/trail >= 0x80/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        wrong-base) sed 's/base = 0xBFC69D68/base = 0xBFC69D6A/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        wrong-bound) sed 's/code < 0x84BF/code <= 0x84BF/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        lost-mask) sed '/code \&= 0xFFFF;/d' "$OUT/good.c" > "$OUT/$mutant.c" ;;
    esac
    ! cmp -s "$OUT/good.c" "$OUT/$mutant.c"
    "${CC:-gcc}" -std=gnu17 -O2 -DTEST_NATIVE_KROM -Ipc_port/src pc_port/tests/bios_krom_mapping_test.c pc_port/src/battle_mips_adapter.c "$OUT/$mutant.c" -o "$OUT/$mutant"
    if timeout 60s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q '^DIFF code=' "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
