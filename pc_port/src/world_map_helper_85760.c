/*
 * World-map XZ triangle classifier 0x80085760.
 *
 * Register-faithful, finite-width transcription of retail world_map.bin
 * [0x80085760, 0x80085CDC): 1404 bytes / 351 instructions.
 * SHA-256 1332a8e87ab36bd5615d9c325076bf0f1864e66d469b507d6d353722cf106f91
 *
 * Direct callees are main-exe PsyQ only.  This helper is not a scheduler
 * callback and is not registered with the world dispatcher.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_85760.h"

#define WM_85760_OBJ_ROOT     0x8009C620u
#define WM_85760_SCRATCH      0x1F800000u
#define WM_85760_SCALE        0x800u
#define WM_85760_C840         0x8009C840u
#define WM_85760_C16C         0x8009C16Cu

/* Fixed-width PsyQ records.  These match PsyCross (int, not host long). */
typedef struct {
    s16 m[3][3];
    s32 t[3];
} Wm85760Matrix;

typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} Wm85760Vector;

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} Wm85760Svector;

extern Wm85760Matrix *ScaleMatrix(Wm85760Matrix *m, Wm85760Vector *v);
extern void SetRotMatrix(Wm85760Matrix *m);
extern void SetTransMatrix(Wm85760Matrix *m);
extern void RotTrans(Wm85760Svector *v0, Wm85760Vector *v1, long *flag);
extern int NormalClip(int sxy0, int sxy1, int sxy2);
extern long VectorNormal(Wm85760Vector *v0, Wm85760Vector *v1);

#if defined(WM_85760_TEST_TRACE)
#define WM_85760_TRACE(kind, address, width, value) \
    wm_85760_test_trace((kind), (address), (width), (value))
#else
#define WM_85760_TRACE(kind, address, width, value) ((void)0)
#endif

static u32 s_wm_80085760_exec_count;

u32 wm_80085760_get_exec_count(void)
{
    return s_wm_80085760_exec_count;
}

void wm_80085760_reset_exec_count(void)
{
    s_wm_80085760_exec_count = 0u;
}

static void *wm_85760_guest(u32 address)
{
    if ((address & UINT32_C(0xFFFFFC00)) == UINT32_C(0x1F800000))
        return g_PsxScratchpad + (address & UINT32_C(0x3FF));
    return PSX_ADDR(address);
}

static u16 wm_85760_load_u16(u32 address)
{
    u16 value;
    memcpy(&value, wm_85760_guest(address), sizeof(value));
    return value;
}

static s16 wm_85760_load_s16(u32 address)
{
    s16 value;
    memcpy(&value, wm_85760_guest(address), sizeof(value));
    return value;
}

static u32 wm_85760_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, wm_85760_guest(address), sizeof(value));
    return value;
}

static void wm_85760_store_u32(u32 address, u32 value)
{
    WM_85760_TRACE(WM_85760_TRACE_STORE, address, 4u, value);
    memcpy(wm_85760_guest(address), &value, sizeof(value));
}

#if defined(WM_85760_MUTANT_STORE_U16)
static void wm_85760_store_u16(u32 address, u16 value)
{
    WM_85760_TRACE(WM_85760_TRACE_STORE, address, 2u, value);
    memcpy(wm_85760_guest(address), &value, sizeof(value));
}
#endif

static s32 wm_85760_bits_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 wm_85760_s32_as_bits(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/* Exact MIPS SRA-by-12 without relying on C signed right shift. */
static s32 wm_85760_sra12(u32 bits)
{
#if defined(WM_85760_MUTANT_UNSIGNED_SHIFT)
    return wm_85760_bits_as_s32(bits >> 12);
#else
    u32 value = bits >> 12u;
    if ((bits & 0x80000000u) != 0u)
        value |= 0xFFF00000u;
    return wm_85760_bits_as_s32(value);
#endif
}

/* Retail: ((((a2<<2)+a2)<<2)+a2)<<2 == a2 * 0x54, wrapping 32-bit. */
static u32 wm_85760_cell_offset(s32 index)
{
    u32 bits = wm_85760_s32_as_bits(index);
#if defined(WM_85760_MUTANT_WRONG_STRIDE)
    return bits * 0x2Cu;
#else
    u32 v0 = (bits << 2) + bits;
    v0 = (v0 << 2) + bits;
    return v0 << 2;
#endif
}

static u32 wm_85760_pack_sxy(u32 z_bits, u32 x_bits)
{
#if defined(WM_85760_MUTANT_WRONG_PACK)
    return (x_bits << 16) | (z_bits & 0xFFFFu);
#else
    return (z_bits << 16) | (x_bits & 0xFFFFu);
#endif
}

static s32 wm_85760_first_clip_hit(int opz)
{
#if defined(WM_85760_MUTANT_WRONG_OPZ)
    return opz >= 0;
#else
    return opz > 0;
#endif
}

static s32 wm_85760_extra_clip_hit(int opz)
{
#if defined(WM_85760_MUTANT_BLEZ_VS_BEQZ)
    return opz > 0;
#else
    return opz != 0;
#endif
}

s32 wm_80085760(u32 pos_addr, u32 projected_addr, s32 cell_index,
                s32 tri_index)
{
    u32 root;
    u32 cell;
    u32 mesh;
    u32 tri;
    u32 verts;
    u32 i;
    s32 mask;
    long flag;
    Wm85760Matrix *scaled;
    Wm85760Vector *scale_vec;
    Wm85760Vector transformed;
    Wm85760Svector *sv;
    u32 packed[3];
    u32 packed_p;
    u32 v_addr[3];

    s_wm_80085760_exec_count += 1u;

    root = wm_85760_load_u32(WM_85760_OBJ_ROOT);
    cell = root + wm_85760_cell_offset(cell_index);

    wm_85760_store_u32(
        WM_85760_SCRATCH + 0x40u,
        wm_85760_s32_as_bits(
            wm_85760_sra12(wm_85760_load_u32(projected_addr)) -
            wm_85760_bits_as_s32(wm_85760_load_u32(cell + 0x08u))));
    wm_85760_store_u32(
        WM_85760_SCRATCH + 0x48u,
        wm_85760_s32_as_bits(
            wm_85760_bits_as_s32(wm_85760_load_u32(cell + 0x10u)) -
            wm_85760_sra12(wm_85760_load_u32(projected_addr + 8u))));

    for (i = 0u; i < 8u; i++) {
        wm_85760_store_u32(WM_85760_SCRATCH + 0xF0u + (i * 4u),
                           wm_85760_load_u32(cell + 0x20u + (i * 4u)));
    }
    wm_85760_store_u32(WM_85760_SCRATCH + 0x10Cu, 0u);
    wm_85760_store_u32(WM_85760_SCRATCH + 0x104u, 0u);
    wm_85760_store_u32(WM_85760_SCRATCH + 0x000u, WM_85760_SCALE);
    wm_85760_store_u32(WM_85760_SCRATCH + 0x004u, WM_85760_SCALE);
    wm_85760_store_u32(WM_85760_SCRATCH + 0x008u, WM_85760_SCALE);
    wm_85760_store_u32(WM_85760_SCRATCH + 0x108u,
                       wm_85760_load_u32(cell + 0x0Cu));

    scaled = (Wm85760Matrix *)wm_85760_guest(WM_85760_SCRATCH + 0xF0u);
    scale_vec = (Wm85760Vector *)wm_85760_guest(WM_85760_SCRATCH);
    WM_85760_TRACE(WM_85760_TRACE_CALL_SCALE, WM_85760_SCRATCH + 0xF0u, 0u,
                   WM_85760_SCALE);
    (void)ScaleMatrix(scaled, scale_vec);

#if !defined(WM_85760_MUTANT_MISSING_GTE)
    WM_85760_TRACE(WM_85760_TRACE_CALL_SETROT, WM_85760_SCRATCH + 0xF0u, 0u,
                   0u);
    SetRotMatrix(scaled);
    WM_85760_TRACE(WM_85760_TRACE_CALL_SETTRANS, WM_85760_SCRATCH + 0xF0u, 0u,
                   0u);
    SetTransMatrix(scaled);
#endif

    mesh = wm_85760_load_u32(cell + 0x44u);
    tri = mesh + 8u +
          (u32)((wm_85760_s32_as_bits(tri_index) * 14) & 0xFFFFFFFFu);
#if defined(WM_85760_MUTANT_WRONG_TRI_STRIDE)
    tri = mesh + 8u + (u32)(tri_index * 16);
#endif
    verts = wm_85760_load_u32(mesh + 4u);
    for (i = 0u; i < 3u; i++) {
        s16 idx = wm_85760_load_s16(tri + (i * 2u));
        v_addr[i] = verts + (u32)((s32)idx * 8);
        sv = (Wm85760Svector *)wm_85760_guest(v_addr[i]);
        WM_85760_TRACE(WM_85760_TRACE_CALL_RTV0TR, v_addr[i], 0u, i);
        RotTrans(sv, &transformed, &flag);
#if defined(WM_85760_MUTANT_STORE_U16)
        wm_85760_store_u16(WM_85760_SCRATCH + (i * 0x10u),
                           (u16)transformed.vx);
        wm_85760_store_u16(WM_85760_SCRATCH + (i * 0x10u) + 4u,
                           (u16)transformed.vy);
        wm_85760_store_u16(WM_85760_SCRATCH + (i * 0x10u) + 8u,
                           (u16)transformed.vz);
#else
        wm_85760_store_u32(WM_85760_SCRATCH + (i * 0x10u),
                           wm_85760_s32_as_bits(transformed.vx));
        wm_85760_store_u32(WM_85760_SCRATCH + (i * 0x10u) + 4u,
                           wm_85760_s32_as_bits(transformed.vy));
        wm_85760_store_u32(WM_85760_SCRATCH + (i * 0x10u) + 8u,
                           wm_85760_s32_as_bits(transformed.vz));
#endif
    }

    packed[0] = wm_85760_pack_sxy(wm_85760_load_u32(WM_85760_SCRATCH + 0x08u),
                                  wm_85760_load_u16(WM_85760_SCRATCH + 0x00u));
    packed[1] = wm_85760_pack_sxy(wm_85760_load_u32(WM_85760_SCRATCH + 0x18u),
                                  wm_85760_load_u16(WM_85760_SCRATCH + 0x10u));
    packed[2] = wm_85760_pack_sxy(wm_85760_load_u32(WM_85760_SCRATCH + 0x28u),
                                  wm_85760_load_u16(WM_85760_SCRATCH + 0x20u));
    packed_p = wm_85760_pack_sxy(wm_85760_load_u32(WM_85760_SCRATCH + 0x48u),
                                 wm_85760_load_u16(WM_85760_SCRATCH + 0x40u));

    wm_85760_store_u32(WM_85760_SCRATCH + 0x30u, packed[0]);
    wm_85760_store_u32(WM_85760_SCRATCH + 0x34u, packed[1]);
    wm_85760_store_u32(WM_85760_SCRATCH + 0x38u, packed_p);

    mask = 0;
#if !defined(WM_85760_MUTANT_SKIP_CLIP0)
    {
        int opz = NormalClip((int)packed[0], (int)packed[1], (int)packed_p);
        WM_85760_TRACE(WM_85760_TRACE_CALL_NCLIP, 0u, 0u, (u32)opz);
        if (wm_85760_first_clip_hit(opz))
            mask |= 1;
    }
#endif
#if !defined(WM_85760_MUTANT_SKIP_CLIP1)
    {
        int opz = NormalClip((int)packed[1], (int)packed[2], (int)packed_p);
        WM_85760_TRACE(WM_85760_TRACE_CALL_NCLIP, 1u, 0u, (u32)opz);
        if (wm_85760_first_clip_hit(opz))
            mask |= 2;
    }
#endif
#if !defined(WM_85760_MUTANT_SKIP_CLIP2)
    {
        int opz = NormalClip((int)packed[2], (int)packed[0], (int)packed_p);
        WM_85760_TRACE(WM_85760_TRACE_CALL_NCLIP, 2u, 0u, (u32)opz);
        if (wm_85760_first_clip_hit(opz))
            mask |= 4;
    }
#endif

    if (mask == 3
#if defined(WM_85760_MUTANT_EXTRA_CLIP)
        || mask == 7
#endif
    ) {
        u32 raw_z = wm_85760_s32_as_bits(
            wm_85760_bits_as_s32(wm_85760_load_u32(cell + 0x10u)) -
            wm_85760_sra12(wm_85760_load_u32(pos_addr + 8u)));
        u32 raw_x = wm_85760_s32_as_bits(
            wm_85760_sra12(wm_85760_load_u32(pos_addr)) -
            wm_85760_bits_as_s32(wm_85760_load_u32(cell + 0x08u)));
        u32 raw = wm_85760_pack_sxy(raw_z, raw_x & 0xFFFFu);
#if defined(WM_85760_MUTANT_WRONG_VERTEX)
        u32 shared = packed[0];
#else
        u32 shared = packed[1];
#endif
        int opz;
        wm_85760_store_u32(WM_85760_SCRATCH + 0x30u, raw);
        wm_85760_store_u32(WM_85760_SCRATCH + 0x34u, shared);
        opz = NormalClip((int)raw, (int)shared, (int)packed_p);
        WM_85760_TRACE(WM_85760_TRACE_CALL_NCLIP, 3u, 0u, (u32)opz);
        if (wm_85760_extra_clip_hit(opz)) {
#if defined(WM_85760_MUTANT_WRONG_REWRITE)
            mask = 2;
#else
            mask = 1;
#endif
        }
    } else if (mask == 5) {
        u32 raw_z = wm_85760_s32_as_bits(
            wm_85760_bits_as_s32(wm_85760_load_u32(cell + 0x10u)) -
            wm_85760_sra12(wm_85760_load_u32(pos_addr + 8u)));
        u32 raw_x = wm_85760_s32_as_bits(
            wm_85760_sra12(wm_85760_load_u32(pos_addr)) -
            wm_85760_bits_as_s32(wm_85760_load_u32(cell + 0x08u)));
        u32 raw = wm_85760_pack_sxy(raw_z, raw_x & 0xFFFFu);
        u32 shared = packed[0];
        int opz;
        wm_85760_store_u32(WM_85760_SCRATCH + 0x30u, raw);
        wm_85760_store_u32(WM_85760_SCRATCH + 0x34u, shared);
        opz = NormalClip((int)raw, (int)shared, (int)packed_p);
        WM_85760_TRACE(WM_85760_TRACE_CALL_NCLIP, 4u, 0u, (u32)opz);
        if (wm_85760_extra_clip_hit(opz))
            mask = 4;
    } else if (mask == 6) {
        u32 raw_z = wm_85760_s32_as_bits(
            wm_85760_bits_as_s32(wm_85760_load_u32(cell + 0x10u)) -
            wm_85760_sra12(wm_85760_load_u32(pos_addr + 8u)));
        u32 raw_x = wm_85760_s32_as_bits(
            wm_85760_sra12(wm_85760_load_u32(pos_addr)) -
            wm_85760_bits_as_s32(wm_85760_load_u32(cell + 0x08u)));
        u32 raw = wm_85760_pack_sxy(raw_z, raw_x & 0xFFFFu);
        u32 shared = packed[2];
        int opz;
        wm_85760_store_u32(WM_85760_SCRATCH + 0x30u, raw);
        wm_85760_store_u32(WM_85760_SCRATCH + 0x34u, shared);
        opz = NormalClip((int)raw, (int)shared, (int)packed_p);
        WM_85760_TRACE(WM_85760_TRACE_CALL_NCLIP, 5u, 0u, (u32)opz);
        if (wm_85760_extra_clip_hit(opz))
            mask = 2;
    }

#if defined(WM_85760_MUTANT_WRONG_VN_ORDER)
    {
        const u32 src_x[3] = { 0x20u, 0x00u, 0x10u };
        const u32 src_z[3] = { 0x28u, 0x08u, 0x18u };
        const u32 sub_x[3] = { 0x00u, 0x20u, 0x10u };
        const u32 sub_z[3] = { 0x08u, 0x28u, 0x18u };
        const u32 dest[3] = { 0x50u, 0x30u, 0x40u };
        for (i = 0u; i < 3u; i++) {
            u32 d = dest[i];
            s32 dx = wm_85760_bits_as_s32(wm_85760_load_u32(
                         WM_85760_SCRATCH + src_x[i])) -
                     wm_85760_bits_as_s32(wm_85760_load_u32(
                         WM_85760_SCRATCH + sub_x[i]));
            s32 dz = wm_85760_bits_as_s32(wm_85760_load_u32(
                         WM_85760_SCRATCH + src_z[i])) -
                     wm_85760_bits_as_s32(wm_85760_load_u32(
                         WM_85760_SCRATCH + sub_z[i]));
            wm_85760_store_u32(WM_85760_SCRATCH + d + 4u, 0u);
            wm_85760_store_u32(WM_85760_SCRATCH + d, wm_85760_s32_as_bits(dx));
            wm_85760_store_u32(WM_85760_SCRATCH + d + 8u,
                               wm_85760_s32_as_bits(dz));
            WM_85760_TRACE(WM_85760_TRACE_CALL_VN, WM_85760_SCRATCH + d, 0u,
                           i);
            (void)VectorNormal((Wm85760Vector *)wm_85760_guest(
                                   WM_85760_SCRATCH + d),
                               (Wm85760Vector *)wm_85760_guest(
                                   WM_85760_SCRATCH + d));
        }
    }
#else
    {
        const u32 src_x[3] = { 0x10u, 0x20u, 0x00u };
        const u32 src_z[3] = { 0x18u, 0x28u, 0x08u };
        const u32 sub_x[3] = { 0x00u, 0x10u, 0x20u };
        const u32 sub_z[3] = { 0x08u, 0x18u, 0x28u };
#if defined(WM_85760_MUTANT_WRONG_SCRATCH)
        const u32 dest[3] = { 0x60u, 0x70u, 0x80u };
#else
        const u32 dest[3] = { 0x30u, 0x40u, 0x50u };
#endif
        for (i = 0u; i < 3u; i++) {
            u32 d = dest[i];
            s32 dx = wm_85760_bits_as_s32(wm_85760_load_u32(
                         WM_85760_SCRATCH + src_x[i])) -
                     wm_85760_bits_as_s32(wm_85760_load_u32(
                         WM_85760_SCRATCH + sub_x[i]));
            s32 dz = wm_85760_bits_as_s32(wm_85760_load_u32(
                         WM_85760_SCRATCH + src_z[i])) -
                     wm_85760_bits_as_s32(wm_85760_load_u32(
                         WM_85760_SCRATCH + sub_z[i]));
            wm_85760_store_u32(WM_85760_SCRATCH + d + 4u, 0u);
            wm_85760_store_u32(WM_85760_SCRATCH + d, wm_85760_s32_as_bits(dx));
            wm_85760_store_u32(WM_85760_SCRATCH + d + 8u,
                               wm_85760_s32_as_bits(dz));
            WM_85760_TRACE(WM_85760_TRACE_CALL_VN, WM_85760_SCRATCH + d, 0u,
                           i);
            (void)VectorNormal((Wm85760Vector *)wm_85760_guest(
                                   WM_85760_SCRATCH + d),
                               (Wm85760Vector *)wm_85760_guest(
                                   WM_85760_SCRATCH + d));
        }
    }
#endif

#if defined(WM_85760_MUTANT_EXTRA_GLOBAL)
    wm_85760_store_u32(WM_85760_C840, 0u);
    wm_85760_store_u32(WM_85760_C16C, 0u);
#endif

    return mask;
}
