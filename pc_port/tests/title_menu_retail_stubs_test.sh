#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/src/menu/main/misc.c"
elf="$root/build/out/menu.elf"

test -f "$src"
test -f "$elf"

# The title frame renderer must not be supplied by generated INCLUDE_ASM
# stubs in the PC path.  Keep the matching build's retail owner intact.
for fn in func_801CF37C func_801CF5E4 func_801CF8D8 func_801CFB48 func_801CFF64 func_801D02D8; do
    grep -q "INCLUDE_ASM(\"../asm/menu/nonmatchings/main/misc\", $fn);" "$src"
    grep -q "#ifndef XENO_PC_PORT" "$src"
done

# Each implementation is required to retain the source-visible retail
# primitive/data operations.  These checks are intentionally structural: they
# reject no-op or host-only replacements without pretending that visual title
# art has been proven here.
for needle in \
    'func_801CE2B4(0x20' \
    'func_8002675C' \
    'GetClut' \
    'RotTransPers4' \
    'RotTransPers3' \
    'func_801CE2B4(2' \
    'func_801CE2B4(0xC' \
    'func_801CE2B4(0xB' \
    'func_801CE2B4(0x10' \
    'AddPrim(&g_Menu->pGfxEnv->ot[4]'; do
    grep -Fq "$needle" "$src"
done

# The retail binary still serves as the dependency authority for the six
# addresses and must contain their non-empty bodies.
for addr in 801CF37C 801CF5E4 801CF8D8 801CFB48 801CFF64 801D02D8; do
    grep -q "<func_$addr>" <(mips-linux-gnu-objdump -d "$elf")
done

# Check the retail control-flow/data details which are easy to lose while
# replacing INCLUDE_ASM on the PC path.  Extract each implementation through
# its matching #endif so similarly named code elsewhere cannot satisfy these.
body() {
    sed -n "/^void $1/,/^#endif/p" "$src"
}

cf37c="$(body func_801CF37C)"
grep -Fq '((u8*)g_Menu)[0x4D8] != 2' <<<"$cf37c"
grep -Fq 'i != D_801E981C[selected]' <<<"$cf37c"
grep -Fq 'slot + 0x140 + g_Menu->renderContext * 12' <<<"$cf37c"
grep -Fq '(long*)(line + 0x14)' <<<"$cf37c"
if grep -Fq 'D_80059460' <<<"$cf37c"; then
    echo 'func_801CF37C still uses the non-retail global state' >&2
    exit 1
fi

cf5e4="$(body func_801CF5E4)"
grep -Fq 'for (s3 = 0; s3 < data[i * 0x200 + 0xB97]; s3++, slot++)' <<<"$cf5e4"
grep -Fq 'if (entry[0x58] == 0) continue;' <<<"$cf5e4"
grep -Fq 'poly[0x15] = source;' <<<"$cf5e4"
grep -Fq 'poly[0x1D] = (u8)(source + 0x10);' <<<"$cf5e4"

cf8d8="$(body func_801CF8D8)"
grep -Fq '((u8*)g_Menu)[0x4D8] == 0' <<<"$cf8d8"
grep -Fq '0x1E + i * 0x90' <<<"$cf8d8"
grep -Fq '0x1B + i * 0x90' <<<"$cf8d8"
grep -Fq '? 0x115 : 0x122' <<<"$cf8d8"

cfb48="$(body func_801CFB48)"
grep -Fq '!((u8*)g_Menu)[0x4D8]' <<<"$cfb48"
grep -Fq 'u8 menuState = ((u8*)g_Menu)[0x4D8];' <<<"$cfb48"
grep -Fq 'D_801E981C[*(s32*)(data + 0x4F7C)] == i' <<<"$cfb48"
grep -Fq 'slot + 0x50 + g_Menu->renderContext * 24' <<<"$cfb48"
if grep -Fq 'D_80059460' <<<"$cfb48"; then
    echo 'func_801CFB48 still uses the non-retail global state' >&2
    exit 1
fi

d02d8="$(body func_801D02D8)"
grep -Fq 'if (!data[0x2DBC])' <<<"$d02d8"
grep -Fq '} else {' <<<"$d02d8"
grep -Fq 'poly[0xA0C] = (u8)value;' <<<"$d02d8"
grep -Fq 'for (i = 0; i < 3; i++)' <<<"$d02d8"
grep -Fq 'u8* meta = data + i * 0x87C;' <<<"$d02d8"
grep -Fq 'u8* slot = meta + 0xA98;' <<<"$d02d8"
grep -Fq 'func_801CE2B4(0xB, data + 0x240C' <<<"$d02d8"

echo 'title menu retail stubs: PASS'
