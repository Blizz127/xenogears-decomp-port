/* Bounded loader regression: raw retail assets -> node-list selection -> topology.
 *
 * The runner extracts the production 742C prefix verbatim through its node call.
 * Only that call's name is redirected to CaptureNodes; real native 801DC2D0 and
 * its pointer-width adapter still execute. No clip/loader suffix is compiled.
 * The independent oracle executes archive 6B9 at 801E758C after relocation,
 * through the 801E77D4 call, then executes retail 801DC2D0 itself.
 *
 * Controlled boundaries: already-relocated oracle archives, model count reader
 * (internal geometry relocation omitted), heap, texture upload, packet creation
 * and packet setup. This proves list identity and every node's model/index/parent
 * for flags 0/0x40; it does not prove those boundaries or full-loader parity.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#error "Run through run_field_gear_model_list_retail_test.sh"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"

enum { MAX_NODES = 128, HALT_PC = 0xfffffffcu };
static u8 ram[0x200000];
static struct {
    u32 tex[0x10000 / 4], model[0x2000 / 4];
    u32 heap[0x20000 / 4], table[2];
    u8 packets[MAX_NODES][2][8192];
} fixture;
static u8 raw_tex[44984], raw_model[1256];
static unsigned heap_used, packet_count;
static int capture_oracle, mutate_after_latch;
static u8 *native_nodes;
static struct Selection {
    u32 list, groups;
    s32 mode, setup;
    s16 xy[4];
    unsigned calls;
} selected, wanted;
struct Topology {
    unsigned count;
    struct { s32 parent; u16 model, index; } node[MAX_NODES];
};

static u8 *address(u32 a, unsigned width)
{
    if (a >= 0x80000000u && (uint64_t)a + width <= 0x80200000u)
        return ram + (a & 0x1fffffu);
    uintptr_t first = (uintptr_t)&fixture;
    if (a >= first && (uint64_t)a + width <= first + sizeof(fixture))
        return (u8 *)(uintptr_t)a;
    return NULL;
}

static int rd(void *unused, u32 a, unsigned width, u32 *value)
{
    (void)unused;
    u8 *p = address(a, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i) *value |= (u32)p[i] << (i * 8);
    return 0;
}

static int wr(void *unused, u32 a, unsigned width, u32 value)
{
    (void)unused;
    u8 *p = address(a, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; ++i) p[i] = (u8)(value >> (i * 8));
    return 0;
}

static u32 read_word(u32 a)
{
    u32 value;
    assert(!rd(NULL, a, 4, &value));
    return value;
}

void HeapChangeCurrentUser(u32 tag, char **names)
{
    assert(tag == 4 && names == NULL);
}

void *HeapAlloc(u32 bytes, u32 flags)
{
    assert(flags <= 1 && bytes <= sizeof(fixture.heap) - heap_used);
    void *p = (u8 *)fixture.heap + heap_used;
    heap_used += (bytes + 3u) & ~3u;
    /* Retail keeps the list in s0 across this copy allocation. A helper-side
     * change to the archive header must not cause a later pointer reread. */
    if (flags == 1 && mutate_after_latch) fixture.tex[3] = fixture.model[2];
    return p;
}

u_int HeapFree(void *p) { (void)p; return 0; }

unsigned int ResolveArchiveEntryPointers(u32 *archive)
{
    assert(address((u32)(uintptr_t)archive, 4));
    u32 count = archive[0], base = (u32)(uintptr_t)archive;
    assert(count < 0x400 && address(base, (count + 1) * 4));
    for (u32 i = 1; i <= count; ++i) archive[i] += base;
    return count;
}

int func_8002C3E8(u8 *model)
{
    /* Group count is a retail input; packet geometry is outside this test. */
    u32 count = read_word((u32)(uintptr_t)model);
    assert(count > 0 && count < MAX_NODES);
    return (int)count;
}

s32 func_8002DDE4(void *image, s32 mode, s32 x, s32 y,
                   s32 clut_mode, s32 z, s32 w)
{
    (void)image; (void)mode; (void)x; (void)y;
    (void)clut_mode; (void)z; (void)w;
    return 0;
}

void func_8002CB54(u8 *model, u32 *first, u32 *second)
{
    assert(packet_count < MAX_NODES);
    assert(read_word((u32)(uintptr_t)model + 0x34) <= sizeof(fixture.packets[0][0]));
    *first = (u32)(uintptr_t)fixture.packets[packet_count][0];
    *second = (u32)(uintptr_t)fixture.packets[packet_count++][1];
}

void func_8002CC10(u16 x, u16 y) { (void)x; (void)y; }
void func_8002CC74(u16 x, u16 y) { (void)x; (void)y; }
void func_8002C8CC(u8 *model, void *packet, s32 mode)
{ (void)model; (void)packet; (void)mode; }

static u8 *CaptureNodes(OvlyPtrTab *tab, u16 *list, s32 mode, s32 setup,
                        s16 x, s16 y, s16 z, s16 w)
{
    selected.list = (u32)(uintptr_t)list;
    selected.groups = (u32)tab->count;
    selected.mode = mode;
    selected.setup = setup;
    selected.xy[0] = x; selected.xy[1] = y;
    selected.xy[2] = z; selected.xy[3] = w;
    ++selected.calls;
    /* Bad-source mutants must fail with a selection verdict, never by an
     * uncontrolled pointer dereference or an unterminated list scan. */
    for (unsigned i = 0; i < MAX_NODES - 1; ++i) {
        u32 value;
        if (rd(NULL, selected.list + i * 4, 2, &value)) return NULL;
        if (value != 0xffff && value >= selected.groups) {
            native_nodes = OvlyBuildNodes(tab, list, mode, setup, x, y, z, w);
            return native_nodes;
        }
    }
    return NULL;
}

static int bridge(void *unused, PcPortMipsCpu *cpu, u32 target)
{
    (void)unused;
    u32 *a = cpu->gpr + 4;
    switch (target) {
    case 0x801dc2d0:
        if (!capture_oracle) return 0;
        selected.list = a[1];
        selected.groups = read_word(a[0] + 4);
        selected.mode = (s32)a[2]; selected.setup = (s32)a[3];
        for (unsigned i = 0; i < 4; ++i)
            selected.xy[i] = (s16)read_word(cpu->gpr[29] + 0x10 + i * 4);
        fixture.table[0] = read_word(a[0]);
        fixture.table[1] = selected.groups;
        ++selected.calls;
        cpu->gpr[31] = HALT_PC;
        return 1;
    case 0x80032498: HeapChangeCurrentUser(a[0], (char **)(uintptr_t)a[1]); break;
    case 0x80031bdc: cpu->gpr[2] = (u32)(uintptr_t)HeapAlloc(a[0], a[1]); break;
    case 0x800320e8: cpu->gpr[2] = HeapFree((void *)(uintptr_t)a[0]); break;
    case 0x8002c3e8: cpu->gpr[2] = func_8002C3E8((u8 *)(uintptr_t)a[0]); break;
    case 0x8002dde4: cpu->gpr[2] = 0; break; /* texture upload boundary */
    case 0x8002cb54:
        func_8002CB54((u8 *)(uintptr_t)a[0], (u32 *)(uintptr_t)a[1], (u32 *)(uintptr_t)a[2]);
        break;
    case 0x8002cc10: func_8002CC10(a[0], a[1]); break;
    case 0x8002cc74: func_8002CC74(a[0], a[1]); break;
    case 0x8002c8cc: func_8002C8CC((u8 *)(uintptr_t)a[0], (void *)(uintptr_t)a[1], a[2]); break;
    case 0x8003f968:
        assert(address(a[0], a[2]) && address(a[1], a[2]));
        memcpy(address(a[0], a[2]), address(a[1], a[2]), a[2]);
        break;
    default: return target >= 0x801dc000u && target < 0x801e8590u ? 0 : -1;
    }
    return 1;
}

static void run_cpu(PcPortMipsCpu *cpu, u32 entry)
{
    int result = PcPortMipsRun(cpu, entry, HALT_PC, 100000);
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "GEAR MODEL LIST FAIL oracle entry=%08x pc=%08x: %s\n", entry, cpu->pc, cpu->error);
        assert(result == PC_PORT_MIPS_HALTED);
    }
}

static struct Topology topology(u8 *root)
{
    struct Topology result = {0};
    if (!root) return result;
    result.count = *(u16 *)(root + 10);
    assert(result.count < MAX_NODES);
    for (unsigned i = 0; i < result.count; ++i) {
        u8 *node = root + i * 0x7c;
        u32 parent = *(u32 *)node;
        if (parent) {
            assert(parent >= (u32)(uintptr_t)root);
            u32 offset = parent - (u32)(uintptr_t)root;
            assert(offset % 0x7c == 0 && offset / 0x7c < result.count);
            result.node[i].parent = (s32)(offset / 0x7c);
        } else result.node[i].parent = -1;
        result.node[i].model = *(u16 *)(node + 8);
        result.node[i].index = *(u16 *)(node + 10);
    }
    return result;
}

static void reset_fixture(unsigned records, unsigned variant, u16 object_flags)
{
    memset(&fixture, 0, sizeof(fixture));
    memcpy(fixture.tex, raw_tex, sizeof(raw_tex));
    memcpy(fixture.model, raw_model, sizeof(raw_model));
    heap_used = packet_count = 0;
    native_nodes = NULL;
    memset(&selected, 0, sizeof(selected));
    memset(s_ptrTab, 0, sizeof(s_ptrTab));
    memset(D_801E8670, 0, sizeof(D_801E8670));
    memset(ram + 0x1e85f4, 0, 0x100);
    if (variant) {
        /* Relocate only the hierarchy's offset and vary all its records.
         * Mesh +8 and the other archive's +8 retain distinct valid contents. */
        fixture.tex[3] = 0xb100 + variant * 0x100;
        u16 *list = (u16 *)((u8 *)fixture.tex + fixture.tex[3]);
        for (unsigned i = 0; i < records; ++i) {
            list[i * 2] = i % 3 == 0 ? 0xffff : (u16)((i + variant) % 24);
            list[i * 2 + 1] = i == 0 || i % 5 == 0 ? 0xffff : (u16)(i - 1);
        }
        list[records * 2] = 24;
        list[records * 2 + 1] = 0;
    }
    u8 *header = (u8 *)fixture.tex + fixture.tex[4];
    u8 *settings = header + *(u32 *)(header + 4);
    *(u16 *)(settings + 0xc) = object_flags;
}

static void relocate_oracle_archives(void)
{
    /* Entry 758C assumes the original routine has already completed these
     * relocations. The earlier flags/animation gates are explicitly excluded. */
    ResolveArchiveEntryPointers(fixture.tex);
    ResolveArchiveEntryPointers((u32 *)(uintptr_t)fixture.tex[4]);
    ResolveArchiveEntryPointers(fixture.model);
    ResolveArchiveEntryPointers((u32 *)(uintptr_t)fixture.model[2]);
    u32 *sub = (u32 *)(uintptr_t)fixture.model[1];
    ResolveArchiveEntryPointers(sub);
    ResolveArchiveEntryPointers((u32 *)(uintptr_t)sub[1]);
}

static int compare(unsigned records, unsigned variant, s32 flags, u16 object_flags,
                   int mutate_header)
{
    const s16 xy[4] = {-32768, 32767, -1, 513};
    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
    PcPortMipsCpu cpu;
    mutate_after_latch = mutate_header;
    reset_fixture(records, variant, object_flags);
    relocate_oracle_archives();
    u32 latched_list = fixture.tex[3];
    u8 *object = HeapAlloc(0x134, 0);
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[19] = (u32)(uintptr_t)object;
    cpu.gpr[20] = (u32)(uintptr_t)fixture.tex;
    cpu.gpr[29] = 0x801ff000;
    cpu.gpr[31] = HALT_PC;
    wr(NULL, 0x801ff050, 4, 0);
    wr(NULL, 0x801ff058, 2, flags);
    for (unsigned i = 0; i < 4; ++i) wr(NULL, 0x801ff068 + i * 8, 2, (u16)xy[i]);
    capture_oracle = 1;
    run_cpu(&cpu, 0x801e758c);
    assert(selected.calls == 1 && selected.groups == 24);
    wanted = selected;
    assert(wanted.list == latched_list); /* corroborates 801E75A8's lw s0,12(s4) */
    capture_oracle = 0;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (u32)(uintptr_t)fixture.table;
    cpu.gpr[5] = wanted.list;
    cpu.gpr[6] = wanted.mode; cpu.gpr[7] = wanted.setup;
    cpu.gpr[29] = 0x801ff000; cpu.gpr[31] = HALT_PC;
    for (unsigned i = 0; i < 4; ++i) wr(NULL, 0x801ff010 + i * 4, 4, (u32)(s32)wanted.xy[i]);
    run_cpu(&cpu, 0x801dc2d0);
    struct Topology expected = topology((u8 *)(uintptr_t)cpu.gpr[2]);
    unsigned expected_count = records ? records + 1 : 0;
    assert(expected.count == expected_count);

    reset_fixture(records, variant, object_flags);
    func_801E742C(0, flags, fixture.model, fixture.tex, xy[0], xy[1], xy[2], xy[3], NULL);
    struct Topology actual = topology(native_nodes);
    int failed = selected.calls != 1 || selected.list != wanted.list ||
        selected.groups != wanted.groups || selected.mode != wanted.mode ||
        selected.setup != wanted.setup || memcmp(&actual, &expected, sizeof(actual));
    if (selected.setup && memcmp(selected.xy, wanted.xy, sizeof(selected.xy))) failed = 1;
    if (failed) {
        fprintf(stderr, "GEAR MODEL LIST FAIL variant=%u flags=%02x object_flags=%x mutate_header=%d "
                "list=%08x expected=%08x groups=%u/%u nodes=%u/%u calls=%u\n",
                variant, flags, object_flags, mutate_header, selected.list, wanted.list,
                selected.groups, wanted.groups, actual.count, expected.count, selected.calls);
        return 1;
    }
    return 0;
}

static void disc_read(FILE *disc, u8 *out, unsigned first_sector, unsigned size)
{
    for (unsigned offset = 0; offset < size; offset += 2048) {
        unsigned bytes = size - offset < 2048 ? size - offset : 2048;
        assert(!fseek(disc, (first_sector + offset / 2048) * 2352L + 24, SEEK_SET));
        assert(fread(out + offset, 1, bytes, disc) == bytes);
    }
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) < UINT32_MAX);
    FILE *disc = fopen("disc/disc1.bin", "rb");
    assert(disc);
    disc_read(disc, ram + 0x1dc000, 231361, 25 * 2048);
    disc_read(disc, raw_model, 232272, sizeof(raw_model));
    disc_read(disc, raw_tex, 232273, sizeof(raw_tex));
    assert(!fclose(disc));
    unsigned cases = 0;
    const unsigned sizes[] = {46, 0, 1, 3, 7, 46};
    for (unsigned variant = 0; variant < sizeof(sizes) / sizeof(sizes[0]); ++variant)
        for (s32 flags = 0; flags <= 0x40; flags += 0x40)
            for (u16 object_flags = 0; object_flags <= 4; object_flags += 4)
                for (int mutate_header = 0; mutate_header <= 1; ++mutate_header) {
                    if (compare(sizes[variant], variant, flags, object_flags, mutate_header)) return 1;
                    ++cases;
                }
    printf("GEAR MODEL LIST PASS %u cases: retail asset (24 groups, 46 children, 47 nodes), "
           "relocated list offsets, empty/1/3/7/46 records, flags 0/0x40, object flag 4, "
           "post-latch header mutation; "
           "pointer identity and complete node topology at a controlled call boundary\n", cases);
    return 0;
}
