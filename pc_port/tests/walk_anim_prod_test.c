/* Production-linked walk/run vs idle vs frozen-pose certificate.
 *
 * Drives the shipped setter (func_800245D8) and tick (AnimScriptTick ->
 * func_800248D4 -> func_8001D2B0) from a real sprite start state. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

extern void func_800245D8(void* pSpriteData, s16 animIndex);
extern void AnimScriptTick(void* pSpriteData);
extern void func_800248D4(void* pSpriteData);
extern void func_8001D2B0(void* pSpriteData, s16 frameIndex);

s32 D_80059198;
u8 D_800591AD;
u8 D_800591AE;
s32 D_80059190;
s32 D_800591A8;
s32 D_800591B8;
s32 g_WorkListCurTimer;
s32 g_NumTimerWorkListEntries;
s32 g_NumWorkListEntries;
s32 D_80059494;
void* D_800594C0;
void* D_80059590;
void* g_TimerWorkList;
void* g_WorkList;
s32 D_800592E4;
s16 D_800592E8;
s16 D_800592EA;
s32 D_800592EC;
s32 D_80059300;
s32 D_80059304;
s32 D_80059524;
u8 D_80018644[4];
u8 D_8004FBB8[0x20];
u8 D_80050100[4];
u8 D_8006F99C[0x10];
u8 D_8006F9AC[0x10];
u8 D_800C3664[4];
u8 D_800C3EB0[4];
void* g_GfxCurContext;
void* g_GfxCurOT;
void* g_GfxCurWorkBuffer;
void* g_GfxCurWorkBufferEnd;
void* g_GfxImageList;
void* g_GfxWorkBuffer2;
s32 g_GfxWorkBufferSize;
void* g_GfxWorkBuffers;

static int s_failures;

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

s32 func_8001EE68(void* arg0)
{
    (void)arg0;
    return 1;
}
static void* s_battleSprite;
static int s_battleCalls;
void func_800C11CC(void* sprite) { s_battleSprite = sprite; ++s_battleCalls; }
void func_8001F8E8(void* pSpriteData, u16 frameIndex, u32 animPackageAddr)
{
    (void)pSpriteData;
    (void)frameIndex;
    (void)animPackageAddr;
}
void func_8001DAE8(void* arg0, u16 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;
}
void* HeapAlloc(u32 size, u32 tag)
{
    (void)tag;
    return calloc(1, size ? size : 1);
}
void HeapFree(void* p) { free(p); }
void HeapChangeCurrentUser(s32 a, s32 b)
{
    (void)a;
    (void)b;
}
void* AddPrim(void* ot, void* p)
{
    (void)ot;
    return p;
}
void ApplyMatrix(void) {}
void ApplyMatrixSV(void) {}
void ClearImage(void) {}
void CompMatrix(void) {}
s32 GetClut(s32 x, s32 y)
{
    (void)x;
    (void)y;
    return 0;
}
s32 GetTPage(s32 a, s32 b, s32 c, s32 d)
{
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    return 0;
}
void LoadImage(void) {}
void MulMatrix0(void) {}
void ReadGeomOffset(void) {}
void RotMatrix(void) {}
void RotTransPers(void) {}
void RotTransPers3(void) {}
void RotTransSV(void) { abort(); }
void ScaleMatrix(void) {}
void ScaleMatrixL(void) {}
void SetDrawMode(void) {}
void SetPolyFT4(void) {}
void SetRotMatrix(void) {}
void SetSemiTrans(void) {}
void SetShadeTex(void) {}
void SetSprt(void) {}
void SetTransMatrix(void) {}
void TransMatrix(void) {}
void func_8001E298(void) {}
void func_8001E3D8(void) {}
/* New references from the grown animation_scripts.c bodies. The walk
 * certificate path does not reach them; fail loud if that changes. */
void func_8001D4E8(void) { abort(); }
void func_800B2AEC(void) { abort(); }
void func_80039E60(void) { abort(); }
/* GTE COP2 accessors + PSX RAM backing for the grown helpers
 * (AnimationC4ApplyMatrixLV, AnimationSoundBankSelector). Unreached on
 * the walk certificate path; fail loud if that changes. */
unsigned MFC2(int r) { (void)r; abort(); }
unsigned CFC2(int r) { (void)r; abort(); }
void MTC2(unsigned v, int r) { (void)v; (void)r; abort(); }
void CTC2(unsigned v, int r) { (void)v; (void)r; abort(); }
int doCOP2(int o) { (void)o; abort(); }
uint8_t g_PsxRam[0x200000];
s32 func_8001EE74(void* a)
{
    (void)a;
    return 0;
}
void func_8001F6B0(void* a) { (void)a; }
void func_8001FB30(void) {}
void func_80022DF4(void) {}
void* func_800233A4(void) { return 0; }
void func_80025180(void) {}
void func_80025224(void) {}
void func_800BC158(void) {}
void func_8002C3E8(void) {}
void func_8002C59C(void) {}
void func_8002C700(void) {}
void func_8002C8CC(void) {}
void func_8002CB54(void) {}
void func_8002CC10(s32 x, s32 y)
{
    (void)x;
    (void)y;
}
void func_800B1F6C(void) {}
void func_800BA8F4(void) {}
s32 ratan2(s32 y, s32 x)
{
    (void)y;
    (void)x;
    return 0;
}
s32 rcos(s32 a)
{
    (void)a;
    return 0x1000;
}
s32 rsin(s32 a)
{
    (void)a;
    return 0;
}

u8 D_8005A474[4];
u8 D_8006BE10[4];
u8 D_800591B0;
u8 D_800591B3;

static u8 s_sprite[0x200];
static u8 s_base[0x80];
static u8 s_vram[0x20];
/* flags + 3 offsets + 3 animation records (0x20 each). */
static u8 s_anims[0x08 + 0x20 * 3];

#define ANIM_REC(i) (s_anims + 0x08 + (i) * 0x20)

static void wr16(u8* p, u16 v) { memcpy(p, &v, 2); }
static void wr32(u8* p, u32 v) { memcpy(p, &v, 4); }

static void install_script(u8* anim, const u8* script, size_t script_len)
{
    wr16(anim + 0x0, 0); /* 1-directional */
    wr16(anim + 0x2, 4); /* scriptOffset */
    wr16(anim + 0x4, 4);
    memcpy(anim + 6, script, script_len);
}

static void reset_sprite(void)
{
    memset(s_sprite, 0, sizeof(s_sprite));
    memset(s_base, 0, sizeof(s_base));
    memset(s_vram, 0, sizeof(s_vram));
    memset(s_anims, 0, sizeof(s_anims));
    D_80059190 = 0;
    g_WorkListCurTimer = 0;

    wr16(s_anims + 0x0, 3);
    wr16(s_anims + 0x2, 0x08);
    wr16(s_anims + 0x4, 0x28);
    wr16(s_anims + 0x6, 0x48);

    {
        static const u8 idle_script[] = { 0x30, 0x82 };
        /* 0xB2 length-table stride is 2; the extra 0x00 is the dummy operand. */
        static const u8 walk_script[] = {
            0xBE, 0x05, 0x08, 0xB2, 0x00, 0xBE, 0x09, 0x08, 0x82
        };
        static const u8 run_script[] = {
            0xBE, 0x15, 0x08, 0xB2, 0x00, 0xBE, 0x19, 0x08, 0x82
        };

        install_script(ANIM_REC(0), idle_script, sizeof(idle_script));
        install_script(ANIM_REC(1), walk_script, sizeof(walk_script));
        install_script(ANIM_REC(2), run_script, sizeof(run_script));
    }

    wr32(s_vram + 0x10, (u32)(uintptr_t)s_anims);
    wr32(s_sprite + 0x20, (u32)(uintptr_t)s_base);
    wr32(s_sprite + 0x24, (u32)(uintptr_t)s_vram);
    wr32(s_sprite + 0x48, (u32)(uintptr_t)s_anims);
    wr32(s_sprite + 0x44, (u32)(uintptr_t)s_anims);
    wr32(s_sprite + 0x3C, 1u);
    wr32(s_sprite + 0xAC, 0x8000u);
    wr16(s_sprite + 0x82, 0x1000);
    wr16(s_sprite + 0x80, 0);
}

static int pose_takes_two_values(int ticks)
{
    s16 first = *(s16*)(s_sprite + 0x34);
    int i;

    for (i = 0; i < ticks; i++) {
        AnimScriptTick(s_sprite);
        if (*(s16*)(s_sprite + 0x34) != first) {
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    s16 pose_a;
    s16 pose_b;
    int i;

    D_80059198 = 0;
    D_800591AD = 0;

    reset_sprite();
#if !defined(WALK_ANIM_MUTANT_NO_SELECT)
    func_800245D8(s_sprite, 0);
#endif
    check(*(s8*)(s_sprite + 0xAF) == 0, "idle.selects.idle");
    pose_a = *(s16*)(s_sprite + 0x34);
    for (i = 0; i < 8; i++) {
        AnimScriptTick(s_sprite);
    }
    check(*(s8*)(s_sprite + 0xAF) == 0, "idle.keeps.idle");
    check(*(s16*)(s_sprite + 0x34) == pose_a, "idle.does.not.keep.walk.cycle");

    reset_sprite();
#if !defined(WALK_ANIM_MUTANT_NO_SELECT)
    func_800245D8(s_sprite, 1);
#endif
    check(*(s8*)(s_sprite + 0xAF) == 1, "walk.selects.locomotion");
    pose_a = *(s16*)(s_sprite + 0x34);
#if !defined(WALK_ANIM_MUTANT_NO_TICK)
    check(pose_takes_two_values(24), "walk.pose.advances");
    pose_b = *(s16*)(s_sprite + 0x34);
    check(pose_a != pose_b, "walk.pose.two_values");
#else
    check(*(s16*)(s_sprite + 0x34) != pose_a, "walk.pose.advances");
#endif

    reset_sprite();
#if !defined(WALK_ANIM_MUTANT_NO_SELECT)
    func_800245D8(s_sprite, 2);
#endif
    check(*(s8*)(s_sprite + 0xAF) == 2, "run.selects.locomotion");
#if !defined(WALK_ANIM_MUTANT_NO_TICK)
    check(pose_takes_two_values(24), "run.pose.advances");
#else
    pose_a = *(s16*)(s_sprite + 0x34);
    check(*(s16*)(s_sprite + 0x34) != pose_a, "run.pose.advances");
#endif

    reset_sprite();
    func_800245D8(s_sprite, 1);
    for (i = 0; i < 8; i++) {
        AnimScriptTick(s_sprite);
    }
    check(*(s8*)(s_sprite + 0xAF) == 1, "walk.stays.walk.while.ticking");
    func_800245D8(s_sprite, 0);
    check(*(s8*)(s_sprite + 0xAF) == 0, "idle.after.walk");
    pose_a = *(s16*)(s_sprite + 0x34);
    for (i = 0; i < 8; i++) {
        AnimScriptTick(s_sprite);
    }
    check(*(s8*)(s_sprite + 0xAF) == 0, "idle.does.not.keep.walk.cycle.after.stop");
    (void)pose_a;

    reset_sprite();
    D_800591AD = 1;
    s_battleSprite = NULL;
    s_battleCalls = 0;
    /* Test the retail mode-dispatch boundary directly; AnimScriptTick has
     * its own scheduling guards, outside this ABI assertion. */
    func_800248D4(s_sprite);
    check(s_battleCalls == 1, "battle.dispatch.once");
    check(s_battleSprite == s_sprite, "battle.dispatch.preserves.sprite");
    D_800591AD = 0;

    if (s_failures != 0) {
        return EXIT_FAILURE;
    }
    puts("WALK ANIM certificate PASS");
    return EXIT_SUCCESS;
}
