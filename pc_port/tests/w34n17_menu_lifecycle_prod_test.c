#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_menu_lifecycle.h"

u8 D_80059179;
void* D_8005945C;

enum Event {
    E_93, E_966, E_861, E_866, E_891, E_FREE, E_ALLOC, E_71,
    E_STORE, E_MOVE, E_DRAW, E_72DB4, E_ASYNC, E_LZSS, E_CDSYNC,
    E_HEAP_USER, E_ARCHIVE_SET, E_72BB0, E_DECODE, E_READ, E_OT,
    E_8440, E_LOAD, E_978, E_890, E_865, E_85F, E_PALETTE, E_VSYNC,
    E_CONTROLLER, E_75D
};

typedef struct Rect { s16 x, y, w, h; } Rect;
static int events[128], event_count;
static void* free_ptrs[16];
static int free_count, alloc_count, sync_count, failures;
static u32 alloc_sizes[8], alloc_flags[8];
static Rect stores[4], moves[8], loads[4];
static int store_count, move_count, load_count;
static u32 ot_base, ot_count;
static int call_72db4[8], call_72db4_count;
static int return_test;

static void ev(int value) { events[event_count++] = value; }
static void check(int ok, const char* name) {
    if (!ok) { fprintf(stderr, "ASSERTION %s\n", name); failures++; }
}
static void wr32(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u32 rd32(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static u16 rd16(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }

s32 wm_80093F18(u32 a) { ev(E_93); check(a == 0x8009D55Cu, "entry.93F18.argument"); return 4; }
void wm_80096694(void) { ev(E_966); }
void wm_80086124(void) { ev(E_861); }
void wm_800866C8(void) { ev(E_866); }
void wm_80089128(void) { ev(E_891); }
void wm_80071FEC(void) { ev(E_71); }
void wm_80072BB0(void) { ev(E_72BB0); }
int wm_80072DB4(s32 a, s32 b, s32 c, s32 d) {
    ev(E_72DB4); call_72db4[call_72db4_count++] = a;
    call_72db4[call_72db4_count++] = b; call_72db4[call_72db4_count++] = c;
    call_72db4[call_72db4_count++] = d; return 0;
}
int wm_8008440C(void) { ev(E_8440); return 0; }
void wm_800978FC(void) { ev(E_978); }
void wm_8008901C(void) { ev(E_890); }
void wm_800865A0(void) { ev(E_865); }
void wm_80085FE0(void) { ev(E_85F); }
void wm_80075D4C(void) { ev(E_75D); }
void wm_ot_clear_r_guest(u32 base, u32 count) { ev(E_OT); ot_base = base; ot_count = count; }

void HeapChangeCurrentUser(u_int u, char** c) { ev(E_HEAP_USER); check(u == 3u && c == NULL, "return.heap.user.3"); }
int ArchiveSetIndex(int d, int e) { ev(E_ARCHIVE_SET); check(d == 0x24 && e == 0, "return.archive.index.24"); return 0; }
int ArchiveDecodeAlignedSize(unsigned int i) { ev(E_DECODE); check(i == 0x123u, "return.decode.dynamic.C174"); return 0x2200; }
int ArchiveReadFileToBuffer(int i, void* p, int a, int f) { ev(E_READ); check(i == 0x123 && p == g_PsxRam + 0x1A0000 && a == 0 && f == 0, "return.read.BD20"); return 0; }
int ArchiveDataSync(void) { static const int v[3] = {3,2,1}; ev(E_ASYNC); return v[sync_count++]; }
void ArchiveCdDataSync(int m) { ev(E_CDSYNC); check(m == 0, "archive.cd.sync.zero"); }
void* HeapAlloc(u_int size, u_int flags) {
    static const u32 offsets[5] = {0x1C5000u,0x190000u,0x100000u,0x110000u,0x1A0000u};
    void* p = g_PsxRam + (return_test ? 0x1A0000u : offsets[alloc_count]); ev(E_ALLOC);
    alloc_sizes[alloc_count] = size; alloc_flags[alloc_count] = flags; alloc_count++; return p;
}
unsigned int HeapFree(void* p) { ev(E_FREE); free_ptrs[free_count++] = p; return 0; }
void* LZSSDecompress(void* s, void* d) { ev(E_LZSS); check(s == g_PsxRam + 0x160000 && d == g_PsxRam + 0x190000, "entry.lzss.pointers"); return d; }
int StoreImage(Rect* r, u32* d) { ev(E_STORE); stores[store_count++] = *r; (void)d; return 0; }
int LoadImage(Rect* r, u32* d) { ev(E_LOAD); loads[load_count++] = *r; (void)d; return 0; }
int MoveImage(Rect* r, int x, int y) { ev(E_MOVE); moves[move_count++] = *r; moves[move_count-1].x = (s16)x; moves[move_count-1].y = (s16)y; return 0; }
int DrawSync(int mode) { ev(E_DRAW); check(mode == 0, "drawsync.zero"); return 0; }
int VSync(int mode) { ev(E_VSYNC); check(mode == 0, "vsync.zero"); return 0; }
void SystemTransferPaletteToVRAM(short x, short y) { ev(E_PALETTE); check(x == 304 && y == 480, "return.palette.xy"); }
void ControllerResetState(void) { ev(E_CONTROLLER); }

static int find_after(int start, int event) {
    int i; for (i = start; i < event_count; i++) if (events[i] == event) return i; return -1;
}
static void reset_trace(void) {
    event_count=free_count=alloc_count=sync_count=store_count=move_count=load_count=call_72db4_count=0;
    memset(events,0,sizeof(events)); memset(free_ptrs,0,sizeof(free_ptrs));
}

static void test_entry(void) {
    int p93,p861,p866,p891,p71,pstore,pmove,pfade,plzss;
    memset(g_PsxRam,0,PSX_RAM_SIZE); reset_trace(); return_test=0; D_80059179=7;
    wr32(0x8009BD3Au,0x3800u); *(u8*)PSX_ADDR(0x8006F8E5u)=9;
    *(u8*)PSX_ADDR(0x8006F8E6u)=8; *(u8*)PSX_ADDR(0x8006F8E7u)=7;
    wr32(0x8009BC3Cu,0x80012000u); wr32(0x8009BCB4u,0x80013000u);
    wr32(0x8009D528u,0x80160000u); wr32(0x8009D7F0u,0u);
    wm_800758C0();
    p93=find_after(0,E_93); p861=find_after(p93+1,E_861); p866=find_after(p861+1,E_866);
    p891=find_after(p866+1,E_891); p71=find_after(p891+1,E_71);
    pstore=find_after(p71+1,E_STORE); pmove=find_after(pstore+1,E_MOVE);
    pfade=find_after(pmove+1,E_72DB4); plzss=find_after(pfade+1,E_LZSS);
    check(p93>=0&&p861>p93&&p866>p861&&p891>p866&&p71>p891,
          "entry.retail.teardown.order");
    check(pstore>p71&&pmove>pstore&&pfade>pmove&&plzss>pfade,
          "entry.readback.fade.decompress.order");
    check(rd16(0x8006EE6Au)==1u && rd16(0x8006EE70u)==9u &&
          rd16(0x8006EE72u)==8u && rd16(0x8006EE74u)==7u,
          "entry.controller.state.snapshot");
    check(rd32(0x8009D14Cu)==7u && D_80059179==1u,
          "entry.saves.and.selects.field.flag");
    check(alloc_count==4 && alloc_sizes[0]==4u && alloc_sizes[1]==4u &&
          alloc_sizes[2]==0x10000u && alloc_sizes[3]==0xC800u,
          "entry.alloc.sizes.and.scratch.math");
    check(alloc_flags[0]==1u&&alloc_flags[1]==1u&&alloc_flags[2]==0u&&alloc_flags[3]==0u,
          "entry.alloc.flags");
    check(sync_count==3,"entry.archive.polls.until.below.two");
    check(move_count==3,"entry.conditional.and.fixed.moves");
    check(call_72db4_count==4&&call_72db4[0]==16&&call_72db4[1]==0&&call_72db4[2]==8&&call_72db4[3]==2,
          "entry.transition.arguments");
}

static void test_return(void) {
    int p978,p890,p865,p85f,p75d; reset_trace(); return_test=1;
    D_8005945C=g_PsxRam+0x140000; D_80059179=0xEEu;
    wr32(0x8009C7E4u,0x80190000u); wr32(0x8009C174u,0x123u);
    wr32(0x8009C800u,0x80100000u); wr32(0x8009C890u,0x80110000u);
    wr32(0x8009BE3Cu,0x8009E000u); wr32(0x8009E070u,0x800A2228u);
    wr32(0x8009D14Cu,0x5Au); wr32(0x8009D804u,9u);
    { u16 one=1; memcpy(PSX_ADDR(0x8006EE6Au),&one,2); }
    wm_80075B58();
    check(alloc_count==1&&alloc_sizes[0]==0x2200u&&alloc_flags[0]==1u&&
          rd32(0x8009BD20u)==0x801A0000u,"return.reloads.BD20.as.guest");
    check(move_count==2&&loads[0].x==384&&loads[1].y==432,
          "return.restores.both.vram.regions");
    check(ot_base==0x800A2228u&&ot_count==0x400u,"return.clears.active.guest.ot");
    check(find_after(0,E_8440)>=0,"return.reenters.8440C");
    p978=find_after(0,E_978); p890=find_after(p978+1,E_890); p865=find_after(p890+1,E_865);
    p85f=find_after(p865+1,E_85F); p75d=find_after(p85f+1,E_75D);
    check(p978>=0&&p890>p978&&p865>p890&&p85f>p865&&p75d>p85f,
          "return.rebuild.and.reconcile.order");
    check(rd32(0x8009D804u)==0u&&rd16(0x8006EE6Au)==0u&&D_80059179==0x5Au,
          "return.restores.final.state");
}

int main(void) {
    test_entry(); test_return();
    if(failures) return 1;
    puts("W34N17 MENU LIFECYCLE PRODUCTION CERTIFICATE PASS"); return 0;
}
