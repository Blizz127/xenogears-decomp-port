#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
OUT=pc_port/build_native/field_overlay_init_retail_test
defines=()
if [ "${1:-}" = --uv ]; then
    OUT=pc_port/build_native/field_overlay_uv_retail_test
    defines=(-DTEST_OVERLAY_UV)
fi
mkdir -p "$OUT"
read -r routine_hash _ < <(dd if=disc/field.bin bs=1 skip=$((0x800a8ba4-0x8006faf0)) count=$((0x308)) status=none | sha256sum)
test "$routine_hash" = 0ad29bfc1b27aeafdff5fa8b850bc1485fe92167ed61bb93b380306c06fd69d3
read -r uv_hash _ < <(dd if=disc/field.bin bs=1 skip=$((0x800a8408-0x8006faf0)) count=$((0xb8)) status=none | sha256sum)
test "$uv_hash" = de23a6240991b148db8a4e979c7d3b1d7423754f932cb8fde494086bd5ebe25e
common=("${defines[@]}" -std=gnu17 -fno-pie -ffunction-sections -fdata-sections -include assert.h
    -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for opt in O0 O2 UBSan; do
    flags=(-"$opt")
    if [ "$opt" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    gcc "${common[@]}" "${flags[@]}" -Wall -Wextra -Werror -c pc_port/tests/field_overlay_init_retail_test.c -o "$OUT/$opt.test.o"
    for source in src/field/main/misc5.c src/field/main/misc4.c pc_port/src/data_field.c pc_port/src/battle_mips_adapter.c; do
        name=$(basename "$source")
        gcc "${common[@]}" "${flags[@]}" -fpermissive -w -c "$source" -o "$OUT/$opt.$name.o"
    done
    # Test-only asset boundary; leave the production initializer intact.
    objcopy --weaken-symbol=func_800A8314 "$OUT/$opt.misc5.c.o"
    clang -no-pie "${flags[@]}" -Wl,--gc-sections "$OUT/$opt."*.o -o "$OUT/$opt.test"
    "$OUT/$opt.test"
done
if [ "${1:-}" = --uv ]; then
    for mutant in page endpoint vertical; do
        case "$mutant" in
            page) expression='s/g_FieldCurRenderContextIndex \* 4/0/' ;;
            endpoint) expression='s/s32 x2 = x + w;/s32 x2 = x + w - 1;/' ;;
            vertical) expression='s/x, y2, x2, y2, x, y, x2, y/x, y, x2, y, x, y2, x2, y2/' ;;
        esac
        sed "/^void func_800A8408(/,/^}/ { $expression; }" src/field/main/misc5.c > "$OUT/$mutant.c"
        gcc "${common[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
        objcopy --weaken-symbol=func_800A8314 "$OUT/$mutant.o"
        clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" \
            "$OUT/O2.misc4.c.o" "$OUT/O2.data_field.c.o" "$OUT/O2.battle_mips_adapter.c.o" -o "$OUT/$mutant.test"
        if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
            echo "OVERLAY UV mutant survived: $mutant" >&2; exit 1
        fi
        rg -q 'OVERLAY UV FAIL' "$OUT/$mutant.log"
    done
    echo 'OVERLAY UV negative controls PASS page/endpoint/vertical-orientation'
    exit 0
fi
for mutant in count blend uv copy; do
    case "$mutant" in
        count) expression='s/i < 109/i < 108/' ;;
        blend) expression='s/else if (((flags >> 4) \& 15) == 1) blend = 2;/else blend = 2;/' ;;
        uv) expression='s/u + w - 1/u + w/' ;;
        copy) expression='s/j < 10/j < 9/' ;;
    esac
    sed "/^void func_800A8BA4(/,/^}/ { $expression; }" src/field/main/misc5.c > "$OUT/$mutant.c"
    gcc "${common[@]}" -O2 -fpermissive -w -c "$OUT/$mutant.c" -o "$OUT/$mutant.o"
    objcopy --weaken-symbol=func_800A8314 "$OUT/$mutant.o"
    clang -no-pie -Wl,--gc-sections "$OUT/$mutant.o" "$OUT/O2.test.o" \
        "$OUT/O2.misc4.c.o" "$OUT/O2.data_field.c.o" "$OUT/O2.battle_mips_adapter.c.o" -o "$OUT/$mutant.test"
    if "$OUT/$mutant.test" > "$OUT/$mutant.log" 2>&1; then
        echo "OVERLAY INIT mutant survived: $mutant" >&2; exit 1
    fi
    rg -q 'OVERLAY INIT FAIL' "$OUT/$mutant.log"
done
echo 'OVERLAY INIT negative controls PASS quad-count/blend-retention/UV-endpoint/copy-size'
