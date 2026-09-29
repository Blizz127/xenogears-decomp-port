#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/src/field/main/misc9.c"
data="$root/pc_port/src/data_field.c"
bin="$root/pc_port/build_native/xeno-port"
stubs="$root/pc_port/build_native/stubs.c"

test -f "$bin"
test -f "$stubs"

# The four FE60 routines must be owned by the port translation unit.  A
# generated fallback here would make the transition appear to work while
# silently skipping its packet construction/decoder.
for symbol in func_800AC0F0 func_800AC308 func_800AC3AC func_800AC99C; do
    if grep -Eq "^(long|unsigned char) ${symbol}(\\[|\\()" "$stubs"; then
        echo "generated fallback remains for $symbol" >&2
        exit 1
    fi
    nm -g --defined-only "$bin" | awk -v sym="$symbol" '$3 == sym {found=1} END {exit !found}'
done

# AC99C's MIPS loop uses 0x100-byte rows, 0x30-byte context records, 0x14-byte
# sprites, 0x0C-byte draw-mode packets, and 24-bit guest OT links.
for needle in 'row < 0x1000' 'idx * 0x30' \
              'idx * 0x50' \
              'strip += 0x14' 'prim += 0x0C' \
              'PsxMemory_GuestAddr(p)' 'AC99C_PACK_ADDR(strip)' \
              'phaseSource + 0x6A' 'phaseBase + 0x6A' \
              'phaseBase + 0x7E' 'phaseBase + 0x92' 'phaseBase + 0xA6'; do
    grep -Fq "$needle" "$src"
done

# ABFDC/AC0F0 must retain the retail decoder contract: custom glyphs report
# selector 1, BIOS glyph lookup takes the encoded code, and AC0F0 consumes a
# private 64-byte copy while starting with a fully transparent 0x120-byte
# upload buffer.
grep -Fq '*pOutIndex = 1;' "$src"
grep -Fq 'return Krom2RawAdd(code);' "$src"
grep -Fq 'return (intptr_t)PcPortKromFont(code, 30);' "$src"
grep -Fq 'intptr_t raw = func_800ABFDC' "$src"
grep -Fq 'u8 encoded[0x40];' "$src"
grep -Fq 'i < 0x40' "$src"
grep -Fq 'encoded[i] = ((u8*)pStream)[i];' "$src"
test "$(grep -Fc 'for (i = 0x11F; i >= 0; i--)' "$src")" -ge 2
test "$(grep -Fc 'image[i] = 0;' "$src")" -ge 2
grep -Fq 'move.w = 9;' "$src"
grep -Fq 'r * 9 + 0x380' "$src"

# AC3AC's second retail quad covers the bottom 24 scanlines, and SetDrawMode
# receives a NULL texture window.  An eighth SPRT tag copy would cross the
# 0x100-byte group boundary and corrupt the following group.
for needle in 'second->x0 = 0;' 'second->y0 = 0xC8;' 'second->x1 = 0x280;' \
              'second->y1 = 0xC8;' 'second->x2 = 0;' 'second->y2 = 0xE0;' \
              'second->x3 = 0x280;' 'second->y3 = 0xE0;'; do
    grep -Fq "$needle" "$src"
done
grep -Fq 'setRGB1(first, 0xFF, 0xFF, 0xFF);' "$src"
grep -Fq 'setRGB2(first, 0, 0, 0);' "$src"
grep -Fq 'setRGB0(second, 0, 0, 0);' "$src"
grep -Fq 'setRGB2(second, 0xFF, 0xFF, 0xFF);' "$src"
grep -Fq 'SetDrawMode((DR_MODE*)(packet + strip * 0x0C), 0, 0,' "$src"
grep -Fq 'page, NULL);' "$src"
grep -Fq 'SetDrawMode((DR_MODE*)(packet + 0x30 + strip * 0x0C), 0, 0,' "$src"
if grep -Fq '8 * 0x14' "$src"; then
    echo 'AC3AC crosses the retail 0x100-byte packet boundary' >&2
    exit 1
fi

# Retail's packet span is contiguous, but its 4-byte pointer slots cannot be
# overlaid at four-byte strides on the LP64 host.  Native pointers therefore
# have independent storage while the packet aliases retain exact PSX-relative
# offsets from D_800AF788.
grep -Fq 'void* D_800AF76C;' "$data"
grep -Fq 'void* D_800AF770;' "$data"
grep -Fq 'void* D_800AF774;' "$data"
grep -Fq 'void* D_800AF784;' "$data"
grep -Fq 'g_FieldTransitionPackets[0xD0]' "$data"
grep -q 'D_800AF7F0, g_FieldTransitionPackets, 0x068' "$data"
grep -q 'D_800AF824, g_FieldTransitionPackets, 0x09C' "$data"
if grep -Eq 'FIELD_BSS_ALIAS\(D_800AF7(6C|70|74|84)' "$data"; then
    echo 'native FE60 pointer slots overlap at retail 4-byte offsets' >&2
    exit 1
fi

base=$(nm -g --defined-only "$bin" | awk '$3 == "D_800AF788" {print "0x" $1}')
setup=$(nm -g --defined-only "$bin" | awk '$3 == "D_800AF7F0" {print "0x" $1}')
tail=$(nm -g --defined-only "$bin" | awk '$3 == "D_800AF824" {print "0x" $1}')
test -n "$base" -a -n "$setup" -a -n "$tail"
test $((setup - base)) -eq 104
test $((tail - base)) -eq 156

# Source spellings are structural tripwires, not behavioral proof. Execute
# pinned retail-derived tests on the current TU, including their mutants.
bash "$root/pc_port/tests/run_field_glyph_stream_ac0f0_retail_test.sh"
bash "$root/pc_port/tests/run_field_transition_packet_ac3ac_retail_test.sh"
bash "$root/pc_port/tests/run_field_transition_ot_ac99c_retail_test.sh"
echo 'FE60 transition packet dependency: PASS (structural and bounded behavior; not GPU acceptance)'
