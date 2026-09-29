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
u16 D_801E96A8[32];
u32 D_801E96E8[32];

u8 stateStorage[0x4600] __attribute__((aligned(8)));
__asm__(".globl g_GameState\n.set g_GameState, stateStorage\n");

static SystemMenu menu;
static MenuManager manager;
/* SystemMenu expands to 0x20C8 on this host, overlapping the guest manager
 * at 0x80102000 if mapped by sizeof(menu). Keep the retail image separate. */
static u8 retailMenu[0x1E98];
static u8 resources[0xCC];
static MenuUnk6 nativeResources;
static u8 work[0x400];
static u8 listBuf[0xA1C];
static u8 weaponData[256 * 16];
static u8 accessoryData[256 * 16];
static u8 gearWeaponData[256 * 20];
static u8 gearAccessoryData[256 * 28];
static u8 renderGuest[0x800];
static u8 nameStorage[256][32];
static u8 ram[0x200000];

extern s32 func_801DE5CC(s32, s32, s32, s32, s32);
extern u16 func_801C865C(u16, u8);
extern u32 func_801C8678(u32, u8);

static u8 nameGuest[256 * 32];
static int guest_mode;

void* HeapAlloc(s32 size, s32 flag) {
    (void)flag;
    (void)size;
    memset(renderGuest, 0, sizeof renderGuest);
    if (guest_mode) return (void*)(uintptr_t)0x8010a000;
    return renderGuest;
}
void HeapFree(void* p) { (void)p; }
void* GetWeaponName(s32 index) {
    unsigned i = index & 255;
    snprintf((char*)nameStorage[i], 32, "W%u", i);
    memcpy(nameGuest + i * 32, nameStorage[i], 32);
    if (guest_mode) return (void*)(uintptr_t)(0x8010b000 + i * 32);
    return nameStorage[i];
}
void* GetAccessoryName(s32 index) {
    unsigned i = index & 255;
    snprintf((char*)nameStorage[i], 32, "A%u", i);
    memcpy(nameGuest + i * 32, nameStorage[i], 32);
    if (guest_mode) return (void*)(uintptr_t)(0x8010b000 + i * 32);
    return nameStorage[i];
}
void* func_80033A5C(s32 index) { return GetWeaponName(index); }
void* func_80033A2C(s32 index) { return GetAccessoryName(index); }
s32 SystemRenderStringEntry(void* string, void* workBuf, s32 height, s32 field) {
    (void)string;
    (void)workBuf;
    (void)height;
    return 8 + (field & 3);
}
void func_80033B34(u16* src, u8* dst, s32 count) {
    s32 i;
    for (i = 0; i < count; i++) dst[i] = (u8)src[i];
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
void __wrap_func_801D36E0(MenuString* p, s32 slot, s32 variant, s32 style) {
    (void)p;
    (void)slot;
    (void)variant;
    (void)style;
}
u16 g_SystemPalette1;
u16 g_SystemPalette2;
u8 D_801EA17C[64];
u8 D_801EA18C[64];
u8 D_801EA578[64];
u8 D_801EA584[64];
u8 D_801EA5C4[64];
u16 D_801EA5D0[64];


static void pattern(void* p, size_t n, unsigned seed) {
    u8* b = p;
    size_t i;
    for (i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        b[i] = (u8)(seed >> 24);
    }
}

static void init_tables(void) {
    unsigned i;
    for (i = 0; i < 32; i++) {
        D_801E96A8[i] = (u16)(1u << (i & 15));
        D_801E96E8[i] = 1u << (i & 31);
    }
    for (i = 0; i < 256; i++) {
        u8* w = weaponData + i * 16;
        u8* a = accessoryData + i * 16;
        u8* gw = gearWeaponData + i * 20;
        u8* ga = gearAccessoryData + i * 28;
        memset(w, 0, 16);
        memset(a, 0, 16);
        memset(gw, 0, 20);
        memset(ga, 0, 28);
        *(u16*)w = 0xFFFF;
        w[6] = (u8)(i < 50 ? (i % 5) : (1 + (i % 4)));
        *(u16*)a = 0xFFFF;
        *(u16*)(a + 0xE) = (u16)(i & 7);
        *(u32*)(gw + 4) = 0xFFFFFFFFu;
        gw[0xF] = (u8)(i < 50 ? (i % 5) : (1 + (i % 4)));
        *(u32*)ga = 0xFFFFFFFFu;
        *(u16*)(ga + 8) = (u16)(i & 7);
    }
}

static void store_ptr(void* slot, void* p) {
    u32 truncated = (u32)(uintptr_t)p;
    memcpy(slot, &truncated, 4);
}

static void init_case(unsigned seed, unsigned ch, unsigned gear, unsigned slot,
                      int guest_ptrs) {
    unsigned i;
    pattern(stateStorage, sizeof stateStorage, seed);
    memset(work, 0, sizeof work);
    memset(listBuf, 0, sizeof listBuf);
    memset(D_801EA730, 0, sizeof D_801EA730);
    memset(&menu, 0, sizeof menu);
    memset(&manager, 0, sizeof manager);
    memset(retailMenu, 0, sizeof retailMenu);
    memset(&resources, 0, sizeof resources);
    g_Menu = &menu;
    menu.pManager = &manager;
    memset(&nativeResources, 0, sizeof nativeResources);
    menu.unk330 = &nativeResources;
    ((u8*)&menu)[0x308] = (u8)(seed & 1);
    retailMenu[0x308] = (u8)(seed & 1);
    /* C path reads remapped PSX slots (unk358[2], unk42C[2]).  Guest MIPS
     * still overlays retail byte offsets onto the same SystemMenu object. */
    menu.unk42C[2] = (u32)(uintptr_t)listBuf;
    *(u32*)&menu.unk358[8] = (u32)(uintptr_t)work;
    store_ptr((u8*)&menu + 0x360, work);
    store_ptr((u8*)&menu + 0x434, listBuf);
    manager.currentCharacterIDs[slot] = (u8)ch;
    stateStorage[0x30C + ch * 0xA4] = (u8)gear;
    if (guest_ptrs) {
        store_ptr(resources + 0x0, (void*)(uintptr_t)0x80106000);
        store_ptr(resources + 0x4, (void*)(uintptr_t)0x80107000);
        store_ptr(resources + 0x14, (void*)(uintptr_t)0x80109000);
        store_ptr(resources + 0x18, (void*)(uintptr_t)0x80108000);
    } else {
        /* Native pointers and the remaining PSX-width Gear slots are distinct. */
        nativeResources.pWeaponsData = (void*)weaponData;
        nativeResources.pAccessoriesData = (void*)accessoryData;
        store_ptr(nativeResources.unk8 + 0xC, gearAccessoryData);
        store_ptr(nativeResources.unk8 + 0x10, gearWeaponData);
    }
    for (i = 0; i < 8; i++) {
        work[0x29C + i] = (u8)((seed + i * 3) & 0x3F);
        work[0x2A5 + i] = (u8)((seed + i * 5) & 0x3F);
    }
    for (i = 0; i < 100; i++) {
        stateStorage[0x1D9C + i] = (u8)((seed + i * 7) % 80);
        stateStorage[0x1D38 + i] = (u8)(1 + ((seed + i) % 9));
        stateStorage[0x2120 + i] = (u8)((seed + i * 11) % 80);
        stateStorage[0x20BC + i] = (u8)(1 + ((seed + i * 2) % 9));
    }
    for (i = 0; i < 200; i++) {
        stateStorage[0x1EC8 + i] = (u8)((seed + i * 13) % 90);
        stateStorage[0x1E00 + i] = (u8)(1 + ((seed + i * 3) % 9));
    }
    for (i = 0; i < 150; i++) {
        stateStorage[0x221A + i] = (u8)((seed + i * 17) % 90);
        stateStorage[0x2184 + i] = (u8)(1 + ((seed + i * 4) % 9));
    }
}

static int memory(u32 a, unsigned w, u32* v, int write) {
    u8* p = NULL;
    if (w == 4 && !write) {
        switch (a) {
        case 0x800625a0: *v = 0x80100000; return 0;
        case 0x8010033c: *v = 0x80102000; return 0;
        case 0x80100330: *v = 0x80103000; return 0;
        case 0x80100360: *v = 0x80105000; return 0;
        case 0x80100434: *v = 0x80104000; return 0;
        default: break;
        }
    }
    if (a >= 0x8006d634 && (uint64_t)a + w <= 0x8006d634 + sizeof stateStorage)
        p = stateStorage + a - 0x8006d634;
    else if (a >= 0x80100000 && (uint64_t)a + w <= 0x80100000 + sizeof retailMenu)
        p = retailMenu + (a - 0x80100000);
    else if (a >= 0x80102000 && (uint64_t)a + w <= 0x80102000 + sizeof manager)
        p = (u8*)&manager + (a - 0x80102000);
    else if (a >= 0x80103000 && (uint64_t)a + w <= 0x80103000 + sizeof resources)
        p = (u8*)&resources + a - 0x80103000;
    else if (a >= 0x80104000 && (uint64_t)a + w <= 0x80104000 + sizeof listBuf)
        p = listBuf + a - 0x80104000;
    else if (a >= 0x80105000 && (uint64_t)a + w <= 0x80105000 + sizeof work)
        p = work + a - 0x80105000;
    else if (a >= 0x80106000 && (uint64_t)a + w <= 0x80106000 + sizeof weaponData)
        p = weaponData + a - 0x80106000;
    else if (a >= 0x80107000 && (uint64_t)a + w <= 0x80107000 + sizeof accessoryData)
        p = accessoryData + a - 0x80107000;
    else if (a >= 0x80108000 && (uint64_t)a + w <= 0x80108000 + sizeof gearWeaponData)
        p = gearWeaponData + a - 0x80108000;
    else if (a >= 0x80109000 && (uint64_t)a + w <= 0x80109000 + sizeof gearAccessoryData)
        p = gearAccessoryData + a - 0x80109000;
    else if (a >= 0x8010a000 && (uint64_t)a + w <= 0x8010a000 + sizeof renderGuest)
        p = renderGuest + a - 0x8010a000;
    else if (a >= 0x8010b000 && (uint64_t)a + w <= 0x8010b000 + sizeof nameGuest)
        p = nameGuest + a - 0x8010b000;
    else if (a >= 0x801ea730 && (uint64_t)a + w <= 0x801ea730 + sizeof D_801EA730)
        p = D_801EA730 + a - 0x801ea730;
    else if (a >= 0x801e96a8 && (uint64_t)a + w <= 0x801e96a8 + sizeof D_801E96A8)
        p = (u8*)D_801E96A8 + a - 0x801e96a8;
    else if (a >= 0x801e96e8 && (uint64_t)a + w <= 0x801e96e8 + sizeof D_801E96E8)
        p = (u8*)D_801E96E8 + a - 0x801e96e8;
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
        memset(renderGuest, 0, sizeof renderGuest);
        c->gpr[2] = 0x8010a000;
        return 1;
    case 0x800320e8:
        return 1;
    case 0x80033848:
        guest_mode = 1;
        c->gpr[2] = (u32)(uintptr_t)GetWeaponName((s32)c->gpr[4]);
        guest_mode = 0;
        return 1;
    case 0x800337e8:
        guest_mode = 1;
        c->gpr[2] = (u32)(uintptr_t)GetAccessoryName((s32)c->gpr[4]);
        guest_mode = 0;
        return 1;
    case 0x80033a5c:
        guest_mode = 1;
        c->gpr[2] = (u32)(uintptr_t)func_80033A5C((s32)c->gpr[4]);
        guest_mode = 0;
        return 1;
    case 0x80033a2c:
        guest_mode = 1;
        c->gpr[2] = (u32)(uintptr_t)func_80033A2C((s32)c->gpr[4]);
        guest_mode = 0;
        return 1;
    case 0x80034eac:
        c->gpr[2] = (u32)SystemRenderStringEntry(NULL, NULL, (s32)c->gpr[6],
                                                 (s32)c->gpr[7]);
        return 1;
    case 0x80033b34: {
        s32 count = (s32)c->gpr[6];
        s32 i;
        for (i = 0; i < count; i++) {
            u32 half = 0, byte;
            rd(NULL, c->gpr[4] + (u32)i * 2, 2, &half);
            byte = half & 0xff;
            wr(NULL, c->gpr[5] + (u32)i, 1, byte);
        }
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
    case 0x801d36e0:
        return 1;
    default:
        return 0;
    }
}

int main(void) {
    FILE* f = fopen("disc/menu.bin", "rb");
    unsigned cases = 0;
    if (!f) return 2;
    if (fread(ram + 0x1c5000, 1, 0x3b000, f) < 0x1ae2c) {
        fclose(f);
        return 2;
    }
    fclose(f);
    init_tables();

    for (unsigned seed = 0; seed < 32; seed++) {
        for (int cat = 0; cat <= 4; cat++) {
                    /* Coverage: character/gear weapons and character accessories.
                     * Open gaps: gear accessories (mode&&cat), and group!=0 with
                     * category==4 (type becomes 5 without mask init in retail). */
                    for (unsigned mode = 0; mode < 2; mode++) {
                     if (mode && cat) continue;
                for (unsigned group = 0; group < 2; group++) {
                     if (group && cat == 4) continue;
                    unsigned ch = seed % 8, gear = (seed / 3) % 8, slot = seed % 3;
                    unsigned page = seed % 5;
                    u32 args[] = {0xffffff00u | slot, page, (u32)cat,
                                  0xffffff00u | group, 0x12345600u | mode};
                    u8 expIds[0x190], expCounts[0xC8];
                    s32 expRet;
                    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
                    PcPortMipsCpu c;

                    init_case(seed, ch, gear, slot, 1);
                    /* Catch native-structure ranges shadowing the next guest allocation. */
                    {
                        u32 actualChar, actualGear;
                        if (rd(NULL, 0x80102030 + slot, 1, &actualChar) ||
                            actualChar != ch ||
                            rd(NULL, 0x8006D634 + 0x30C + ch * 0xA4, 1, &actualGear) ||
                            actualGear != gear) {
                            fprintf(stderr, "FAIL guest character/Gear mapping seed=%u\n", seed);
                            return 1;
                        }
                    }
                    PcPortMipsCpuInit(&c, &bus);
                    for (unsigned i = 0; i < 4; i++) c.gpr[4 + i] = args[i];
                    c.gpr[29] = 0x801ff000;
                    c.gpr[31] = 0xfffffffc;
                    wr(NULL, 0x801ff010, 4, args[4]);
                    if (PcPortMipsRun(&c, 0x801de5cc, 0xfffffffc, 5000000)) {
                        fprintf(stderr,
                                "FAIL retail seed=%u cat=%d mode=%u group=%u %s\n",
                                seed, cat, mode, group, c.error);
                        return 1;
                    }
                    expRet = (s32)c.gpr[2];
                    memcpy(expIds, D_801EA730, 0xC8);
                    memcpy(expCounts, D_801EA7F8, 0xC8);

                    init_case(seed, ch, gear, slot, 0);
                    {
                        s32 got = func_801DE5CC(args[0], args[1], args[2], args[3],
                                                args[4]);
                        if (got != expRet || memcmp(D_801EA730, expIds, 0xC8) ||
                            memcmp(D_801EA7F8, expCounts, 0xC8)) {
                            fprintf(stderr,
                                    "FAIL list seed=%u cat=%d mode=%u group=%u "
                                    "ret %d vs %d\n",
                                    seed, cat, mode, group, got, expRet);
                            return 1;
                        }
                    }
                    cases++;
                }
            }
        }
    }
    printf("EQUIP LIST PASS cases=%u\n", cases);
    return 0;
}
