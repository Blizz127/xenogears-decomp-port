#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${BATTLE_CDK_SPAWNERS_BUILD_DIR:-$ROOT/pc_port/build_native/battle_cdk_spawners}"
CC="${CC:-gcc}"
mkdir -p "$BUILD_DIR"
cd "$ROOT"

BATTLE_SHA="1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291"
actual="$(sha256sum disc/battle.bin | awk '{print $1}')"
if [[ "$actual" != "$BATTLE_SHA" ]]; then
    echo "ERROR: disc/battle.bin SHA-256 mismatch: $actual" >&2
    exit 1
fi
for slice in "800B5924 98" "800B5C18 a8" "800BF7C8 94"; do
    set -- $slice
    echo "retail func_$1 slice sha256: $(dd if=disc/battle.bin bs=1 \
        skip=$((0x$1 - 0x8006FAF0)) count=$((0x$2)) status=none | sha256sum | awk '{print $1}')"
done

BASE=(-std=gnu17 -fno-pie -no-pie -fno-builtin -fno-stack-protector
      -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
      -include assert.h -fpermissive -ffunction-sections -fdata-sections)
INC=(-Ipc_port/include_shim -Iinclude -Ipc_port/extern/PsyCross/include
     -Ipc_port/extern/PsyCross/include/psx -Ipc_port/src)

mapfile -t TUS < <(ls src/battle/main*.c)

build_and_run() {
    local name="$1"
    shift
    local linker="$CC"
    local sources=() flags=() objs=() a o
    for a in "$@"; do
        case "$a" in
            *.c) sources+=("$a") ;;
            *) flags+=("$a") ;;
        esac
    done
    for a in "${sources[@]}"; do
        o="$BUILD_DIR/$name.$(basename "$a").o"
        "$CC" "${BASE[@]}" "${INC[@]}" -w -fno-inline -fno-builtin -fno-ipa-ra \
            -fno-sanitize=object-size "${flags[@]}" -c "$a" -o "$o"
        # func_800B7424 is a landed body in main89.c; the test replaces it with a
        # recording stub, so weaken the TU's definition instead of stubbing twice.
        objcopy --weaken-symbol=func_800B7424 "$o" 2>/dev/null || true
        objs+=("$o")
    done
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/tests/battle_cdk_task_spawners_retail_test.c \
        -o "$BUILD_DIR/$name.test.o"
    "$CC" "${BASE[@]}" "${INC[@]}" -Wall -Wextra -Werror "${flags[@]}" \
        -c pc_port/src/battle_mips_adapter.c -o "$BUILD_DIR/$name.cpu.o"
    if [[ "$name" == "UBSan" ]] && command -v clang >/dev/null 2>&1; then
        linker=clang
    fi
    "$linker" -no-pie "${flags[@]}" "$BUILD_DIR/$name.test.o" \
        "$BUILD_DIR/$name.cpu.o" "${objs[@]}" -Wl,--gc-sections \
        -o "$BUILD_DIR/$name"
    "$BUILD_DIR/$name" >"$BUILD_DIR/$name.stdout" 2>"$BUILD_DIR/$name.stderr"
    test ! -s "$BUILD_DIR/$name.stderr"
}

build_and_run O0 "${TUS[@]}" -O0 -g
build_and_run O2 "${TUS[@]}" -O2
build_and_run UBSan "${TUS[@]}" -O1 -g \
    -fsanitize=undefined -fno-sanitize-recover=all

cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/O2.stdout"
cmp "$BUILD_DIR/O0.stdout" "$BUILD_DIR/UBSan.stdout"
rg -q '^BATTLE CDK SPAWNERS PASS checks=[0-9]+' "$BUILD_DIR/O0.stdout"

echo "BATTLE CDK SPAWNERS O0/O2/UBSAN PASS"

run_mutant() {
    local name="$1" fn="$2" expression="$3"
    local target sources=() s
    target="$(rg -l --glob 'src/battle/main*.c' "/\* ${fn}\\.s" src/battle 2>/dev/null | head -1)"
    if [[ -z "$target" ]]; then
        echo "MUTANT TARGET NOT FOUND: $name ($fn)" >&2
        exit 1
    fi
    sed "$expression" "$target" > "$BUILD_DIR/$name.c"
    if cmp -s "$target" "$BUILD_DIR/$name.c"; then
        echo "MUTANT PATTERN DID NOT APPLY: $name" >&2
        exit 1
    fi
    for s in "${TUS[@]}"; do
        if [ "$s" = "$target" ]; then
            sources+=("$BUILD_DIR/$name.c")
        else
            sources+=("$s")
        fi
    done
    set +e
    build_and_run "$name" "${sources[@]}" -O0 -g
    local rc=$?
    set -e
    if [[ "$rc" -eq 0 ]] || ! rg -q '^BATTLE CDK FAIL' "$BUILD_DIR/$name.stderr"; then
        echo "MUTANT NOT DETECTED: $name rc=$rc" >&2
        sed -n '1,20p' "$BUILD_DIR/$name.stderr" >&2 || true
        exit 1
    fi
    echo "  mutant rejected: $name"
}

run_mutant size_5924 func_800B5924 's/(\*(u32\*)(a0 + 0x6C), 0x18)/(*(u32*)(a0 + 0x6C), 0x1C)/'
run_mutant off_5c18 func_800B5C18 's/\*(s32\*)(s1 + 0x74)/*(s32*)(s1 + 0x78)/'
run_mutant mask_f7c8 func_800BF7C8 's/a0 + 0xAC) |= 0x20/a0 + 0xAC) |= 0x10/'
run_mutant flag_5dc4 func_800B5DC4 's/a0 + 0x34) = 1/a0 + 0x34) = 2/'
run_mutant arg_56e4 func_800B56E4 's/func_800C08CC(5,/func_800C08CC(6,/'
run_mutant cd_dc78 func_800BDC78 's/s0 + 0x88) = 0x10/s0 + 0x88) = 0x11/'
run_mutant idx_64d4 func_800B64D4 's/0x8C8C/0x8C90/'
run_mutant size_73a0 func_800B73A0 's/0x10F7C/0x10F80/'
run_mutant inc_bf5e8 func_800BF5E8 's/D_800C3CE8\[0\]++/D_800C3CE8[0] += 2/'
run_mutant off_b6438 func_800B6438 's/0x6C) + 0x1C/0x6C) + 0x20/'
run_mutant xor_7adf4 func_8007ADF4 's/\] \^= /] |= /'
run_mutant mul_b16a4 func_800B16A4 's/(q\[0\] + 1) \* 4/(q[0] + 1) * 8/'
run_mutant size_35c0 func_800B35C0 's/TimerWorkListAllocateTask(0, 0x24)/TimerWorkListAllocateTask(0, 0x28)/'
run_mutant store_7c28 func_800B7C28 's/D_800D2FDC\[0\] = 0;/D_800D2FDC[0] = 1;/'
run_mutant store_bcd8c func_800BCD8C 's/D_800C3D14\[0\] = 0;/D_800C3D14[0] = 1;/'
run_mutant arg_bf730 func_800BF730 's/D_800C3628\[0\] = v;/D_800C3628[0] = v + 1;/'
run_mutant zero_b8054 func_800B8054 's/D_800591B1\[0\] = 0;/D_800591B1[0] = 1;/'
run_mutant copy_b9b30 func_800B9B30 's/0x49\] = D_800C3610\[0\]\[0x1C\]/0x49] = D_800C3610[0][0x1D]/'
run_mutant store_8048 func_800B8048 's/D_800C3E1C\[0\] = v;/D_800C3E1C[0] = v + 1;/'
run_mutant source_7134 func_800B7134 's/D_800C3CB4\[0\] = g_GfxCurOT\[0\];/D_800C3CB4[0] = g_GfxCurOT[0] + 1;/'
run_mutant step_16f0 func_800B16F0 's/(n + 1) << 2/(n + 2) << 2/'
run_mutant flag_8068 func_800B8068 's/D_800591B1\[0\] = 1;/D_800591B1[0] = 2;/'
run_mutant slot_3588 func_800B3588 's/D_800C3548\[0\] = 0;/D_800C3548[0] = 1;/'
run_mutant free_c0f70 func_800C0F70 's/if (D_800C3A6C\[0\] != 0) {/if (D_800C3A6C[0] == 0) {/'
run_mutant arg_397c func_800B397C 's/a2 & 0xFF/a2 \& 0xFE/'
run_mutant counter_ede8 func_800BEDE8 's/D_80059464\[0\] = 0;/D_80059464[0] = 1;/'
run_mutant guard_f3a4 func_800BF3A4 's/if (D_800D3350\[0\] != 0) {/if (D_800D3350[0] == 0) {/'
run_mutant slot_3c2c func_800B3C2C 's/D_800C3560\[0\] = 0;/D_800C3560[0] = 1;/'
run_mutant size_bb7f8 func_800BB7F8 's/D_800C3674\[0\] = 0x200;/D_800C3674[0] = 0x201;/'
run_mutant step_f998 func_800BF998 's/if (a == 2) {/if (a == 3) {/'
run_mutant band_ebc4 func_800BEBC4 's/& 0x100) != 0/\& 0x200) != 0/'
run_mutant seed_8840 func_800B8840 's/D_800591A8\[0\] = 0x2000;/D_800591A8[0] = 0x2001;/'
run_mutant buf_8840 func_800B8840 's/GfxAllocateWorkBuffers(0x5000, 0);/GfxAllocateWorkBuffers(0x5001, 0);/'

echo 'BATTLE CDK SPAWNERS mutants rejected: size_5924 off_5c18 mask_f7c8 flag_5dc4 arg_56e4 cd_dc78 idx_64d4 size_73a0 inc_bf5e8 off_b6438 xor_7adf4 mul_b16a4 size_35c0 store_7c28 store_bcd8c arg_bf730 zero_b8054 copy_b9b30 store_8048 source_7134 step_16f0 flag_8068 slot_3588 free_c0f70 arg_397c counter_ede8 guard_f3a4 slot_3c2c size_bb7f8 step_f998 band_ebc4 seed_8840 buf_8840 dir_f9ec guard_f600 guard_beb04 idx_beb04 idx_bfba0 loop_b8774 inc_9258 guard_e0dc'
