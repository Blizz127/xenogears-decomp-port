#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
misc5="$root/src/field/main/misc5.c"
misc9="$root/src/field/main/misc9.c"

# Both helpers retain the matching-build asm owner and provide a separate
# XENO_PC_PORT transcription.  This prevents a host-only implementation from
# silently changing the retail overlay build.
grep -q '^#ifdef XENO_PC_PORT$' "$misc5"
grep -q '^INCLUDE_ASM("asm/field/nonmatchings/main/misc5", func_800A7948);$' "$misc5"
grep -q '^#ifdef XENO_PC_PORT$' "$misc9"
grep -q '^INCLUDE_ASM("asm/field/nonmatchings/main/misc9", func_800AC99C);$' "$misc9"

# Retail's FE60 path is a bounded 0x687..0x18e2 frame window and uses the
# exact fixed regions/24-bit OT masks from 800A7948/800AC99C.
for needle in 'D_800B06A0 < 0x687' 'D_800B06A0 >= 0x18E2' '0xB580' '0x3674' '0x372C' '0x80F0' 'D_801E89E0 = 0' 'D_801E89E0 = 1' 'PsxMemory_GuestAddr(D_800ADB30)' '0xFFE2CFF8u'; do
    grep -Fq "$needle" "$misc5"
done
if grep -q 'transitionSize = archiveA9Size' "$misc5"; then
    echo 'title transition helper retail structure: FAIL (A9-size heap substitute)' >&2
    exit 1
fi
for needle in 'D_800AF770' 'D_800AF7F0' 'g_FieldCurRenderContextIndex' \
              '0x80D4' 'row < 0x1000' 'packet + row' '0x00FFFFFF' \
              'renderContextIndex * 0x30' 'renderContextIndex * 0x50' \
              'source + 0x6A' 'source + 0x7E' 'source + 0x92' \
              'source + 0xA6' 'prim += 0x0C'; do
    grep -Fq "$needle" "$misc9"
done

if grep -Eq 'g_FieldRenderContexts.*0x6BF0|storage \+ 0x8854|storage \+ 0x88D4' "$misc9"; then
    echo 'title transition helper retail structure: FAIL (false render-context-relative BSS offsets)' >&2
    exit 1
fi

echo 'title transition helper retail structure: PASS'
