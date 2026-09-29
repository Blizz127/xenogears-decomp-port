#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT="${SPRITE_RESTORE_BUILD_DIR:-pc_port/build_native/sprite_snapshot_restore_pointer}"
mkdir -p "$OUT"
ulimit -c 0
read -r actual _ < <(dd if=disc/SLUS_006.64 bs=1 skip=$((0x12550)) \
    count=$((0x16c)) status=none | sha256sum)
test "$actual" = bb94bfdd989bd09720f7be46406f4855b62e957deb9eca24ea1c7f0af6e889b3
source_file=src/slus_006.64/system/animation_scripts.c
base=(-std=gnu17 -fno-pie -fno-inline -ffunction-sections -fdata-sections)
includes=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
    -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src
    -iquote src/slus_006.64/system)
build() {
    local name="$1" source="$2"; shift 2
    gcc "${base[@]}" "$@" "${includes[@]}" -w -fpermissive -DXENO_PC_PORT -DSKIP_ASM \
        -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0 -include assert.h \
        -c "$source" -o "$OUT/$name.body.o"
    objcopy --weaken-symbol=AnimScriptTick --weaken-symbol=func_800245D8 \
        "$OUT/$name.body.o"
    gcc "${base[@]}" "$@" -Wall -Wextra -Werror \
        -c pc_port/tests/sprite_snapshot_restore_pointer_test.c -o "$OUT/$name.test.o"
    clang -no-pie "$@" "$OUT/$name.test.o" "$OUT/$name.body.o" \
        -Wl,--gc-sections -o "$OUT/$name"
}
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [[ "$opt" == UBSan ]]; then
        flags=(-O2 -g -fsanitize=undefined -fno-sanitize-recover=all)
    fi
    build "$opt" "$source_file" "${flags[@]}"
    for mode in 1 2 3; do "$OUT/$opt" "$mode"; done
done
# Independently reintroduce each host-width load defect in a generated copy.
for slot in 20 7C; do
    sed "s/(void\*)(uintptr_t)\*(u32\*)((u8\*)arg0 + 0x$slot)/\*(void\*\*)((u8*)arg0 + 0x$slot)/g" \
        "$source_file" > "$OUT/wide-$slot.c"
    build "wide-$slot" "$OUT/wide-$slot.c" -O0 -g
    mode=1; if [[ "$slot" == 7C ]]; then mode=2; fi
    rc=0
    "$OUT/wide-$slot" "$mode" > "$OUT/wide-$slot.log" 2>&1 || rc=$?
    if [[ "$rc" != 139 ]]; then
        echo "SPRITE RESTORE wide-$slot control expected SIGSEGV, got $rc" >&2
        exit 1
    fi
    echo "SPRITE RESTORE wide-$slot negative control detected"
done
echo 'SPRITE RESTORE O0/O2/UBSan and independent pointer-width controls PASS'
