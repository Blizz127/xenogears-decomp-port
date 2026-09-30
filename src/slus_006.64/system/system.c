#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/memory.h"
#include "system/memory.h"
#include "system/controller.h"
#include "main/game.h"
#ifdef XENO_PC_PORT
#include "guest_prim_link.h"
#define SYSTEM_ADD_PRIM(ot, prim) PcPort_AddPrimDomainAware((ot), (prim))
#else
#define SYSTEM_ADD_PRIM(ot, prim) AddPrim((ot), (prim))
#endif


/* Retail reaches all of these absolutely (lui %hi / %lo), i.e. this TU saw
 * them as externs; a tentative definition here would make cc1 -G8 address
 * them gp-relative. The port still needs the storage. */
#ifdef XENO_PC_PORT
#define SYSTEM_BSS
#else
#define SYSTEM_BSS extern
#endif
SYSTEM_BSS void* g_SystemDataFile;
SYSTEM_BSS void* g_SystemFontFile;
SYSTEM_BSS void* g_SystemDataEntries;
SYSTEM_BSS u32 D_8005934C;
SYSTEM_BSS u32 D_80059350;
SYSTEM_BSS u32 D_80059354;
SYSTEM_BSS u32 D_80059358;
SYSTEM_BSS u32 D_8005935C;
SYSTEM_BSS u32 D_80059364;
/* Real retail data already defined in asm/slus_006.64/data/3F290.sdata.s
 * (24 bytes, verified byte-identical to the values previously duplicated
 * here); a second C definition of the same symbol caused a link-time
 * "multiple definition" error against the .sdata original. */
extern u16 D_800501D0[];

extern u8 g_SystemPaletteData[];
extern s16 g_SystemPalette1;
extern s16 g_SystemPalette2;

void func_80031798(void* ot, void* prim);
#ifdef XENO_PC_PORT
void func_80033DD4(void* arg0, void* arg1);
/* The synchronous string renderer owns one packed retail descriptor. Keep
 * its full host pointers separately: stack strings/work buffers need not be
 * below 4GB, and widening the packed slots would overwrite adjacent fields.
 * Ordinary heap-backed windows retain their existing four-byte addresses.
 * Slots correspond to 1C,20,24(unused),28,2C. Lifetime/non-reentrancy is the
 * same as the retail static descriptor, not a registry of window pointers. */
static unsigned char s_StringRenderDescriptor[0x100];
static uintptr_t s_StringRenderPointers[5];
static uintptr_t SystemTextAddress(u8* window, unsigned offset) {
    if (window == s_StringRenderDescriptor) {
        return s_StringRenderPointers[(offset - 0x1C) / 4];
    }
    return *(u32*)(window + offset);
}
static void SystemTextSetAddress(u8* window, unsigned offset, uintptr_t address) {
    if (window == s_StringRenderDescriptor) {
        s_StringRenderPointers[(offset - 0x1C) / 4] = address;
    }
    *(u32*)(window + offset) = (u32)address;
}
#define SYSTEM_TEXT_ADDRESS(window, offset) SystemTextAddress((u8*)(window), (offset))
#define SYSTEM_TEXT_SET_ADDRESS(window, offset, address) \
    SystemTextSetAddress((u8*)(window), (offset), (uintptr_t)(address))
#else
void func_80033DD4(void* arg0, s32 arg1);
#define SYSTEM_TEXT_ADDRESS(window, offset) (*(u32*)((u8*)(window) + (offset)))
#define SYSTEM_TEXT_SET_ADDRESS(window, offset, address) \
    (*(u32*)((u8*)(window) + (offset)) = (u32)(address))
#endif
#define SYSTEM_TEXT_ADVANCE(window, amount) \
    SYSTEM_TEXT_SET_ADDRESS(window, 0x1C, SYSTEM_TEXT_ADDRESS(window, 0x1C) + (amount))
void func_80033DF0(void* arg0);
int func_80034F98(s32 arg0, s32 arg1);
void func_80034FFC(s32 arg0, s32 arg1, void* arg2, s32 arg3, s32 arg4);
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80032F54);
#else
void func_80032F54(void* arg0, s32 tpageX, s32 tpageY, s32 x, s32 y, s32 width, s32 mode, s32 height) {
    u8* pWindow = arg0;
    s32 i;
    s32 textureUBase;
    s32 rowStride;
    u8* rowPrim;
    s16 tpage0;
    s16 tpage1;

    (void)mode;

    /* Retail stores/reloads signed halfword coordinates and row count.
     * Normalizing the texture coordinates also keeps later arithmetic in
     * range; their retained UV/RECT fields use only the same low bits. */
    tpageX = (s16)tpageX;
    tpageY = (s16)tpageY;
    x = (s16)x;
    y = (s16)y;
    height = (s16)height;

    *(s16*)(pWindow + 0x04) = x;
    *(s16*)(pWindow + 0x10) = 0;
    *(s16*)(pWindow + 0x84) = 0;
    *(s32*)(pWindow + 0x8C) = 0;
    *(s16*)(pWindow + 0x82) = 0;
    *(s16*)(pWindow + 0x14) = 0x0E;
    *(u8*)(pWindow + 0x68) = 1;
    *(u8*)(pWindow + 0x69) = 1;
    *(s16*)(pWindow + 0x0A) = width;
    *(u8*)(pWindow + 0x6C) = 0;
    *(u8*)(pWindow + 0x6A) = 0;
    *(u8*)(pWindow + 0x6D) = 0;
    *(u8*)(pWindow + 0x6B) = 0;
    *(u8*)(pWindow + 0x6E) = 0xFF;
    *(s16*)(pWindow + 0x0E) = tpageY;
    *(s16*)(pWindow + 0x06) = y;
    *(s16*)(pWindow + 0x0C) = height;
    *(s16*)(pWindow + 0x0A) = *(u16*)(pWindow + 0x0A) | 1;
    *(u16*)(pWindow + 0x08) = (u32)*(u16*)(pWindow + 0x0A) << 2;
    *(s16*)(pWindow + 0x12) = *(u16*)(pWindow + 0x0A) + 3;

    HeapSetCurrentContentType(0x29);
    *(u32*)(pWindow + 0x28) = (u32)(uintptr_t)HeapAlloc(height * 0x60, 2);
    HeapSetCurrentContentType(0x28);
    *(u32*)(pWindow + 0x2C) = (u32)(uintptr_t)HeapAlloc(*(s16*)(pWindow + 0x12) * 0x1C, 2);

    *(u8*)(pWindow + 0x4B) = 3;
    *(u32*)(pWindow + 0x4C) = 0x60000000;
    *(u32*)(pWindow + 0x50) = ((u32)(y - 5) << 16) | (u32)(x - 7);
    *(u32*)(pWindow + 0x54) = ((u32)(height * 0x0E + 0x0A) << 16) | (u32)(*(s16*)(pWindow + 0x0A) * 4 + 0x0D);
    SetSemiTrans(pWindow + 0x48, 1);

    *(u32*)(pWindow + 0x58) = *(u32*)(pWindow + 0x48);
    *(u32*)(pWindow + 0x5C) = *(u32*)(pWindow + 0x4C);
    *(u32*)(pWindow + 0x60) = *(u32*)(pWindow + 0x50);
    *(u32*)(pWindow + 0x64) = *(u32*)(pWindow + 0x54);

    textureUBase = (tpageX & 0x3F) << 2;
    rowStride = *(s16*)(pWindow + 0x14);
    rowPrim = (u8*)(uintptr_t)*(u32*)(pWindow + 0x28);
    for (i = 0; i < height; i++, rowPrim += 0x60) {
        s32 pairIndex = i / 2;
        s32 oddRow = i & 1;
        s32 textureX = tpageY + pairIndex * 0x0D;
        u16 uv = textureUBase | ((textureX & 0xFF) << 8);
        u32 tpageLeft = (*(s16*)(pWindow + 0x08) < 0x101) ? (0x000D0000u | (u32)(s32)*(s16*)(pWindow + 0x08)) : 0x000D0100;
        u32 tpageRight = (*(s16*)(pWindow + 0x08) < 0x101) ? 0x000D0000 : (0x000D0000 | (*(s16*)(pWindow + 0x08) - 0xF0));

        *(u32*)(rowPrim + 0x08) = ((u32)(y + rowStride * i) << 16) | (u32)x;
        *(u32*)(rowPrim + 0x1C) = ((u32)(y + rowStride * i) << 16) | (u32)(x + 0x100);
        *(u32*)(rowPrim + 0x10) = tpageLeft;
        *(u32*)(rowPrim + 0x24) = tpageRight;
        *(u16*)(rowPrim + 0x0C) = uv;
        *(u16*)(rowPrim + 0x20) = uv;
        *(u8*)(rowPrim + 0x03) = 4;
        *(u8*)(rowPrim + 0x07) = 0x65;
        *(u8*)(rowPrim + 0x17) = 4;
        *(u8*)(rowPrim + 0x1B) = 0x65;

        *(u32*)(rowPrim + 0x28) = *(u32*)(rowPrim + 0x00);
        *(u32*)(rowPrim + 0x2C) = *(u32*)(rowPrim + 0x04);
        *(u32*)(rowPrim + 0x30) = *(u32*)(rowPrim + 0x08);
        *(u32*)(rowPrim + 0x34) = *(u32*)(rowPrim + 0x0C);
        *(u32*)(rowPrim + 0x38) = *(u32*)(rowPrim + 0x10);
        *(u32*)(rowPrim + 0x3C) = *(u32*)(rowPrim + 0x14);
        *(u32*)(rowPrim + 0x40) = *(u32*)(rowPrim + 0x18);
        *(u32*)(rowPrim + 0x44) = *(u32*)(rowPrim + 0x1C);
        *(u32*)(rowPrim + 0x48) = *(u32*)(rowPrim + 0x20);
        *(u32*)(rowPrim + 0x4C) = *(u32*)(rowPrim + 0x24);
        *(s16*)(rowPrim + 0x50) = tpageX;
        *(s16*)(rowPrim + 0x52) = textureX;
        *(s16*)(rowPrim + 0x54) = *(u16*)(pWindow + 0x12);
        *(s16*)(rowPrim + 0x56) = 0x0D;
        *(s16*)(rowPrim + 0x58) = 0;
        *(s16*)(rowPrim + 0x5E) = oddRow ? g_SystemPalette2 : g_SystemPalette1;
        *(u8*)(rowPrim + 0x5C) = textureX;
        *(u8*)(rowPrim + 0x5A) = oddRow;
        *(u8*)(rowPrim + 0x5B) = i;
    }

    tpage0 = GetTPage(0, 0, tpageX, tpageY);
    SetDrawMode((DR_MODE*)(pWindow + 0x30), 0, 0, tpage0, NULL);
    tpage1 = GetTPage(0, 0, tpageX + 0x40, tpageY);
    SetDrawMode((DR_MODE*)(pWindow + 0x3C), 0, 0, tpage1, NULL);
}
#endif

unsigned int ResolveArchiveEntryPointers(u32* pFile) {
    int i;
    u32* pEntries;
    u32 offset;

    offset = (u32)pFile;
    pEntries = pFile;
    for (i = 1; i <= *pFile; i++) {
        pFile[i] += offset;
    }
    
    return *pFile;
}

// Same as ResolveArchiveEntryPointers, but no return value
void ResolveFileEntryPointers(u32* pFile) {
    int i;
    u32* pEntries;
    u32 offset;

    offset = (u32)pFile;
    pEntries = pFile;
    for (i = 1; i <= *pFile; i++) {
        pFile[i] += offset;
    }
}

void* GetSystemFontFile(void) {
    return g_SystemFontFile;
}

void* GetSystemDataFile(void) {
    return g_SystemDataFile;
}

void SystemFreeFont(void) {
    HeapUnpinBlock(g_SystemFontFile);
    HeapFree(g_SystemFontFile);
    g_SystemFontFile = NULL;
}

void SystemFreeData(void) {
    HeapUnpinBlock(g_SystemDataFile);
    HeapFree(g_SystemDataFile);
    g_SystemDataFile = NULL;
}


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", SystemInitializeFont);
#else
void SystemInitializeFont(void* pSystemFont) {
    if (pSystemFont == NULL) {
        HeapSetCurrentContentType(0x20);
        return;
    }

    HeapPinBlock(pSystemFont);
    g_SystemFontFile = pSystemFont;
    D_8005935C = (u32)(uintptr_t)pSystemFont;
    D_8005934C = *(u16*)((u8*)pSystemFont + 0x04);
    D_80059350 = *(u16*)((u8*)pSystemFont + 0x06);
    D_80059354 = *(u16*)((u8*)pSystemFont + 0x08);
    D_80059358 = *(u16*)((u8*)pSystemFont + 0x0A);
    D_8005935C = (u32)(uintptr_t)((u8*)pSystemFont + *(u16*)((u8*)pSystemFont + 0x02));
    D_80059364 = *(u16*)((u8*)pSystemFont + 0x0C);
}
#endif

void SystemInitializeData(void* pSystemData) {
    if (pSystemData == 0) {
        HeapSetCurrentContentType(HEAP_CONTENT_NONE);
        return;
    }
    HeapPinBlock(pSystemData);
    g_SystemDataFile = pSystemData;
    g_SystemDataEntries = pSystemData;
    ResolveArchiveEntryPointers(pSystemData);
    g_SystemDataEntries += 4;
}

void SystemInitialize(void* pSystemFont, void* pSystemData) {
    SystemInitializeFont(pSystemFont);
    SystemInitializeData(pSystemData);
}


void SystemTransferPaletteToVRAM(short xDest, short yDest) {
    RECT dest;
    setRECT(&dest, xDest, yDest, 32, 1);
    LoadImage(&dest, &g_SystemPaletteData);
    g_SystemPalette1 = GetClut(xDest, yDest);
    g_SystemPalette2 = GetClut(xDest + 16, yDest);
}

void* GetStringEntry(void* arg0, s32 arg1) {
    return (u8*)arg0 + *(u16*)((u8*)arg0 + arg1 * 2 + 4);
}

// TODO: Cleanup code
// Dialog data format:
// 0x0: Offset to dimension data
// 0x2: ?
// 0x4: ?
// 0x6: u16 dimensions[dialog_count]
u8 DialogGetWidth(u16* pDialogData, int dialogIndex) {
    u8* pDialog = (pDialogData + 3) + *pDialogData + dialogIndex;
    return pDialog[0];
}

u8 DialogGetHeight(u16* pDialogData, int dialogIndex) {
    u8* pDialog = (pDialogData + 3) + *pDialogData + dialogIndex;
    return pDialog[1];
}

void* func_80033784(s32 tableIndex, s32 stringIndex) {
    /* ResolveArchiveEntryPointers keeps four-byte slots even on the host.
     * Match the retail LW here and in the fixed-offset readers below. */
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + tableIndex * 4), stringIndex);
}

void* func_800337B8(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x40), index);
}

void* GetAccessoryName(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x44), index);
}

void* GetItemName(s32 index) {
    /* The system archive table contains PSX-width pointer slots.  Keep the
     * retail lw read on the 64-bit port instead of consuming two entries. */
    return GetStringEntry(
        (void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x58), index);
}

void* GetWeaponName(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x5C), index);
}

void* func_80033878(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x60), index);
}

void* func_800338A8(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x64), index);
}

void* func_800338D8(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x48), index);
}

void* func_80033908(s32 index) {
    /* The entries table holds four-byte PSX pointer slots: read the slot as
     * u32 (the func_80033B34 idiom), not as a native void**.  Latent until
     * func_801DC3D8 (A1b-1) became the first real caller. */
    return GetStringEntry(
        (void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x50), index);
}

void* func_80033938(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x4C), index);
}

void* func_80033968(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x54), index);
}

void* func_80033998(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x6C), index);
}

void* func_800339C8(s32 tableIndex, s32 stringIndex) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + tableIndex * 4 + 0x70), stringIndex);
}

void* func_800339FC(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0xC0), index);
}

void* func_80033A2C(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0xC8), index);
}

void* func_80033A5C(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0xCC), index);
}

void* func_80033A8C(s32 index) {
    return GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0xD0), index);
}

extern u8 D_8005A0E4[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80033ABC);
#else
/* Expands glyph indices to their one- or two-byte codes in D_8005A0E4. */
void func_80033ABC(u16* pIndices) {
    u8* pOut = D_8005A0E4;
    u16 idx = pIndices[0];
    u8* pTable = (u8*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x6C);

    while (idx != 0xFFFF) {
        u8* pEntry = idx * 2 + pTable;

        u8 first = pEntry[0];
        u8 second;

        pIndices++;
        if (first != 0) {
            *pOut++ = first;
            second = pEntry[1];
        } else {
            second = pEntry[1];
        }
        *pOut++ = second;
        idx = pIndices[0];
    }
    *pOut = 0;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80033B34);
#else
/* Save-format name decode (asm 80033B34): each u16 char code indexes a
 * 2-byte glyph pair in the table at g_SystemDataEntries+0x6C; byte 0 is
 * written only when nonzero, byte 1 always; NUL-terminated.  Used by the
 * new-game init (func_8001B970) to decode the template's character names. */
void func_80033B34(u16* src, u8* dst, s32 count) {
    u8* table =
        (u8*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x6C);
    s32 i;

    for (i = count - 1; i >= 0; i--) {
        u8* e = table + *src * 2;

        src++;
        if (e[0] != 0) {
            *dst++ = e[0];
        }
        *dst++ = e[1];
    }
    *dst = 0;
}
#endif

s32 func_80033BAC(u8 a, u8 b) {
    u8* pTable = (u8*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x6C);
    s16 i;
    for (i = 0; i < 0x144; i++) {
        u8* pEntry = (u8*)((uintptr_t)(i * 2) + (uintptr_t)pTable);
        if (pEntry[0] == a && pEntry[1] == b) return i;
    }
    return 0x8000;
}

/* Converts a script string to glyph indices (one-byte codes below
 * D_8005934C, two-byte codes otherwise); -1 on an unknown pair. */
s32 func_80033C20(u8* pString, u16* pOutIndices) {
    u8 c = *pString++;
    u8 first;
    u8 second;
    u16 idx;

    while (c != 0) {
        if ((s32)c < (s32)D_8005934C) {
            first = 0;
            second = c;
        } else {
            first = c;
            second = *pString++;
        }
        idx = func_80033BAC(first, second);
        *pOutIndices++ = idx;
        if (idx == 0x8000) {
            return -1;
        }
        c = *pString++;
    }
    return 0;
}

s32 func_80033CD0(void* arg0) {
    if (*(u16*)((u8*)arg0 + 0x10) & 0x8) {
        return *(u8*)((u8*)arg0 + 0x6B);
    }
    return 0;
}

#ifdef XENO_PC_PORT
/* Retail lays the number scratch out contiguously: sentinel halfword at
 * 0x8005A0C8 (tail of D_8005A0C2), ten digit indices at D_8005A0CA, and the
 * 0xFFFF terminator at D_8005A0DE. func_80033CF0 walks across all three
 * (asm 80033D68-80033DB8), so the port needs one contiguous buffer rather
 * than the three separately-stubbed symbols. */
static u16 s_NumberScratch[12];
#define NUM_SCRATCH_SENTINEL (&s_NumberScratch[0])
#define NUM_SCRATCH_DIGITS   (&s_NumberScratch[1])
#define NUM_SCRATCH_TERM     (&s_NumberScratch[11])
#else
extern u16 D_8005A0CA[];
extern u16 D_8005A0DE;
#define NUM_SCRATCH_SENTINEL (&D_8005A0CA[-1])
#define NUM_SCRATCH_DIGITS   (&D_8005A0CA[0])
#define NUM_SCRATCH_TERM     (&D_8005A0DE)
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80033CF0);
#else
/* Render a decimal number into the glyph-index scratch and decode it into
 * D_8005A0E4 (asm 80033CF0). Args follow the asm: a0 value, a1 font offset
 * (glyph row, <<4), a2 signed mode. Signed mode always emits a sign glyph:
 * 0xB for >= 0, 0xA for < 0 (80033D04-80033D18). Leading zeros are skipped
 * down to the last digit; a sentinel equal to the zero glyph sits one slot
 * before the digits so the scan loop's pre-increment lands on digit 0. */
void func_80033CF0(s32 value, s32 fontOffset, s32 signedMode) {
    u32 divisor = 1000000000;
    s32 signChar = 0;
    u16* pDigits = NUM_SCRATCH_DIGITS;
    u16* p;
    s32 i;

    fontOffset <<= 4;
    if (signedMode != 0) {
        if (value >= 0) {
            signChar = 0xB;
        } else {
            value = -value;
            signChar = 0xA;
        }
    }

    for (i = 0; i < 10; i++) {
        pDigits[i] = (u16)((u32)value / divisor + fontOffset);
        value = (u32)value % divisor;
        divisor /= 10;
    }

    *NUM_SCRATCH_TERM = 0xFFFF;
    p = NUM_SCRATCH_SENTINEL;
    *p = (u16)fontOffset;

    if ((fontOffset & 0xFFF0) == fontOffset) {
        u16* pLast = p + 10;
        while (p != pLast) {
            p++;
            if (*p != (u16)fontOffset) {
                break;
            }
        }
    }

    if (signChar != 0) {
        p--;
        *p = (u16)(signChar + fontOffset);
    }

    func_80033ABC(p);
}
#endif

#ifdef XENO_PC_PORT
void func_80033DD4(void* arg0, void* arg1) {
    SYSTEM_TEXT_SET_ADDRESS(arg0, 0x20, SYSTEM_TEXT_ADDRESS(arg0, 0x1C));
    SYSTEM_TEXT_SET_ADDRESS(arg0, 0x1C, arg1);
#else
void func_80033DD4(void* arg0, s32 arg1) {
    *(s32*)((u8*)arg0 + 0x20) = *(s32*)((u8*)arg0 + 0x1C);
    *(s32*)((u8*)arg0 + 0x1C) = arg1;
#endif
    *(u16*)((u8*)arg0 + 0x10) |= 0x80;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80033DF0);
#else
void func_80033DF0(void* arg0) {
    u8* pWindow = arg0;
    s32 count;

    count = pWindow[0x69];

    if (*(s16*)(pWindow + 0x0A) < *(s16*)(pWindow + 0x00)) {
        s16 row = *(s16*)(pWindow + 0x02) + 1;
        s16 pageCount;

        /* No raster clear here (asm 80033E30-80033E6C). Rows 2k/2k+1 share
         * one 13-line VRAM strip through the two nibble planes of the same
         * scratch buffer; zeroing it at row start wiped the partner row. */
        *(s16*)(pWindow + 0x00) = 0;
        *(s16*)(pWindow + 0x02) = row;
        *(s16*)(pWindow + 0x18) += 1;
        if (row >= *(s16*)(pWindow + 0x0C)) {
            *(s16*)(pWindow + 0x02) = 0;
            *(u16*)(pWindow + 0x10) |= 1;
        }

        if (*(u16*)(pWindow + 0x10) & 1) {
            u8* pRows = (u8*)(uintptr_t)SYSTEM_TEXT_ADDRESS(pWindow, 0x28);
            s16 oldBase = *(s16*)(pWindow + 0x16);

            *(s16*)(pRows + oldBase * 0x60 + 0x58) = 0;
            oldBase++;
            *(s16*)(pWindow + 0x16) = oldBase;
            if (oldBase >= *(s16*)(pWindow + 0x0C)) {
                *(s16*)(pWindow + 0x16) = 0;
            }
        }

        pageCount = *(s16*)(pWindow + 0x18) % (*(s16*)(pWindow + 0x0C) + 1);
        {
            s16 row = *(s16*)(pWindow + 0x02);
            u8* pRow = (u8*)(uintptr_t)SYSTEM_TEXT_ADDRESS(pWindow, 0x28) + row * 0x60;
            s32 shade = (pageCount + ((u32)pageCount >> 31)) >> 1;
            s32 color = *(u8*)(pWindow + 0x0E) + shade * 13;

            pRow[0x5C] = color;
            *(u16*)(pRow + 0x5E) = (pageCount & 1) ? g_SystemPalette2 : g_SystemPalette1;
            pRow[0x5A] = pageCount & 1;
            pRow[0x5B] = pageCount;
            *(u16*)(pRow + 0x52) = *(u16*)(pWindow + 0x0E) + shade * 13;
        }
    }

    if (pWindow[0x6C] != 0) {
        pWindow[0x6C] = 0;
        *(u16*)(pWindow + 0x10) &= 0xFFFB;
        return;
    }

    count--;
    if (count == -1) {
        return;
    }

    /* Control-code interpreter, asm 80034024-800345B0. `count` is the
     * per-frame glyph budget (s3); codes that push a nested string
     * (func_80033DD4) or advance a bare parameter leave it unchanged, glyphs
     * and skipped codes consume one. */
    while (count != -1) {
        u8* pScript = (u8*)(uintptr_t)SYSTEM_TEXT_ADDRESS(pWindow, 0x1C);
        s32 code = pScript[0];

        if (code == 0) {
            /* End of (possibly nested) string: pop back to the caller string,
             * or arm the wait-for-input state. */
            u16 flags = *(u16*)(pWindow + 0x10);

            if (flags & 0x80) {
                *(u16*)(pWindow + 0x10) = flags & 0xFF7F;
                SYSTEM_TEXT_SET_ADDRESS(pWindow, 0x1C, SYSTEM_TEXT_ADDRESS(pWindow, 0x20) + 1);
            } else {
                *(u16*)(pWindow + 0x10) = flags | 0x8;
                pWindow[0x6B] = 1;
                pWindow[0x6C] = 1;
                return;
            }
        } else if (code == 3) {
            /* Wait for input, keep page contents (8003 3FD0). */
            pWindow[0x6B] = 3;
            *(u16*)(pWindow + 0x10) |= 0x8;
            SYSTEM_TEXT_ADVANCE(pWindow, 1);
            return;
        } else if (code == 2) {
            /* Wait for input, then clear the page: 0x40 turns into the 0x20
             * row-reset in func_80034888 once the wait (0x8) releases
             * (80034490-800344C8). A directly following line-break (01) is
             * swallowed. */
            pWindow[0x6B] = 2;
            *(u16*)(pWindow + 0x10) |= 0x48;
            SYSTEM_TEXT_ADVANCE(pWindow, 1);
            if (pScript[1] == 1) {
                SYSTEM_TEXT_ADVANCE(pWindow, 1);
            }
            return;
        } else if (code == 1) {
            /* Line break: force the row-overflow path on the next call. */
            *(s16*)(pWindow + 0x00) = 0x64;
            SYSTEM_TEXT_ADVANCE(pWindow, 1);
            return;
        } else if (code == 0x0F) {
            s32 subcode = pScript[1];

            if (subcode >= 0x10) {
                /* 800340A0: out-of-range subcode consumes budget only. */
                count--;
                continue;
            }

            switch (subcode) {
            case 0x0: /* delay N frames */
                *(s16*)(pWindow + 0x84) = pScript[2];
                SYSTEM_TEXT_ADVANCE(pWindow, 3);
                return;

            case 0x1: { /* text speed override / restore */
                s32 value = pScript[2];

                if (value != 0) {
                    u8 old = pWindow[0x68];

                    count += value;
                    pWindow[0x68] = value;
                    pWindow[0x69] = value;
                    pWindow[0x6A] = old;
                } else {
                    pWindow[0x68] = pWindow[0x6A];
                    pWindow[0x69] = pWindow[0x6A];
                    pWindow[0x6A] = 0;
                }
                SYSTEM_TEXT_ADVANCE(pWindow, 3);
                count--;
                continue;
            }

            case 0x2: /* delay N frames, suppress auto-advance */
                pWindow[0x6C] = 1;
                *(s16*)(pWindow + 0x84) = pScript[2];
                SYSTEM_TEXT_ADVANCE(pWindow, 3);
                return;

            case 0x3: { /* system string table[a] entry b */
                void* table = (void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + pScript[2] * 4);
                s32 entry = pScript[3];

                SYSTEM_TEXT_ADVANCE(pWindow, 3);
                func_80033DD4(pWindow, GetStringEntry(table, entry));
                continue;
            }

            case 0x4: { /* name of the item in window+0x80 (class in the high byte) */
                s32 item = *(s16*)(pWindow + 0x80);
                s32 cls = item & 0xFF00;
                s32 tableOfs;

                SYSTEM_TEXT_ADVANCE(pWindow, 1);
                if (cls == 0x000) {
                    tableOfs = 0x58;
                } else if (cls == 0x100) {
                    tableOfs = 0x5C;
                } else if (cls == 0x200) {
                    tableOfs = 0x44;
                } else if (cls == 0x300) {
                    tableOfs = 0xCC;
                } else if (cls == 0x400) {
                    tableOfs = 0xC8;
                } else {
                    count--;
                    continue;
                }
                func_80033DD4(pWindow,
                              GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + tableOfs),
                                             item & 0xFF));
                count--;
                continue;
            }

            case 0x5: { /* character name: direct id, or party slot (>= 0x80) */
                s32 id = pScript[2];
                void* name;

                SYSTEM_TEXT_ADVANCE(pWindow, 2);
                if (id >= 0x80) {
                    id = *((u8*)&g_GameState + 0x1CB4 + id);
                    if (id == 0xFF) {
                        name = GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0x68), 0);
                        func_80033DD4(pWindow, name);
                        continue;
                    }
                }
                name = (u8*)&g_GameState + id * 20;
                func_80033DD4(pWindow, name);
                continue;
            }

            case 0x6: { /* weapon name; retail 80034304 loads +0x5C */
                s32 entry = pScript[2];

                SYSTEM_TEXT_ADVANCE(pWindow, 2);
                func_80033DD4(pWindow,
                              GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries +
                                                                       0x5C),
                                             entry));
                continue;
            }

            case 0x7: { /* table +0x60 name; retail 8003432C loads +0x60 */
                s32 entry = pScript[2];

                SYSTEM_TEXT_ADVANCE(pWindow, 2);
                func_80033DD4(pWindow,
                              GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries +
                                                                       0x60),
                                             entry));
                continue;
            }

            case 0x8: { /* table +0x64 name; retail 80034354 loads +0x64 */
                s32 entry = pScript[2];

                SYSTEM_TEXT_ADVANCE(pWindow, 2);
                func_80033DD4(pWindow,
                              GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries +
                                                                       0x64),
                                             entry));
                continue;
            }

            case 0x9:   /* number, font row 0, unsigned */
            case 0xA:   /* number, font row 1, unsigned */
            case 0xC: { /* number, font row 1, signed */
                s32 var = pScript[2];
                s32 fontRow = (subcode == 0x9) ? 0 : 1;
                s32 signedMode = (subcode == 0xC) ? 1 : 0;

                SYSTEM_TEXT_ADVANCE(pWindow, 2);
                func_80033CF0(*(s32*)(pWindow + 0x70 + var * 4), fontRow, signedMode);
                func_80033DD4(pWindow, D_8005A0E4);
                continue;
            }

            case 0xB: /* set byte +0x6D */
                pWindow[0x6D] = pScript[2];
                SYSTEM_TEXT_ADVANCE(pWindow, 2);
                count--;
                continue;

            case 0xD: /* delay N, suppress auto-advance, flag 0x200 */
                pWindow[0x6C] = 1;
                *(u16*)(pWindow + 0x10) |= 0x200;
                *(s16*)(pWindow + 0x84) = pScript[2];
                SYSTEM_TEXT_ADVANCE(pWindow, 3);
                return;

            case 0xE: /* reset speed to 1, set per-glyph delay */
                pWindow[0x6A] = pWindow[0x68];
                pWindow[0x68] = 1;
                pWindow[0x69] = 1;
                *(s16*)(pWindow + 0x88) = pScript[2];
                *(s16*)(pWindow + 0x86) = pScript[2];
                SYSTEM_TEXT_ADVANCE(pWindow, 3);
                return;

            case 0xF: { /* button name via controller mapping */
                s32 button = pScript[2];

                SYSTEM_TEXT_ADVANCE(pWindow, 2);
                func_80033DD4(pWindow,
                              GetStringEntry((void*)(uintptr_t)*(u32*)((u8*)g_SystemDataEntries + 0xC4),
                                             g_ControllerButtonMappings[button]));
                continue;
            }
            }
        } else {
            s32 lead;
            s32 trail;
            s32 consumed;
            s32 width;
            s32 oldX;

            if (code < D_8005934C) {
                lead = 0;
                trail = code & 0xFF;
                consumed = 1;
            } else {
                lead = code & 0xFF;
                trail = pScript[1];
                consumed = 2;
            }

            width = func_80034F98(lead, trail);
            oldX = *(s16*)(pWindow + 0x00);
            if (*(s16*)(pWindow + 0x0A) < oldX + (width & 0xFFFF)) {
                *(s16*)(pWindow + 0x00) = oldX + width;
                return;
            }

            {
                s16 row = *(s16*)(pWindow + 0x02);
                u8* pRow = (u8*)(uintptr_t)SYSTEM_TEXT_ADDRESS(pWindow, 0x28) + row * 0x60;
                u8 rowPage = pRow[0x5A];

                func_80034FFC(lead, trail, (u8*)(uintptr_t)SYSTEM_TEXT_ADDRESS(pWindow, 0x2C) + oldX * 2,
                              *(s16*)(pWindow + 0x12), rowPage);
                SYSTEM_TEXT_ADVANCE(pWindow, consumed);
                *(s16*)(pWindow + 0x00) = oldX + width;
                *(s16*)(pRow + 0x58) = oldX + width;
            }
        }

        count--;
    }
}
#endif

void func_800345E0(void* a0) {
    u16 flags = *(u16*)((u8*)a0 + 0x10);
    *(u16*)((u8*)a0 + 0x10) = flags & ~0x8;
    if (flags & 0x200) {
        *(u16*)((u8*)a0 + 0x84) = 0;
        *(u8*)((u8*)a0 + 0x6C) = 0;
        *(u16*)((u8*)a0 + 0x10) &= ~0x200;
    }
}

void func_80034614(void* a0) {
    if (*(s16*)((u8*)a0 + 0x84) == 0) {
        *(u8*)((u8*)a0 + 0x6C) = 0;
        *(u16*)((u8*)a0 + 0x10) &= 0x2;
    }
}

void func_8003463C(void* arg0) {
    u8* pWindow = arg0;
    u32 nodeAddr;

    if (*(s16*)(pWindow + 0x84) != 0) {
        return;
    }

    nodeAddr = *(u32*)(pWindow + 0x8C);
    while (nodeAddr != 0) {
        u8* node = (u8*)(uintptr_t)nodeAddr;
        nodeAddr = *(u32*)node;
        HeapFree(node);
    }
    *(u32*)(pWindow + 0x8C) = 0;
    *(s16*)(pWindow + 0x82) = 0;
}

void func_800346A4(void* a0) {
    u16 v = *(u16*)((u8*)a0 + 0x10);
    *(u8*)((u8*)a0 + 0x6C) = 0;
    *(u16*)((u8*)a0 + 0x84) = 0;
    *(u16*)((u8*)a0 + 0x10) = v & 0x2;
    func_8003463C(a0);
}

void func_800346D4(void* arg0) {
    u8* pWindow = arg0;

    func_800346A4(arg0);
    HeapFree((void*)(uintptr_t)*(u32*)(pWindow + 0x28));
    HeapFree((void*)(uintptr_t)*(u32*)(pWindow + 0x2C));
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80034714);
#else
void func_80034714(void* arg0, void* arg1) {
    u8* pWindow = arg0;
    u32 oldHead;
    u8* node;

    HeapSetCurrentContentType(0x2A);
    *(u16*)(pWindow + 0x82) = *(u16*)(pWindow + 0x82) + 1;
    oldHead = *(u32*)(pWindow + 0x8C);
    node = HeapAlloc(8, 2);
    *(u32*)(node + 0x00) = 0;
    *(u32*)(node + 0x04) = (u32)(uintptr_t)arg1;

    if (oldHead == 0) {
        *(u32*)(pWindow + 0x8C) = (u32)(uintptr_t)node;
        return;
    }

    while (*(u32*)(uintptr_t)oldHead != 0) {
        oldHead = *(u32*)(uintptr_t)oldHead;
    }
    *(u32*)(uintptr_t)oldHead = (u32)(uintptr_t)node;
}
#endif

s32 func_800347AC(void* arg0) {
    return *(s16*)((u8*)arg0 + 0x4) + (*(s16*)((u8*)arg0 + 0x0) << 2);
}

/* Retail 800347C0 (asm/slus_006.64/matchings/system/system/func_800347C0.s).
 *
 * This is NOT the vertical twin of func_800347AC above.  It was written as if
 * it were -- same shape with the offsets bumped by 2 and the same `<< 2` --
 * and that is wrong in three ways.  Retail:
 *
 *   lh   v1, 0x02(a0)        ; current row
 *   lh   v0, 0x16(a0)        ; scroll base row
 *   subu v1, v1, v0
 *   bgez v1, .L800347E4      ; if it went negative, wrap to the last row
 *   lh   v0, 0x0C(a0)        ;   row count
 *   addiu v1, v0, -1
 * .L800347E4:
 *   lh   v0, 0x14(a0)        ; row stride in pixels
 *   mult v1, v0
 *   lh   v0, 0x06(a0)        ; window origin y
 *   mflo a1
 *   addu v0, v0, a1
 *
 * The old body ignored the scroll base at +0x16, dropped the negative-wrap
 * clamp entirely, and multiplied by a hardcoded 4 instead of the row stride
 * at +0x14 -- which func_80032F54 initialises to 0x0E.  So the dialogue
 * continue-arrow's y was ~3.5x too small and did not follow scrolled text.
 * Horizontally 4 is right (func_800347AC, character cell width); vertically
 * it never was.  func_80034888 places every text row at
 * `*(u16*)(pWindow + 0x06) + *(s16*)(pWindow + 0x14) * rowIndex`, which is
 * the same formula this must agree with.
 */
s32 func_800347C0(void* arg0) {
    s32 row = *(s16*)((u8*)arg0 + 0x02) - *(s16*)((u8*)arg0 + 0x16);

    if (row < 0) {
        row = *(s16*)((u8*)arg0 + 0x0C) - 1;
    }
    return *(s16*)((u8*)arg0 + 0x06) + row * *(s16*)((u8*)arg0 + 0x14);
}

/* Sets the RGB of the four glyph quads of every text row of a window. */
void func_80034800(void* arg0, s32 r, s32 g, s32 b) {
    u8 unused[1]; /* retail reserves an 8-byte frame it never touches */
    s32 i;

    for (i = 0; i < *(s16*)((u8*)arg0 + 0x0C); i++) {
        u8* pRow = (u8*)(uintptr_t)*(u32*)((u8*)arg0 + 0x28) + i * 0x60;

        pRow[0x40] = r;
        pRow[0x2C] = r;
        pRow[0x18] = r;
        pRow[0x04] = r;
        pRow[0x41] = g;
        pRow[0x2D] = g;
        pRow[0x19] = g;
        pRow[0x05] = g;
        pRow[0x42] = b;
        pRow[0x2E] = b;
        pRow[0x1A] = b;
        pRow[0x06] = b;
    }
}

void func_80034874(void* arg0, u8 arg1) {
    *(u8*)((u8*)arg0 + 0x6E) = arg1;
}

int func_8003487C(void* arg0) {
    int value = 0xFF;
    *(u8*)((u8*)arg0 + 0x6E) = value;
    return value;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80034888);
#else
void func_80034888(void* arg0, void* ot, s32 renderContextIndex) {
    u8* pWindow = arg0;
    u16 flags;
    u8* pRows;
    s32 rowIndex;
    s32 rowOffset;
    s32 ctxOffset;

    flags = *(u16*)(pWindow + 0x10);
    if (!(flags & 0x4)) {
        u32 nodeAddr = *(u32*)(pWindow + 0x8C);

        if (*(s16*)(pWindow + 0x82) == 0) {
            return;
        }

        *(u32*)(pWindow + 0x1C) = *(u32*)(uintptr_t)(nodeAddr + 4);
        *(u32*)(pWindow + 0x8C) = *(u32*)(uintptr_t)nodeAddr;
        HeapFree((void*)(uintptr_t)nodeAddr);

        flags = (*(u16*)(pWindow + 0x10) & 0x2) | 0x24;
        *(s16*)(pWindow + 0x82) -= 1;
        *(u16*)(pWindow + 0x10) = flags;
        if (*(u8*)(pWindow + 0x6A) != 0) {
            u8 value = *(u8*)(pWindow + 0x6A);

            *(u8*)(pWindow + 0x68) = value;
            *(u8*)(pWindow + 0x6A) = 0;
            *(u8*)(pWindow + 0x69) = value;
        }
        *(s16*)(pWindow + 0x88) = 0;
        *(s16*)(pWindow + 0x86) = 0;
        *(u8*)(pWindow + 0x69) = *(u8*)(pWindow + 0x68);
    }

    flags = *(u16*)(pWindow + 0x10);
    if (flags & 0x100) {
        *(u8*)(pWindow + 0x69) = *(u8*)(pWindow + 0x68) * 3;
    } else {
        *(u8*)(pWindow + 0x69) = *(u8*)(pWindow + 0x68);
    }

    flags = *(u16*)(pWindow + 0x10);
    if ((flags & 0x40) && !(flags & 0x8)) {
        *(u16*)(pWindow + 0x10) = (flags & 0xFFBF) | 0x20;
    }

    pRows = (u8*)(uintptr_t)*(u32*)(pWindow + 0x28);
    if (*(u16*)(pWindow + 0x10) & 0x20) {
        *(s16*)(pWindow + 0x16) = 0;
        *(s16*)(pWindow + 0x18) = 0;
        *(s16*)(pWindow + 0x02) = 0;
        *(s16*)(pWindow + 0x00) = 0;
        pRows[0x5C] = *(u16*)(pWindow + 0x0E);
        *(u16*)(pRows + 0x5E) = g_SystemPalette1;
        pRows[0x5A] = 0;
        *(u16*)(pRows + 0x52) = *(u16*)(pWindow + 0x0E);

        for (rowIndex = 0; rowIndex < *(s16*)(pWindow + 0x0C); rowIndex++) {
            *(s16*)(pRows + rowIndex * 0x60 + 0x58) = 0;
        }

        *(u16*)(pWindow + 0x10) &= 0xFFDE;
    }

    ctxOffset = renderContextIndex * 0x28;
    rowIndex = 0;
    rowOffset = *(s16*)(pWindow + 0x16) * 0x60;
    while (rowIndex < *(s16*)(pWindow + 0x0C)) {
        u8* row = pRows + rowOffset;
        u8* prim = row + ctxOffset;

        if (*(u8*)(pWindow + 0x6E) != rowIndex) {
            prim[0x1B] |= 1;
        } else {
            prim[0x1B] &= 0xFE;
        }

        if (*(s16*)(row + 0x58) >= 0x41) {
            prim[0x21] = row[0x5C];
            *(u16*)(prim + 0x22) = *(u16*)(row + 0x5E);
            *(u16*)(prim + 0x1E) = *(u16*)(pWindow + 0x06) + *(s16*)(pWindow + 0x14) * rowIndex;
            *(u16*)(prim + 0x24) = (*(s16*)(row + 0x58) - 0x40) << 2;
            func_80031798(ot, prim + 0x14);
        }

        rowIndex++;
        rowOffset += 0x60;
        if (rowIndex >= *(s16*)(pWindow + 0x0C)) {
            rowOffset = 0;
        }
    }

    SYSTEM_ADD_PRIM(ot, pWindow + 0x3C);

    rowIndex = 0;
    rowOffset = *(s16*)(pWindow + 0x16) * 0x60;
    while (rowIndex < *(s16*)(pWindow + 0x0C)) {
        u8* row = pRows + rowOffset;
        u8* prim = row + ctxOffset;

        if (*(u8*)(pWindow + 0x6E) != rowIndex) {
            prim[0x07] |= 1;
        } else {
            prim[0x07] &= 0xFE;
        }

        if (*(s16*)(row + 0x58) != 0) {
            prim[0x0D] = row[0x5C];
            *(u16*)(prim + 0x0E) = *(u16*)(row + 0x5E);
            *(u16*)(prim + 0x0A) = *(u16*)(pWindow + 0x06) + *(s16*)(pWindow + 0x14) * rowIndex;
            *(u16*)(prim + 0x10) = (*(s16*)(row + 0x58) < 0x41) ? (*(s16*)(row + 0x58) << 2) : 0x100;
            func_80031798(ot, prim);
        }

        rowIndex++;
        rowOffset += 0x60;
        if (rowIndex >= *(s16*)(pWindow + 0x0C)) {
            rowOffset = 0;
        }
    }

    if (*(s16*)(pWindow + 0x84) != 0) {
        *(s16*)(pWindow + 0x84) -= 1;
    } else if (*(s16*)(pWindow + 0x86) != 0) {
        *(s16*)(pWindow + 0x86) -= 1;
    } else {
        *(s16*)(pWindow + 0x86) = *(s16*)(pWindow + 0x88);
        if ((*(u16*)(pWindow + 0x10) & 0x58) == 0) {
            func_80033DF0(pWindow);
            LoadImage((RECT*)(pRows + *(s16*)(pWindow + 0x02) * 0x60 + 0x50),
                      (u_long*)(uintptr_t)*(u32*)(pWindow + 0x2C));
        }
    }

    if (*(s16*)(pWindow + 0x84) != 0) {
        *(s16*)(pWindow + 0x84) -= 1;
        if (*(s16*)(pWindow + 0x84) == -1) {
            *(u16*)(pWindow + 0x10) &= 0xFFEF;
        }
    }

    if (!(*(u16*)(pWindow + 0x10) & 0x2)) {
        u8* prim = pWindow + renderContextIndex * 0x10;
        s32 xy = (*(s16*)(pWindow + 0x04) - 7) | ((*(s16*)(pWindow + 0x06) - 5) << 16);
        s32 wh = (((*(s16*)(pWindow + 0x0A) | 1) << 2) + 0xD) |
                 (((*(s16*)(pWindow + 0x0C) * *(s16*)(pWindow + 0x14) + 0xA) << 16));

        *(s32*)(prim + 0x50) = xy;
        *(s32*)(prim + 0x54) = wh;
        SYSTEM_ADD_PRIM(ot, prim + 0x48);
    }

    *(u16*)(pWindow + 0x10) &= 0xFEFF;
    SYSTEM_ADD_PRIM(ot, pWindow + 0x30);
}
#endif

// Render string entry to a buffer
#ifdef XENO_PC_PORT
/* Text-render descriptor: retail lays D_80059FD8..D_8005A0C2 out as ONE
 * contiguous ~0xF0-byte region (the row buffer D_8005A068 sits at +0x90).
 * func_80033DF0 (already ported) reads it via arg0+offset, so it MUST be one
 * contiguous buffer, not 18 separate zeroed symbols (the overlay-data alias
 * trap).  Nothing else references these symbols (verified), so a single static
 * buffer is safe. Full-width pointer storage is declared above the shared
 * interpreter; packed pointer words are only low-word mirrors for this
 * descriptor, never the authority for its native pointer consumers. */

s32 SystemRenderStringEntry(void* pString, void* pWork, s32 height, s32 flag) {
    u8* d = s_StringRenderDescriptor;

    *(u16*)(d + 0x0A) = (u16)height;
    *(u16*)(d + 0x0C) = 1;
    height |= 1;
    *(u16*)(d + 0x08) = (u16)(((s32)((u32)height << 16)) >> 14);
    *(u16*)(d + 0x0A) = (u16)height;
    height += 3;
    SYSTEM_TEXT_SET_ADDRESS(d, 0x1C, pString);
    *(u8*)(d + 0x68) = 1;
    *(u16*)(d + 0x12) = (u16)height;
    *(u16*)(d + 0x84) = 0;
    *(u8*)(d + 0x6C) = 0;
    *(u8*)(d + 0x6A) = 0;
    SYSTEM_TEXT_SET_ADDRESS(d, 0x2C, pWork);
    *(u16*)(d + 0x10) = 0;
    *(u16*)(d + 0x02) = 0;
    *(u16*)(d + 0x00) = 0;
    *(u8*)(d + 0x69) = 0x64;
    SYSTEM_TEXT_SET_ADDRESS(d, 0x28, d + 0x90);
    *(u16*)(d + 0xE8) = 0;
    *(u8*)(d + 0xEA) = (u8)(flag & 1);
    func_80033DF0(d);
    return (s32)(*(s16*)(d + 0xE8)) << 2;
}
#else
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", SystemRenderStringEntry);
#endif

s32 func_80034F98(s32 arg0, s32 arg1) {
    u16 lead = arg0;
    u16 trail = arg1;

    if (lead == 0) {
        if ((s32)(trail - D_80059364) < (s32)D_80059354) {
            return 2;
        }
        return 3;
    }

    if (lead == D_8005934C) {
        if ((s32)trail < (s32)D_80059358) {
            return 2;
        }
    }

    return 3;
}

#ifdef XENO_PC_PORT
static void func_80034FFC_row0(u16* prev, u16* cur, u16* next, u16 glyph) {
    u16 v;
    u16 w;

    v = (glyph & 0x80) ? 0x222 : 0;
    if (glyph & 0x40) v |= 0x2220;
    if (glyph & 0x20) v |= 0x2200;
#if defined(WM_34FFC_MUTANT_REVERSE_ROW0_OUTLINE)
    w = (glyph & 0x10) ? v : (v | 0x2000);
#else
    /* Retail 0x8003518C..0x80035198 adds the outline shade when
     * the corresponding glyph pixel is present.  The old ternary had
     * this polarity reversed and painted every empty pixel instead. */
    w = (glyph & 0x10) ? (v | 0x2000) : v;
#endif
    prev[0] |= w;
    next[0] |= w;

    v = (glyph & 0x80) ? 0x212 : 0;
    if (glyph & 0x40) v |= 0x2120;
    if (glyph & 0x20) v |= 0x1200;
    cur[0] |= (glyph & 0x10) ? (u16)(v | 0x2000) : v;

    if (glyph & 0x08) {
        v = 0x222;
    } else if (glyph & 0x10) {
        v = 0x22;
    } else {
        v = (glyph >> 4) & 0x2;
    }
    if (glyph & 0x04) v |= 0x2220;
    if (glyph & 0x02) v |= 0x2200;
#if defined(WM_34FFC_MUTANT_REVERSE_ROW0_OUTLINE)
    w = (glyph & 0x01) ? v : (v | 0x2000);
#else
    w = (glyph & 0x01) ? (v | 0x2000) : v;
#endif
    prev[1] |= w;
    next[1] |= w;

    v = (glyph >> 4) & 0x2;
    if (glyph & 0x10) v |= 0x21;
    if (glyph & 0x08) v |= 0x212;
    if (glyph & 0x04) v |= 0x2120;
    if (glyph & 0x02) v |= 0x1200;
    cur[1] |= (glyph & 0x01) ? (u16)(v | 0x2000) : v;

    if (glyph & 0x8000) {
        v = 0x222;
    } else if (glyph & 0x0001) {
        v = 0x22;
    } else {
        v = glyph & 0x2;
    }
    if (glyph & 0x4000) v |= 0x2220;
    if (glyph & 0x2000) v |= 0x2200;
#if defined(WM_34FFC_MUTANT_REVERSE_ROW0_OUTLINE)
    w = (glyph & 0x1000) ? v : (v | 0x2000);
#else
    w = (glyph & 0x1000) ? (v | 0x2000) : v;
#endif
    prev[2] |= w;
    next[2] |= w;

    v = glyph & 0x2;
    if (glyph & 0x0001) v |= 0x21;
    if (glyph & 0x8000) v |= 0x212;
    if (glyph & 0x4000) v |= 0x2120;
    if (glyph & 0x2000) v |= 0x1200;
    cur[2] |= (glyph & 0x1000) ? (u16)(v | 0x2000) : v;
}
#endif

#ifdef XENO_PC_PORT
static void func_80034FFC_row1(u16* prev, u16* cur, u16* next, u16 glyph) {
    u16 v;
    u16 w;

    v = (glyph & 0x80) ? 0x888 : 0;
    if (glyph & 0x40) v |= 0x8880;
    if (glyph & 0x20) v |= 0x8800;
#if defined(WM_34FFC_MUTANT_REVERSE_ROW1_OUTLINE)
    w = (glyph & 0x10) ? v : (v | 0x8000);
#else
    /* Retail 0x80035474..0x80035480 has the same present-pixel
     * polarity for the other interleaved texture page. */
    w = (glyph & 0x10) ? (v | 0x8000) : v;
#endif
    prev[0] |= w;
    next[0] |= w;

    v = (glyph & 0x80) ? 0x848 : 0;
    if (glyph & 0x40) v |= 0x8480;
    if (glyph & 0x20) v |= 0x4800;
    cur[0] |= (glyph & 0x10) ? (u16)(v | 0x8000) : v;

    if (glyph & 0x08) {
        v = 0x888;
    } else if (glyph & 0x10) {
        v = 0x88;
    } else {
        v = (glyph >> 2) & 0x8;
    }
    if (glyph & 0x04) v |= 0x8880;
    if (glyph & 0x02) v |= 0x8800;
#if defined(WM_34FFC_MUTANT_REVERSE_ROW1_OUTLINE)
    w = (glyph & 0x01) ? v : (v | 0x8000);
#else
    w = (glyph & 0x01) ? (v | 0x8000) : v;
#endif
    prev[1] |= w;
    next[1] |= w;

    v = (glyph >> 2) & 0x8;
    if (glyph & 0x10) v |= 0x84;
    if (glyph & 0x08) v |= 0x848;
    if (glyph & 0x04) v |= 0x8480;
    if (glyph & 0x02) v |= 0x4800;
    cur[1] |= (glyph & 0x01) ? (u16)(v | 0x8000) : v;

    if (glyph & 0x8000) {
        v = 0x888;
    } else if (glyph & 0x0001) {
        v = 0x88;
    } else {
        v = (glyph << 2) & 0x8;
    }
    if (glyph & 0x4000) v |= 0x8880;
    if (glyph & 0x2000) v |= 0x8800;
#if defined(WM_34FFC_MUTANT_REVERSE_ROW1_OUTLINE)
    w = (glyph & 0x1000) ? v : (v | 0x8000);
#else
    w = (glyph & 0x1000) ? (v | 0x8000) : v;
#endif
    prev[2] |= w;
    next[2] |= w;

    v = (glyph << 2) & 0x8;
    if (glyph & 0x0001) v |= 0x84;
    if (glyph & 0x8000) v |= 0x848;
    if (glyph & 0x4000) v |= 0x8480;
    if (glyph & 0x2000) v |= 0x4800;
    cur[2] |= (glyph & 0x1000) ? (u16)(v | 0x8000) : v;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/system", func_80034FFC);
#else
void func_80034FFC(s32 arg0, s32 arg1, void* arg2, s32 arg3, s32 arg4) {
    u16 lead = arg0;
    u16 trail = arg1;
    u8* glyphData;
    u16* prev;
    u16* cur;
    u16* next;
    s32 stride = (s16)arg3;
    s32 step = stride * 2;
    s32 i;
    u16 clearMask;

    if (lead == 0) {
        /* Original PS1 asm computes the glyph offset in signed 32-bit arithmetic,
           so low glyph codes (trail < D_80059364) index NEGATIVELY into the font
           block below D_8005935C. Compute the delta as a signed s32 so it sign-
           extends when added to the 64-bit host pointer; unsigned subtraction here
           would zero-extend (e.g. -286 -> 0xFFFFFEE2) and corrupt the address. */
        s32 glyphOffset = ((s32)trail - (s32)D_80059364) * 0x16;
        glyphData = (u8*)(uintptr_t)D_8005935C + glyphOffset;
    } else if (lead == 0xFF && trail == 0xFF) {
        glyphData = (u8*)D_800501D0;
    } else {
        s32 leadOffset = ((s32)lead - (s32)D_8005934C) * 0x1600;
        s32 trailOffset = (s32)trail * 0x16;
        glyphData = (u8*)(uintptr_t)D_8005935C + D_80059350 + trailOffset + leadOffset;
    }

    clearMask = arg4 ? 0x3333 : 0xCCCC;
    prev = arg2;
    prev[0] &= clearMask;
    prev[1] &= clearMask;
    prev[2] &= clearMask;

    cur = (u16*)((u8*)arg2 + step);
    cur[0] &= clearMask;
    cur[1] &= clearMask;
    cur[2] &= clearMask;

    next = (u16*)((u8*)cur + step);

    for (i = 0; i < 0xB; i++) {
        u16 glyph = *(u16*)glyphData;

        next[0] &= clearMask;
        next[1] &= clearMask;
        next[2] &= clearMask;

        if (arg4) {
            func_80034FFC_row1(prev, cur, next, glyph);
        } else {
            func_80034FFC_row0(prev, cur, next, glyph);
        }

        glyphData += 2;
        prev = (u16*)((u8*)prev + step);
        cur = (u16*)((u8*)cur + step);
        next = (u16*)((u8*)next + step);
    }
}
#endif
