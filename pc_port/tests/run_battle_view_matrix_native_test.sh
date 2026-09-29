#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib/sanitizer_preflight.sh"; ubsan_preflight
cd "$(dirname "$0")/../.."
ulimit -c 0
out=$(mkdir -p scratchpad && mktemp -d scratchpad/battle-view-matrix-native.XXXXXX)
echo "Evidence: $out"
read -r pin _ < <(sha256sum disc/battle.bin)
test "$pin" = 1830b4ef1fe37129972fc310dfad534f8161d6c0b123e74254c3711334a3e291
read -r pin _ < <(sha256sum disc/SLUS_006.64)
test "$pin" = dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119
sha256sum src/battle/mainc114.c src/slus_006.64/system/animation_scripts.c \
    pc_port/src/psyq_compat.c pc_port/src/battle_mips_adapter.c \
    pc_port/tests/battle_view_matrix_native_test.c \
    pc_port/extern/PsyCross/src/psx/LIBGTE.C \
    pc_port/extern/PsyCross/src/psx/INLINE_C.C \
    pc_port/extern/PsyCross/src/gte/PsyX_GTE.cpp > "$out/source-pins.txt"
common=(-fno-pie -DUSE_EXTENDED_PRIM_POINTERS=0 -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude -Ipc_port/src
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx)
for mode in O0 O2 UBSan O2Alias UBSanAlias; do
    case "$mode" in
        O0|O2) flags=(-"$mode");;
        UBSan) flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all);;
        O2Alias) flags=(-O2 -fno-strict-aliasing -DXBT_VIEW_ALIAS);;
        UBSanAlias) flags=(-O1 -fsanitize=undefined -fno-sanitize-recover=all -fno-strict-aliasing -DXBT_VIEW_ALIAS);;
    esac
    for source in psx/LIBGTE.C psx/INLINE_C.C gte/PsyX_GTE.cpp; do
        name=${source##*/}
        g++ -std=c++17 "${common[@]}" "${flags[@]}" -fpermissive -w \
            -include pc_port/src/port_compat.h -c "pc_port/extern/PsyCross/src/$source" -o "$out/$mode.$name.o"
    done
    for source in pc_port/tests/battle_view_matrix_native_test.c pc_port/src/battle_mips_adapter.c \
        pc_port/src/psyq_compat.c src/slus_006.64/system/animation_scripts.c src/battle/mainc114.c; do
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
    clang++ -no-pie "${flags[@]}" -Wl,--gc-sections "$out/$mode."*.o -o "$out/$mode"
    "$out/$mode" | tee "$out/$mode.log"
done
for control in 1 2 3 4 5; do
    cc -std=gnu17 "${common[@]}" -O2 -DXBT_OBSERVATION_CONTROL="$control" \
        -Wall -Wextra -Werror -c pc_port/tests/battle_view_matrix_native_test.c -o "$out/observe-$control.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$out/observe-$control.o" \
        "$out/O2.mainc114.c.o" "$out/O2.battle_mips_adapter.c.o" \
        "$out/O2.psyq_compat.c.o" "$out/O2.animation_scripts.c.o" \
        "$out/O2.LIBGTE.C.o" "$out/O2.INLINE_C.C.o" "$out/O2.PsyX_GTE.cpp.o" -o "$out/observe-$control"
    case "$control" in
        1) reason='GTE data state';;
        2) reason='GTE control state';;
        3) reason='matrix stack saved bytes';;
        4|5) reason='matrix/input/guards';;
    esac
    rc=0
    "$out/observe-$control" > "$out/observe-$control.log" 2>&1 || rc=$?
    test "$rc" = 1
    rg -q "^VIEW MATRIX FAIL case=0 $reason$" "$out/observe-$control.log"
    echo "VIEW MATRIX observation control rejected: $control ($reason)"
done
perl - "$out" <<'PERL'
use strict; use warnings;
open my $f,'<','src/battle/mainc114.c' or die $!; local $/; my $source=<$f>;
my @controls=(
 ['baseline','',''],
 ['cross-order','OuterProduct12(&vertical, &forward, &temp);','OuterProduct12(&forward, &vertical, &temp);'],
 ['wrong-row','matrix->m[1][1] = vertical.vy;','matrix->m[1][1] = vertical.vx;'],
 ['translation-sign','matrix->t[0] = 0u - (u32)temp.vx;','matrix->t[0] = (u32)temp.vx;'],
 ['missing-pop','    PopMatrix();','    /* Pop omitted by negative control. */'],
 ['missing-normal','VectorNormal(&temp, &right);','right = temp;']
);
for my $c (@controls) {
 my ($name,$old,$new)=@$c; my $s=$source;
 if(length $old) { my $n=($s =~ s/\Q$old\E/$new/g); die "$name replacements=$n" unless $n==1; }
 open my $o,'>',"$ARGV[0]/$name.c" or die $!; print $o $s; close $o;
}
PERL
for control in baseline cross-order wrong-row translation-sign missing-pop missing-normal; do
    cc -std=gnu17 "${common[@]}" -O2 -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
        -DXENO_BATTLE_VIEW_MATRIX_STAGED -include assert.h -include string.h \
        -DXENO_BATTLE_OVERLAY_HOST_BODIES -fpermissive -w -c "$out/$control.c" -o "$out/$control.o"
    clang++ -no-pie -O2 -Wl,--gc-sections "$out/$control.o" \
        "$out/O2.battle_view_matrix_native_test.c.o" "$out/O2.battle_mips_adapter.c.o" \
        "$out/O2.psyq_compat.c.o" "$out/O2.animation_scripts.c.o" \
        "$out/O2.LIBGTE.C.o" "$out/O2.INLINE_C.C.o" "$out/O2.PsyX_GTE.cpp.o" -o "$out/$control"
    rc=0
    "$out/$control" > "$out/$control.log" 2>&1 || rc=$?
    if [ "$control" = baseline ]; then
        test "$rc" = 0; cat "$out/$control.log"
    else
        test "$rc" = 1; rg -q '^VIEW MATRIX FAIL' "$out/$control.log"
        echo "VIEW MATRIX control rejected: $control"
    fi
done
