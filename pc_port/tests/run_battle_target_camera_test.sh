#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
runtime_mode=${XBT_CAMERA_RUNTIME_RUN:-0}
test "$runtime_mode" = 0 || test "$runtime_mode" = 1
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-target-camera.XXXXXX)
echo "Evidence: $out"
read -r pin _ < <(sha256sum disc/battle.bin)
test "$pin" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
read -r pin _ < <(sha256sum disc/SLUS_006.64)
test "$pin" = dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119
sha256sum pc_port/src/battle_target_camera.c pc_port/src/battle_target_camera.h \
    pc_port/src/battle_target_bounds.c src/battle/mainc114.c \
    src/slus_006.64/system/animation_scripts.c pc_port/src/retail_leaf_adapters.c \
    pc_port/src/psyq_compat.c pc_port/src/battle_mips_adapter.c \
    pc_port/tests/battle_target_camera_test.c pc_port/extern/PsyCross/src/psx/LIBGTE.C \
    pc_port/extern/PsyCross/src/psx/INLINE_C.C pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp \
    > "$out/source-pins.txt"
common=(-fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
link_flags=()
if [ "$runtime_mode" = 1 ]; then
    # Match the production map's ELF classification. Name-only inference can
    # bind libc functions (e.g. memchr) as data overlapping retail instructions.
    test -f build/out/slus_006.64.elf
    python3 tools/scripts/gen_battle_bridge_map.py \
        --elf build/out/slus_006.64.elf \
        --symbols config/symbol_addrs.slus_006.64.txt \
        --symbols linker/undefined_funcs_auto.battle.txt \
        --symbols linker/undefined_syms_auto.battle.txt --out "$out/battle_bridge_map.inc"
    common+=(-DXBT_CAMERA_RUNTIME -I"$out")
    # Only the tested data symbols need dlsym visibility. Exporting every
    # unrelated function in the full SDK/animation TUs would defeat section GC.
    link_flags=(-Wl,--export-dynamic-symbol=g_GameState \
        -Wl,--export-dynamic-symbol=D_800D30A0 \
        -Wl,--export-dynamic-symbol=D_800C3CDC \
        -Wl,--export-dynamic-symbol=D_800C3678 -ldl)
    sha256sum build/out/slus_006.64.elf pc_port/src/battle_mips_runtime.c tools/scripts/gen_battle_bridge_map.py \
        config/symbol_addrs.slus_006.64.txt linker/undefined_funcs_auto.battle.txt \
        linker/undefined_syms_auto.battle.txt > "$out/runtime-source-pins.txt"
fi
for mode in O0 O2 UBSan; do
    flags=(-"$mode"); if [ "$mode" = UBSan ]; then flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all); fi
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$out/$mode.$name.o"
    done
    for source in pc_port/tests/battle_target_camera_test.c pc_port/src/battle_mips_adapter.c \
        pc_port/src/psyq_compat.c src/slus_006.64/system/animation_scripts.c src/battle/mainc114.c \
        pc_port/src/retail_leaf_adapters.c pc_port/src/battle_target_bounds.c pc_port/src/battle_target_camera.c; do
        name=${source##*/}
        warnings=(-Wall -Wextra -Werror)
        case "$source" in
            pc_port/src/psyq_compat.c|src/slus_006.64/system/animation_scripts.c) warnings=(-fpermissive -w);;
            # src/battle TUs read battle guest RAM through the host-body
            # aliases (include/common.h -> battle_overlay_guest_ram.h).
            src/battle/*) warnings+=(-DXENO_BATTLE_OVERLAY_HOST_BODIES);;
        esac
        cc -std=gnu17 "${common[@]}" "${flags[@]}" -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
            -DXENO_BATTLE_VIEW_MATRIX_STAGED -include assert.h -include string.h \
            "${warnings[@]}" -c "$source" -o "$out/$mode.$name.o"
    done
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$mode."*.o "${link_flags[@]}" -o "$out/$mode"
    "$out/$mode" 2>"$out/$mode.stderr" | tee "$out/$mode.log" || { cat "$out/$mode.stderr"; exit 1; }
done
dependencies=("$out/O2.battle_mips_adapter.c.o" "$out/O2.psyq_compat.c.o" \
    "$out/O2.animation_scripts.c.o" "$out/O2.mainc114.c.o" \
    "$out/O2.retail_leaf_adapters.c.o" "$out/O2.battle_target_bounds.c.o" \
    "$out/O2.LIBGTE.C.o" "$out/O2.INLINE_C.C.o" "$out/O2.PsyX_GTE.cpp.o")
perl - "$out" <<'PERL'
use strict; use warnings;
open my $f,'<','pc_port/src/battle_target_camera.c' or die $!; local $/; my $s=<$f>;
my @controls=(
 ['baseline','',''],
 ['mask-width','0x800c3678u,4,mask','0x800c3678u,4,(uint16_t)mask'],
 ['height-gate','if(gate) continue;','if(!gate) continue;'],
 ['height-sign','(uint32_t)(uint16_t)point.vy-height','(uint32_t)(uint16_t)point.vy+height'],
 ['eye-y-sign','(uint32_t)(uint16_t)center->vy+(uint32_t)offset->vy','(uint32_t)(uint16_t)center->vy-(uint32_t)offset->vy'],
 ['distance-shift','signed32((product>>14)|((0u-(product>>31))<<18))','(int32_t)(product>>14)'],
 ['camera-store-width','0x800d30a4u,2,(uint16_t)eye.vz','0x800d30a4u,4,(uint16_t)eye.vz'],
 ['radius-branch','if(radius<120)','if(radius<100)']
);
for my $c (@controls) {
 my ($name,$old,$new)=@$c; my $copy=$s;
 if(length $old) { my $n=($copy =~ s/\Q$old\E/$new/g); die "$name replacements=$n" unless $n==1; }
 open my $o,'>',"$ARGV[0]/$name.c" or die $!; print $o $copy; close $o;
}
PERL
for control in baseline mask-width height-gate height-sign eye-y-sign distance-shift camera-store-width radius-branch; do
    cc -std=gnu17 "${common[@]}" -O2 -Wall -Wextra -Werror \
        -c "$out/$control.c" -o "$out/$control.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$out/$control.o" \
        "$out/O2.battle_target_camera_test.c.o" "${dependencies[@]}" "${link_flags[@]}" -o "$out/$control"
    rc=0
    "$out/$control" > "$out/$control.log" 2>&1 || rc=$?
    if [ "$control" = baseline ]; then
        test "$rc" = 0; cat "$out/$control.log"
    else
        reason='public RAM/state'
        if [ "$control" = height-gate ]; then reason='GTE data'; fi
        test "$rc" = 1; rg -q "^TARGET CAMERA FAIL case=[0-9]+ $reason$" "$out/$control.log"
        echo "TARGET CAMERA control rejected: $control"
    fi
done
for control in 1 2 3 4 5 6; do
    cc -std=gnu17 "${common[@]}" -O2 -Wall -Wextra -Werror \
        -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C -include assert.h \
        -DXBT_CAMERA_OBSERVATION="$control" -c pc_port/tests/battle_target_camera_test.c \
        -o "$out/observe-$control.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$out/observe-$control.o" \
        "$out/O2.battle_target_camera.c.o" "${dependencies[@]}" "${link_flags[@]}" -o "$out/observe-$control"
    rc=0
    "$out/observe-$control" > "$out/observe-$control.log" 2>&1 || rc=$?
    case "$control" in
        1|6) reason='public RAM/state';;
        2) reason='SDK stack bytes';;
        3) reason='GTE data';;
        4) reason='GTE control';;
        5) reason='architectural VZ0';;
    esac
    test "$rc" = 1; rg -q "^TARGET CAMERA FAIL case=0 $reason$" "$out/observe-$control.log"
    echo "TARGET CAMERA observation control rejected: $control ($reason)"
done
if [ "$runtime_mode" = 1 ]; then
    for control in RAW_SHARED SHADOW_WRITE; do
        cc -std=gnu17 "${common[@]}" -O2 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
            -DXBT_CAMERA_"$control" -include assert.h -Wall -Wextra -Werror \
            -c pc_port/tests/battle_target_camera_test.c -o "$out/$control.o"
        clang++ -no-pie -O2 -Wl,--gc-sections "$out/$control.o" \
            "$out/O2.battle_target_camera.c.o" "${dependencies[@]}" "${link_flags[@]}" -o "$out/$control"
        rc=0
        "$out/$control" > "$out/$control.log" 2>&1 || rc=$?
        reason='public RAM/state'
        if [ "$control" = SHADOW_WRITE ]; then reason='overlay placeholders untouched'; fi
        test "$rc" = 1; rg -q "^TARGET CAMERA FAIL case=[0-9]+ $reason$" "$out/$control.log"
        echo "TARGET CAMERA runtime control rejected: $control"
    done
fi
