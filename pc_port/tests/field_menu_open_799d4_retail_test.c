/*
 * Retail certificate for func_800799D4 (field menu open/close).
 *
 * Retail (func_800799D4.s 0x800799D4-0x8007A448): early-out if request==0x80
 * and D_800B21D0!=0. TILE 0x140x0xE0 fade, overlay HeapAlloc from
 * ArchiveDecodeAlignedSize((id+5)&0x7F) when D_8004F370==1, D_80059460 =
 * request&0x7F, MenuMain, then D_800ADB64=0xFF and D_8004F350=0.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"
#include "field/main.h"
#include "system/archive.h"

extern void func_800799D4(void);

s32 D_800ADB64;
u8 D_800B21D0[4];
s32 D_800B2264;
s32 D_8004F370;
s32 D_8004F350;
s32 D_800AFE84;
s32 D_800ADB50;
s32 D_800ADB60;
s32 D_800ADBEC;
s32 D_80050100;
s32 D_8004F320;
s32 D_8004F31C;
s32 g_GameSceneMapNum;
s32 g_GamePartyMemberSkins[3];
u8 D_80059460;
u8 D_800594D0;
u8 D_800B02C8;
u8 D_800ADB05;
u8 g_MenuDebugEnabled;
void* D_8005945C;
void* D_800ADB20;
void* D_800ADB30;
void* D_8005A4AC;
void* D_8005A4B0;
void* g_pGameState;
void* g_PartyDataBuffers[3];
s16 D_8006BE2C[3];
TILE D_800AFE54[2];
TILE D_800AFE64;
DR_MODE D_800AFE24[2];
RECT D_800AFE4C;
FieldScene g_Scene;
RenderContext g_FieldRenderContexts[2];
RenderContext* g_FieldCurRenderContext;
int g_PcPortFieldMenuOpened;
unsigned short vram[1];
s32 g_PlayerActorIndex;
s32 D_800ADBFC;
s32 g_FieldNumActors;
void* g_FieldActors;

typedef struct {
    u16 x;
    u16 y;
} FieldVramCoord;

FieldVramCoord D_800ADCB0[6];
FieldVramCoord D_800ADCC8[6];

static u8 s_state[0x2400];
static u8 s_heap[0x20000];
static size_t s_heap_off;
static unsigned s_checks;
static int s_menu_main;
static int s_tpage_count;
static int s_tpage_tp[4];
static int s_tpage_abr[4];
static int s_free_skin;
static int s_alloc_skin;
static int s_decode_last;
static int s_overlay_size;

int ArchiveDecodeAlignedSize(int entryIndex)
{
    s_decode_last = entryIndex;
    return 0x100;
}

int ArchiveSetIndex(int directoryIndex, int entryIndex)
{
    (void)directoryIndex;
    (void)entryIndex;
    return 0;
}

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    void* p;

    (void)allocFlags;
    if (s_decode_last == ((D_800ADB64 + 5) & 0x7F) && allocSize == 0x100) {
        s_overlay_size = (int)allocSize;
    }
    if (s_heap_off + allocSize + 16 > sizeof(s_heap)) {
        fprintf(stderr, "ASSERTION heap.exhausted\n");
        exit(1);
    }
    p = s_heap + s_heap_off;
    s_heap_off += (allocSize + 15) & ~15u;
    return p;
}

void HeapFree(void* p)
{
    (void)p;
}

void SetSemiTrans(void* p, int abe)
{
    (void)p;
    (void)abe;
}

u_short GetTPage(int tp, int abr, int x, int y)
{
    (void)x;
    (void)y;
    if (s_tpage_count < 4) {
        s_tpage_tp[s_tpage_count] = tp;
        s_tpage_abr[s_tpage_count] = abr;
    }
    s_tpage_count++;
    return 0x55;
}

void SetDrawMode(DR_MODE* p, int dfe, int dtd, int tpage, RECT* tw)
{
    (void)p;
    (void)dfe;
    (void)dtd;
    (void)tpage;
    (void)tw;
}

int MoveImage(RECT* rect, int x, int y)
{
    (void)rect;
    (void)x;
    (void)y;
    return 0;
}

int DrawSync(int mode)
{
    (void)mode;
    return 0;
}

int StoreImage(RECT* rect, u_long* p)
{
    (void)rect;
    (void)p;
    return 0;
}

int LoadImage(RECT* rect, u_long* p)
{
    (void)rect;
    (void)p;
    return 0;
}

void FieldPartyFreeSkinDataBuffers(void)
{
    s_free_skin++;
}

void FieldPartyAllocateSkinDataBuffers(void)
{
    s_alloc_skin++;
}

void FieldRenderSync(void) {}
void FieldSwapRenderContext(void) {}
void func_80079784(int color) { (void)color; }
void func_800798BC(void) {}
void func_8007995C(short w, short h, short x, short y, int destX, int destY)
{
    (void)w;
    (void)h;
    (void)x;
    (void)y;
    (void)destX;
    (void)destY;
}
void FieldRenderSyncAndFlush(void) {}
void func_800A4748(void) {}
void FontFree(void) {}
void ArchiveCdDataSync(int mode) { (void)mode; }
int func_80029AFC(StreamDataQueueEntry* pEntries, int arg1, int arg2)
{
    (void)pEntries;
    (void)arg1;
    (void)arg2;
    return 0;
}
void MenuMain(void)
{
    s_menu_main++;
}
void FieldScriptMemoryWriteU16(int index, int value)
{
    (void)index;
    (void)value;
}
DISPENV* PutDispEnv(DISPENV* env) { return env; }
DRAWENV* PutDrawEnv(DRAWENV* env) { return env; }
void HeapChangeCurrentUser(int user, void* p) { (void)user; (void)p; }
void func_80070488(void) {}
void func_80070508(void) {}
void SetGeomOffset(int x, int y) { (void)x; (void)y; }
void SetGeomScreen(int h) { (void)h; }
void GamePartySyncSkinData(void) {}
void GamePartySyncStreamedData(void) {}
s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags)
{
    (void)index;
    (void)pBuffer;
    (void)arg2;
    (void)flags;
    return 0;
}
u32 LZSSDecompress(void* src, void* dst)
{
    (void)src;
    (void)dst;
    return 0;
}
void func_800A2488(void) {}
void func_80077544(void)
{
#ifndef MENU_MUTANT_SKIP_WAIT_CLEAR
    (void)0;
#endif
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
    fail("menu.open", detail);
}

static void reset_common(void)
{
    memset(&D_800AFE54, 0, sizeof(D_800AFE54));
    memset(&D_800AFE64, 0, sizeof(D_800AFE64));
    memset(s_state, 0, sizeof(s_state));
    memset(s_heap, 0, sizeof(s_heap));
    memset(D_800ADCB0, 0, sizeof(D_800ADCB0));
    memset(D_800ADCC8, 0, sizeof(D_800ADCC8));
    memset(&g_Scene, 0, sizeof(g_Scene));
    memset(g_FieldRenderContexts, 0, sizeof(g_FieldRenderContexts));
    s_heap_off = 0;
    s_menu_main = 0;
    s_tpage_count = 0;
    s_free_skin = 0;
    s_alloc_skin = 0;
    s_decode_last = -1;
    s_overlay_size = -1;
    g_pGameState = s_state;
    g_FieldCurRenderContext = &g_FieldRenderContexts[0];
    D_800B2264 = 0;
    D_8004F370 = 1;
    D_800AFE84 = 0;
    D_8004F350 = 7;
    D_800594D0 = 0;
    D_800ADB30 = (void*)(uintptr_t)0x801C6000;
    D_800ADB20 = s_heap;
    g_GamePartyMemberSkins[0] = 0xFF;
    g_GamePartyMemberSkins[1] = 0xFF;
    g_GamePartyMemberSkins[2] = 0xFF;
    D_800B21D0[0] = 0;
    g_PcPortFieldMenuOpened = 0;
}

int main(void)
{
    /* Early-out: request 0x80 with D_800B21D0 set. */
    reset_common();
    D_800ADB64 = 0x80;
    D_800B21D0[0] = 1;
    D_8004F350 = 7;
    func_800799D4();
    expect_eq_s32("early.menu", s_menu_main, 0);
    expect_eq_s32("early.wait", D_8004F350, 7);
    expect_eq_s32("early.req", D_800ADB64, 0x80);
    expect_eq_s32("early.free", s_free_skin, 0);

    /* Open menu id 3, D_8004F370==1 overlay size path. */
    reset_common();
    D_800ADB64 = 3;
    func_800799D4();
    expect_eq_s32("open.tile.w", D_800AFE54[0].w, 0x140);
    expect_eq_s32("open.tile.h", D_800AFE54[0].h, 0xE0);
    expect_eq_s32("open.tile.r", D_800AFE54[0].r0, 0);
    expect_eq_s32("open.copy.w", D_800AFE64.w, 0x140);
    expect_eq_s32("open.tpage0.tp", s_tpage_tp[0], 0);
    expect_eq_s32("open.tpage0.abr", s_tpage_abr[0], 2);
    expect_eq_s32("open.tpage.count", s_tpage_count, 2);
    expect_eq_s32("open.free", s_free_skin, 1);
    expect_eq_s32("open.alloc", s_alloc_skin, 1);
    expect_eq_s32("open.overlay", s_overlay_size, 0x100);
    expect_eq_s32("open.req", (s32)D_80059460, 3);
    expect_eq_s32("open.menumain", s_menu_main, 1);
    expect_eq_s32("open.wait", D_8004F350, 0);
    expect_eq_s32("open.done", D_800ADB64, 0xFF);
    expect_eq_s32("open.sync", D_80050100, 2);

    printf("FIELD MENU OPEN 799D4 certificate PASS checks=%u\n", s_checks);
    return 0;
}
