/*
 * Retail certificate for func_800ABA98 (map cutscene from D_800AF47C).
 *
 * Retail (func_800ABA98.s 0x800ABA98-0x800ABD14): search stride-0x20 table
 * for g_GameSceneMapNum&0x3FFF, terminator 0xFFFF; func_800AB328(entry+0x10)
 * ==-1 returns; StoreImage 0x300,0x100,0xA0,0x100; archive (entry+0xC)+0x7FB;
 * FieldLoadTIMWithClut(..., 0x300, 0x100, 0, 0xF6, 0, 0); entry+0x1C==1
 * calls func_800AB808.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"

extern void func_800ABA98(void);
extern s32 func_800AB328(u8 characterId);

void* g_pGameState;
s32 g_GameSceneMapNum;
s32 D_800AF47C[16];
s32 D_800C3914;
s32 D_800C3A18;
s32 D_800AFE78;
s32 D_800AFE7C;
u16 D_800C3900;
u32 D_800C3A3C;
u32 D_800B1DF0;

static u8 s_state[0x2100];
static u8 s_shot[64];
static u8 s_tim[64];
static unsigned s_checks;
static int s_alloc_14000;
static int s_store_count;
static RECT s_store_rect;
static int s_archive_index = -1;
static int s_setindex_dir = -1;
static int s_loadtim_x, s_loadtim_y, s_loadtim_clutx, s_loadtim_cluty;
static int s_loadtim_clutw, s_loadtim_cluth;
static int s_ab808_calls;
static int s_aaf80_calls;
static int s_poll_calls;
static int s_display_calls;

void func_800AAF80(void)
{
    s_aaf80_calls++;
    D_800C3A3C = 0xC3A3Cu;
    D_800B1DF0 = 0xB1DF0u;
}

void func_800AB378(s8 color)
{
    (void)color;
}

void func_800AB808(void)
{
    s_ab808_calls++;
}

int ArchiveDecodeAlignedSize(int entryIndex)
{
    s_archive_index = entryIndex;
    return (int)sizeof(s_tim);
}

s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags)
{
    (void)arg2;
    if (index != s_archive_index || flags != 0x80 || pBuffer != s_tim) {
        fprintf(stderr, "ASSERTION archive.read idx=%d\n", index);
        exit(1);
    }
    return 0;
}

void ArchiveCdDataSync(int mode)
{
    (void)mode;
}

int ArchiveSetIndex(int directoryIndex, int entryIndex)
{
    s_setindex_dir = directoryIndex;
    (void)entryIndex;
    return 0;
}

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocFlags;
    if (allocSize == 0x14000) {
        s_alloc_14000++;
        return s_shot;
    }
    if (allocSize == sizeof(s_tim)) {
        return s_tim;
    }
    fprintf(stderr, "ASSERTION heap.size 0x%x\n", allocSize);
    exit(1);
}

void HeapFree(void* p)
{
    (void)p;
}

int StoreImage(RECT* rect, u_long* p)
{
    (void)p;
    s_store_rect = *rect;
    s_store_count++;
    return 0;
}

int LoadImage(RECT* rect, u_long* p)
{
    (void)rect;
    (void)p;
    return 0;
}

int DrawSync(int mode)
{
    (void)mode;
    return 0;
}

void FieldLoadTIMWithClut(u_long* pTimData, short x, short y, short clutX,
                          short clutY, short clutWidth, short clutHeight)
{
    if (pTimData != (u_long*)s_tim) {
        fprintf(stderr, "ASSERTION loadtim.buf\n");
        exit(1);
    }
    s_loadtim_x = x;
    s_loadtim_y = y;
    s_loadtim_clutx = clutX;
    s_loadtim_cluty = clutY;
    s_loadtim_clutw = clutWidth;
    s_loadtim_cluth = clutHeight;
}

void FieldClearAndSwapOTag(void) {}

void FieldDisplay(void)
{
    s_display_calls++;
}

void FieldPollControllers(void)
{
    s_poll_calls++;
    D_800C3900 = 0x100;
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
    fail("map.cutscene", detail);
}

static void reset_table(void)
{
    memset(D_800AF47C, 0, sizeof(D_800AF47C));
    D_800AF47C[0] = 0xFFFF;
    s_alloc_14000 = 0;
    s_store_count = 0;
    s_archive_index = -1;
    s_ab808_calls = 0;
    s_aaf80_calls = 0;
    s_poll_calls = 0;
    s_display_calls = 0;
    D_800C3900 = 0;
    D_800C3914 = 0;
    D_800C3A18 = 0;
    D_800AFE78 = 0;
    D_800AFE7C = 0;
}

static void put_entry(s32 slot, s32 mapId, s32 archive, s32 charId, s32 flag)
{
    s32* e = D_800AF47C + slot * 8;
    e[0] = mapId;
    e[1] = 11;
    e[2] = 22;
    e[3] = archive;
    e[4] = charId;
    e[5] = 33;
    e[6] = 44;
    e[7] = flag;
}

static void party_has(u8 charId)
{
    memset(s_state, 0, sizeof(s_state));
    s_state[0x2026] = charId;
    s_state[0x1F90] = 1;
    g_pGameState = s_state;
}

int main(void)
{
    uintptr_t addr = (uintptr_t)s_state;

    if (addr > UINT32_MAX) {
        fail("fixture.address.lp32", "build must use -fno-pie -no-pie");
    }

    /* Terminator: no match, no screenshot alloc. */
    reset_table();
    g_GameSceneMapNum = 1;
    party_has(7);
    func_800ABA98();
    expect_eq_s32("term.alloc", s_alloc_14000, 0);

    /* Map matches but character absent: AB328==-1, no alloc. */
    reset_table();
    put_entry(0, 1, 3, 7, 1);
    D_800AF47C[8] = 0xFFFF;
    g_GameSceneMapNum = 1;
    memset(s_state, 0, sizeof(s_state));
    g_pGameState = s_state;
    func_800ABA98();
    expect_eq_s32("noparty.alloc", s_alloc_14000, 0);

    /* Happy path: mask 0x3FFF, archive+0x7FB, AB808, screenshot rect. */
    reset_table();
    put_entry(0, 5, 3, 7, 1);
    D_800AF47C[8] = 0xFFFF;
    g_GameSceneMapNum = 0x4005;
    party_has(7);
    func_800ABA98();
    expect_eq_s32("ok.alloc", s_alloc_14000, 1);
    expect_eq_s32("ok.store", s_store_count, 1);
    expect_eq_s32("ok.rect.x", s_store_rect.x, 0x300);
    expect_eq_s32("ok.rect.y", s_store_rect.y, 0x100);
    expect_eq_s32("ok.rect.w", s_store_rect.w, 0xA0);
    expect_eq_s32("ok.rect.h", s_store_rect.h, 0x100);
    expect_eq_s32("ok.scaleX", D_800C3914, 11);
    expect_eq_s32("ok.archive", s_archive_index, 3 + 0x7FB);
    expect_eq_s32("ok.setindex", s_setindex_dir, 4);
    expect_eq_s32("ok.tim.x", s_loadtim_x, 0x300);
    expect_eq_s32("ok.tim.cluty", s_loadtim_cluty, 0xF6);
    expect_eq_s32("ok.ab808", s_ab808_calls, 1);
    expect_eq_s32("ok.aaf80", s_aaf80_calls, 1);
    expect_eq_s32("ok.poll", s_poll_calls, 1);

    /* flag != 1: no AB808. */
    reset_table();
    put_entry(0, 5, 3, 7, 0);
    D_800AF47C[8] = 0xFFFF;
    g_GameSceneMapNum = 5;
    party_has(7);
    func_800ABA98();
    expect_eq_s32("noflag.ab808", s_ab808_calls, 0);
    expect_eq_s32("noflag.alloc", s_alloc_14000, 1);

    printf("FIELD MAP CUTSCENE ABA98 certificate PASS checks=%u\n", s_checks);
    return 0;
}
