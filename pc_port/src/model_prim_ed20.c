/*
 * Retail 0x8002ED20..0x8002EEF8 (disc/SLUS_006.64), with the shared walker
 * tail at 0x8002E1F4: D_8004FE50 row 0x00 variant 1, the lit small
 * three-vertex packet walker (packet 0x14, tag length 0x04000000). It is
 * the sibling of func_8002F2E0 (row 0x01 variant 1): the same RTPT
 * pipeline with lookahead, but a twelve-byte lighting record per primitive
 * at D_80059498 (colour word, then the face normal) fed through NCCS, and
 * the colour word's code byte kept for the packet's RGB word. The
 * field object overlay draws its models with variant 1 (func_801DCEC8), so
 * this row was reached by MAP16's follower models.
 *
 * As in func_8002F2E0: the lookahead and the final RTPT are kept, including
 * for count == 0; rejected triangles still write XY words; the MFC2 $31 gate
 * reads LZCR (inert on hardware); the 24-bit packet-pointer mask is left to
 * the link adapter; there is no OTZ-zero rejection in this walker.
 */
#include <stdint.h>

#include "common.h"
#include "psx/inline_c.h"
#include "model_prim_link.h"

extern u32 D_8005953C;
extern u32 D_80059498;
extern u8* D_80059424;
extern u32 D_80059568;
extern s32 D_80059578;
extern s32 D_800500F8;
extern s32 D_800500FC;
extern s32 D_80050100;

s32 func_8002ED20(u8* pCmd, s32 count)
{
    u8* vertices = (u8*)(uintptr_t)D_8005953C;
    u8* records = (u8*)(uintptr_t)D_80059498;
    uintptr_t packet = (uintptr_t)D_80059424 - 0x14u;
    u32* ot = (u32*)(uintptr_t)D_80059568;
    u32 emitted = (u32)D_80059578;
    u32 yBound = (u32)D_800500FC;
    u32 xBound = (u32)D_800500F8;
    u32 shift = (u32)D_80050100 & 31u;
    u32 remaining = (u32)count;
    u32 cmd = *(u32*)pCmd;
    u8* nextV0 = vertices + ((cmd & 0xffffu) << 3);
    u8* nextV1 = vertices + ((cmd >> 13) & 0xfff8u);
    u8* nextV2 = vertices + ((u32)*(u16*)(pCmd + 4) << 3);

    MTC2(*(u32*)nextV1, 2);
    MTC2(*(u32*)(nextV1 + 4), 3);
    MTC2(*(u32*)nextV2, 4);
    MTC2(*(u32*)(nextV2 + 4), 5);
    for (;;) {
        u32 xy0, xy1, xy2, colour, rgb;
        s32 otz;
        u8* out;

        MTC2(*(u32*)nextV0, 0);
        MTC2(*(u32*)(nextV0 + 4), 1);
        gte_rtpt();
        if (remaining == 0) break;
        remaining--;
        pCmd += 8;
        packet += 0x14u;
        cmd = *(u32*)pCmd;
        nextV0 = vertices + ((cmd & 0xffffu) << 3);
        records += 0xC;
        nextV1 = vertices + ((cmd >> 13) & 0xfff8u);
        nextV2 = vertices + ((u32)*(u16*)(pCmd + 4) << 3);
        MTC2(*(u32*)nextV1, 2);
        MTC2(*(u32*)(nextV1 + 4), 3);
        MTC2(*(u32*)nextV2, 4);
        MTC2(*(u32*)(nextV2 + 4), 5);

        /* MFC2 $31 reads LZCR, not the FLAG control register. */
        if ((s32)MFC2(31) < 0) continue;
        xy0 = MFC2(12);
        xy1 = MFC2(13);
        xy2 = MFC2(14);
        gte_nclip();
        if (!(xy0 < yBound || xy1 < yBound || xy2 < yBound)) continue;
        if (!((xy0 & 0xffffu) < xBound ||
              (xy1 & 0xffffu) < xBound ||
              (xy2 & 0xffffu) < xBound)) continue;

        out = (u8*)packet;
        *(u32*)(out + 0x08) = xy0;
        gte_avsz3();
        *(u32*)(out + 0x0C) = xy1;
        if ((s32)MFC2(24) <= 0) continue;
        *(u32*)(out + 0x10) = xy2; /* delay slot, even for backfaces */
        otz = (s32)MFC2(7);
        emitted++;
        colour = *(u32*)(records - 0xC);
        MTC2(*(u32*)(records - 8), 0);
        MTC2(*(u32*)(records - 4), 1);
        MTC2(colour, 6);
        gte_nccs();
        rgb = MFC2(22);
        *(u32*)(out + 0x04) = (colour & 0xff000000u) | (rgb & 0x00ffffffu);
        PcPort_LinkModelPrim(ot, otz >> shift, out, 0x04000000u);
    }
    D_80059498 = (u32)(uintptr_t)records;
    D_80059578 = (s32)emitted;
    D_80059424 = (u8*)(packet + 0x14u);
    return (s32)yBound;
}
