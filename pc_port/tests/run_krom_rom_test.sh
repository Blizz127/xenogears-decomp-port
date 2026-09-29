#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
ulimit -c 0
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d "${TMPDIR:-/tmp}/krom-rom.XXXXXX")
echo "OUTPUT $OUT"
pkg-config --modversion libcrypto
echo '11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef  disc/scph5500.bin' | sha256sum -c -
for mode in O0 O2 UBSan ASan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    if [[ "$mode" == ASan ]]; then flags=(-O1 -fsanitize=address -fno-omit-frame-pointer); fi
    "${CC:-gcc}" -std=gnu17 -Wall -Wextra -Werror "${flags[@]}" -Ipc_port/src \
        pc_port/tests/krom_rom_test.c pc_port/src/krom_mapping.c \
        "${KROM_ROM_SOURCE:-pc_port/src/krom_rom.c}" $(pkg-config --cflags --libs libcrypto) \
        -Wl,--wrap=malloc,--wrap=free,--wrap=read,--wrap=__read_chk,--wrap=close -o "$OUT/$mode"
    timeout 60s "$OUT/$mode" "$OUT/input.bin"
done
cp "${KROM_ROM_SOURCE:-pc_port/src/krom_rom.c}" "$OUT/good.c"
for mutant in no-pin wrong-size stale-out lost-selector; do
    case "$mutant" in
        no-pin) sed 's/memcmp(digest, expected, sizeof expected) != 0/(memcmp(digest, expected, sizeof expected), 0)/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        wrong-size) sed 's/st.st_size != 0x80000/st.st_size != 0x80001/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        stale-out) sed '/\*out = NULL;/d' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        lost-selector) sed 's/rom->bytes, code, selector, length/rom->bytes, code, NULL, length/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
    esac
    ! cmp -s "$OUT/good.c" "$OUT/$mutant.c"
    "${CC:-gcc}" -std=gnu17 -O2 -Ipc_port/src pc_port/tests/krom_rom_test.c \
        pc_port/src/krom_mapping.c "$OUT/$mutant.c" $(pkg-config --cflags --libs libcrypto) \
        -Wl,--wrap=malloc,--wrap=free,--wrap=read,--wrap=__read_chk,--wrap=close -o "$OUT/$mutant"
    if timeout 60s "$OUT/$mutant" "$OUT/$mutant.bin" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    fi
    grep -q Assertion "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
