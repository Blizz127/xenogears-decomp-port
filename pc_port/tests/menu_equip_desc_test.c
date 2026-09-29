#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SystemMenu* g_Menu;
u8 D_801EA730[0x190];
__asm__(".globl D_801EA7F8\n.set D_801EA7F8, D_801EA730 + 0xC8\n");
extern u8 D_801EA7F8[];

u8 stateStorage[0x4600] __attribute__((aligned(8)));
__asm__(".globl g_GameState\n.set g_GameState, stateStorage\n");

static SystemMenu menu;
static MenuManager manager;
static u8 work[0x400];
static u8 listBuf[0xA1C];
static u8 renderGuest[0x800];
static u8 descBundles[4][0x800];
static u8 ram[0x200000];

extern void func_801DFF5C(s32, s32, s32, s32, s32, s32, s32);

static int guest_mode;

void* HeapAlloc(s32 size, s32 flag) {
    (void)flag;
    (void)size;
    memset(renderGuest, 0, sizeof renderGuest);
    if (guest_mode) return (void*)(uintptr_t)0x8010a000;
    return renderGuest;
}
void HeapFree(void* p) { (void)p; }
void bzero(void* p, s32 n) { memset(p, 0, (size_t)n); }
/* Retail DFF5C never calls this; the decomp references it only from an
 * XENO_FIELD_TEST-gated diagnostic block.  Satisfy the link. */
static u8 emptyName[8];
void* GetAccessoryName(s32 index) { (void)index; return emptyName; }

void* GetStringEntry(void* bundle, s32 index) {
    return (u8*)bundle + *(u16*)((u8*)bundle + index * 2 + 4);
}

s32 SystemRenderStringEntry(void* string, void* workBuf, s32 height, s32 field) {
    u8* s = string;
    (void)workBuf;
    (void)height;
    return 8 + (field & 3) + (s ? (s[0] & 7) : 0);
}

int LoadImage(RECT* rect, u_long* data) {
    (void)rect;
    (void)data;
    return 0;
}
int DrawSync(int mode) {
    (void)mode;
    return 0;
}
u_short GetTPage(int tp, int abr, int x, int y) {
    (void)tp;
    (void)abr;
    (void)x;
    (void)y;
    return 0;
}
void SetSemiTrans(void* p, int abe) {
    (void)p;
    (void)abe;
}
void SetPolyFT4(POLY_FT4* p) { (void)p; }
void SetShadeTex(void* p, int tge) {
    (void)p;
    (void)tge;
}

static void pattern(void* p, size_t n, unsigned seed) {
    u8* b = p;
    size_t i;
    for (i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        b[i] = (u8)(seed >> 24);
    }
}

static void store_ptr(void* slot, void* p) {
    u32 truncated = (u32)(uintptr_t)p;
    memcpy(slot, &truncated, 4);
}

static void init_bundle(u8* bundle, unsigned seed, unsigned bank) {
    unsigned i;
    memset(bundle, 0, 0x800);
    for (i = 0; i < 0x300; i++) {
        u16 off = (u16)(0x610 + (i % 32) * 4);
        memcpy(bundle + 4 + i * 2, &off, 2);
    }
    for (i = 0; i < 32; i++) {
        bundle[0x610 + i * 4] = (u8)(0x20 + ((seed + bank + i) & 0x1F));
        bundle[0x611 + i * 4] = (u8)(bank + 1);
        bundle[0x612 + i * 4] = 0;
        bundle[0x613 + i * 4] = 0;
    }
}

static void init_case(unsigned seed, unsigned ch, unsigned gear, unsigned slot,
                      int guest_ptrs) {
    unsigned i;
    pattern(stateStorage, sizeof stateStorage, seed ^ 0xA5A5u);
    memset(work, 0, sizeof work);
    memset(listBuf, 0, sizeof listBuf);
    memset(D_801EA730, 0, sizeof D_801EA730);
    memset(&menu, 0, sizeof menu);
    memset(&manager, 0, sizeof manager);
    g_Menu = &menu;
    menu.pManager = &manager;
    ((u8*)&menu)[0x308] = (u8)(seed & 3);
    menu.unk42C[2] = (u32)(uintptr_t)listBuf;
    *(u32*)&menu.unk358[8] = (u32)(uintptr_t)work;
    store_ptr((u8*)&menu + 0x360, work);
    store_ptr((u8*)&menu + 0x434, listBuf);
    manager.currentCharacterIDs[slot] = (u8)ch;
    stateStorage[0x30C + ch * 0xA4] = (u8)gear;

    for (i = 0; i < 4; i++) init_bundle(descBundles[i], seed, i);
    if (guest_ptrs) {
        store_ptr(listBuf + 0xA00, (void*)(uintptr_t)0x80106000);
        store_ptr(listBuf + 0xA04, (void*)(uintptr_t)0x80106800);
        store_ptr(listBuf + 0xA08, (void*)(uintptr_t)0x80107000);
        store_ptr(listBuf + 0xA0C, (void*)(uintptr_t)0x80107800);
    } else {
        store_ptr(listBuf + 0xA00, descBundles[0]);
        store_ptr(listBuf + 0xA04, descBundles[1]);
        store_ptr(listBuf + 0xA08, descBundles[2]);
        store_ptr(listBuf + 0xA0C, descBundles[3]);
    }

    for (i = 0; i < 0x40; i++) {
        D_801EA730[i] = (u8)((seed + i * 3) % 60);
        if (i == ((seed + 7) % 16)) D_801EA730[i] = 0;
    }

    stateStorage[0x2D6 + ch * 0xA4] = (u8)(1 + (seed % 40));
    for (i = 0; i < 5; i++) {
        stateStorage[0x2DB + ch * 0xA4 + i] = (u8)(2 + ((seed + i) % 40));
        stateStorage[0x2DF + ch * 0xA4 + i] = (u8)(3 + ((seed + i * 2) % 40));
    }
    stateStorage[0x984 + gear * 0xA4] = (u8)(4 + (seed % 40));
    for (i = 0; i < 5; i++) {
        stateStorage[0x97C + gear * 0xA4 + i] = (u8)(5 + ((seed + i) % 40));
        stateStorage[0x980 + gear * 0xA4 + i] = (u8)(6 + ((seed + i * 3) % 40));
    }
    if ((seed % 11) == 0) stateStorage[0x2D6 + ch * 0xA4] = 0;
}

static int memory(u32 a, unsigned w, u32* v, int write) {
    u8* p = NULL;
    if (w == 4 && !write) {
        switch (a) {
        case 0x800625a0: *v = 0x80100000; return 0;
        case 0x8010033c: *v = 0x80102000; return 0;
        case 0x80100434: *v = 0x80104000; return 0;
        default: break;
        }
    }
    if (a >= 0x8006d634 && (uint64_t)a + w <= 0x8006d634 + sizeof stateStorage)
        p = stateStorage + a - 0x8006d634;
    /* Manager must win over inflated native SystemMenu (0x2318 bytes). */
    else if (a >= 0x80102000 && (uint64_t)a + w <= 0x80102000 + sizeof manager)
        p = (u8*)&manager + a - 0x80102000;
    else if (a >= 0x80100000 && (uint64_t)a + w <= 0x80100000 + 0x1E98)
        p = (u8*)&menu + a - 0x80100000;
    else if (a >= 0x80104000 && (uint64_t)a + w <= 0x80104000 + sizeof listBuf)
        p = listBuf + a - 0x80104000;
    else if (a >= 0x80105000 && (uint64_t)a + w <= 0x80105000 + sizeof work)
        p = work + a - 0x80105000;
    else if (a >= 0x80106000 && (uint64_t)a + w <= 0x80106000 + sizeof descBundles)
        p = ((u8*)descBundles) + a - 0x80106000;
    else if (a >= 0x8010a000 && (uint64_t)a + w <= 0x8010a000 + sizeof renderGuest)
        p = renderGuest + a - 0x8010a000;
    else if (a >= 0x801ea730 && (uint64_t)a + w <= 0x801ea730 + sizeof D_801EA730)
        p = D_801EA730 + a - 0x801ea730;
    else if (a >= 0x801c5000 && (uint64_t)a + w <= 0x80200000)
        p = ram + (a & 0x1fffff);
    if (!p) return -1;
    if (write) {
        unsigned i;
        for (i = 0; i < w; i++) p[i] = (u8)(*v >> (8 * i));
    } else {
        unsigned i;
        *v = 0;
        for (i = 0; i < w; i++) *v |= (u32)p[i] << (8 * i);
    }
    return 0;
}

static int rd(void* u, u32 a, unsigned w, u32* v) {
    (void)u;
    return memory(a, w, v, 0);
}
static int wr(void* u, u32 a, unsigned w, u32 v) {
    (void)u;
    return memory(a, w, &v, 1);
}

static int bridge(void* u, PcPortMipsCpu* c, u32 pc) {
    (void)u;
    switch (pc) {
    case 0x80031bdc:
        guest_mode = 1;
        memset(renderGuest, 0, sizeof renderGuest);
        c->gpr[2] = 0x8010a000;
        guest_mode = 0;
        return 1;
    case 0x800320e8:
        return 1;
    case 0x8003f8e8: {
        u32 addr = c->gpr[4];
        s32 n = (s32)c->gpr[5];
        s32 i;
        for (i = 0; i < n; i++) wr(NULL, addr + (u32)i, 1, 0);
        return 1;
    }
    case 0x80033728: {
        u32 bundle = c->gpr[4];
        s32 index = (s32)c->gpr[5];
        u32 off = 0;
        rd(NULL, bundle + (u32)index * 2 + 4, 2, &off);
        c->gpr[2] = bundle + (off & 0xffff);
        return 1;
    }
    case 0x80034eac: {
        u32 str = c->gpr[4];
        u32 first = 0;
        if (str) rd(NULL, str, 1, &first);
        c->gpr[2] = (u32)(8 + ((s32)c->gpr[7] & 3) + (first & 7));
        return 1;
    }
    case 0x80044894:
        return 1;
    case 0x800445d0:
        return 1;
    case 0x801e7c50:
        return 1;
    case 0x801c851c:
        return 1;
    default:
        return 0;
    }
}

int main(void) {
    FILE* f = fopen("disc/menu.bin", "rb");
    unsigned cases = 0;
    if (!f) return 2;
    if (fread(ram + 0x1c5000, 1, 0x3b000, f) < 0x1b430) {
        fclose(f);
        return 2;
    }
    fclose(f);

    for (unsigned seed = 0; seed < 48; seed++) {
        for (int cat = 0; cat <= 4; cat++) {
            for (unsigned mode = 0; mode < 2; mode++) {
                for (unsigned group = 0; group < 2; group++) {
                    for (unsigned flag = 0; flag < 2; flag++) {
                        unsigned ch = seed % 8;
                        unsigned gear = (seed / 3) % 8;
                        unsigned slot = seed % 3;
                        unsigned row = seed % 8;
                        unsigned page = seed % 5;
                        u32 args[] = {(u32)cat, row, page,
                                      0xffffff00u | group,
                                      0x12345600u | mode,
                                      0xffffff00u | flag,
                                      0xffffff00u | slot};
                        u8 expBuf[0xA1C];
                        PcPortMipsBus bus = {
                            .read = rd, .write = wr, .bridge = bridge};
                        PcPortMipsCpu c;

                        init_case(seed, ch, gear, slot, 1);
                        PcPortMipsCpuInit(&c, &bus);
                        for (unsigned i = 0; i < 4; i++) c.gpr[4 + i] = args[i];
                        c.gpr[29] = 0x801ff000;
                        c.gpr[31] = 0xfffffffc;
                        wr(NULL, 0x801ff010, 4, args[4]);
                        wr(NULL, 0x801ff014, 4, args[5]);
                        wr(NULL, 0x801ff018, 4, args[6]);
                        if (PcPortMipsRun(&c, 0x801dff5c, 0xfffffffc, 2000000)) {
                            fprintf(stderr,
                                    "FAIL retail seed=%u cat=%d mode=%u "
                                    "group=%u flag=%u %s\n",
                                    seed, cat, mode, group, flag, c.error);
                            return 1;
                        }
                        memcpy(expBuf, listBuf, sizeof listBuf);

                        init_case(seed, ch, gear, slot, 0);
                        func_801DFF5C(args[0], args[1], args[2], args[3],
                                      args[4], args[5], args[6]);
                        if (memcmp(listBuf + 0x880, expBuf + 0x880, 0x180) ||
                            listBuf[0xA18] != expBuf[0xA18]) {
                            unsigned i;
                            fprintf(stderr,
                                    "FAIL desc seed=%u cat=%d mode=%u group=%u "
                                    "flag=%u vis %u vs %u\n",
                                    seed, cat, mode, group, flag, listBuf[0xA18],
                                    expBuf[0xA18]);
                            for (i = 0; i < 0x180; i++) {
                                if (listBuf[0x880 + i] != expBuf[0x880 + i]) {
                                    fprintf(stderr,
                                            "  first diff @+0x%X got %02X exp "
                                            "%02X\n",
                                            0x880 + i, listBuf[0x880 + i],
                                            expBuf[0x880 + i]);
                                    break;
                                }
                            }
                            return 1;
                        }
                        cases++;
                    }
                }
            }
        }
    }
    printf("EQUIP DESC PASS cases=%u\n", cases);
    return 0;
}
