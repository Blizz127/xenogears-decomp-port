/*
 * W34B20-B: retail world-map callback 0x800925A0.
 *
 * This is a finite-width transcription of world_map.bin
 * [0x800925A0,0x80092BE4).  It updates the fade POLY_G4 colors, performs
 * the two retail inline OT insertions (quad, then DR_TPAGE), advances the
 * slot-local fade state, and returns the scheduler state.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_925a0.h"

typedef struct wm_925a0_dr_tpage {
    u32 tag;
    u32 code[1];
} wm_925a0_dr_tpage;

u16 GetTPage(int tp, int abr, int x, int y);
void SetDrawTPage(wm_925a0_dr_tpage* packet, int dfe, int dtd, int tpage);

#define WM_925A0_POOL_PTR       0x8009BE24u
#define WM_925A0_RENDER_CTX_PTR 0x8009BE3Cu
#define WM_925A0_FADE_ACTIVE    0x8009C178u
#define WM_925A0_TPAGE_ABR      0x8009CCA4u
#define WM_925A0_FADE_QUADS     0x8009CE6Cu
#define WM_925A0_DR_TPAGE       0x8009D310u
#define WM_925A0_FADE_SPEED     0x8009D3CCu
#define WM_925A0_BUFFER_INDEX   0x8009D7F0u

#define WM_925A0_SLOT_MODE      0x04u
#define WM_925A0_SLOT_FADE_MODE 0x20u
#define WM_925A0_SLOT_COUNTER   0x22u
#define WM_925A0_SLOT_FADE      0x50u
#define WM_925A0_SLOT_STRIDE    0x80u
#define WM_925A0_QUAD_STRIDE    0x24u

#define WM_925A0_TAG_LENGTH_MASK 0xFF000000u
#define WM_925A0_TAG_POINTER_MASK 0x00FFFFFFu

#if defined(WM_925A0_TEST_TRACE)
#define WM_925A0_TRACE(pc, kind, address, width, value) \
    wm_925a0_test_trace((pc), (kind), (address), (width), (value))
#else
#define WM_925A0_TRACE(pc, kind, address, width, value) ((void)0)
#endif

typedef struct wm_925a0_render_layout {
    u32 index_pc[12];
    u32 fade_pc[12];
    u32 color_pc[12];
    u32 setup_index_pc;
    u32 setup_context_pc;
    u32 quad_context_pc[2];
    u32 quad_tag_pc;
    u32 quad_ot_pc[2];
    u32 quad_tag_store_pc;
    u32 quad_ot_store_pc;
    u32 dr_context_pc[2];
    u32 dr_tag_pc;
    u32 dr_ot_pc[2];
    u32 dr_tag_store_pc;
    u32 dr_ot_store_pc;
} wm_925a0_render_layout;

static const u32 s_color_offsets[12] = {
    0x04u, 0x05u, 0x06u,
    0x0Cu, 0x0Du, 0x0Eu,
    0x14u, 0x15u, 0x16u,
    0x1Cu, 0x1Du, 0x1Eu
};

static const wm_925a0_render_layout s_fade_out_layout = {
    { 0x80092664u, 0x8009268Cu, 0x800926B4u, 0x800926DCu,
      0x80092704u, 0x8009272Cu, 0x80092754u, 0x8009277Cu,
      0x800927A4u, 0x800927CCu, 0x800927F4u, 0x8009281Cu },
    { 0x80092674u, 0x8009269Cu, 0x800926C4u, 0x800926ECu,
      0x80092714u, 0x8009273Cu, 0x80092764u, 0x8009278Cu,
      0x800927B4u, 0x800927DCu, 0x80092804u, 0x8009282Cu },
    { 0x80092684u, 0x800926ACu, 0x800926D4u, 0x800926FCu,
      0x80092724u, 0x8009274Cu, 0x80092774u, 0x8009279Cu,
      0x800927C4u, 0x800927ECu, 0x80092814u, 0x8009283Cu },
    0x8009284Cu, 0x80092854u,
    { 0x80092868u, 0x80092884u },
    0x8009286Cu, { 0x80092870u, 0x8009288Cu },
    0x80092880u, 0x8009289Cu,
    { 0x800928A8u, 0x800928C4u },
    0x800928ACu, { 0x800928B0u, 0x800928CCu },
    0x800928C0u, 0x800928DCu
};

static const wm_925a0_render_layout s_fade_in_layout = {
    { 0x8009290Cu, 0x80092934u, 0x8009295Cu, 0x80092984u,
      0x800929ACu, 0x800929D4u, 0x800929FCu, 0x80092A24u,
      0x80092A4Cu, 0x80092A74u, 0x80092A9Cu, 0x80092AC4u },
    { 0x8009291Cu, 0x80092944u, 0x8009296Cu, 0x80092994u,
      0x800929BCu, 0x800929E4u, 0x80092A0Cu, 0x80092A34u,
      0x80092A5Cu, 0x80092A84u, 0x80092AACu, 0x80092AD4u },
    { 0x8009292Cu, 0x80092954u, 0x8009297Cu, 0x800929A4u,
      0x800929CCu, 0x800929F4u, 0x80092A1Cu, 0x80092A44u,
      0x80092A6Cu, 0x80092A94u, 0x80092ABCu, 0x80092AE4u },
    0x80092AF4u, 0x80092AFCu,
    { 0x80092B10u, 0x80092B2Cu },
    0x80092B14u, { 0x80092B18u, 0x80092B34u },
    0x80092B28u, 0x80092B44u,
    { 0x80092B50u, 0x80092B6Cu },
    0x80092B54u, { 0x80092B58u, 0x80092B74u },
    0x80092B68u, 0x80092B84u
};

static u32 wm_925a0_load_u32(u32 pc, u32 address)
    __attribute__((noinline));
static u32 wm_925a0_load_u32(u32 pc, u32 address)
{
    u32 value;
    (void)pc;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    WM_925A0_TRACE(pc, WM_925A0_TRACE_LW, address, 4u, value);
    return value;
}

static s16 wm_925a0_load_s16(u32 pc, u32 address)
    __attribute__((noinline));
static s16 wm_925a0_load_s16(u32 pc, u32 address)
{
    u16 raw;
    (void)pc;
    memcpy(&raw, PSX_ADDR(address), sizeof(raw));
    WM_925A0_TRACE(pc, WM_925A0_TRACE_LH, address, 2u, (u32)raw);
    return (s16)raw;
}

static u16 wm_925a0_load_u16(u32 pc, u32 address)
    __attribute__((noinline));
static u16 wm_925a0_load_u16(u32 pc, u32 address)
{
    u16 value;
    (void)pc;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    WM_925A0_TRACE(pc, WM_925A0_TRACE_LHU, address, 2u, (u32)value);
    return value;
}

static void wm_925a0_store_u32(u32 pc, u32 address, u32 value)
    __attribute__((noinline));
static void wm_925a0_store_u32(u32 pc, u32 address, u32 value)
{
    (void)pc;
    memcpy(PSX_ADDR(address), &value, sizeof(value));
    WM_925A0_TRACE(pc, WM_925A0_TRACE_SW, address, 4u, value);
}

static void wm_925a0_store_u16(u32 pc, u32 address, u16 value)
    __attribute__((noinline));
static void wm_925a0_store_u16(u32 pc, u32 address, u16 value)
{
    (void)pc;
    memcpy(PSX_ADDR(address), &value, sizeof(value));
    WM_925A0_TRACE(pc, WM_925A0_TRACE_SH, address, 2u, (u32)value);
}

static void wm_925a0_store_u8(u32 pc, u32 address, u8 value)
    __attribute__((noinline));
static void wm_925a0_store_u8(u32 pc, u32 address, u8 value)
{
    (void)pc;
    memcpy(PSX_ADDR(address), &value, sizeof(value));
    WM_925A0_TRACE(pc, WM_925A0_TRACE_SB, address, 1u, (u32)value);
}

static u32 wm_925a0_quad_address(u32 index)
{
    return WM_925A0_FADE_QUADS + index * WM_925A0_QUAD_STRIDE;
}

static void wm_925a0_insert_quad(const wm_925a0_render_layout* layout,
                                 u32 context, u32 quad)
    __attribute__((unused));
static void wm_925a0_insert_quad(const wm_925a0_render_layout* layout,
                                 u32 context, u32 quad)
{
    u32 ot_field = 0x70u;
    u32 ot_address;
    u32 quad_tag;
    u32 old_ot;
    u32 linked_tag;
    u32 linked_head;

#if defined(WM_925A0_MUTANT_M11)
    ot_field = 0x74u;
#endif

    ot_address = wm_925a0_load_u32(layout->quad_context_pc[0],
                                  context + ot_field);
    quad_tag = wm_925a0_load_u32(layout->quad_tag_pc, quad);
    old_ot = wm_925a0_load_u32(layout->quad_ot_pc[0], ot_address);
    linked_tag = (quad_tag & WM_925A0_TAG_LENGTH_MASK) |
                 (old_ot & WM_925A0_TAG_POINTER_MASK);

#if defined(WM_925A0_MUTANT_M13)
    ot_address = wm_925a0_load_u32(layout->quad_context_pc[1],
                                  context + ot_field);
    old_ot = wm_925a0_load_u32(layout->quad_ot_pc[1], ot_address);
    linked_head = (old_ot & WM_925A0_TAG_LENGTH_MASK) |
                  (quad & WM_925A0_TAG_POINTER_MASK);
    wm_925a0_store_u32(layout->quad_ot_store_pc, ot_address, linked_head);
    wm_925a0_store_u32(layout->quad_tag_store_pc, quad, linked_tag);
#else
    wm_925a0_store_u32(layout->quad_tag_store_pc, quad, linked_tag);
    ot_address = wm_925a0_load_u32(layout->quad_context_pc[1],
                                  context + ot_field);
    old_ot = wm_925a0_load_u32(layout->quad_ot_pc[1], ot_address);
    linked_head = (old_ot & WM_925A0_TAG_LENGTH_MASK) |
                  (quad & WM_925A0_TAG_POINTER_MASK);
    wm_925a0_store_u32(layout->quad_ot_store_pc, ot_address, linked_head);
#endif
}

static void wm_925a0_insert_dr_tpage(const wm_925a0_render_layout* layout,
                                     u32 context)
    __attribute__((unused));
static void wm_925a0_insert_dr_tpage(const wm_925a0_render_layout* layout,
                                     u32 context)
{
    u32 ot_field = 0x70u;
    u32 ot_address;
    u32 dr_tag;
    u32 old_ot;
    u32 linked_tag;
    u32 linked_head;

#if defined(WM_925A0_MUTANT_M11)
    ot_field = 0x74u;
#endif

    ot_address = wm_925a0_load_u32(layout->dr_context_pc[0],
                                  context + ot_field);
    dr_tag = wm_925a0_load_u32(layout->dr_tag_pc, WM_925A0_DR_TPAGE);
    old_ot = wm_925a0_load_u32(layout->dr_ot_pc[0], ot_address);
    linked_tag = (dr_tag & WM_925A0_TAG_LENGTH_MASK) |
                 (old_ot & WM_925A0_TAG_POINTER_MASK);
    wm_925a0_store_u32(layout->dr_tag_store_pc, WM_925A0_DR_TPAGE,
                       linked_tag);

    ot_address = wm_925a0_load_u32(layout->dr_context_pc[1],
                                  context + ot_field);
    old_ot = wm_925a0_load_u32(layout->dr_ot_pc[1], ot_address);
    linked_head = (old_ot & WM_925A0_TAG_LENGTH_MASK) |
                  (WM_925A0_DR_TPAGE & WM_925A0_TAG_POINTER_MASK);
    wm_925a0_store_u32(layout->dr_ot_store_pc, ot_address, linked_head);
}

static void wm_925a0_render(u32 slot,
                            const wm_925a0_render_layout* layout)
{
    u32 index;
    u32 fade;
    u32 quad;
    u32 context;
    u8 color;
    unsigned i;

    for (i = 0u; i < 12u; i++) {
        index = wm_925a0_load_u32(layout->index_pc[i],
                                 WM_925A0_BUFFER_INDEX);
        fade = wm_925a0_load_u32(layout->fade_pc[i],
                                slot + WM_925A0_SLOT_FADE);
#if defined(WM_925A0_MUTANT_M6)
        color = (u8)(fade + 1u);
#else
        color = (u8)fade;
#endif
        quad = wm_925a0_quad_address(index);
        wm_925a0_store_u8(layout->color_pc[i], quad + s_color_offsets[i],
                          color);
    }

    index = wm_925a0_load_u32(layout->setup_index_pc,
                             WM_925A0_BUFFER_INDEX);
    context = wm_925a0_load_u32(layout->setup_context_pc,
                               WM_925A0_RENDER_CTX_PTR);
    quad = wm_925a0_quad_address(index);

#if defined(WM_925A0_MUTANT_M10)
#if !defined(WM_925A0_MUTANT_M9)
    wm_925a0_insert_dr_tpage(layout, context);
#endif
#if !defined(WM_925A0_MUTANT_M8)
    wm_925a0_insert_quad(layout, context, quad);
#endif
#else
#if !defined(WM_925A0_MUTANT_M8)
    wm_925a0_insert_quad(layout, context, quad);
#endif
#if !defined(WM_925A0_MUTANT_M9)
    wm_925a0_insert_dr_tpage(layout, context);
#endif
#endif
}

static void wm_925a0_initialize_fade(u32 slot, s16 mode)
{
    u32 abr;
    u16 tpage;

    if (mode == 13) {
        abr = wm_925a0_load_u32(0x800925E8u, WM_925A0_TPAGE_ABR);
        wm_925a0_store_u16(0x800925F0u, slot + WM_925A0_SLOT_MODE, 0u);
#if defined(WM_925A0_MUTANT_M12)
        wm_925a0_store_u16(0x800925F8u, slot + WM_925A0_SLOT_FADE_MODE,
                           0u);
#else
        wm_925a0_store_u16(0x800925F8u, slot + WM_925A0_SLOT_FADE_MODE,
                           1u);
#endif
    } else {
        abr = wm_925a0_load_u32(0x80092608u, WM_925A0_TPAGE_ABR);
        wm_925a0_store_u16(0x80092610u, slot + WM_925A0_SLOT_MODE, 0u);
        wm_925a0_store_u16(0x80092614u, slot + WM_925A0_SLOT_FADE_MODE,
                           0u);
    }

    wm_925a0_store_u32(0x8009261Cu, WM_925A0_FADE_ACTIVE, 1u);
    WM_925A0_TRACE(0x80092620u, WM_925A0_TRACE_CALL, 0x80043A1Cu,
                   0u, abr);
    tpage = GetTPage(0, (int)(s32)abr, 896, 256);
    WM_925A0_TRACE(0x80092628u, WM_925A0_TRACE_RETURN, 0x80043A1Cu,
                   0u, (u32)tpage);
    WM_925A0_TRACE(0x80092638u, WM_925A0_TRACE_CALL, 0x80043E20u,
                   0u, (u32)tpage);
#if defined(WM_925A0_MUTANT_M7)
    SetDrawTPage((wm_925a0_dr_tpage*)PSX_ADDR(WM_925A0_DR_TPAGE), 1, 0,
                 (int)((u32)tpage ^ 1u));
#else
    SetDrawTPage((wm_925a0_dr_tpage*)PSX_ADDR(WM_925A0_DR_TPAGE), 1, 0,
                 (int)(u32)tpage);
#endif
    WM_925A0_TRACE(0x80092640u, WM_925A0_TRACE_RETURN, 0x80043E20u,
                   0u, 0u);
}

s32 wm_800925A0(s32 slot_index)
{
    u32 pool;
    u32 slot;
    u32 fade;
    u32 speed;
    u32 next_fade;
    u16 counter;
    s16 mode;
    s16 fade_mode;
    s32 result = 1;

    WM_925A0_TRACE(0x800925A0u, WM_925A0_TRACE_ENTRY,
                   (u32)slot_index, 0u, 0u);
    pool = wm_925a0_load_u32(0x800925ACu, WM_925A0_POOL_PTR);
    slot = pool + (u32)slot_index * WM_925A0_SLOT_STRIDE;
    mode = wm_925a0_load_s16(0x800925C0u, slot + WM_925A0_SLOT_MODE);

    if (mode == 12 || mode == 13)
        wm_925a0_initialize_fade(slot, mode);

    fade_mode = wm_925a0_load_s16(0x80092640u,
                                  slot + WM_925A0_SLOT_FADE_MODE);
#if defined(WM_925A0_MUTANT_M2)
    if (fade_mode == 0)
        fade_mode = 1;
    else if (fade_mode == 1)
        fade_mode = 0;
#endif
    if (fade_mode != 0 && fade_mode != 1) {
#if defined(WM_925A0_MUTANT_M1)
        result = 2;
#endif
        WM_925A0_TRACE(0x80092658u, WM_925A0_TRACE_CALLBACK_RETURN,
                       0u, 0u, (u32)result);
        return result;
    }

    if (fade_mode == 0) {
        wm_925a0_render(slot, &s_fade_out_layout);
        fade = wm_925a0_load_u32(0x800928E0u,
                                slot + WM_925A0_SLOT_FADE);
        speed = wm_925a0_load_u32(0x800928E8u, WM_925A0_FADE_SPEED);
#if defined(WM_925A0_MUTANT_M3)
        next_fade = fade + speed;
#else
        next_fade = fade - speed;
#endif
        wm_925a0_store_u32(0x800928F8u, slot + WM_925A0_SLOT_FADE,
                           next_fade);
        if ((next_fade & 0x80000000u) != 0u) {
#if defined(WM_925A0_MUTANT_M5)
            result = 1;
#else
            result = 3;
#endif
            wm_925a0_store_u32(0x80092904u, slot + WM_925A0_SLOT_FADE,
                               0u);
            wm_925a0_store_u32(0x80092BB4u, WM_925A0_FADE_ACTIVE, 0u);
        }
    } else {
        wm_925a0_render(slot, &s_fade_in_layout);
        fade = wm_925a0_load_u32(0x80092B88u,
                                slot + WM_925A0_SLOT_FADE);
        speed = wm_925a0_load_u32(0x80092B90u, WM_925A0_FADE_SPEED);
#if defined(WM_925A0_MUTANT_M3)
        next_fade = fade - speed;
#else
        next_fade = fade + speed;
#endif
        wm_925a0_store_u32(0x80092B9Cu, slot + WM_925A0_SLOT_FADE,
                           next_fade);
        if ((next_fade & 0x80000000u) == 0u && next_fade >= 255u) {
#if !defined(WM_925A0_MUTANT_M4)
            wm_925a0_store_u32(0x80092BACu,
                               slot + WM_925A0_SLOT_FADE, 255u);
#endif
            wm_925a0_store_u32(0x80092BB4u, WM_925A0_FADE_ACTIVE, 0u);
        }
    }

    counter = wm_925a0_load_u16(0x80092BB8u,
                                slot + WM_925A0_SLOT_COUNTER);
    counter = (u16)(counter + 1u);
    wm_925a0_store_u16(0x80092BC4u, slot + WM_925A0_SLOT_COUNTER,
                       counter);

#if defined(WM_925A0_MUTANT_M1)
    result = 2;
#elif defined(WM_925A0_MUTANT_M14)
    if (result == 3)
        result = 1;
#endif
    WM_925A0_TRACE(0x80092BC8u, WM_925A0_TRACE_CALLBACK_RETURN,
                   0u, 0u, (u32)result);
    return result;
}
