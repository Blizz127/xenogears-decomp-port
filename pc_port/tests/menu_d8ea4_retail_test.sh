#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/src/menu/main/misc.c"
elf="$root/build/out/menu.elf"

test -f "$src"
test -f "$elf"

# Matching builds call these helpers before their definitions too, so their
# declarations must be visible before the first PC-only preprocessing block.
# Keep stdio itself inside that block.
pre_pc_prefix="$(sed -n '1,/^#ifdef XENO_PC_PORT$/p' "$src")"
grep -Fq 'static u8* MenuRawPointer(u32 offset);' <<<"$pre_pc_prefix"
grep -Fq 'static void MenuStoreRawPointer(u32 offset, void* pointer);' \
    <<<"$pre_pc_prefix"
if grep -Fq '#include <stdio.h>' <<<"$pre_pc_prefix"; then
    echo 'stdio escaped the native-port preprocessing guard' >&2
    exit 1
fi

# Retail authority: func_801D8EA4's menu.elf interval (from its symbol, now
# that menu.bin links every function at its retail address) directly calls
# only this closed dependency set.
read -r start size <<<"$(mips-linux-gnu-nm -S "$elf" | awk '$4 == "func_801D8EA4" {print $1, $2}')"
test -n "$start" && test -n "$size"
stop=$(printf '0x%x' $((0x$start + 0x$size)))
retail="$(mips-linux-gnu-objdump -d --start-address=0x$start \
    --stop-address=$stop "$elf")"
grep -Fq "$(printf '%x' $((0x$start))) <func_801D8EA4>:" <<<"$retail"

actual_callees="$(sed -n 's/.*jal[[:space:]][^<]*<\([^>]*\)>.*/\1/p' \
    <<<"$retail" | sort -u)"
expected_callees="$(printf '%s\n' \
    DrawSync GetAccessoryName GetTPage GetWeaponName HeapAlloc HeapFree \
    LoadImage SystemRenderStringEntry func_80033A2C func_80033A5C \
    func_801C851C func_801E7C50 | sort -u)"
test "$actual_callees" = "$expected_callees"

# Matching builds retain INCLUDE_ASM; the native port must own a substantive
# translation instead of falling through to a generated no-op stub.
grep -Fq 'INCLUDE_ASM("../asm/menu/nonmatchings/main/misc", func_801D8EA4);' "$src"
body="$(sed -n '/^void func_801D8EA4(s32 slotArg, s32 modeArg, s32 buildTextArg,/,/^#endif/p' "$src")"
test -n "$body"

# The existing call-site declaration is s32-shaped.  The native definition
# keeps that ABI and explicitly narrows all four inputs at the retail andi
# boundary instead of changing an out-of-scope declaration.
for narrow in \
    'u8 slot = (u8)slotArg;' \
    'u8 mode = (u8)modeArg;' \
    'u8 buildText = (u8)buildTextArg;' \
    'u8 renderSlot = (u8)renderSlotArg;'; do
    grep -Fq "$narrow" <<<"$body"
done

# High-risk retail invariants: one 0x3F6 scratch allocation paired with one
# free, the 5/4 row split, render buffer stride, image staging rectangle, and
# final ownership/render latches.
test "$(grep -Fc 'HeapAlloc(0x3F6, 0)' <<<"$body")" -eq 1
test "$(grep -Fc 'HeapFree(renderBuffer)' <<<"$body")" -eq 1
grep -Fq 'rowCount = 5;' <<<"$body"
grep -Fq 'rowCount = 4;' <<<"$body"
grep -Fq 'work + row * 0x80' <<<"$body"
grep -Fq 'rect.w = 0x28;' <<<"$body"
grep -Fq 'rect.h = 0xD;' <<<"$body"
grep -Fq 'MenuString nativeLine;' <<<"$body"
grep -Fq 'func_801E7C50(&nativeLine, row, 0xC, 0);' <<<"$body"
grep -Fq 'u8* work = MenuRawPointer(0x360);' <<<"$body"
if grep -Fq 'u8 upload;' <<<"$body"; then
    echo 'D8EA4 leaves the first upload decision indeterminate' >&2
    exit 1
fi
grep -Fq 'u8 upload = 0;' <<<"$body"
if grep -Fq 'if (name != NULL)' <<<"$body"; then
    echo 'D8EA4 adds a non-retail name resolver guard' >&2
    exit 1
fi
grep -Fq 'work[0x299] = (u8)g_Menu->renderContext;' <<<"$body"
grep -Fq 'g_Menu->pManager->unk4A[1] = 1;' <<<"$body"

# Every retail direct dependency must remain visible in the translation.
for callee in DrawSync GetAccessoryName GetTPage GetWeaponName HeapAlloc \
    HeapFree LoadImage SystemRenderStringEntry func_80033A2C func_80033A5C \
    func_801C851C func_801E7C50; do
    grep -Fq "$callee(" <<<"$body"
done

# The retail work pointers are three adjacent four-byte slots.  A host
# void** access is eight bytes and overlaps its neighbor on x64.
if grep -En '\*\(void\*\*\).*0x(358|35C|360)' "$src"; then
    echo 'overlapping host pointer access remains in a retail work slot' >&2
    exit 1
fi
if grep -En '\*\(u32\*\).*0x(358|35C|360)' "$src"; then
    echo 'inline raw-slot access bypasses the pointer ABI adapter' >&2
    exit 1
fi
for offset in 358 35C 360; do
    grep -Eq "(MenuRawPointer|MENU_RAW)\(0x$offset\)" "$src"
    grep -Eq "(MenuStoreRawPointer|MENU_STORE_RAW)\(0x$offset" "$src"
done

out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT
body_file="$out/d8ea4-body.inc"
support_file="$out/d8ea4-support.h"

# Compile the exact production function text in isolation.  This avoids
# linking unrelated menu bodies while ensuring the behavioral harness cannot
# drift into a copied implementation.
production_body="$(awk '/^void func_801D8EA4\(s32 slotArg, s32 modeArg, s32 buildTextArg,/,/^}/' "$src")"
printf '%s\n' "$production_body" > "$body_file"
source_body_hash="$(printf '%s' "$production_body" | sha256sum | cut -d' ' -f1)"
extracted_body="$(<"$body_file")"
extracted_body_hash="$(printf '%s' "$extracted_body" | sha256sum | cut -d' ' -f1)"
test "$source_body_hash" = "$extracted_body_hash"

{
    printf '%s\n' \
        '#include "common.h"' \
        '#include "main/game.h"' \
        '#include "system/memory.h"' \
        '#include "system/menu.h"' \
        'extern void* GetWeaponName(s32);' \
        'extern void* GetAccessoryName(s32);' \
        'extern void* func_80033A2C(s32);' \
        'extern void* func_80033A5C(s32);' \
        'extern s32 SystemRenderStringEntry(void*, void*, s32, s32);' \
        'extern u16 D_801E9D88[];' \
        'extern u8 D_801EA584[];' \
        'extern u16 D_801EA5D0[];' \
        'extern u16 g_SystemPalette1;' \
        'extern u16 g_SystemPalette2;' \
        'extern void func_801E7C50(MenuString*, s32, s32, s32);' \
        'extern void func_801C851C(SVECTOR*, s32, s32, s32, s32);'
    awk '/^static u8\* MenuRawPointer\(u32 offset\) \{$/,/^}/' "$src"
} > "$support_file"

cc_flags=(
    -std=gnu17 -fpermissive -DXENO_PC_PORT -DXENO_FIELD_OBJECT_OVERLAY
    -DSKIP_ASM -D_LANGUAGE_C -DUSE_EXTENDED_PRIM_POINTERS=0
    -include assert.h -w -O0 -g -m64 -fno-builtin
    -ffunction-sections -fdata-sections
    -Ipc_port/include_shim -Iinclude
    -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx
)
podman run --rm --userns=keep-id \
    -v "$root:/workspace:Z" -v "$out:/test-out:Z" -w /workspace \
    localhost/xenogears-dev-current:temp bash -lc \
    'gcc -x c "$@" -include /test-out/d8ea4-support.h \
         -c /test-out/d8ea4-body.inc -o /test-out/d8ea4.o &&
     gcc -c pc_port/tests/menu_d8ea4_retail_test.c "$@" -o /test-out/test.o &&
     gcc -Wl,--gc-sections /test-out/d8ea4.o /test-out/test.o \
         -o /test-out/test &&
     /test-out/test' bash "${cc_flags[@]}"

stubs="$root/pc_port/build_native/stubs.c"
if [[ -f "$stubs" && "$stubs" -nt "$src" ]]; then
    if grep -Eq '^(long|unsigned char) func_801D8EA4(\[|\()' "$stubs"; then
        echo 'fresh native build still owns D8EA4 through a generated stub' >&2
        exit 1
    fi
    echo 'menu D8EA4 native-link ownership: PASS'
else
    echo 'menu D8EA4 native-link ownership: NOT_RUN (build artifact is stale)'
fi

echo 'menu D8EA4 retail translation: PASS'
