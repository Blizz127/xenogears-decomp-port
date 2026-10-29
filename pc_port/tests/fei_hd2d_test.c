#include <assert.h>
#include "../src/fei_hd2d.c"

uint8_t g_PsxRam[PSX_RAM_SIZE];
unsigned short vram[1024 * 512];
unsigned g_PcPortPresentedFrames;

static FieldActor field[1];
static ActorData actor;
static unsigned char sprite[0x164];
static unsigned char work[4096] __attribute__((aligned(16)));
FieldActor* volatile g_FieldActors = field;
s32 g_PlayerActorIndex, g_FieldNumActors = 1;
u8 D_800ADB04 = 1;
void *g_GfxCurWorkBuffer, *g_GfxCurWorkBufferEnd;
static int fieldActive = 1, links, textures;
static void *linked[64], *linkedOt[64];
int PcPort_QuickCheckpointFieldIsActive(void) { return fieldActive; }
unsigned int GR_CreateRGBATexture(int w, int h, unsigned char *data)
{ assert(w > 0 && h > 0 && data); return 100 + textures++; }
void PcPort_AddPrimDomainAware(void *ot, void *p)
{ assert(ot); linkedOt[links] = ot; linked[links++] = p; }
void SetPsyXTexture(DR_PSYX_TEX *p, unsigned int t, int w, int h)
{ setlen(p, 2); p->code[0] = 0xB1000000 | t; p->code[1] = w | (h<<16); }

/* A retail part: 16x16 texels at (u, v) of a 15-bit page, drawn at (x, y). */
static void Part(POLY_FT4 *p, int x, int y, int u, int v)
{
    memset(p, 0, sizeof *p);
    setPolyFT4(p);
    p->code |= 1; /* raw-colour sprite */
    p->tpage = 2 << 7; /* 15-bit direct, page 0 */
    setXY4(p, x, y, x + 16, y, x, y + 16, x + 16, y + 16);
    setUV4(p, u, v, u + 15, v, u, v + 15, u + 15, v + 15);
}
static int Cell(void) { return 0; }

int main(void)
{
    PcPortFeiHd2dBatch b;
    POLY_FT4 original = {0}, top, legs, *replacement;
    unsigned int ot, otLegs;
    int u, v, c;
    field[0].pActorData = (u32)(uintptr_t)&actor;
    field[0].pSpriteData = (u32)(uintptr_t)sprite;
    g_GfxCurWorkBuffer = work; g_GfxCurWorkBufferEnd = work + sizeof(work);
    setenv("XENO_FEI_HD2D", "0", 1);
    assert(PcPort_FeiHd2dEnabled() == 0);
    enabled = -1;
    unsetenv("XENO_FEI_HD2D");
    assert(PcPort_FeiHd2dEnabled() == 1);
    PcPort_FeiHd2dToggle(); assert(!PcPort_FeiHd2dEnabled());
    PcPort_FeiHd2dBegin(&b, sprite, 1); assert(!b.active);
    PcPort_FeiHd2dToggle();
    assert(LoadTexture());
    assert(textures == CELLS);
    for (c = 0; c < CELLS; ++c) /* padded: half the art box on every side */
        assert(cellTex[c].w == cells[c].x1 - cells[c].x0 + 1 + 2 * cellTex[c].padX &&
               cellTex[c].padX >= (cells[c].x1 - cells[c].x0 + 1) / 2 &&
               cellTex[c].padY >= (cells[c].y1 - cells[c].y0 + 1) / 2);

    /* Two parts at different OT depths (the legs one slot nearer the camera,
     * as a table in front would be): each is replaced in its own quad and
     * its own OT slot. Texels: head part fully opaque; legs part opaque but
     * its bottom 4 rows transparent (padding the silhouette must skip). */
    for (v = 0; v < 32; ++v) for (u = 0; u < 16; ++u) vram[v * 1024 + u] = v < 28 ? 0x7FFF : 0;
    *(u16 *)(sprite + 0x80) = 1024; /* camera-relative front facing: cell 0 */
    Part(&top, 100, 50, 0, 0);
    Part(&legs, 100, 66, 0, 16);
    links = 0;
    PcPort_FeiHd2dBegin(&b, sprite, 2); assert(b.active);
    assert(PcPort_FeiHd2dCapture(&b, &top, &ot));
    assert(PcPort_FeiHd2dCapture(&b, &legs, &otLegs));
    PcPort_FeiHd2dEnd(&b);
    assert(links == 6);
    assert(linkedOt[0] == &ot && linkedOt[1] == &ot && linkedOt[2] == &ot);
    assert(linkedOt[3] == &otLegs && linkedOt[4] == &otLegs && linkedOt[5] == &otLegs);
    assert(linked[1] == &top && linked[4] == &legs); /* the retail packets, reused */
    assert((((DR_PSYX_TEX*)linked[0])->code[0] & 0xFFFFFF) == 0);          /* reset */
    assert((((DR_PSYX_TEX*)linked[2])->code[0] & 0xFFFFFF) == cellTex[Cell()].tex); /* bind */
    /* footprint: screen quads untouched */
    assert(top.x0 == 100 && top.y0 == 50 && top.x3 == 116 && top.y3 == 66);
    assert(legs.x0 == 100 && legs.y0 == 66 && legs.x3 == 116 && legs.y3 == 82);
    /* UVs: silhouette box (100,50)-(116,78) lands on the art box; seam shared */
    {
        const int w = cellTex[0].w, h = cellTex[0].h;
        const int bw = cells[0].x1 - cells[0].x0 + 1, bh = cells[0].y1 - cells[0].y0 + 1;
        const int artL = cellTex[0].padX * 256 / w, artT = cellTex[0].padY * 256 / h;
        const int artR = (cellTex[0].padX + bw) * 256 / w, artB = (cellTex[0].padY + bh) * 256 / h;
        assert(abs(top.u0 - artL) <= 1 && abs(top.u1 - artR) <= 1 && abs(top.v0 - artT) <= 1);
        assert(top.v2 == legs.v0 && top.u2 == legs.u0);
        /* y = 78 is the art box bottom, so the part's bottom edge (82) is below it */
        assert(legs.v2 > artB && abs((legs.v2 - artT) * 28 - (artB - artT) * 32) <= 28 + 32);
    }
    assert(top.r0 == 255 && !(top.code & 1) && (top.code & 2));
    assert(top.tpage == 0 && top.clut == 0);

    /* colour: modulated retail colour doubled as before */
    Part(&top, 100, 50, 0, 0); top.code &= ~1; top.r0 = 64; top.g0 = 32; top.b0 = 128;
    links = 0;
    PcPort_FeiHd2dBegin(&b, sprite, 1);
    PcPort_FeiHd2dCapture(&b, &top, &ot); PcPort_FeiHd2dEnd(&b);
    assert(links == 3 && linked[1] == &top);
    assert(top.r0 == 128 && top.g0 == 64 && top.b0 == 255);

    /* test cycle: retail / HD / hidden by present index */
    setenv("XENO_FEI_HD2D_TEST_CYCLE", "1", 1); s_testCycle = -1;
    g_PcPortPresentedFrames = 2; /* next present 3: block 0, retail */
    Part(&top, 100, 50, 0, 0); links = 0;
    PcPort_FeiHd2dBegin(&b, sprite, 1); PcPort_FeiHd2dCapture(&b, &top, &ot); PcPort_FeiHd2dEnd(&b);
    assert(links == 1 && linked[0] == &top && top.u1 == 15 && top.tpage == (2 << 7));
    g_PcPortPresentedFrames = 2 * TEST_BLOCK + 3; /* block 2: hidden */
    links = 0;
    PcPort_FeiHd2dBegin(&b, sprite, 1); PcPort_FeiHd2dCapture(&b, &top, &ot); PcPort_FeiHd2dEnd(&b);
    assert(links == 0);
    unsetenv("XENO_FEI_HD2D_TEST_CYCLE"); s_testCycle = -1;

    /* no opaque texel: retail */
    memset(vram, 0, sizeof vram);
    Part(&top, 100, 50, 0, 0); links = 0;
    PcPort_FeiHd2dBegin(&b, sprite, 1); PcPort_FeiHd2dCapture(&b, &top, &ot); PcPort_FeiHd2dEnd(&b);
    assert(links == 1 && linked[0] == &top && top.tpage == (2 << 7));

    actor.characterId = 1; PcPort_FeiHd2dBegin(&b, sprite, 1); assert(!b.active);
    actor.characterId = 0; actor.curAnimationId = 3;
    PcPort_FeiHd2dBegin(&b, sprite, 1); assert(!b.active);
    actor.curAnimationId = 0; fieldActive = 0;
    PcPort_FeiHd2dBegin(&b, sprite, 1); assert(!b.active);
    fieldActive = 1; D_800ADB04 = 0;
    PcPort_FeiHd2dBegin(&b, sprite, 1); assert(!b.active);
    D_800ADB04 = 1; g_GfxCurWorkBufferEnd = (u8*)g_GfxCurWorkBuffer + 40;
    PcPort_FeiHd2dBegin(&b, sprite, 1); assert(!b.active);
    g_GfxCurWorkBufferEnd = work + sizeof(work); texture = 0;
    PcPort_FeiHd2dBegin(&b, sprite, 1); assert(!b.active);
    (void)original; (void)replacement;
    puts("Fei HD2D: toggle, padded cells, per-part quads and OT slots, silhouette UV map, colour, test cycle, fallbacks PASS");
    return 0;
}
