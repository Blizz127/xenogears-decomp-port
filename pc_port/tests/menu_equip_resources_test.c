/* Complete retail resource dispatcher: Equip load/free paths 3/0x13 and 7/0x17. */
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SystemMenu* g_Menu;
static SystemMenu menu;
static MenuUnk6 resources;
static u8 listBuf[0xA1C];
static unsigned char ram[0x200000];
static u32 archive[64];
static u32 events[64][3], expected_events[64][3];
static unsigned count, sequence;

static void event(u32 code, u32 a, u32 b) {
    if (count >= 64) abort();
    events[count][0] = code;
    events[count][1] = a;
    events[count++][2] = b;
}
static u32 norm(void* p) {
    return p == archive ? 0x80110000 : (u32)(uintptr_t)p;
}
static void store_u32(void* slot, u32 v) { memcpy(slot, &v, 4); }
static u32 load_u32(const void* slot) {
    u32 v;
    memcpy(&v, slot, 4);
    return v;
}

int ArchiveSetIndex(int a, int b) {
    event(1, a, b);
    return 0;
}
u32 ArchiveDecodeAlignedSize(int a) {
    event(2, a, 0);
    return sizeof archive;
}
void* HeapAlloc(u32 a, u32 b) {
    event(3, a, b);
    return archive;
}
void ArchiveReadFileToBuffer(int a, void* p, int c, int d) {
    if (c) abort();
    event(4, a, norm(p));
    event(5, d, 0);
}
int ArchiveCdDataSync(int a) {
    event(6, a, 0);
    return 0;
}
void ResolveArchiveEntryPointers(void* p) { event(7, norm(p), 0); }
void* LZSSHeapDecompress(void* p, int a) {
    event(8, norm(p), a);
    return (void*)(uintptr_t)(0x100000 + 0x1000 * ++sequence);
}
u32 HeapFree(void* p) {
    event(9, norm(p), 0);
    return 0;
}

extern void func_801C72BC(s32 mode);

static u32 slot_read(unsigned off) {
    if (off == 0) return (u32)(uintptr_t)resources.pWeaponsData;
    if (off == 4) return (u32)(uintptr_t)resources.pAccessoriesData;
    u32 v;
    memcpy(&v, resources.unk8 + off - 8, 4);
    return v;
}
static void slot_write(unsigned off, u32 v) {
    if (off == 0) resources.pWeaponsData = (void*)(uintptr_t)v;
    else if (off == 4) resources.pAccessoriesData = (void*)(uintptr_t)v;
    else memcpy(resources.unk8 + off - 8, &v, 4);
}

static int memory(u32 a, unsigned w, u32* v, int write) {
    if (w == 4 && a == 0x800625a0) {
        if (write) return -1;
        *v = 0x80100000;
        return 0;
    }
    if (w == 4 && a == 0x80100330) {
        if (write) return -1;
        *v = 0x80101000;
        return 0;
    }
    if (w == 4 && a == 0x80100434) {
        if (write) return -1;
        *v = 0x80102000;
        return 0;
    }
    if (w == 4 && a >= 0x80101000 && a <= 0x80101018) {
        unsigned off = a - 0x80101000;
        if (write) slot_write(off, *v);
        else *v = slot_read(off);
        return 0;
    }
    if (a >= 0x80102000 && (uint64_t)a + w <= 0x80102000 + sizeof listBuf) {
        u8* p = listBuf + a - 0x80102000;
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
    if (a >= 0x80110000 && (uint64_t)a + w <= 0x80110100) {
        if (write || w != 4) return -1;
        *v = archive[(a - 0x80110000) / 4];
        return 0;
    }
    if (a < 0x80000000 || (uint64_t)a + w > 0x80200000) return -1;
    {
        unsigned char* p = ram + (a & 0x1fffff);
        if (write) {
            unsigned i;
            for (i = 0; i < w; i++) p[i] = *v >> (8 * i);
        } else {
            unsigned i;
            *v = 0;
            for (i = 0; i < w; i++) *v |= (u32)p[i] << (8 * i);
        }
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
static int bridge(void* u, PcPortMipsCpu* c, u32 t) {
    (void)u;
    u32 a = c->gpr[4], b = c->gpr[5];
    switch (t) {
    case 0x80028470:
        c->gpr[2] = ArchiveSetIndex(a, b);
        break;
    case 0x800288ec:
        c->gpr[2] = ArchiveDecodeAlignedSize(a);
        break;
    case 0x80031bdc:
        HeapAlloc(a, b);
        c->gpr[2] = 0x80110000;
        break;
    case 0x800295d8:
        ArchiveReadFileToBuffer(a, archive, c->gpr[6], c->gpr[7]);
        break;
    case 0x80028a60:
        c->gpr[2] = ArchiveCdDataSync(a);
        break;
    case 0x8003342c:
        ResolveArchiveEntryPointers(archive);
        break;
    case 0x80032e88:
        c->gpr[2] = (u32)(uintptr_t)LZSSHeapDecompress((void*)(uintptr_t)a, b);
        break;
    case 0x800320e8:
        HeapFree(a == 0x80110000 ? archive : (void*)(uintptr_t)a);
        break;
    default:
        return 0;
    }
    return 1;
}

static void init(unsigned seed, int guest_ptrs) {
    memset(&menu, 0, sizeof menu);
    memset(&resources, seed, sizeof resources);
    memset(listBuf, seed ^ 0x5A, sizeof listBuf);
    g_Menu = &menu;
    menu.unk330 = &resources;
    resources.pWeaponsData = (void*)(uintptr_t)(0x230000 + seed * 4);
    resources.pAccessoriesData = (void*)(uintptr_t)(0x240000 + seed * 4);
    /* Seed description-bank slots for free path. */
    store_u32(listBuf + 0xA00, 0x310000 + seed * 4);
    store_u32(listBuf + 0xA04, 0x320000 + seed * 4);
    store_u32(listBuf + 0xA08, 0x330000 + seed * 4);
    store_u32(listBuf + 0xA0C, 0x340000 + seed * 4);
    if (guest_ptrs) {
        store_u32((u8*)&menu + 0x434, 0x80102000);
        menu.unk42C[2] = 0x80102000;
    } else {
        store_u32((u8*)&menu + 0x434, (u32)(uintptr_t)listBuf);
        menu.unk42C[2] = (u32)(uintptr_t)listBuf;
    }
    for (unsigned i = 0; i < 64; i++)
        archive[i] = 0x80120000 + i * 0x100 + seed * 4;
    count = sequence = 0;
    memset(events, 0, sizeof events);
    memset(ram + 0x1ff000, seed, 128);
}

static int compare_case(u32 mode, unsigned seed) {
    MenuUnk6 expected_res;
    u8 expected_list[0xA1C];
    unsigned expected_count;
    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
    PcPortMipsCpu c;

    init(seed, 1);
    PcPortMipsCpuInit(&c, &bus);
    c.gpr[4] = mode;
    c.gpr[29] = 0x801ff000;
    c.gpr[31] = 0xfffffffc;
    if (PcPortMipsRun(&c, 0x801c72bc, 0xfffffffc, 4000)) {
        fprintf(stderr, "FAIL retail mode=%08x %s\n", mode, c.error);
        return 1;
    }
    expected_res = resources;
    expected_count = count;
    memcpy(expected_events, events, sizeof events);
    memcpy(expected_list, listBuf, sizeof listBuf);

    init(seed, 0);
    func_801C72BC((s32)mode);
    if (count != expected_count ||
        memcmp(events, expected_events, sizeof events) ||
        memcmp(&resources, &expected_res, sizeof resources) ||
        memcmp(listBuf + 0xA00, expected_list + 0xA00, 0x10)) {
        fprintf(stderr,
                "FAIL equip resource mode=%08x seed=%u events=%u/%u "
                "desc %08x/%08x\n",
                mode, seed, count, expected_count, load_u32(listBuf + 0xA00),
                load_u32(expected_list + 0xA00));
        return 1;
    }
    return 0;
}

int main(void) {
    FILE* f = fopen("disc/menu.bin", "rb");
    unsigned cases = 0;
    u32 upper[] = {0, 0x100, 0x80000000, 0xffffff00};
    u32 modes[] = {3, 0x13, 7, 0x17};
    unsigned mi, seed, hi;

    if (!f) return 2;
    fseek(f, 0, SEEK_END);
    {
        long n = ftell(f);
        rewind(f);
        if (n < 0 || n > 0x3b000 ||
            fread(ram + 0x1c5000, 1, n, f) != (size_t)n) {
            fclose(f);
            return 2;
        }
    }
    fclose(f);

    for (seed = 0; seed < 64; seed++) {
        for (mi = 0; mi < 4; mi++) {
            for (hi = 0; hi < 4; hi++) {
                if (compare_case(upper[hi] | modes[mi], seed)) return 1;
                cases++;
            }
        }
    }
    printf("EQUIP RESOURCES PASS cases=%u\n", cases);
    return 0;
}
