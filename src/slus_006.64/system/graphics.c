#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"

RECT* D_800592F0;
u_long* D_800592F4;

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/graphics", GfxLoadImageAccelerated);
#else
void GfxLoadImageAccelerated(void) {
    LoadImage(D_800592F0, D_800592F4);
}
#endif
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/graphics", GfxLoadClutsAccelerated);
#else
/*
Matches on ASPSX 2.56, GCC 2.6.0

void GfxLoadImageAccelerated(void) {
    char* pBuffer;

    pBuffer = HeapAlloc(0x2000, 1);
    
    asm(""
        "\tmove  $t0, %0\n"
        "\tsw    $sp, 0($t0)\n"
        "\taddiu $t0, $t0, -4\n"
        "\taddu  $sp, $t0, $zero\n"
    :
    : "r"(pBuffer + 0x1F00)
    : "t0", "memory"
    );
    
    LoadImage(D_800592F0, D_800592F4);
    
    asm(""
        "\taddiu $sp, $sp, 4\n"
        "\tlw    $sp, 0($sp)\n"
    ::
    : "memory"
    );
    
    HeapFree(pBuffer);
}
*/

void GfxLoadClutsAccelerated(void* pClutData, s32 x, s32 y) {
    RECT rect;
    u8* pData = pClutData;
    s32 count = *(s32*)pData;
    s32 i;

    pData += 4;
    for (i = 0; i < count; i++) {
        u8* pClut = (u8*)pClutData + ((s32*)pData)[i];
        rect.x = x + (i * 0x40);
        rect.y = y;
        rect.w = *(u16*)(pClut + 0x0);
        rect.h = *(u16*)(pClut + 0x2);
        D_800592F0 = &rect;
        D_800592F4 = (u_long*)(pClut + 0x4);
        GfxLoadImageAccelerated();
    }
}
#endif
