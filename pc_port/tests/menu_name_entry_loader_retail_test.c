/* Retail differential test for the name-entry resource loader
 * NameEntryMenu_801C5318 (menu mode 3, archive dir 0x10 file 8), whose port
 * body is NON_MATCHING: retail MIPS from disc/name_entry_menu.bin runs in the
 * interpreter, the port body from src/name_entry_menu/main/misc.c runs
 * natively, against the same guest RAM (see lib/menu_retail_harness.h).
 *
 * File I/O is stubbed: the resource archive D_8005945C is a synthetic table
 * whose entries LZSSHeapDecompress maps to seeded guest buffers (save icon
 * TIM, font, sprite atlas, strings, portrait TIMs 0xB20 apart, name table),
 * and the debug-only SEDS load goes through recorded Archive* stubs.  Both
 * worlds must make the same calls with the same arguments, pass the same
 * RECTs to LoadImage, and leave the same RAM, SystemMenu fields and save
 * header block (g_Menu->unk32C, MenuUnk2, mirrored field-wise because its
 * TIM_IMAGE holds host pointers in the port).
 *
 * One divergence is documented and pinned: retail parks the name-table
 * pointer in g_Menu+0x1E24 (unk1E24[0..3]) and frees the table before
 * returning; no other code in the overlay reads that word.  The port keeps
 * the pointer in a local, so unk1E24[0..3] stays untouched natively while
 * unk1E24[4..6] (the per-id bytes) must match. */
#include "common.h"
#include "system/menu.h"
#include "menu_retail_harness.h"
#include <stddef.h>

#define G_MENU_PTR 0x800625A0u
#define D_RESOURCES 0x8005945Cu
#define D_SEDS 0x8006259Cu
#define D_NAME_ID 0x80059171u
#define D_DEBUG 0x80059178u
#define GAMESTATE 0x8006D634u
#define MENU_BLOCK 0x80070000u
#define MANAGER 0x800A0000u
#define WORK 0x800A1000u      /* g_Menu->unk1E20 (MenuUnk7) */
#define U2_RETAIL 0x800B0000u /* g_Menu->unk32C, retail layout (retail world) */
#define U2_NATIVE 0x800C0000u /* the same block, native layout (native world) */
#define RES 0x800D0000u       /* resource archive: count + offsets, blobs at RES + 0x100 * k */
#define BUF_ICON 0x800D1000u
#define BUF_FONT 0x800D2000u
#define BUF_ATLAS 0x800D3000u
#define BUF_TEXT 0x800D4000u
#define BUF_NAMES 0x800D5000u
#define BUF_FACES 0x800E0000u /* 11 portrait TIMs, 0xB20 apart */
#define SEDS_OLD 0x800F0000u
#define SEDS_NEW 0x800F8000u
#define ENTRY 0x801C5318u
#define ENTRY_END 0x801C58B8u /* NameEntryMenuInitialize */

#define NELEM(a) ((unsigned)(sizeof(a) / sizeof((a)[0])))
static const HxField menu_fields[] = {
#include "system_menu_mirror.inc"
};
static const HxField u2_fields[] = {
#include "menu_unk2_mirror.inc"
};
static SystemMenu native_menu;
SystemMenu* g_Menu = &native_menu;
void* D_8005945C;
void* D_8006259C;
extern void NameEntryMenu_801C5318(void);

enum {
    F_RESOLVE = 0x8003342Cu, F_LZSS = 0x80032E88u, F_OPENTIM = 0x800471B4u, F_READTIM = 0x800471C4u,
    F_BZERO = 0x8003F8E8u, F_MEMMOVE = 0x8003F99Cu, F_FREE = 0x800320E8u, F_FONT = 0x8002DD20u,
    F_SPRITE = 0x80026338u, F_LOADIMAGE = 0x80044894u, F_DRAWSYNC = 0x800445D0u,
    F_SETINDEX = 0x80028470u, F_ALLOC = 0x80031BDCu, F_ALIGNED = 0x800288ECu,
    F_READFILE = 0x800295D8u, F_CDSYNC = 0x80028A60u, F_SEDS = 0x80038428u,
};
const HxCallee hx_callees[] = {
    {F_RESOLVE, 1, "ResolveArchiveEntryPointers"}, {F_LZSS, 2, "LZSSHeapDecompress"},
    {F_OPENTIM, 1, "OpenTIM"}, {F_READTIM, 1, "ReadTIM"}, {F_BZERO, 2, "bzero"},
    {F_MEMMOVE, 3, "memmove"}, {F_FREE, 1, "HeapFree"}, {F_FONT, 1, "func_8002DD20"},
    {F_SPRITE, 8, "func_80026338"}, {F_LOADIMAGE, 2, "LoadImage"}, {F_DRAWSYNC, 1, "DrawSync"},
    {F_SETINDEX, 2, "ArchiveSetIndex"}, {F_ALLOC, 2, "HeapAlloc"},
    {F_ALIGNED, 1, "ArchiveDecodeAlignedSize"}, {F_READFILE, 4, "ArchiveReadFileToBuffer"},
    {F_CDSYNC, 1, "ArchiveCdDataSync"}, {F_SEDS, 1, "SoundAddSedsEntry"},
};
const unsigned hx_ncallees = NELEM(hx_callees);

static unsigned pattern;
static uint32_t opened[2];   /* OpenTIM address, per world */
static uintptr_t pos_base[2]; /* first func_80026338 output pointer, per world */
static uint8_t rects[2][16][8];
static unsigned nrects[2];

static uint32_t sprite_value(uint32_t id, unsigned k) { return (id * 0x111u + k * 0x2Bu + pattern * 7u) & 0x3FFu; }
static int in_stack(uint32_t a) { return a >= HX_STACK_LOW && a < HX_STACK_TOP; }

/* Retail-layout guest address of a native pointer: inside the native
 * MenuUnk2 it is translated member-wise, anything else must be guest RAM. */
static uint32_t xlat(const void* p) {
    uintptr_t v = (uintptr_t)p;
    if (v >= U2_NATIVE && v < U2_NATIVE + sizeof(MenuUnk2)) {
        size_t off = v - U2_NATIVE;
        unsigned i;
        for (i = 0; i < NELEM(u2_fields); ++i) {
            const HxField* f = &u2_fields[i];
            if (off >= f->native_off && off < f->native_off + f->native_elem * f->count) {
                HX_REQUIRE(f->native_elem * f->count == f->retail_span, "pointer into resized member %s", f->name);
                return U2_RETAIL + f->retail_off + (uint32_t)(off - f->native_off);
            }
        }
        HX_REQUIRE(0, "native MenuUnk2 offset %zx in no member", off);
    }
    return hx_addr(p);
}

uint32_t hx_effect(uint32_t fn, const uint32_t* a, unsigned index, int side) {
    uint32_t* args = (uint32_t*)(uintptr_t)a; /* this event's stored arguments */
    uint32_t k;
    (void)index;
    switch (fn) {
    case F_RESOLVE: { /* offsets -> pointers, in place */
        uint32_t n = hx_r32(a[0]);
        for (k = 1; k <= n; ++k) hx_w32(a[0] + 4 * k, hx_r32(a[0] + 4 * k) + a[0]);
        return n;
    }
    case F_LZSS: {
        static const uint32_t out[7] = {0, BUF_ICON, BUF_FONT, BUF_ATLAS, BUF_TEXT, BUF_FACES, BUF_NAMES};
        for (k = 1; k <= 6; ++k)
            if (a[0] == RES + 0x100u * k) return out[k];
        HX_REQUIRE(0, "LZSSHeapDecompress of %08x, not a resource entry", a[0]);
        return 0;
    }
    case F_OPENTIM: opened[side] = a[0]; return 0;
    case F_READTIM: /* side 1: the native stub filled its TIM_IMAGE */
        if (side == 0) {
            hx_w32(a[0] + 0x00, 8u | (opened[0] & 0xF0u));
            hx_w32(a[0] + 0x04, opened[0] + 0x0C);
            hx_w32(a[0] + 0x08, opened[0] + 0x14);
            hx_w32(a[0] + 0x0C, opened[0] + 0x40);
            hx_w32(a[0] + 0x10, opened[0] + 0x48);
            args[0] = a[0] == U2_RETAIL + 0xB80u ? 1u : in_stack(a[0]) ? 2u : 0xBADu;
        }
        return 0;
    case F_SPRITE: /* 6 s32 outputs (pos.word[row][0..5]); side 1 wrote its own */
        if (side == 0) {
            if (!pos_base[0]) pos_base[0] = a[2];
            for (k = 0; k < 6; ++k) {
                hx_w32(a[2 + k], sprite_value(a[1], k));
                args[2 + k] = 0x5AC00000u | (a[2 + k] - (uint32_t)pos_base[0]);
            }
        }
        return 0;
    case F_MEMMOVE: /* side 1: done by the native stub on native pointers */
        if (side == 0) {
            uint8_t tmp[0x100];
            HX_REQUIRE(a[2] <= sizeof tmp, "memmove size %u", a[2]);
            for (k = 0; k < a[2]; ++k) tmp[k] = hx_r8(a[1] + k);
            for (k = 0; k < a[2]; ++k) hx_w8(a[0] + k, tmp[k]);
        }
        return a[0];
    case F_BZERO:
        if (side == 0)
            for (k = 0; k < a[1]; ++k) hx_w8(a[0] + k, 0);
        return 0;
    case F_LOADIMAGE:
        HX_REQUIRE(nrects[side] < 16, "too many LoadImage calls");
        memcpy(rects[side][nrects[side]++], hx_p(a[0]), 8);
        return 0;
    case F_ALLOC: return SEDS_NEW;
    case F_ALIGNED: return 0x1C40u + pattern;
    case F_READFILE:
        for (k = 0; k < 0x40; ++k) hx_w8(a[1] + k, (uint8_t)(k * 11 + pattern + a[0]));
        return 0;
    default: return 0;
    }
}

/* ---- native recording stubs (same effects through hx_effect) ---------- */
#define N(fn, n, ...) hx_native(fn, n, __VA_ARGS__)
unsigned int ResolveArchiveEntryPointers(u32* table) { return N(F_RESOLVE, 1, hx_addr(table)); }
void* LZSSHeapDecompress(void* src, int flag) { return (void*)(uintptr_t)N(F_LZSS, 2, hx_addr(src), (uint32_t)flag); }
int OpenTIM(u_long* addr) { return (int)N(F_OPENTIM, 1, hx_addr(addr)); }
TIM_IMAGE* ReadTIM(TIM_IMAGE* t) {
    uintptr_t v = (uintptr_t)t;
    uint32_t token = t == &((MenuUnk2*)(uintptr_t)U2_NATIVE)->tim ? 1u
                   : (v < HX_BASE || v >= (uintptr_t)HX_BASE + HX_SIZE) ? 2u : 0xBADu;
    t->mode = 8u | (opened[1] & 0xF0u);
    t->crect = (void*)(uintptr_t)(opened[1] + 0x0C);
    t->caddr = (void*)(uintptr_t)(opened[1] + 0x14);
    t->prect = (void*)(uintptr_t)(opened[1] + 0x40);
    t->paddr = (void*)(uintptr_t)(opened[1] + 0x48);
    N(F_READTIM, 1, token);
    return t;
}
void* NeHxMemmove(void* d, const void* s, uint32_t n) {
    uint8_t tmp[0x100];
    uint32_t k;
    HX_REQUIRE(n <= sizeof tmp, "memmove size %u", n);
    for (k = 0; k < n; ++k) tmp[k] = ((const uint8_t*)s)[k];
    for (k = 0; k < n; ++k) ((uint8_t*)d)[k] = tmp[k];
    N(F_MEMMOVE, 3, xlat(d), hx_addr(s), n);
    return d;
}
void NeHxBzero(void* p, uint32_t n) {
    uint32_t k;
    for (k = 0; k < n; ++k) ((uint8_t*)p)[k] = 0;
    N(F_BZERO, 2, xlat(p), n);
}
void HeapFree(void* p) { N(F_FREE, 1, hx_addr(p)); }
void func_8002DD20(u32* p) { N(F_FONT, 1, hx_addr(p)); }
void func_80026338(u8* atlas, s32 id, s32* p0, s32* p1, s32* p2, s32* p3, s32* p4, s32* p5) {
    s32* p[6] = {p0, p1, p2, p3, p4, p5};
    uint32_t tok[6];
    unsigned k;
    if (!pos_base[1]) pos_base[1] = (uintptr_t)p0;
    for (k = 0; k < 6; ++k) {
        *p[k] = (s32)sprite_value((uint32_t)id, k);
        tok[k] = 0x5AC00000u | (uint32_t)((uintptr_t)p[k] - pos_base[1]);
    }
    N(F_SPRITE, 8, hx_addr(atlas), (uint32_t)id, tok[0], tok[1], tok[2], tok[3], tok[4], tok[5]);
}
int LoadImage(RECT16* r, u_long* p) { return (int)N(F_LOADIMAGE, 2, hx_addr(r), hx_addr(p)); }
int DrawSync(int mode) { return (int)N(F_DRAWSYNC, 1, (uint32_t)mode); }
void ArchiveSetIndex(s32 dir, s32 file) { N(F_SETINDEX, 2, (uint32_t)dir, (uint32_t)file); }
void* HeapAlloc(s32 size, s32 flag) { return (void*)(uintptr_t)N(F_ALLOC, 2, (uint32_t)size, (uint32_t)flag); }
s32 ArchiveDecodeAlignedSize(s32 file) { return (s32)N(F_ALIGNED, 1, (uint32_t)file); }
void ArchiveReadFileToBuffer(s32 file, void* buf, s32 off, s32 flags) {
    N(F_READFILE, 4, (uint32_t)file, hx_addr(buf), (uint32_t)off, (uint32_t)flags);
}
void ArchiveCdDataSync(s32 mode) { N(F_CDSYNC, 1, (uint32_t)mode); }
void SoundAddSedsEntry(void* p) { N(F_SEDS, 1, hx_addr(p)); }

/* ---- cases --------------------------------------------------------------- */
static void fill(uint32_t a, uint32_t n, unsigned mul, unsigned seed) {
    uint32_t i;
    for (i = 0; i < n; ++i) hx_w8(a + i, (uint8_t)(i * mul + seed + (i >> 8)));
}

static void setup(unsigned id, unsigned flag, unsigned dbg) {
    static const uint8_t slots[4][2] = {{1, 2}, {0xFF, 5}, {3, 0xFF}, {0xFF, 0xFF}};
    const uint8_t* s = slots[(id + flag + dbg * 2) % 4];
    MenuUnk2* u2 = (MenuUnk2*)(uintptr_t)U2_NATIVE;
    uint32_t k, gs;

    fill(GAMESTATE, 0x2300, 7, pattern * 3);
    gs = hx_r32(GAMESTATE + offsetof(GameState, unk1D30));
    hx_w32(GAMESTATE + offsetof(GameState, unk1D30), flag ? gs | 0x400u : gs & ~0x400u);
    fill(MANAGER, sizeof(MenuManager), 29, pattern * 5);
    hx_w8(MANAGER + offsetof(MenuManager, currentCharacterIDs) + 1, s[0]);
    hx_w8(MANAGER + offsetof(MenuManager, currentCharacterIDs) + 2, s[1]);
    fill(WORK, sizeof(MenuUnk7), 13, pattern);
    hx_w32(RES, 6);
    for (k = 1; k <= 6; ++k) hx_w32(RES + 4 * k, 0x100u * k);
    fill(RES + 0x100, 0x600, 3, pattern);
    fill(BUF_ICON, 0x200, 5, pattern + 1);
    fill(BUF_FONT, 0x100, 9, pattern + 2);
    fill(BUF_ATLAS, 0x100, 15, pattern + 3);
    fill(BUF_TEXT, 0x100, 17, pattern + 4);
    fill(BUF_NAMES, 0x80, 19, pattern + 5);
    for (k = 0; k < 20; ++k) hx_w8(BUF_NAMES + k, (uint8_t)((k * 3 + pattern) % 11)); /* Gear pilots */
    fill(BUF_FACES, 11 * 0xB20, 23, pattern + 6);
    fill(SEDS_OLD, 0x40, 27, pattern);
    fill(SEDS_NEW, 0x40, 31, pattern);

    /* save header block: native layout seeded, mirrored to the retail layout */
    fill(U2_NATIVE, sizeof(MenuUnk2), 37, pattern);
    memset(&u2->tim, 0, sizeof(u2->tim));
    fill(U2_RETAIL, 0x5034, 41, pattern); /* overwritten member-wise below */
    hx_mirror_out(u2_fields, NELEM(u2_fields), u2, U2_RETAIL);
    for (k = 0; k < 0x14; k += 4) hx_w32(U2_RETAIL + 0xB80 + k, 0);

    memset(&native_menu, 0, sizeof(native_menu));
    native_menu.renderContext = (int)(pattern & 1);
    native_menu.pManager = (MenuManager*)(uintptr_t)MANAGER;
    native_menu.unk32C = u2;
    native_menu.unk1E20 = (MenuUnk7*)(uintptr_t)WORK;
    native_menu.unk2DC = (void*)(uintptr_t)(SEDS_OLD + 0x10);
    native_menu.unk2E0 = (void*)(uintptr_t)(SEDS_OLD + 0x20);
    native_menu.unk2E4 = (void*)(uintptr_t)(SEDS_OLD + 0x30);
    for (k = 0; k < 8; ++k) native_menu.unk1E24[k] = (u8)(0xE0 + k + pattern);
    hx_mirror_out(menu_fields, NELEM(menu_fields), &native_menu, MENU_BLOCK);
    hx_w32(MENU_BLOCK + 0x32C, U2_RETAIL);
    hx_w32(G_MENU_PTR, MENU_BLOCK);

    hx_w8(D_NAME_ID, (uint8_t)id);
    hx_w8(D_DEBUG, (uint8_t)dbg);
    hx_w32(D_RESOURCES, RES);
    D_8005945C = (void*)(uintptr_t)RES;
    hx_w32(D_SEDS, SEDS_OLD);
    D_8006259C = (void*)(uintptr_t)SEDS_OLD;
}

static void reset_world(int side) {
    opened[side] = 0;
    pos_base[side] = 0;
    nrects[side] = 0;
    memset(rects[side], 0, sizeof(rects[side]));
}

static void finish(void) {
    static const uint32_t skip[][2] = {
        {MENU_BLOCK, MENU_BLOCK + 0x1E98}, {U2_RETAIL, U2_RETAIL + 0x5034},
        {U2_NATIVE, U2_NATIVE + sizeof(MenuUnk2)}, {D_SEDS, D_SEDS + 4},
    };
    const MenuUnk2* u2 = (const MenuUnk2*)(uintptr_t)U2_NATIVE;
    unsigned k;

    hx_compare_events();
    HX_REQUIRE(nrects[0] == nrects[1], "LoadImage count retail=%u native=%u", nrects[0], nrects[1]);
    for (k = 0; k < nrects[0]; ++k)
        HX_REQUIRE(memcmp(rects[0][k], rects[1][k], 8) == 0, "LoadImage %u RECT contents", k);
    hx_compare_ram(skip, NELEM(skip));
    HX_REQUIRE(hx_retail_r32(D_SEDS) == hx_addr(D_8006259C), "D_8006259C retail=%08x native=%08x",
               hx_retail_r32(D_SEDS), hx_addr(D_8006259C));

    /* documented divergence: retail's parked (then freed) table pointer */
    HX_REQUIRE(hx_retail_r32(MENU_BLOCK + 0x1E24) == BUF_NAMES, "retail did not park the name table at 0x1E24");
    for (k = 0; k < 4; ++k)
        HX_REQUIRE(native_menu.unk1E24[k] == (u8)(0xE0 + k + pattern), "port wrote unk1E24[%u]", k);
    memcpy(native_menu.unk1E24, hx_retail_after + (MENU_BLOCK + 0x1E24 - HX_BASE), 4);
    native_menu.unk32C = (MenuUnk2*)(uintptr_t)U2_RETAIL; /* same block, other layout */
    hx_mirror_compare(menu_fields, NELEM(menu_fields), &native_menu, MENU_BLOCK);

    hx_mirror_compare(u2_fields, NELEM(u2_fields), u2, U2_RETAIL);
    HX_REQUIRE(hx_retail_r32(U2_RETAIL + 0xB80) == u2->tim.mode, "tim.mode");
    HX_REQUIRE(hx_retail_r32(U2_RETAIL + 0xB84) == hx_addr(u2->tim.crect), "tim.crect");
    HX_REQUIRE(hx_retail_r32(U2_RETAIL + 0xB88) == hx_addr(u2->tim.caddr), "tim.caddr");
    HX_REQUIRE(hx_retail_r32(U2_RETAIL + 0xB8C) == hx_addr(u2->tim.prect), "tim.prect");
    HX_REQUIRE(hx_retail_r32(U2_RETAIL + 0xB90) == hx_addr(u2->tim.paddr), "tim.paddr");
}

int main(int argc, char** argv) {
    unsigned id, flag, dbg, hit;
    HX_REQUIRE(argc == 2, "usage: test disc/name_entry_menu.bin");
    hx_map();
    HX_REQUIRE(hx_load(argv[1], 0x801C5000u, 0) == 0x7800, "overlay image size");
    hx_coverage(ENTRY, ENTRY_END);
    for (id = 0; id <= 30; ++id)          /* the whole 31-entry name table */
    for (flag = 0; flag < 2; ++flag)      /* g_GameState.unk1D30 & 0x400 (Gear 20's pilot) */
    for (dbg = 0; dbg < 2; ++dbg) {       /* g_MenuDebugEnabled: SEDS bank load */
        ++hx_case;
        pattern = (id * 5 + flag * 3 + dbg) % 7;
        setup(id, flag, dbg);
        hx_begin_case();
        reset_world(0);
        hx_run_retail(ENTRY, 0, 0, 0, 0, 400000);
        hx_retail_done();
        reset_world(1);
        NameEntryMenu_801C5318();
        finish();
    }
    hit = hx_coverage_report("NameEntryMenu_801C5318", NULL, 0);
    HX_REQUIRE(hit == (ENTRY_END - ENTRY) / 4, "retail coverage %u/%u", hit, (ENTRY_END - ENTRY) / 4);
    printf("NAME ENTRY LOADER PASS cases=%u checks=%u\n", hx_case, hx_checks);
    return 0;
}
