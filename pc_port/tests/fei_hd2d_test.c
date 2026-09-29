#include <assert.h>
#include "../src/fei_hd2d.c"

static FieldActor field[1];
static ActorData actor;
static unsigned char sprite[0x164];
static unsigned char work[4096] __attribute__((aligned(16)));
FieldActor* volatile g_FieldActors = field;
s32 g_PlayerActorIndex, g_FieldNumActors = 1;
u8 D_800ADB04 = 1;
void *g_GfxCurWorkBuffer, *g_GfxCurWorkBufferEnd;
static int fieldActive = 1, links;
static void *linked[64];
int PcPort_QuickCheckpointFieldIsActive(void) { return fieldActive; }
unsigned int GR_CreateRGBATexture(int w, int h, unsigned char *data)
{ assert(w == ATLAS_SIZE && h == ATLAS_SIZE && data); return 7; }
void PcPort_AddPrimDomainAware(void *ot, void *p)
{ assert(ot); linked[links++] = p; }
void SetPsyXTexture(DR_PSYX_TEX *p, unsigned int t, int w, int h)
{ setlen(p, 2); p->code[0] = 0xB1000000 | t; p->code[1] = w | (h<<16); }

int main(void)
{
    PcPortFeiHd2dBatch b;
    POLY_FT4 original = {0}, *replacement;
    unsigned int ot;
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
    PcPort_FeiHd2dBegin(&b, sprite, 1); assert(b.active);
    setPolyFT4(&original); original.code |= 1; /* retail raw-color sprite */
    setXY4(&original, 100, 50, 120, 50, 100, 90, 120, 90);
    assert(PcPort_FeiHd2dCapture(&b, &original, &ot));
    PcPort_FeiHd2dEnd(&b);
    assert(links == 3);
    replacement = linked[1];
    assert(replacement->r0 == 255 && replacement->g0 == 255 && replacement->b0 == 255);
    assert(replacement->y0 == 50 && replacement->y3 == 90);
    assert((((DR_PSYX_TEX*)linked[0])->code[0] & 0xFFFFFF) == 0);
    assert((((DR_PSYX_TEX*)linked[2])->code[0] & 0xFFFFFF) == 7);
    assert(original.r0 == 0 && original.code == 0x2d);
    links = 0; original.code &= ~1; original.r0 = 64; original.g0 = 32; original.b0 = 128;
    PcPort_FeiHd2dBegin(&b, sprite, 1);
    PcPort_FeiHd2dCapture(&b, &original, &ot); PcPort_FeiHd2dEnd(&b);
    replacement = linked[1];
    assert(replacement->r0 == 128 && replacement->g0 == 64 && replacement->b0 == 255);
    /* Parts at different OT depths must remain separate retail primitives. */
    links = 0;
    PcPort_FeiHd2dBegin(&b, sprite, 2);
    PcPort_FeiHd2dCapture(&b, &original, &ot);
    PcPort_FeiHd2dCapture(&b, &original, &actor);
    PcPort_FeiHd2dEnd(&b);
    assert(links == 2 && linked[0] == &original && linked[1] == &original);
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
    puts("Fei HD2D: asset alpha, toggle, identity, field lifetime, poses, color, bounds, packet order and exhaustion PASS");
    return 0;
}
