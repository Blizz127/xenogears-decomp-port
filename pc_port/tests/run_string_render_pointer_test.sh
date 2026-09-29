#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
OUT=$(mktemp -d "${TMPDIR:-/tmp}/string-render-pointer.XXXXXX")
echo "OUTPUT $OUT"
BASE=(-std=gnu17 -include assert.h -include stdint.h -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections -Ipc_port/src -Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for model in low-data high-data; do
    address_flags=(-fno-pie -no-pie)
    if [[ "$model" == high-data ]]; then address_flags=(-fPIE -pie -DEXPECT_HIGH_DATA); fi
for mode in O0 O2 UBSan; do
    flags=(-"$mode")
    if [[ "$mode" == UBSan ]]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    "${CC:-gcc}" "${BASE[@]}" "${address_flags[@]}" "${flags[@]}" -w -c "${STRING_RENDER_SOURCE:-src/slus_006.64/system/system.c}" -o "$OUT/$model.$mode.system.o"
    "${CC:-gcc}" "${BASE[@]}" "${address_flags[@]}" "${flags[@]}" pc_port/tests/string_render_pointer_test.c "$OUT/$model.$mode.system.o" -Wl,--gc-sections -o "$OUT/$model.$mode"
    echo "RUN $model $mode real string renderer"
    timeout 10s "$OUT/$model.$mode"
done
done
cp "${STRING_RENDER_SOURCE:-src/slus_006.64/system/system.c}" "$OUT/good.c"
for mutant in narrow-store narrow-saved narrow-nested raw-advance raw-work raw-rows; do
    case "$mutant" in
        narrow-store) sed 's/] = address;/] = (u32)address;/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        narrow-saved) sed 's/SYSTEM_TEXT_SET_ADDRESS(arg0, 0x20, SYSTEM_TEXT_ADDRESS(arg0, 0x1C))/SYSTEM_TEXT_SET_ADDRESS(arg0, 0x20, (u32)SYSTEM_TEXT_ADDRESS(arg0, 0x1C))/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        narrow-nested) sed 's/SYSTEM_TEXT_SET_ADDRESS(arg0, 0x1C, arg1)/SYSTEM_TEXT_SET_ADDRESS(arg0, 0x1C, (u32)(uintptr_t)arg1)/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        raw-advance) sed 's/SYSTEM_TEXT_SET_ADDRESS(window, 0x1C, SYSTEM_TEXT_ADDRESS(window, 0x1C) + (amount))/(*(u32*)((u8*)(window) + 0x1C) += (amount))/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        raw-work) sed 's/SYSTEM_TEXT_ADDRESS(pWindow, 0x2C)/(*(u32*)(pWindow + 0x2C))/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
        raw-rows) sed 's/SYSTEM_TEXT_ADDRESS(pWindow, 0x28)/(*(u32*)(pWindow + 0x28))/' "$OUT/good.c" > "$OUT/$mutant.c" ;;
    esac
    ! cmp -s "$OUT/good.c" "$OUT/$mutant.c"
    "${CC:-gcc}" "${BASE[@]}" -fPIE -pie -DEXPECT_HIGH_DATA -O2 -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    "${CC:-gcc}" "${BASE[@]}" -fPIE -pie -DEXPECT_HIGH_DATA -O2 pc_port/tests/string_render_pointer_test.c "$OUT/$mutant.o" -Wl,--gc-sections -o "$OUT/$mutant"
    if timeout 10s "$OUT/$mutant" > "$OUT/$mutant.log" 2>&1; then
        echo "FAIL mutant survived: $mutant"; exit 1
    else
        rc=$?
    fi
    [[ "$rc" == 134 || "$rc" == 139 ]]
    echo "REJECTED $mutant"
done
