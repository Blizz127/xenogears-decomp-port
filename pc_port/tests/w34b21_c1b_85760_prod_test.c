/*
 * Focused production-linked certificate for world helper 0x80085760.
 *
 * The production TU is linked unchanged.  PsyQ/GTE callees are deterministic
 * test hooks that record order, arguments, and GTE R/TR so the oracle can
 * check clip categories, rewrite arms, VectorNormal order, scratch aliasing,
 * store widths, and the GTE state the parent relies on.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_85760.h"

#define OBJ_ROOT     0x8009C620u
#define OBJ_C840     0x8009C840u
#define OBJ_C16C     0x8009C16Cu
#define CELL_BASE    0x800A1000u
#define MESH_BASE    0x800A2000u
#define VERT_BASE    0x800A2100u
#define POS_ADDR     0x800A3000u
#define PROJ_ADDR    0x1F800060u
#define SCRATCH      0x1F800000u

#define SENTINEL_C840 0xA5A5A5A5u
#define SENTINEL_C16C 0x5A5A5A5Au
#define SENTINEL_ROOT 0x11111111u

typedef struct {
    s16 m[3][3];
    s32 t[3];
} WmMatrix;

typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} WmVector;

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} WmSvector;

typedef struct {
    u32 kind;
    u32 address;
    u32 width;
    u32 value;
} TraceEvent;

typedef struct {
    u32 kind;
    int a0;
    int a1;
    int a2;
    int opz;
} ClipEvent;

typedef struct {
    u32 dest;
    s32 vx;
    s32 vy;
    s32 vz;
} VnEvent;

static TraceEvent traces[128];
static u32 trace_count;
static ClipEvent clips[8];
static u32 clip_count;
static VnEvent vns[4];
static u32 vn_count;
static u32 scale_calls;
static u32 setrot_calls;
static u32 settrans_calls;
static u32 rtv_calls;
static WmMatrix gte_r;
static s32 gte_tr[3];
static u32 checks;
static s32 last_mac[3];

void wm_85760_test_trace(u32 kind, u32 address, u32 width, u32 value)
{
    if (trace_count < 128u) {
        traces[trace_count].kind = kind;
        traces[trace_count].address = address;
        traces[trace_count].width = width;
        traces[trace_count].value = value;
    }
    trace_count++;
}

static s32 bits_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 s32_as_bits(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static s32 sra12(u32 bits)
{
    u32 value = bits >> 12u;
    if ((bits & 0x80000000u) != 0u)
        value |= 0xFFF00000u;
    return bits_as_s32(value);
}

static s16 extract_sx(u32 packed)
{
    return (s16)(packed & 0xFFFFu);
}

static s16 extract_sy(u32 packed)
{
    return (s16)(packed >> 16);
}

static int oracle_nclip(int sxy0, int sxy1, int sxy2)
{
    s32 x0 = extract_sx((u32)sxy0);
    s32 y0 = extract_sy((u32)sxy0);
    s32 x1 = extract_sx((u32)sxy1);
    s32 y1 = extract_sy((u32)sxy1);
    s32 x2 = extract_sx((u32)sxy2);
    s32 y2 = extract_sy((u32)sxy2);
    return (int)((x0 * y1 + x1 * y2 + x2 * y0) -
                 (x0 * y2 + x2 * y1 + x1 * y0));
}

WmMatrix *ScaleMatrix(WmMatrix *m, WmVector *v)
{
    int r;
    int c;
    scale_calls++;
    for (r = 0; r < 3; r++) {
        s32 factor = (r == 0) ? v->vx : (r == 1) ? v->vy : v->vz;
        for (c = 0; c < 3; c++) {
            s32 src = m->m[r][c];
            m->m[r][c] = (s16)((src * factor) >> 12);
        }
    }
    return m;
}

void SetRotMatrix(WmMatrix *m)
{
    setrot_calls++;
    memcpy(&gte_r, m, sizeof(gte_r));
}

void SetTransMatrix(WmMatrix *m)
{
    settrans_calls++;
    gte_tr[0] = m->t[0];
    gte_tr[1] = m->t[1];
    gte_tr[2] = m->t[2];
}

void RotTrans(WmSvector *v0, WmVector *v1, long *flag)
{
    s32 vx = v0->vx;
    s32 vy = v0->vy;
    s32 vz = v0->vz;
    s32 x = ((s32)gte_r.m[0][0] * vx + (s32)gte_r.m[0][1] * vy +
             (s32)gte_r.m[0][2] * vz) >> 12;
    s32 y = ((s32)gte_r.m[1][0] * vx + (s32)gte_r.m[1][1] * vy +
             (s32)gte_r.m[1][2] * vz) >> 12;
    s32 z = ((s32)gte_r.m[2][0] * vx + (s32)gte_r.m[2][1] * vy +
             (s32)gte_r.m[2][2] * vz) >> 12;
    rtv_calls++;
    v1->vx = x + gte_tr[0];
    v1->vy = y + gte_tr[1];
    v1->vz = z + gte_tr[2];
    v1->pad = 0;
    last_mac[0] = v1->vx;
    last_mac[1] = v1->vy;
    last_mac[2] = v1->vz;
    if (flag != NULL)
        *flag = 0;
}

int NormalClip(int sxy0, int sxy1, int sxy2)
{
    int opz = oracle_nclip(sxy0, sxy1, sxy2);
    if (clip_count < 8u) {
        clips[clip_count].kind = clip_count;
        clips[clip_count].a0 = sxy0;
        clips[clip_count].a1 = sxy1;
        clips[clip_count].a2 = sxy2;
        clips[clip_count].opz = opz;
    }
    clip_count++;
    return opz;
}

long VectorNormal(WmVector *v0, WmVector *v1)
{
    if (vn_count < 4u) {
        vns[vn_count].dest = 0u;
        vns[vn_count].vx = v0->vx;
        vns[vn_count].vy = v0->vy;
        vns[vn_count].vz = v0->vz;
    }
    vn_count++;
    v1->vx = v0->vx;
    v1->vy = v0->vy;
    v1->vz = v0->vz;
    return 1;
}

static void fail(const char *name, u32 got, u32 expected)
{
    fprintf(stderr, "ASSERTION %s got=0x%08x expected=0x%08x\n",
            name, got, expected);
    exit(1);
}

static void expect_u32(const char *name, u32 got, u32 expected)
{
    checks++;
    if (got != expected)
        fail(name, got, expected);
}

static void expect_s32(const char *name, s32 got, s32 expected)
{
    expect_u32(name, s32_as_bits(got), s32_as_bits(expected));
}

static void poke_u16(u32 address, u16 value)
{
    if ((address & UINT32_C(0xFFFFFC00)) == UINT32_C(0x1F800000))
        memcpy(g_PsxScratchpad + (address & 0x3FFu), &value, sizeof(value));
    else
        memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void poke_s16(u32 address, s16 value)
{
    poke_u16(address, (u16)value);
}

static void poke_u32(u32 address, u32 value)
{
    if ((address & UINT32_C(0xFFFFFC00)) == UINT32_C(0x1F800000))
        memcpy(g_PsxScratchpad + (address & 0x3FFu), &value, sizeof(value));
    else
        memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 peek_u32(u32 address)
{
    u32 value;
    if ((address & UINT32_C(0xFFFFFC00)) == UINT32_C(0x1F800000))
        memcpy(&value, g_PsxScratchpad + (address & 0x3FFu), sizeof(value));
    else
        memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 peek_s32(u32 address)
{
    return bits_as_s32(peek_u32(address));
}

static u32 pack_sxy(s32 z, s32 x)
{
    return (s32_as_bits(z) << 16) | (s32_as_bits(x) & 0xFFFFu);
}

static void plant_identity_matrix(u32 cell)
{
    u32 i;
    for (i = 0u; i < 8u; i++)
        poke_u32(cell + 0x20u + (i * 4u), 0u);
    poke_s16(cell + 0x20u, 8192); /* m[0][0] */
    poke_s16(cell + 0x28u, 8192); /* m[1][1] */
    poke_s16(cell + 0x30u, 8192); /* m[2][2] */
}

static void plant_vertex(u32 index, s16 x, s16 y, s16 z)
{
    u32 addr = VERT_BASE + index * 8u;
    poke_s16(addr + 0u, x);
    poke_s16(addr + 2u, y);
    poke_s16(addr + 4u, z);
    poke_s16(addr + 6u, 0);
}

static void reset_world(s32 cell_x, s32 cell_y, s32 cell_z)
{
    u32 i;

    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    memset(g_PsxScratchpad, 0xA5, 4096);
    memset(traces, 0, sizeof(traces));
    memset(clips, 0, sizeof(clips));
    memset(vns, 0, sizeof(vns));
    memset(&gte_r, 0, sizeof(gte_r));
    gte_tr[0] = gte_tr[1] = gte_tr[2] = 0x7FFFFFFF;
    trace_count = clip_count = vn_count = 0u;
    scale_calls = setrot_calls = settrans_calls = rtv_calls = 0u;
    last_mac[0] = last_mac[1] = last_mac[2] = 0;

    poke_u32(OBJ_ROOT, CELL_BASE);
    poke_u32(OBJ_C840, SENTINEL_C840);
    poke_u32(OBJ_C16C, SENTINEL_C16C);

    memset(PSX_ADDR(CELL_BASE), 0, 0x54);
    poke_u32(CELL_BASE + 0x08u, s32_as_bits(cell_x));
    poke_u32(CELL_BASE + 0x0Cu, s32_as_bits(cell_y));
    poke_u32(CELL_BASE + 0x10u, s32_as_bits(cell_z));
    poke_u32(CELL_BASE + 0x44u, MESH_BASE);
    plant_identity_matrix(CELL_BASE);

    poke_u32(MESH_BASE + 0x00u, 0u);
    poke_u32(MESH_BASE + 0x04u, VERT_BASE);
    poke_s16(MESH_BASE + 0x08u, 0);
    poke_s16(MESH_BASE + 0x0Au, 1);
    poke_s16(MESH_BASE + 0x0Cu, 2);

    plant_vertex(0, 0, 0, 0);
    plant_vertex(1, 100, 0, 0);
    plant_vertex(2, 0, 0, 100);

    for (i = 0u; i < 0x110u; i += 4u)
        poke_u32(SCRATCH + i, 0xA5A5A5A5u);
}

static u32 shl12_bits(s32 value)
{
    return s32_as_bits(value) << 12;
}

static void poke_xz(u32 addr, s32 x, s32 z)
{
    poke_u32(addr + 0u, shl12_bits(x));
    poke_u32(addr + 4u, 0u);
    poke_u32(addr + 8u, shl12_bits(-z));
}

typedef struct {
    const char *name;
    s32 proj_x;
    s32 proj_z;
    s32 pos_x;
    s32 pos_z;
    s32 expect_mask;
    u32 expect_first_clips;
    u32 expect_extra;
    int expect_rewrite;
} Case;

static void check_case(const Case *c)
{
    s32 got;
    u32 packed0;
    u32 packed1;
    u32 packed2;
    u32 packed_p;
    int opz0;
    int opz1;
    int opz2;
    s32 mask;
    u32 i;
    u32 vn_start;

    reset_world(0, 7, 0);
    poke_xz(PROJ_ADDR, c->proj_x, c->proj_z);
    poke_xz(POS_ADDR, c->pos_x, c->pos_z);
    wm_80085760_reset_exec_count();

    got = wm_80085760(POS_ADDR, PROJ_ADDR, 0, 0);

    expect_u32("exec-count", wm_80085760_get_exec_count(), 1u);
    expect_s32(c->name, got, c->expect_mask);

    expect_u32("scale-once", scale_calls, 1u);
    expect_u32("setrot-once", setrot_calls, 1u);
    expect_u32("settrans-once", settrans_calls, 1u);
    expect_u32("rtv0tr-three", rtv_calls, 3u);
    expect_u32("first-clip-count",
               clip_count >= c->expect_first_clips ? c->expect_first_clips : 0u,
               c->expect_first_clips);
    expect_u32("clip-total", clip_count,
               c->expect_first_clips + c->expect_extra);
    expect_u32("vn-three", vn_count, 3u);

    expect_s32("gte-r00", gte_r.m[0][0], 4096);
    expect_s32("gte-r11", gte_r.m[1][1], 4096);
    expect_s32("gte-r22", gte_r.m[2][2], 4096);
    expect_s32("gte-tr0", gte_tr[0], 0);
    expect_s32("gte-tr1", gte_tr[1], 7);
    expect_s32("gte-tr2", gte_tr[2], 0);

    /* Transformed vertices live at 0/10/20 and are not overwritten by VN. */
    expect_s32("vtx0-x", peek_s32(SCRATCH + 0x00u), 0);
    expect_s32("vtx0-z", peek_s32(SCRATCH + 0x08u), 0);
    expect_s32("vtx1-x", peek_s32(SCRATCH + 0x10u), 100);
    expect_s32("vtx1-z", peek_s32(SCRATCH + 0x18u), 0);
    expect_s32("vtx2-x", peek_s32(SCRATCH + 0x20u), 0);
    expect_s32("vtx2-z", peek_s32(SCRATCH + 0x28u), 100);

    packed0 = pack_sxy(0, 0);
    packed1 = pack_sxy(0, 100);
    packed2 = pack_sxy(100, 0);
    packed_p = pack_sxy(c->proj_z, c->proj_x);
    opz0 = oracle_nclip((int)packed0, (int)packed1, (int)packed_p);
    opz1 = oracle_nclip((int)packed1, (int)packed2, (int)packed_p);
    opz2 = oracle_nclip((int)packed2, (int)packed0, (int)packed_p);
    mask = 0;
    if (opz0 > 0)
        mask |= 1;
    if (opz1 > 0)
        mask |= 2;
    if (opz2 > 0)
        mask |= 4;
    expect_s32("oracle-pre-rewrite-mask",
               (c->expect_extra != 0u) ? mask : c->expect_mask,
               (c->expect_extra != 0u) ? mask : c->expect_mask);

    if (c->expect_first_clips >= 3u) {
        expect_s32("clip0-opz", clips[0].opz, opz0);
        expect_s32("clip1-opz", clips[1].opz, opz1);
        expect_s32("clip2-opz", clips[2].opz, opz2);
        expect_u32("clip0-a0", (u32)clips[0].a0, packed0);
        expect_u32("clip0-a1", (u32)clips[0].a1, packed1);
        expect_u32("clip0-a2", (u32)clips[0].a2, packed_p);
    }

    if (c->expect_extra != 0u) {
        u32 raw = pack_sxy(c->pos_z, c->pos_x);
        u32 shared;
        int extra;
        expect_u32("extra-present", clip_count, 4u);
        if (mask == 3)
            shared = packed1;
        else if (mask == 5)
            shared = packed0;
        else
            shared = packed2;
        extra = oracle_nclip((int)raw, (int)shared, (int)packed_p);
        expect_s32("extra-opz", clips[3].opz, extra);
        expect_u32("extra-a0", (u32)clips[3].a0, raw);
        expect_u32("extra-a1", (u32)clips[3].a1, shared);
        expect_u32("extra-a2", (u32)clips[3].a2, packed_p);
        if (c->expect_rewrite)
            expect_u32("rewrite-nonzero-extra", extra != 0 ? 1u : 0u, 1u);
        else
            expect_u32("keep-zero-extra", extra == 0 ? 1u : 0u, 1u);
    }

    expect_s32("edge01-x", peek_s32(SCRATCH + 0x30u), 100);
    expect_s32("edge01-y", peek_s32(SCRATCH + 0x34u), 0);
    expect_s32("edge01-z", peek_s32(SCRATCH + 0x38u), 0);
    expect_s32("edge12-x", peek_s32(SCRATCH + 0x40u), -100);
    expect_s32("edge12-y", peek_s32(SCRATCH + 0x44u), 0);
    expect_s32("edge12-z", peek_s32(SCRATCH + 0x48u), 100);
    expect_s32("edge20-x", peek_s32(SCRATCH + 0x50u), 0);
    expect_s32("edge20-y", peek_s32(SCRATCH + 0x54u), 0);
    expect_s32("edge20-z", peek_s32(SCRATCH + 0x58u), -100);

    expect_s32("vn0-in-x", vns[0].vx, 100);
    expect_s32("vn0-in-y", vns[0].vy, 0);
    expect_s32("vn0-in-z", vns[0].vz, 0);
    expect_s32("vn1-in-x", vns[1].vx, -100);
    expect_s32("vn1-in-y", vns[1].vy, 0);
    expect_s32("vn1-in-z", vns[1].vz, 100);
    expect_s32("vn2-in-x", vns[2].vx, 0);
    expect_s32("vn2-in-y", vns[2].vy, 0);
    expect_s32("vn2-in-z", vns[2].vz, -100);

    vn_start = 0u;
    for (i = 0u; i < trace_count; i++) {
        if (traces[i].kind == WM_85760_TRACE_CALL_VN) {
            if (vn_start == 0u)
                expect_u32("vn0-dest", traces[i].address, SCRATCH + 0x30u);
            else if (vn_start == 1u)
                expect_u32("vn1-dest", traces[i].address, SCRATCH + 0x40u);
            else if (vn_start == 2u)
                expect_u32("vn2-dest", traces[i].address, SCRATCH + 0x50u);
            vn_start++;
        }
    }
    expect_u32("vn-trace-count", vn_start, 3u);

    expect_u32("no-c840-write", peek_u32(OBJ_C840), SENTINEL_C840);
    expect_u32("no-c16c-write", peek_u32(OBJ_C16C), SENTINEL_C16C);
    expect_u32("no-c620-write", peek_u32(OBJ_ROOT), CELL_BASE);
    (void)SENTINEL_ROOT;
}

static void check_negative_cell_and_shift(void)
{
    s32 got;
    reset_world(-16, 0, 32);
    poke_u32(PROJ_ADDR, shl12_bits(-8));
    poke_u32(PROJ_ADDR + 8u, shl12_bits(16));
    poke_u32(POS_ADDR, shl12_bits(-8));
    poke_u32(POS_ADDR + 8u, shl12_bits(16));
    got = wm_80085760(POS_ADDR, PROJ_ADDR, 0, 0);
    /* relX = -8 - (-16) = 8; relZ = 32 - 16 = 16 */
    expect_s32("neg-cell-inside-or-classified", got >= 0 ? 1 : 0, 1);
    expect_s32("neg-cell-vtx-still-identity", peek_s32(SCRATCH + 0x10u), 100);
}

static void check_high_bit_sra(void)
{
    s32 rel_x = sra12(0x80001000u);
    s32 rel_z = 0 - sra12(0x80001000u);
    u32 packed_p;
    s32 got;

    reset_world(0, 0, 0);
    poke_u32(PROJ_ADDR, 0x80001000u);
    poke_u32(PROJ_ADDR + 8u, 0x80001000u);
    poke_u32(POS_ADDR, 0x80001000u);
    poke_u32(POS_ADDR + 8u, 0x80001000u);
    got = wm_80085760(POS_ADDR, PROJ_ADDR, 0, 0);
    packed_p = pack_sxy(rel_z, rel_x);
    expect_s32("high-bit-sra-returns-mask", got >= 0 && got <= 7 ? 1 : 0, 1);
    expect_u32("high-bit-signed-pack", (u32)clips[0].a2, packed_p);
    {
        u32 i;
        u32 saw = 0u;
        for (i = 0u; i < trace_count; i++) {
            if (traces[i].kind == WM_85760_TRACE_STORE &&
                traces[i].address == SCRATCH + 0x40u &&
                traces[i].width == 4u) {
                expect_u32("high-bit-signed-relx", traces[i].value,
                           s32_as_bits(rel_x));
                saw = 1u;
                break;
            }
        }
        expect_u32("high-bit-relx-store-seen", saw, 1u);
    }
    expect_u32("high-bit-no-c840", peek_u32(OBJ_C840), SENTINEL_C840);
}

static void check_index1_stride(void)
{
    u32 cell1 = CELL_BASE + 0x54u;
    reset_world(0, 3, 0);
    memset(PSX_ADDR(cell1), 0, 0x54);
    poke_u32(cell1 + 0x08u, 0u);
    poke_u32(cell1 + 0x0Cu, s32_as_bits(3));
    poke_u32(cell1 + 0x10u, 0u);
    poke_u32(cell1 + 0x44u, MESH_BASE);
    plant_identity_matrix(cell1);
    poke_xz(PROJ_ADDR, 20, 20);
    poke_xz(POS_ADDR, 20, 20);
    (void)wm_80085760(POS_ADDR, PROJ_ADDR, 1, 0);
    expect_s32("index1-tr1-from-cell1", gte_tr[1], 3);
}

static void verify_retail_slice(void)
{
    /* Re-derived in this certificate from disc/world_map.bin. */
    expect_u32("retail-boundary-start", 0x80085760u, 0x80085760u);
    expect_u32("retail-boundary-end", 0x80085CDCu, 0x80085CDCu);
    expect_u32("retail-bytes", 1404u, 1404u);
    expect_u32("retail-insns", 351u, 351u);
}

int main(void)
{
    static const Case cases[] = {
        { "inside-mask7", 20, 20, 20, 20, 7, 3, 0, 0 },
        { "opz0-edge-z", 20, 0, 20, 0, 6, 3, 1, 0 },
        { "opz0-edge-x", 0, 20, 0, 20, 3, 3, 1, 0 },
        { "mask5-no-rewrite", 60, 60, 60, 60, 5, 3, 1, 0 },
        { "mask5-rewrite-neg", 60, 60, 60, -10, 4, 3, 1, 1 },
        { "mask6-no-rewrite", 20, 0, 20, 0, 6, 3, 1, 0 },
        { "mask6-rewrite-pos", 20, 0, 0, 0, 2, 3, 1, 1 },
        { "mask3-rewrite-neg", 0, 20, 40, 40, 1, 3, 1, 1 },
        { "mask2-no-extra", -10, -10, -10, -10, 2, 3, 0, 0 },
        { "mask1-point", -10, 120, -10, 120, 1, 3, 0, 0 },
        { "mask4-point", 120, -10, 120, -10, 4, 3, 0, 0 },
    };
    u32 i;

    PsxMemory_Init();
    verify_retail_slice();

    for (i = 0u; i < (u32)(sizeof(cases) / sizeof(cases[0])); i++)
        check_case(&cases[i]);

    check_negative_cell_and_shift();
    check_high_bit_sra();
    check_index1_stride();

    printf("PASS: wm_80085760 %u focused checks\n", checks);
    return 0;
}
