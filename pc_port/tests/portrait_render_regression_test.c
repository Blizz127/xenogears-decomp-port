/* Production-linked regression for the retail dialogue portrait OT block. */
#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"

#include <stdio.h>
#include <string.h>

extern void func_8007E1C0(void* ot, s32 renderContextIndex, s32 textBoxIndex);
extern s32 func_8007F8DC(s32 x, s32 y, s32 stringIndex, s32 textBoxIndex,
                         s32 width, s32 height, s32 ownerActorIndex,
                         s32 talkingActorIndex, s32 mode, s32 orientationFlags,
                         s32 dialogFlags);

FieldTextBox g_FieldTextBoxes[4];
s16 D_800B21D6 = 8;
s32 D_800B068C[4];
s32 D_800ADE90;
s32 D_800ADE94;
/* Retail cursor/face RECT tables read by func_8007E1C0 (indexed by
 * D_800ADE94 * 8). Zeroed fixture; port copies in data_field.c. */
RECT D_800ADEDC[8];
RECT D_800ADF04[8];
/* Verbatim leaf logic from src/slus_006.64/system/system.c (per-arrow words). */
s32 func_800347AC(void* arg0)
{
    return *(s16*)((u8*)arg0 + 0x4) + (*(s16*)((u8*)arg0 + 0x0) << 2);
}
s32 func_800347C0(void* arg0)
{
    /* Kept in sync with src/slus_006.64/system/system.c: row index minus
     * scroll base, wrapping to the last row when negative, times the row
     * stride at +0x14 -- not a hardcoded <<2. */
    u8* pWindow = (u8*)arg0;
    s32 row = *(s16*)(pWindow + 0x02) - *(s16*)(pWindow + 0x16);

    if (row < 0) {
        row = *(s16*)(pWindow + 0x0C) - 1;
    }
    return *(s16*)(pWindow + 0x06) + row * *(s16*)(pWindow + 0x14);
}
u16 D_800ADF54;
u16 D_800ADF56;
void* D_800ADBF0;
FieldScene g_Scene;
FieldActor* volatile g_FieldActors;
u8 D_800ADF34[64];

static FieldActor s_fieldActors[2];
static ActorData s_speaker;
static ActorData s_actor;

/* Minimal production-call-closure fakes for func_8007F8DC. */
s32 FieldScriptVMGetVariableValue(s32 index) { (void)index; return 0; }
void func_80032F54(void* a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g, s32 h)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h;
}
void* GetStringEntry(void* data, s32 index)
{
    (void)data; (void)index; return (void*)(uintptr_t)0x100;
}
MATRIX* CompMatrix(MATRIX* a, MATRIX* b, MATRIX* out)
{
    (void)a; (void)b; return out;
}
void SetRotMatrix(MATRIX* matrix) { (void)matrix; }
void SetTransMatrix(MATRIX* matrix) { (void)matrix; }
int RotTransPers(SVECTOR* vector, int* screenXY, long* depth, long* flag)
{
    (void)vector; *screenXY = 0x004000a0; *depth = 0; *flag = 0; return 0;
}
u_short GetClut(int x, int y) { (void)x; return (u_short)y; }

static int s_failures;

static u32 low24(const void* value)
{
    return (u32)(uintptr_t)value & 0x00ffffff;
}

static u32 tag24(const void* value)
{
    return *(const u32*)value & 0x00ffffff;
}

static void check(const char* label, int condition)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", label);
        s_failures++;
    }
}

static FieldTextBox* prepare_box(s16 x, s16 y, s16 w, s16 h, u16 flags,
                                 u8 shouldRender)
{
    FieldTextBox* box = &g_FieldTextBoxes[0];
    u8* raw = (u8*)box;

    memset(g_FieldTextBoxes, 0, sizeof(g_FieldTextBoxes));
    *(s16*)(raw + 0x0ac) = x;
    *(s16*)(raw + 0x0ae) = y;
    *(s16*)(raw + 0x0b0) = w;
    *(s16*)(raw + 0x0b2) = h;
    box->flags = flags;
    box->visibility = 0;
    box->portrait.shouldRenderPortrait = shouldRender;

    /* Preserve recognizable primitive-length bytes while OT links change tags. */
    *(u32*)&box->portrait.polys[0] = 0x0c000000;
    *(u32*)&box->portrait.polys[1] = 0x0c000000;
    *(u32*)&box->portrait.drawModes[0] = 0x02000000;
    *(u32*)&box->portrait.drawModes[1] = 0x02000000;
    return box;
}

static void run_visible_context_and_clamp(void)
{
    FieldTextBox* box = prepare_box(10, 20, 100, 80, 0x40, 1);
    POLY_FT4* poly = &box->portrait.polys[1];
    DR_MODE* drawMode = &box->portrait.drawModes[1];
    u32 ot = 0x00abcdef;
    int before = s_failures;

    func_8007E1C0(&ot, 1, 0);

    check("context 1 portrait poly is the OT payload", tag24(drawMode) == low24(poly));
    check("context 1 portrait draw mode is the OT head", (ot & 0x00ffffff) == low24(drawMode));
    check("portrait poly retains the prior OT tail", tag24(poly) == 0x00abcdef);
    check("unselected context poly is not linked", tag24(&box->portrait.polys[0]) == 0);
    check("unselected context draw mode is not linked", tag24(&box->portrait.drawModes[0]) == 0);
    check("large portrait width clamps to 64",
          poly->x0 == 13 && poly->x1 == 77 && poly->x2 == 13 && poly->x3 == 77);
    check("large portrait height clamps to 64",
          poly->y0 == 24 && poly->y1 == 24 && poly->y2 == 88 && poly->y3 == 88);

    printf("Visible portrait, context selection, clamp, and OT order: %s\n",
           s_failures == before ? "PASS" : "FAIL");
}

static int retail_background_portrait_order_ok(
    u32 ot, FieldTextBoxBackground* background,
    FieldTextBoxPortrait* portrait, s32 renderContextIndex)
{
    DR_MODE* backgroundMode = &background->drawModes[renderContextIndex];
    TILE* backgroundTile = &background->tiles[renderContextIndex];
    DR_MODE* portraitMode = &portrait->drawModes[renderContextIndex];
    POLY_FT4* portraitPoly = &portrait->polys[renderContextIndex];

    /* addPrim is push-front.  Walking the finished OT must therefore execute
     * the subtractive window background before the opaque portrait. */
    return (ot & 0x00ffffff) == low24(backgroundMode) &&
           tag24(backgroundMode) == low24(backgroundTile) &&
           tag24(backgroundTile) == low24(portraitMode) &&
           tag24(portraitMode) == low24(portraitPoly);
}

static void run_background_before_portrait_order(void)
{
    FieldTextBox* box = prepare_box(10, 20, 100, 80, 0, 1);
    FieldTextBoxBackground* background = &box->background;
    FieldTextBoxPortrait* portrait = &box->portrait;
    u32 ot = 0x00abcdef;
    int before = s_failures;

    func_8007E1C0(&ot, 0, 0);

    check("retail OT draws subtractive background before portrait",
          retail_background_portrait_order_ok(ot, background, portrait, 0));

    printf("Background/portrait retail OT order: %s\n",
           s_failures == before ? "PASS" : "FAIL");
}

static void run_hidden_gate(void)
{
    FieldTextBox* box = prepare_box(10, 20, 100, 80, 0x40, 0);
    u32 ot = 0x00123456;
    int before = s_failures;

    func_8007E1C0(&ot, 0, 0);

    check("hidden portrait leaves OT unchanged", ot == 0x00123456);
    check("hidden portrait poly is not linked", tag24(&box->portrait.polys[0]) == 0);
    check("hidden portrait draw mode is not linked", tag24(&box->portrait.drawModes[0]) == 0);

    printf("shouldRenderPortrait gate: %s\n",
           s_failures == before ? "PASS" : "FAIL");
}

static void run_flip_and_small_box(void)
{
    FieldTextBox* box = prepare_box(10, 20, 60, 50, 0x60, 1);
    POLY_FT4* poly = &box->portrait.polys[0];
    u32 ot = 0x00010203;
    int before = s_failures;

    func_8007E1C0(&ot, 0, 0);

    /* Retail small-box dimensions are window size minus eight: 52x42.  With
     * flag 0x20 the near/far x coordinates are reversed on the right side. */
    check("small portrait uses width minus eight and flips right",
          poly->x0 == 66 && poly->x1 == 14 && poly->x2 == 66 && poly->x3 == 14);
    check("small portrait uses height minus eight",
          poly->y0 == 24 && poly->y1 == 24 && poly->y2 == 66 && poly->y3 == 66);

    printf("Flip flag and small-box sizing: %s\n",
           s_failures == before ? "PASS" : "FAIL");
}

static void run_negative_controls(void)
{
    FieldTextBox* box;
    POLY_FT4* poly;
    u32 ot;
    int before = s_failures;

    /* A positioning-call mutant leaves the zeroed geometry and must be caught. */
    box = prepare_box(10, 20, 100, 80, 0x40, 1);
    poly = &box->portrait.polys[0];
    check("negative control catches omitted positioning call",
          !(poly->x0 == 13 && poly->x1 == 77 && poly->y0 == 24 && poly->y2 == 88));

    /* A visibility-gate mutant would link the hidden primitive; the unchanged
     * OT predicate used above must reject that old/wrong behavior. */
    ot = 0x00123456;
    *(u32*)poly = (*(u32*)poly & 0xff000000) | (ot & 0x00ffffff);
    ot = (ot & 0xff000000) | low24(poly);
    check("negative control catches omitted visibility gate", ot != 0x00123456);

    /* Recreate the bad push-front order: background is inserted first and the
     * portrait second, so the subtractive tile executes over the portrait. */
    box = prepare_box(10, 20, 100, 80, 0, 1);
    {
        FieldTextBoxBackground* background = &box->background;
        FieldTextBoxPortrait* portrait = &box->portrait;
        DR_MODE* backgroundMode = &background->drawModes[0];
        TILE* backgroundTile = &background->tiles[0];
        DR_MODE* portraitMode = &portrait->drawModes[0];
        POLY_FT4* portraitPoly = &portrait->polys[0];

        ot = 0x00010203;
        *(u32*)backgroundTile = (*(u32*)backgroundTile & 0xff000000) |
                               (ot & 0x00ffffff);
        ot = (ot & 0xff000000) | low24(backgroundTile);
        *(u32*)backgroundMode = (*(u32*)backgroundMode & 0xff000000) |
                               (ot & 0x00ffffff);
        ot = (ot & 0xff000000) | low24(backgroundMode);
        *(u32*)portraitPoly = (*(u32*)portraitPoly & 0xff000000) |
                             (ot & 0x00ffffff);
        ot = (ot & 0xff000000) | low24(portraitPoly);
        *(u32*)portraitMode = (*(u32*)portraitMode & 0xff000000) |
                             (ot & 0x00ffffff);
        ot = (ot & 0xff000000) | low24(portraitMode);

        check("negative control catches portrait-before-background mutant",
              !retail_background_portrait_order_ok(ot, background, portrait, 0));
    }

    printf("Portrait mutant negative controls: %s\n",
           s_failures == before ? "PASS" : "FAIL");
}

static void prepare_actor_selector(u32 cacheSlot, u32 actorDirection)
{
    int i;

    memset(&s_actor, 0, sizeof(s_actor));
    memset(s_fieldActors, 0, sizeof(s_fieldActors));
    memset(g_FieldTextBoxes, 0, sizeof(g_FieldTextBoxes));
    memset(D_800ADF34, 0, sizeof(D_800ADF34));
    for (i = 0; i < 4; i++)
        D_800B068C[i] = -1;

    s_actor.faceId = 0;
    s_actor.flags12C_0x2 = cacheSlot;
    s_actor.direction = actorDirection;
    s_fieldActors[0].pActorData = (u32)(uintptr_t)&s_actor;
    g_FieldActors = s_fieldActors;

    /* Every selector has a unique UV pair. */
    for (i = 0; i < 16; i++) {
        D_800ADF34[i * 4 + 0] = (u8)(0x20 + i);
        D_800ADF34[i * 4 + 2] = (u8)(0x40 + i);
    }
}

static void run_retail_cache_slot_selector(void)
{
    POLY_FT4* poly;
    int before = s_failures;

    /* Retail reads the cache slot at ActorData+0x12c bits 2..4, not the actor's
     * facing-direction bits at 9..11. Deliberately make those fields disagree. */
    prepare_actor_selector(2, 7);
    check("portrait textbox opens", func_8007F8DC(10, 20, 0, 0, 30, 4,
                                                   0, 0, 3, 0, 0) == 0);
    poly = &g_FieldTextBoxes[0].portrait.polys[0];
    check("normal orientation uses cache slot 2 odd selector",
          poly->u0 == 0x25 && poly->v0 == 0x45 && poly->clut == 0x00e5);
    check("normal selector is applied to both render contexts",
          g_FieldTextBoxes[0].portrait.polys[1].u0 == 0x25);

    prepare_actor_selector(2, 7);
    check("flipped portrait textbox opens", func_8007F8DC(10, 20, 0, 0, 30, 4,
                                                           0, 0, 3, 0x400, 0) == 0);
    poly = &g_FieldTextBoxes[0].portrait.polys[0];
    check("flipped orientation uses cache slot 2 even selector",
          poly->u0 == 0x24 && poly->v0 == 0x44 && poly->clut == 0x00e4);

    printf("Retail portrait cache-slot selector: %s\n",
           s_failures == before ? "PASS" : "FAIL");
}


static void run_distinct_owner_and_speaker(void)
{
    unsigned slot, flip, ownerFace, speakerFace, ownerHidden, speakerHidden, upper;
    unsigned cases = 0;
    int before = s_failures;
    for (slot = 0; slot < 3; ++slot)
    for (flip = 0; flip < 2; ++flip)
    for (ownerFace = 0; ownerFace < 2; ++ownerFace)
    for (speakerFace = 0; speakerFace < 2; ++speakerFace)
    for (ownerHidden = 0; ownerHidden < 2; ++ownerHidden)
    for (speakerHidden = 0; speakerHidden < 2; ++speakerHidden)
    for (upper = 0; upper < 2; ++upper) {
        unsigned selector = slot * 2 + !flip;
        int result;
        prepare_actor_selector(slot, 7);
        memset(&s_speaker, 0, sizeof(s_speaker));
        s_speaker.faceId = speakerFace ? 0x21 : 0xff;
        s_speaker.flags12C_0x2 = (slot + 1) % 3;
        s_speaker.dialogFlags = flip ? 0 : 0x400;
        s_speaker.flags = speakerHidden ? 0x200 : 0;
        s_actor.faceId = ownerFace ? 0x13 : 0xff;
        s_actor.dialogFlags = upper ? (((flip ? 0x400u : 0) | 1u) << 16) | (flip ? 0 : 0x400) : (flip ? 0x400 : 0);
        s_actor.flags = ownerHidden ? 0x200 : 0;
        s_fieldActors[1].pActorData = (u32)(uintptr_t)&s_speaker;
        result = func_8007F8DC(10, 20, 0, 0, 30, 4, 0, 1, 3, 0, 0);
        check("speaker controls final hidden-actor gate", result == (speakerHidden && !upper ? -1 : 0));
        check("owner controls portrait presence", g_FieldTextBoxes[0].portrait.shouldRenderPortrait == ownerFace);
        check("owner controls portrait identity", g_FieldTextBoxes[0].portrait.portraitID == (ownerFace ? 0x13 : 0x80));
        if (ownerFace) {
            unsigned context;
            for (context = 0; context < 2; ++context) {
                POLY_FT4* poly = &g_FieldTextBoxes[0].portrait.polys[context];
                check("owner controls cache slot and orientation", poly->u0 == 0x20 + selector && poly->v0 == 0x40 + selector && poly->clut == 0xe0 + selector);
            }
        }
        check("owner and speaker IDs retained independently", g_FieldTextBoxes[0].ownerActorID == 0 && g_FieldTextBoxes[0].talkingActorID == 1);
        ++cases;
    }
    printf("Distinct owner/speaker: %u cases %s\n", cases, s_failures == before ? "PASS" : "FAIL");
}

int main(void)
{
    run_visible_context_and_clamp();
    run_background_before_portrait_order();
    run_hidden_gate();
    run_flip_and_small_box();
    run_negative_controls();
    run_retail_cache_slot_selector();
    run_distinct_owner_and_speaker();

    if (s_failures != 0) {
        fprintf(stderr, "Portrait render regression: FAIL (%d check(s))\n", s_failures);
        return 1;
    }
    puts("Portrait render regression: PASS");
    return 0;
}
