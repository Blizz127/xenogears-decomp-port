#!/usr/bin/env bash
# Structural regression for the source-faithful FE60 title transition.
# This intentionally checks the retail authority and the host implementation
# together, so the former completion-flag-only shim cannot return unnoticed.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/src/field/main/misc5.c"
ELF="$ROOT/build/out/field.elf"

test -f "$SRC"
test -f "$ELF"

objdump_text="$(mips-linux-gnu-objdump -dr --start-address=0x800a2218 --stop-address=0x800a28d4 "$ELF")"
grep -q '<func_800A7C58>:' <<<"$objdump_text"
grep -q '340400a9' <<<"$objdump_text"       # ArchiveReadFileToBuffer(0xA9,...)
grep -q '34020140' <<<"$objdump_text"       # LoadImage rect x=0x140
grep -q '34020100' <<<"$objdump_text"       # rect width 0x100
grep -q '340200c0' <<<"$objdump_text"       # rect height 0xC0
grep -q '<StoreImage>' <<<"$objdump_text"
grep -q '<MoveImage>' <<<"$objdump_text"
grep -q '<PutDispEnv>' <<<"$objdump_text"
grep -q '<PutDrawEnv>' <<<"$objdump_text"
grep -Fq 'ArchiveCdSeekOrPause(D_800C3A20);' "$SRC"
if grep -Fq 'func_8002A2D0' "$SRC"; then
    echo 'title transition still calls an address-name stub instead of ArchiveCdSeekOrPause' >&2
    exit 1
fi

grep -q 'ArchiveReadFileToBuffer(0xA9, pArchiveA9, 0, 0x80)' "$SRC"
grep -q 'rect.x = 0x140' "$SRC"
grep -q 'rect.w = 0x100' "$SRC"
grep -q 'rect.h = 0xC0' "$SRC"
grep -q 'StoreImage(&rect, pArchiveA9)' "$SRC"
grep -q 'MoveImage(&rect, 0, 0)' "$SRC"
grep -q 'PutDispEnv' "$SRC"
grep -q 'PutDrawEnv' "$SRC"
grep -q 'D_800ADB7C = 1' "$SRC"
grep -q 'D_800ADB3C = 0x20' "$SRC"
grep -q 'D_800ADB38 = 1' "$SRC"

# func_800A7218's 13-argument STR launch must retain the retail o32 slot
# order. A previous transcription shifted the frame range, flags, and screen
# coordinates into unrelated slots, turning the long opening STR into a
# two-frame rewind loop after New Game.
movie_call="$({
    awk '
        /void func_800A7218\(void\)/ { in_owner = 1 }
        in_owner && /func_801D37CC\(/ { capture = 1 }
        capture { print }
        capture && /\);/ { exit }
    ' "$SRC"
} | tr -d '[:space:]')"
expected_movie_call='func_801D37CC(D_800C3A20+2,D_800C3A2A,D_800C3A2C,D_800C3A2E,1,fadeType,D_800C3A3A,D_800C3A22,D_800C3A24,D_800C3A26,D_800C3A28,0xE0,(void*)func_800A7120);'
test "$movie_call" = "$expected_movie_call"

# The STR callback's display-page selector is D_800ADB78.  Retail instructions
# 800A71D8..800A7204 mask that value and use the resulting RenderContext stride
# before writing g_FieldRenderContexts[page].dispEnv.isrgb24.  The ordinary
# field render index at 800ADB08 is a different variable; using it makes one
# movie page remain 15-bit and renders valid packed RGB24 bytes as rainbow
# noise whenever the two indices disagree.
movie_callback="$(awk '
    /void func_800A7120\(u16 arg0, s32 arg1, u16 arg2\)/ { capture = 1 }
    capture { print }
    capture && /^}/ { exit }
' "$SRC")"
if ! grep -Fq 'g_FieldCurRenderContext = &g_FieldRenderContexts[D_800ADB78];' \
        <<<"$movie_callback"; then
    echo 'field STR callback does not select the retail D_800ADB78 page' >&2
    exit 1
fi
if ! grep -Fq 'g_FieldCurRenderContext->dispEnv.isrgb24 = 1;' \
        <<<"$movie_callback"; then
    echo 'field STR callback does not mark its selected page as RGB24' >&2
    exit 1
fi
if grep -Fq 'g_FieldCurRenderContextIndex' <<<"$movie_callback"; then
    echo 'field STR callback uses the unrelated ordinary render-context index' >&2
    exit 1
fi

# Post-frame retail context sequence is context[1] -> context[0] (clear and
# MoveImage) -> context[1].
mapfile -t context_assignments < <(grep 'g_FieldCurRenderContext = &g_FieldRenderContexts\[' "$SRC")
test "${#context_assignments[@]}" -ge 3
test "${context_assignments[-3]}" = '    g_FieldCurRenderContext = &g_FieldRenderContexts[1];'
test "${context_assignments[-2]}" = '    g_FieldCurRenderContext = &g_FieldRenderContexts[0];'
test "${context_assignments[-1]}" = '    g_FieldCurRenderContext = &g_FieldRenderContexts[1];'

# The completion flag is written at the retail post-transition point; a
# separate D_800ADB60 write here would be an address-mapping regression.
if grep -q 'D_800ADB60 = 1' "$SRC"; then
    echo 'unexpected D_800ADB60 completion write' >&2
    exit 1
fi

# Reject the old immediate-completion implementation explicitly.
if grep -A8 -q 'Minimal port of func_800A7C58' "$SRC"; then
    echo 'title transition still uses the old completion-flag shim' >&2
    exit 1
fi
echo 'title transition retail structure: PASS'
