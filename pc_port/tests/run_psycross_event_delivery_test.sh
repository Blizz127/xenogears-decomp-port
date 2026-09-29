#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/psycross-events.XXXXXX")
echo "OUTPUT $OUT"
python3 - <<'PY'
import pathlib,hashlib,struct
b=pathlib.Path('disc/scph5500.bin').read_bytes()
assert hashlib.sha256(b).hexdigest()=='11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef'
for index,target in [(7,0x1b44),(8,0x1d8c),(9,0x1e1c),(10,0x1e44),(11,0x1ec8),(12,0x1f10),(13,0x1f4c),(32,0x1c5c)]:
    assert struct.unpack_from('<I',b,0x10374+index*4)[0]==target
print('PASS BIOS hash and B0 event vector pins')
PY
read -r -a SDL_FLAGS <<< "$(pkg-config --cflags sdl2)"
read -r -a SDL_LIBS <<< "$(pkg-config --libs sdl2)"
INC=(-Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx -Ipc_port/extern/PsyCross/src/psx -Ipc_port/src)
for mode in O0 O2 UBSan; do
    flags=(-"$mode");if [[ "$mode" == UBSan ]];then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);fi
    "${CXX:-g++}" -std=c++17 -fpermissive -w -ffunction-sections -fdata-sections "${flags[@]}" "${SDL_FLAGS[@]}" "${INC[@]}" -c pc_port/extern/PsyCross/src/psx/LIBAPI.C -o "$OUT/vendor.o"
    "${CC:-gcc}" -std=gnu17 "${flags[@]}" "${INC[@]}" -c pc_port/tests/psycross_event_delivery_test.c -o "$OUT/test.o"
    "${CC:-gcc}" -std=gnu17 "${flags[@]}" "${INC[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/cpu.o"
    "${CXX:-g++}" "${flags[@]}" -Wl,--gc-sections "$OUT/vendor.o" "$OUT/test.o" "$OUT/cpu.o" "${SDL_LIBS[@]}" -o "$OUT/$mode"
    timeout 10s "$OUT/$mode"
    "${CC:-gcc}" -std=gnu17 "${flags[@]}" "${SDL_FLAGS[@]}" "${INC[@]}" -c pc_port/tests/psycross_wait_event_test.c -o "$OUT/wait.o"
    "${CXX:-g++}" "${flags[@]}" -Wl,--gc-sections -Wl,--wrap=SDL_Delay "$OUT/vendor.o" "$OUT/wait.o" "${SDL_LIBS[@]}" -o "$OUT/$mode.wait"
    timeout 10s "$OUT/$mode.wait"
done
"${CC:-gcc}" -std=gnu17 -O2 "${SDL_FLAGS[@]}" "${INC[@]}" -c pc_port/tests/psycross_wait_event_test.c -o "$OUT/wait.o"
for mutant in early-wait-return retained-wait-latch; do
    case "$mutant" in
        early-wait-return) sed 's/while (!TestEvent(event)) SDL_Delay(0);/return 0;/' pc_port/extern/PsyCross/src/psx/LIBAPI.C > "$OUT/$mutant.C" ;;
        retained-wait-latch) sed 's/if (ready) e->pending = 0;/\/\* mutant: retained latch \*\//' pc_port/extern/PsyCross/src/psx/LIBAPI.C > "$OUT/$mutant.C" ;;
    esac
    "${CXX:-g++}" -std=c++17 -O2 -fpermissive -w -ffunction-sections -fdata-sections "${SDL_FLAGS[@]}" "${INC[@]}" -c "$OUT/$mutant.C" -o "$OUT/vendor.o"
    "${CXX:-g++}" -O2 -Wl,--gc-sections -Wl,--wrap=SDL_Delay "$OUT/vendor.o" "$OUT/wait.o" "${SDL_LIBS[@]}" -o "$OUT/$mutant"
    if timeout 5s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then echo "FAIL mutant survived: $mutant";exit 1;fi
    grep -q 'Assertion' "$OUT/$mutant.log"
    echo "REJECTED $mutant"
done
flags=(-O2)
"${CC:-gcc}" -std=gnu17 -O2 "${INC[@]}" -c pc_port/tests/psycross_event_delivery_test.c -o "$OUT/test.o"
"${CC:-gcc}" -std=gnu17 -O2 "${INC[@]}" -c pc_port/src/battle_mips_adapter.c -o "$OUT/cpu.o"
for mutant in lost-delivery stuck-latch wrong-match wrong-mode; do
    case "$mutant" in
        lost-delivery) sed 's/e->pending = 1;/e->pending = 0;/g' pc_port/extern/PsyCross/src/psx/LIBAPI.C > "$OUT/$mutant.C" ;;
        stuck-latch) sed 's/e->pending = 0;/e->pending = e->pending;/g' pc_port/extern/PsyCross/src/psx/LIBAPI.C > "$OUT/$mutant.C" ;;
        wrong-match) sed 's/e->desc != ev1 || e->spec != ev2/0/g' pc_port/extern/PsyCross/src/psx/LIBAPI.C > "$OUT/$mutant.C" ;;
        wrong-mode) sed 's/e->mode == 0x2000/e->mode == 0x1000/g' pc_port/extern/PsyCross/src/psx/LIBAPI.C > "$OUT/$mutant.C" ;;
    esac
    "${CXX:-g++}" -std=c++17 -O2 -fpermissive -w -ffunction-sections -fdata-sections "${SDL_FLAGS[@]}" "${INC[@]}" -c "$OUT/$mutant.C" -o "$OUT/vendor.o"
    "${CXX:-g++}" -O2 -Wl,--gc-sections "$OUT/vendor.o" "$OUT/test.o" "$OUT/cpu.o" "${SDL_LIBS[@]}" -o "$OUT/$mutant"
    rc=0
    timeout 3s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1 || rc=$?
    if [[ "$rc" == 0 ]];then echo "FAIL mutant survived: $mutant";exit 1;fi
    if [[ "$mutant" == lost-delivery ]];then
        [[ "$rc" == 124 ]] || grep -q 'Assertion' "$OUT/$mutant.log"
    else
        grep -q 'Assertion' "$OUT/$mutant.log"
    fi
    echo "REJECTED $mutant"
done
