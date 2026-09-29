/*
 * Retail certificate for func_800A3C8C (field snapshot restore helper).
 *
 * Retail (func_800A3C8C.s 0x800A3C8C-0x800A3F48): g_FieldNumActors from
 * D_8005A4E4[0], memcpy 0x74 at +0x3C into g_Scene+0xC4, mismatch count
 * D_8005A408 vs g_pGameState[0x22B1+i], skip to +0x95C. Per actor: +0xC,
 * patch save+0x20 from unkEA if unk124!=-1, func_80021D50 unless
 * flags&0x1000000 or (D_800B2268 && scriptFlags&0x600 && mismatch),
 * advance 0x168 / +0xC / +0x10.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"
#include "main/game.h"

extern void func_800A3C8C(void);

int g_FieldNumActors;
s32 D_800ADBFC;
s32 D_800B2268;
s32 D_8005A408[3];
u8 D_8005A4E4[0xC00];
u8* D_800AFC50;
/* Retail copies 0x74 bytes at g_Scene+0xC4, 4 bytes past sizeof(FieldScene). */
u8 g_Scene[0x140];
FieldActor g_actors[2];
FieldActor* volatile g_FieldActors = g_actors;
ActorData g_adata[2];
u8 g_sprite[2][4];
u8 g_state[0x2300];
GameState* g_pGameState = (GameState*)g_state;

static unsigned s_checks;
static int s_21d50;
static void* s_21_sprite;
static void* s_21_save;

void func_80021D50(void* pSprite, void* pSave)
{
    s_21d50++;
    s_21_sprite = pSprite;
    s_21_save = pSave;
}

static void fail(const char* name, const char* detail)
{
    fprintf(stderr, "ASSERTION %s %s\n", name, detail);
    exit(1);
}

static void expect_eq_s32(const char* field, s32 actual, s32 expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s actual=%d expected=%d",
             field, (int)actual, (int)expected);
    fail("snap.restore", detail);
}

static void expect_eq_ptr(const char* field, const void* actual,
                          const void* expected)
{
    char detail[160];

    s_checks++;
    if (actual == expected) {
        return;
    }
    snprintf(detail, sizeof(detail), "field=%s actual=%p expected=%p",
             field, actual, expected);
    fail("snap.restore", detail);
}

static void reset_state(void)
{
    memset(D_8005A4E4, 0, sizeof(D_8005A4E4));
    memset(g_Scene, 0, sizeof(g_Scene));
    memset(g_actors, 0, sizeof(g_actors));
    memset(g_adata, 0, sizeof(g_adata));
    memset(g_state, 0, sizeof(g_state));
    D_8005A4E4[0] = 3;
    D_8005A4E4[0x3C] = 0xAA;
    D_8005A4E4[0x3C + 0x73] = 0xBB;
    g_FieldNumActors = 0;
    D_800ADBFC = 0;
    D_800B2268 = 0;
    D_8005A408[0] = 0;
    D_8005A408[1] = 0;
    D_8005A408[2] = 0;
    D_800AFC50 = NULL;
    g_actors[0].pActorData = (u32)(uintptr_t)&g_adata[0];
    g_actors[0].pSpriteData = (u32)(uintptr_t)g_sprite[0];
    g_actors[1].pActorData = (u32)(uintptr_t)&g_adata[1];
    g_actors[1].pSpriteData = (u32)(uintptr_t)g_sprite[1];
    g_adata[0].unk124 = -1;
    g_adata[0].unkAnimationId = 0;
    s_21d50 = 0;
}

int main(void)
{
    u8* rec;

    /* No actors: scene copy, cursor to +0x95C. */
    reset_state();
    func_800A3C8C();
    {
        u8* sceneTail = g_Scene + 0xC4;

        expect_eq_s32("n0.nact", g_FieldNumActors, 3);
        expect_eq_s32("n0.scene0", sceneTail[0], 0xAA);
        expect_eq_s32("n0.scene_end", sceneTail[0x73], 0xBB);
    }
    expect_eq_ptr("n0.cur", D_800AFC50, D_8005A4E4 + 0x95C);
    expect_eq_s32("n0.21d50", s_21d50, 0);

    /* One actor, 21D50 called, advance 0x168 from +0xC. */
    reset_state();
    D_800ADBFC = 1;
    func_800A3C8C();
    expect_eq_s32("a0.21d50", s_21d50, 1);
    expect_eq_ptr("a0.sprite", s_21_sprite, g_sprite[0]);
    expect_eq_ptr("a0.save", s_21_save, D_8005A4E4 + 0x95C + 0xC);
    expect_eq_ptr("a0.cur", D_800AFC50, D_8005A4E4 + 0x95C + 0xC + 0x168);

    /* Patch unkAnimationId at save+0x20. */
    reset_state();
    D_800ADBFC = 1;
    g_adata[0].unk124 = 5;
    g_adata[0].unkAnimationId = 0x12;
    rec = D_8005A4E4 + 0x95C;
    *(s16*)(rec + 0x20) = 0;
    func_800A3C8C();
    expect_eq_s32("anim.patch", *(s16*)(rec + 0x20), 0x12);
    expect_eq_s32("anim.21d50", s_21d50, 1);

    /* flags&0x1000000 skips 21D50. */
    reset_state();
    D_800ADBFC = 1;
    g_adata[0].flags = 0x1000000;
    func_800A3C8C();
    expect_eq_s32("f24.21d50", s_21d50, 0);

    /* mismatch + D_800B2268 + scriptFlags&0x600 skips 21D50. */
    reset_state();
    D_800ADBFC = 1;
    D_800B2268 = 1;
    D_8005A408[0] = 1;
    g_state[0x22B1] = 0;
    g_adata[0].scriptFlags.flags = 0x600;
    func_800A3C8C();
    expect_eq_s32("mis.21d50", s_21d50, 0);

    /* flags134&0x80 adds 0xC. */
    reset_state();
    D_800ADBFC = 1;
    *(u32*)((u8*)&g_adata[0] + 0x134) = 0x80;
    func_800A3C8C();
    expect_eq_ptr("f80.cur", D_800AFC50, D_8005A4E4 + 0x95C + 0xC + 0x174);
    expect_eq_s32("f80.21d50", s_21d50, 1);

    /* +0x12C&0x1000 adds 0x10. */
    reset_state();
    D_800ADBFC = 1;
    *(u32*)((u8*)&g_adata[0] + 0x12C) = 0x1000;
    func_800A3C8C();
    expect_eq_ptr("f12c.cur", D_800AFC50, D_8005A4E4 + 0x95C + 0xC + 0x168 + 0x10);

    printf("FIELD SNAPSHOT RESTORE A3C8C certificate PASS checks=%u\n",
           s_checks);
    return 0;
}
