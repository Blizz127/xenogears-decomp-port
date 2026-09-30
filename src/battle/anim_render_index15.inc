/* Retail main-exe 800257F0 (animation-render callback index 15) and its
 * battle-overlay renderer dependency 800B1F6C / 800B1F0C - native port
 * implementation.
 *
 * This is a behaviour-preserving reimplementation, not a matching
 * transcription: retail 800B1F6C is 2944 bytes of GTE-heavy overlay code and
 * this body compiles to a fraction of that.  The matching build therefore
 * assembles the retail body from splat instead (INCLUDE_ASM in
 * src/battle/main77.c); this file is used by the port only, via
 * pc_port/src/game_overrides.c.  It becomes the matching source again only
 * once it is transcribed retail-shaped and proven byte-exact.
 *
 * Pointer rule: every pointer argument that may be a guest PSX address
 * (KSEG0 0x8000_0000 / KSEG1 0xA000_0000) is translated with PSX_ADDR, and
 * native host pointers are passed through unchanged.  Packed descriptor
 * offsets stay 32-bit and are added to the already-translated base.
 *
 * Packet offsets are PSX bytes, independent of host GPU structure layouts. */
#ifdef XENO_PC_PORT
#include "psx_memory.h"
#include "guest_prim_link.h"
#endif

/* Guest data objects the callback/renderer read or write.  These live in the
 * retail .sdata (0x8004EA90..0x800576E4, copied into g_PsxRam by
 * PsxMemory_LoadStaticData) or the battle overlay image, so the authoritative
 * bytes are the guest-mapped ones, not the zeroed host data stubs. */
#define ANIM_RENDER_COLOR_MATRIX  0x8004FD80u
#define ANIM_RENDER_LIGHT_MATRIX  0x8004FDA0u
#define ANIM_RENDER_TPAGE_PACKET  0x800C3BF8u

/* Translate a possibly-guest pointer.  The two-alias test must run before any
 * translation: feeding a native low pointer through PSX_ADDR would alias into
 * g_PsxRam and corrupt it. */
static u8* BattleAnimRenderAddress(uintptr_t address)
{
    if ((address & ~(uintptr_t)0x1FFFFFu) == 0x80000000u ||
        (address & ~(uintptr_t)0x1FFFFFu) == 0xA0000000u)
        return (u8*)PSX_ADDR(address);
    return (u8*)address;
}

/* The port keeps these as packed 32-bit host addresses (all code/data live
 * below 4 GiB in the -no-pie build), matching temp1.c's other work-buffer
 * users. */
extern u32 g_GfxCurWorkBuffer;
extern u32 g_GfxCurWorkBufferEnd;

/* Retail 800B1F0C: append the fixed 8-byte D_800C3BF8 draw-env packet to the
 * OT slot, provided the work buffer has room for one more packet. */
void func_800B1F0C(u32* otEntry)
{
    u32* work = (u32*)(uintptr_t)g_GfxCurWorkBuffer;
    u32* end = (u32*)(uintptr_t)g_GfxCurWorkBufferEnd;
    u32* packet = (u32*)PSX_ADDR(ANIM_RENDER_TPAGE_PACKET);

    if ((uintptr_t)work + 8u >= (uintptr_t)end)
        return;
    g_GfxCurWorkBuffer = (u32)(uintptr_t)(work + 2);
    work[0] = packet[0];
    work[1] = packet[1];
    PcPort_AddPrimDomainAware(otEntry, work);
}

/* Shared triangle body of the eight jump-table emitters.  Retail order is
 * RTPT, CFC2 FLAG, NCLIP, store MAC0, reject on FLAG bit 14, AVSZ3, reject on
 * NCLIP <= 0, store OTZ, depth, reject >= 0x1000, store SXY0..2, link, and
 * (for the emitters whose tail reaches 800B289C) the optional draw-env
 * packet. */
static void BattleAnimRenderTriangle(u8* v0, u8* v1, u8* v2, u8* out,
                                     u8* otBase, s32 depthBias, s32 tpage,
                                     s32 o0, s32 o1, s32 o2, int drawEnv)
{
    long flag = 0;
    long nclip = 0;
    long otz = 0;
    s32 depth;

    gte_ldv3((SVECTOR*)v0, (SVECTOR*)v1, (SVECTOR*)v2);
    gte_rtpt();
    gte_stflg(&flag);
    gte_nclip();
    gte_stopz(&nclip);
    if (((u32)flag & 0x4000u) != 0)
        return;
    gte_avsz3();
    if ((s32)(u32)nclip <= 0)
        return;
    gte_stotz(&otz);

    depth = (s32)(((s32)(u32)otz) >> ((u32)D_80050100 & 31u)) + depthBias;
    if (depth < 5)
        depth = 5;
    if (depth >= 0x1000)
        return;

    *(u32*)(out + o0) = MFC2(12);
    *(u32*)(out + o1) = MFC2(13);
    *(u32*)(out + o2) = MFC2(14);

    PcPort_AddPrimDomainAware(otBase + (u32)depth * 4u, out);
    if (drawEnv && tpage != 0)
        func_800B1F0C((u32*)(otBase + (u32)depth * 4u));
}

/* Shared quad body of the four jump-table emitters.  RotAverageNclip4 writes
 * all four SXY outputs (conditionally) and returns NCLIP's MAC0. */
static void BattleAnimRenderQuad(u8* v0, u8* v1, u8* v2, u8* v3, u8* out,
                                 u8* otBase, s32 depthBias, s32 tpage,
                                 s32 o0, s32 o1, s32 o2, s32 o3, int drawEnv)
{
    long xy0 = 0;
    long xy1 = 0;
    long xy2 = 0;
    long xy3 = 0;
    long p = 0;
    long otz = 0;
    long flag = 0;
    long result;
    s32 depth;

    result = RotAverageNclip4((SVECTOR*)v0, (SVECTOR*)v1, (SVECTOR*)v2,
                              (SVECTOR*)v3, &xy0, &xy1, &xy2, &xy3,
                              &p, &otz, &flag);
    if ((s32)(u32)result <= 0)
        return;

    depth = (s32)(((s32)(u32)otz) >> ((u32)D_80050100 & 31u)) + depthBias;
    if (depth < 5)
        depth = 5;
    if (((u32)flag & 0x8000u) != 0)
        return;
    if (depth >= 0x1000)
        return;

    *(u32*)(out + o0) = (u32)xy0;
    *(u32*)(out + o1) = (u32)xy1;
    *(u32*)(out + o2) = (u32)xy2;
    *(u32*)(out + o3) = (u32)xy3;

    PcPort_AddPrimDomainAware(otBase + (u32)depth * 4u, out);
    if (drawEnv && tpage != 0)
        func_800B1F0C((u32*)(otBase + (u32)depth * 4u));
}

/* Retail battle-overlay 800B1F6C: walk the indexed-primitive descriptor stream
 * at the geometry header and emit triangles/quads into the output packet
 * buffer, linking each into the current OT. */
void func_800B1F6C(void* model, void* buffer, void* ot, s32 unused,
                   s32 depthBias, s32 tpage)
{
    u8* geom = BattleAnimRenderAddress((uintptr_t)model);
    u8* out = BattleAnimRenderAddress((uintptr_t)buffer);
    u8* otBase = BattleAnimRenderAddress((uintptr_t)ot);
    u32 count;
    u32 index;
    u8* record;
    u8* verts;
    u8* normals;

    (void)unused;

    if (tpage != 0)
        SetDrawTPage((DR_TPAGE*)PSX_ADDR(ANIM_RENDER_TPAGE_PACKET), 0, 0,
                     ((tpage - 1) & 3) << 5);

    count = *(u32*)(geom + 0x14);
    record = geom + *(u32*)(geom + 0x10);
    verts = geom + *(u32*)(geom + 0x00);
    normals = geom + *(u32*)(geom + 0x08);
    if (count == 0)
        return;

    for (index = 0; index != count; ++index) {
        u32 key = (record[3] & 0x1Cu) | (((record[2] ^ 1u) & 1u) << 8);
        u8* v0 = verts;
        u8* v1 = verts;
        u8* v2 = verts;
        u8* v3 = verts;
        u32 shape;

        /* First switch: full key (shape bits plus the colour-source bit). */
        switch (key) {
        case 0x000:
            v0 = verts + (u32)(*(u16*)(record + 0x08)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x0A)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x0C)) * 8u;
            break;
        case 0x004:
            v0 = verts + (u32)(*(u16*)(record + 0x14)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x16)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x18)) * 8u;
            break;
        case 0x008:
            v0 = verts + (u32)(*(u16*)(record + 0x08)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x0A)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x0C)) * 8u;
            v3 = verts + (u32)(*(u16*)(record + 0x0E)) * 8u;
            break;
        case 0x00C:
            v0 = verts + (u32)(*(u16*)(record + 0x18)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x1A)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x1C)) * 8u;
            v3 = verts + (u32)(*(u16*)(record + 0x1E)) * 8u;
            break;
        case 0x010:
            v0 = verts + (u32)(*(u16*)(record + 0x10)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x12)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x14)) * 8u;
            break;
        case 0x014:
            v0 = verts + (u32)(*(u16*)(record + 0x1C)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x1E)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x20)) * 8u;
            break;
        case 0x018:
            v0 = verts + (u32)(*(u16*)(record + 0x14)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x16)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x18)) * 8u;
            v3 = verts + (u32)(*(u16*)(record + 0x1A)) * 8u;
            break;
        case 0x01C:
            v0 = verts + (u32)(*(u16*)(record + 0x24)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x26)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x28)) * 8u;
            v3 = verts + (u32)(*(u16*)(record + 0x2A)) * 8u;
            break;
        case 0x100: {
            u8 saved = out[7];
            NormalColor((SVECTOR*)(normals + (u32)(*(u16*)(record + 0x08)) * 8u),
                        (CVECTOR*)(out + 4));
            out[7] = saved;
            v0 = verts + (u32)(*(u16*)(record + 0x0A)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x0C)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x0E)) * 8u;
            break;
        }
        case 0x104: {
            u8 saved = out[7];
            NormalColor((SVECTOR*)(normals + (u32)(*(u16*)(record + 0x10)) * 8u),
                        (CVECTOR*)(out + 4));
            out[7] = saved;
            v0 = verts + (u32)(*(u16*)(record + 0x12)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x14)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x16)) * 8u;
            break;
        }
        case 0x108: {
            u8 saved = out[7];
            NormalColor((SVECTOR*)(normals + (u32)(*(u16*)(record + 0x08)) * 8u),
                        (CVECTOR*)(out + 4));
            out[7] = saved;
            v0 = verts + (u32)(*(u16*)(record + 0x0A)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x0C)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x0E)) * 8u;
            v3 = verts + (u32)(*(u16*)(record + 0x10)) * 8u;
            break;
        }
        case 0x10C:
            v0 = verts + (u32)(*(u16*)(record + 0x16)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x18)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x1A)) * 8u;
            v3 = verts + (u32)(*(u16*)(record + 0x1C)) * 8u;
            break;
        case 0x110:
            v0 = verts + (u32)(*(u16*)(record + 0x10)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x16)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x1A)) * 8u;
            break;
        case 0x114: {
            u8 saved = out[7];
            NormalColor3((SVECTOR*)(normals + (u32)(*(u16*)(record + 0x10)) * 8u),
                         (SVECTOR*)(normals + (u32)(*(u16*)(record + 0x14)) * 8u),
                         (SVECTOR*)(normals + (u32)(*(u16*)(record + 0x18)) * 8u),
                         (CVECTOR*)(out + 4), (CVECTOR*)(out + 0x10),
                         (CVECTOR*)(out + 0x1C));
            out[7] = saved;
            v0 = verts + (u32)(*(u16*)(record + 0x12)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x16)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x1A)) * 8u;
            break;
        }
        case 0x118:
        case 0x11C:
            v0 = verts + (u32)(*(u16*)(record + 0x16)) * 8u;
            v1 = verts + (u32)(*(u16*)(record + 0x1A)) * 8u;
            v2 = verts + (u32)(*(u16*)(record + 0x1E)) * 8u;
            v3 = verts + (u32)(*(u16*)(record + 0x22)) * 8u;
            break;
        default:
            return;
        }

        /* Second switch: shape bits only. */
        shape = record[3] & 0x1Cu;
        if (shape >= 0x1Du)
            goto next_record;
        switch (shape) {
        case 0x00:
            BattleAnimRenderTriangle(v0, v1, v2, out, otBase, depthBias,
                                      tpage, 0x08, 0x0C, 0x10, 1);
            break;
        case 0x04:
            BattleAnimRenderTriangle(v0, v1, v2, out, otBase, depthBias,
                                      tpage, 0x08, 0x10, 0x18, 0);
            break;
        case 0x08:
            BattleAnimRenderQuad(v0, v1, v2, v3, out, otBase, depthBias,
                                 tpage, 0x08, 0x0C, 0x10, 0x14, 1);
            break;
        case 0x0C:
            BattleAnimRenderQuad(v0, v1, v2, v3, out, otBase, depthBias,
                                 tpage, 0x08, 0x10, 0x18, 0x20, 0);
            break;
        case 0x10:
            BattleAnimRenderTriangle(v0, v1, v2, out, otBase, depthBias,
                                      tpage, 0x08, 0x10, 0x18, 1);
            break;
        case 0x14:
            BattleAnimRenderTriangle(v0, v1, v2, out, otBase, depthBias,
                                      tpage, 0x08, 0x14, 0x20, 0);
            break;
        case 0x18:
            BattleAnimRenderQuad(v0, v1, v2, v3, out, otBase, depthBias,
                                 tpage, 0x08, 0x10, 0x18, 0x20, 1);
            break;
        case 0x1C:
            BattleAnimRenderQuad(v0, v1, v2, v3, out, otBase, depthBias,
                                 tpage, 0x08, 0x14, 0x20, 0x2C, 0);
            break;
        default:
            break;
        }

    next_record:
        out += ((u32)record[0] + 1u) * 4u;
        record += ((u32)record[1] + 1u) * 4u;
    }
}

/* Retail main-exe 800257F0: animation-render callback index 15.  The work-list
 * stores 32-bit packed pointers whose domain may be a native low pointer or a
 * KSEG0/KSEG1 alias, so the two packed loads are translated. */
void func_800257F0(u8* pArg)
{
    u8* pSprite = BattleAnimRenderAddress((uintptr_t)*(u32*)(pArg + 4));
    u8* pData;
    MATRIX matA;
    MATRIX matB;
    MATRIX comp;
    VECTOR trans;
    long geomX = 0;
    long geomY = 0;
    u8 flag;
    s32 halved;
    s32 saved;

    func_80022038(pSprite);
    pData = BattleAnimRenderAddress((uintptr_t)*(u32*)(pSprite + 0x20));
    if (*(u32*)(pData + 0x34) == 0)
        return;

    if (((*(u32*)(pSprite + 0x40) >> 1) & 1) != 0) {
        u16* colorMatrix = (u16*)PSX_ADDR(ANIM_RENDER_COLOR_MATRIX);
        PushMatrix();
        colorMatrix[0] = *(u16*)(pData + 0x4C);
        colorMatrix[3] = *(u16*)(pData + 0x4E);
        colorMatrix[6] = *(u16*)(pData + 0x50);
        RotMatrix((SVECTOR*)(pData + 0x44), &matA);
        MulMatrix0(&matA, (MATRIX*)(pData + 0x0C), &matA);
        MulMatrix0((MATRIX*)PSX_ADDR(ANIM_RENDER_LIGHT_MATRIX), &matA, &matB);
        SetBackColor(0x20, 0x20, 0x20);
        SetColorMatrix((MATRIX*)PSX_ADDR(ANIM_RENDER_COLOR_MATRIX));
        SetLightMatrix(&matB);
        PopMatrix();
    }

    trans.vx = *(s16*)(pSprite + 0x02);
    trans.vy = *(s16*)(pSprite + 0x06);
    trans.vz = *(s16*)(pSprite + 0x0A);
    TransMatrix((MATRIX*)(pData + 0x0C), &trans);

    if ((*(u8*)(pSprite + 0x3F) & 1) == 0) {
        CompMatrix(&D_8004FBB8, (MATRIX*)(pData + 0x0C), &comp);
        SetRotMatrix(&comp);
        SetTransMatrix(&comp);
    } else {
        SetRotMatrix((MATRIX*)(pData + 0x0C));
        SetTransMatrix((MATRIX*)(pData + 0x0C));
    }

    flag = *(u8*)(pSprite + 0x3C);
    if (*(s32*)(pSprite + 0x3C) < 0) {
        ReadGeomOffset(&geomX, &geomY);
        SetGeomOffset(0xA0, 0x70);
    }
    halved = (s32)((*(u32*)(pSprite + 0x3C)) >> 25) & 1;

    if (halved == 0) {
        func_800B1F6C((void*)(uintptr_t)*(u32*)(pData + 0x34),
                      (void*)(uintptr_t)*(u32*)(pData + 0x2C +
                                                (u32)g_GfxCurContext * 4u),
                      g_GfxCurOT, 0,
                      (s32)*(s16*)(pSprite + 0x30), (s32)(flag >> 5));
    } else {
        saved = D_80050100;
        D_80050100 = 0x10;
        func_800B1F6C((void*)(uintptr_t)*(u32*)(pData + 0x34),
                      (void*)(uintptr_t)*(u32*)(pData + 0x2C +
                                                (u32)g_GfxCurContext * 4u),
                      g_GfxCurOT, 0, 0xFEC, (s32)(flag >> 5));
        D_80050100 = saved;
    }

    if (*(s32*)(pSprite + 0x3C) < 0)
        SetGeomOffset((s32)geomX, (s32)geomY);
}

/* Retail main-exe 80025A88: the sibling animation callback the interpreted
 * battle overlay installs as a work-list callback (via func_800B6438).  Same
 * host-unsafe packed loads as 800257F0 plus an ApplyMatrix/camera write back
 * into the sprite.  Retail reads a5 from the low byte at +0x3C, so the top
 * three bits survive (not (word>>5)&1). */
void func_80025A88(u8* pEntry)
{
    u8* pSprite = BattleAnimRenderAddress((uintptr_t)*(u32*)(pEntry + 0x04));
    u8* pSub;
    SVECTOR pos;
    VECTOR result;
    u32 flags3C;
    s32 arg4;
    s32 oldShift;

    func_80022038(pSprite);
    pSub = BattleAnimRenderAddress((uintptr_t)*(u32*)(pSprite + 0x20));
    if (*(u32*)(pSub + 0x34) == 0)
        return;

    pos.vx = *(s16*)(pSprite + 0x02);
    pos.vy = *(s16*)(pSprite + 0x06);
    pos.vz = *(s16*)(pSprite + 0x0A);
    ApplyMatrix(&D_8004FBB8, &pos, &result);

    pSub = BattleAnimRenderAddress((uintptr_t)*(u32*)(pSprite + 0x20));
    *(s32*)(pSub + 0x20) = D_8004FBB8.t[0] + result.vx;
    *(s32*)(pSub + 0x24) = D_8004FBB8.t[1] + result.vy;
    *(s32*)(pSub + 0x28) = D_8004FBB8.t[2] + result.vz;

    SetRotMatrix((MATRIX*)(pSub + 0x0C));
    SetTransMatrix((MATRIX*)(pSub + 0x0C));

    flags3C = *(u32*)(pSprite + 0x3C);
    if ((flags3C >> 25) & 1) {
        oldShift = D_80050100;
        D_80050100 = 0x10;
        arg4 = 0xFEC;
    } else {
        arg4 = *(s16*)(pSprite + 0x30);
    }

    func_800B1F6C((void*)(uintptr_t)*(u32*)(pSub + 0x34),
                  (void*)(uintptr_t)*(u32*)(pSub + 0x2C +
                                            (u32)g_GfxCurContext * 4u),
                  g_GfxCurOT, 0, arg4,
                  (s32)((u32)(*(u8*)(pSprite + 0x3C)) >> 5));

    if ((flags3C >> 25) & 1)
        D_80050100 = oldShift;
}
