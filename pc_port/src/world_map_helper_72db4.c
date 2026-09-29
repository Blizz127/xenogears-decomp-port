/*
 * Retail 0x80072DB4..0x800732FC.
 *
 * Builds three textured strips, an alternating full-screen shaded quad, and
 * a DR_TPAGE packet.  Each iteration clears one of the two retail 0x400-word
 * world OTs, links those five guest packets, presents it, and advances the
 * shade.  The original packets are HeapAlloc results (host views into
 * g_PsxRam); their OT links must therefore use PsxMemory_GuestAddr rather
 * than truncated native pointers.
 */

#include "world_map_helper_72db4.h"

#include "psx_memory.h"
#include "world_map_ot_adapter.h"

#include <stddef.h>
#include <string.h>

#define WM_72DB4_DRAW_ENV_A 0x8009BBC8u
#define WM_72DB4_DRAW_ENV_B 0x8009BC40u
#define WM_72DB4_DISP_ENV_A 0x8009BC24u
#define WM_72DB4_DISP_ENV_B 0x8009BC9Cu
#define WM_72DB4_ENV_OT     0x70u

#define WM_72DB4_FT4_BYTES  40u
#define WM_72DB4_G4_BYTES   36u
#define WM_72DB4_TPAGE_BYTES 8u
#define WM_72DB4_OT_WORDS   0x400u

#define WM_72DB4_LINK_MASK  0x00FFFFFFu
#define WM_72DB4_LEN_MASK   0xFF000000u

extern void* HeapAlloc(u32 size, u32 flags);
extern u32 HeapFree(void* ptr);
extern u16 GetTPage(int tp, int abr, int x, int y);
extern void SetSemiTrans(void* primitive, int enabled);
extern void SetDrawTPage(void* packet, int dfe, int dtd, int tpage);
extern void DrawSync(void (*func)(unsigned long));
extern void Vsync(long mode);
extern void PutDrawEnv(void* env);
extern void PutDispEnv(void* env);

static u32 wm_72db4_read_u32(const void* p)
{
    u32 value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void wm_72db4_write_u16(u8* p, u16 value)
{
    memcpy(p, &value, sizeof(value));
}

static void wm_72db4_write_u32(void* p, u32 value)
{
    memcpy(p, &value, sizeof(value));
}

static u32 wm_72db4_guest_read_u32(u32 address)
{
    return wm_72db4_read_u32(PSX_ADDR(address));
}

static void wm_72db4_guest_write_u32(u32 address, u32 value)
{
    wm_72db4_write_u32(PSX_ADDR(address), value);
}

static void wm_72db4_link_packet(u8* packet, u32 packet_guest,
                                 u32 bucket_guest)
{
    u32 packet_tag = wm_72db4_read_u32(packet);
    u32 bucket_tag = wm_72db4_guest_read_u32(bucket_guest);

    packet_tag = (packet_tag & WM_72DB4_LEN_MASK) |
                 (bucket_tag & WM_72DB4_LINK_MASK);
    bucket_tag = (bucket_tag & WM_72DB4_LEN_MASK) |
                 (packet_guest & WM_72DB4_LINK_MASK);
    wm_72db4_write_u32(packet, packet_tag);
    wm_72db4_guest_write_u32(bucket_guest, bucket_tag);
}

static void wm_72db4_init_ft4(u8* packets)
{
    static const s16 xy[3][4][2] = {
        {{0, 0}, {128, 0}, {0, 239}, {128, 239}},
        {{128, 0}, {256, 0}, {128, 239}, {256, 239}},
        {{256, 0}, {320, 0}, {256, 239}, {320, 239}}
    };
    static const u8 uv[3][4][2] = {
        {{0, 0}, {128, 0}, {0, 239}, {128, 239}},
        {{0, 0}, {128, 0}, {0, 239}, {128, 239}},
        {{0, 0}, {64, 0}, {0, 239}, {64, 239}}
    };
    static const int tpage_x[3] = {704, 832, 960};
    u32 packet_index;

    for (packet_index = 0u; packet_index < 3u; packet_index++) {
        u8* packet = packets + packet_index * WM_72DB4_FT4_BYTES;
        u32 vertex;

        packet[3] = 9u;
        packet[4] = 128u;
        packet[5] = 128u;
        packet[6] = 128u;
        packet[7] = 0x2Du;
        for (vertex = 0u; vertex < 4u; vertex++) {
            u32 xy_offset = 8u + vertex * 8u;
            u32 uv_offset = 12u + vertex * 8u;
            wm_72db4_write_u16(packet + xy_offset,
                               (u16)xy[packet_index][vertex][0]);
            wm_72db4_write_u16(packet + xy_offset + 2u,
                               (u16)xy[packet_index][vertex][1]);
            packet[uv_offset] = uv[packet_index][vertex][0];
            packet[uv_offset + 1u] = uv[packet_index][vertex][1];
        }
        wm_72db4_write_u16(packet + 22u,
                           GetTPage(2, 0, tpage_x[packet_index], 256));
    }
}

static void wm_72db4_init_g4(u8* packets)
{
    u8* packet = packets;

    packet[3] = 8u;
    packet[7] = 0x38u;
    wm_72db4_write_u16(packet + 8u, 0u);
    wm_72db4_write_u16(packet + 10u, 0u);
    wm_72db4_write_u16(packet + 16u, 320u);
    wm_72db4_write_u16(packet + 18u, 0u);
    wm_72db4_write_u16(packet + 24u, 0u);
    wm_72db4_write_u16(packet + 26u, 240u);
    wm_72db4_write_u16(packet + 32u, 320u);
    wm_72db4_write_u16(packet + 34u, 240u);
    SetSemiTrans(packet, 1);
    packet[7] = (u8)(packet[7] | 1u);
    memcpy(packets + WM_72DB4_G4_BYTES, packet, WM_72DB4_G4_BYTES);
}

static void wm_72db4_set_g4_intensity(u8* packet, u8 intensity)
{
    static const u8 color_offsets[12] = {
        4u, 5u, 6u, 12u, 13u, 14u,
        20u, 21u, 22u, 28u, 29u, 30u
    };
    u32 index;

    for (index = 0u; index < 12u; index++)
        packet[color_offsets[index]] = intensity;
}

static void wm_72db4_free_allocations(void* ft4, void* g4, void* tpage)
{
    if (ft4 != NULL)
        (void)HeapFree(ft4);
    if (g4 != NULL)
        (void)HeapFree(g4);
    if (tpage != NULL)
        (void)HeapFree(tpage);
}

int wm_80072DB4(s32 frame_count, s32 initial_intensity,
                s32 intensity_step, s32 abr)
{
    u8* ft4 = (u8*)HeapAlloc(3u * WM_72DB4_FT4_BYTES, 1u);
    u8* g4 = (u8*)HeapAlloc(2u * WM_72DB4_G4_BYTES, 1u);
    u8* tpage = (u8*)HeapAlloc(WM_72DB4_TPAGE_BYTES, 1u);
    u32 ft4_guest;
    u32 g4_guest;
    u32 tpage_guest;
    u32 draw_env = WM_72DB4_DRAW_ENV_A;
    s32 intensity = initial_intensity;
    s32 remaining = frame_count;
    u32 g4_select = 0u;

    if (ft4 == NULL || g4 == NULL || tpage == NULL) {
        wm_72db4_free_allocations(ft4, g4, tpage);
        return -1;
    }

    memset(ft4, 0, 3u * WM_72DB4_FT4_BYTES);
    memset(g4, 0, 2u * WM_72DB4_G4_BYTES);
    memset(tpage, 0, WM_72DB4_TPAGE_BYTES);
    wm_72db4_init_ft4(ft4);
    wm_72db4_init_g4(g4);
    SetDrawTPage(tpage, 0, 1,
                 (int)GetTPage(0, (int)abr, 0, 0));

    ft4_guest = PsxMemory_GuestAddr(ft4);
    g4_guest = PsxMemory_GuestAddr(g4);
    tpage_guest = PsxMemory_GuestAddr(tpage);

    DrawSync(NULL);
    Vsync(0);
    PutDispEnv(PSX_ADDR(WM_72DB4_DISP_ENV_B));
    PutDrawEnv(PSX_ADDR(WM_72DB4_DRAW_ENV_A));

#if defined(W34N8_MUTANT_SHORT_LOOP)
    remaining--;
#endif
    while (remaining-- > 0) {
        u32 ot_guest;
        u8* selected_g4;

#if defined(W34N8_MUTANT_NO_ENV_ALTERNATION)
        draw_env = WM_72DB4_DRAW_ENV_A;
#else
        draw_env = (draw_env == WM_72DB4_DRAW_ENV_A)
                       ? WM_72DB4_DRAW_ENV_B
                       : WM_72DB4_DRAW_ENV_A;
#endif
        ot_guest = wm_72db4_guest_read_u32(draw_env + WM_72DB4_ENV_OT);
        wm_ot_clear_r_guest(ot_guest, WM_72DB4_OT_WORDS);

        wm_72db4_link_packet(ft4, ft4_guest, ot_guest + 4u);
        wm_72db4_link_packet(ft4 + WM_72DB4_FT4_BYTES,
                             ft4_guest + WM_72DB4_FT4_BYTES,
                             ot_guest + 4u);
        wm_72db4_link_packet(ft4 + 2u * WM_72DB4_FT4_BYTES,
                             ft4_guest + 2u * WM_72DB4_FT4_BYTES,
                             ot_guest + 4u);

        g4_select ^= 1u;
        selected_g4 = g4 + g4_select * WM_72DB4_G4_BYTES;
        wm_72db4_set_g4_intensity(selected_g4, (u8)intensity);
#if defined(W34N8_MUTANT_G4_WRONG_BUCKET)
        wm_72db4_link_packet(selected_g4,
                             g4_guest + g4_select * WM_72DB4_G4_BYTES,
                             ot_guest + 4u);
#else
        wm_72db4_link_packet(selected_g4,
                             g4_guest + g4_select * WM_72DB4_G4_BYTES,
                             ot_guest);
#endif
        wm_72db4_link_packet(tpage, tpage_guest, ot_guest);

        DrawSync(NULL);
        Vsync(0);
        PutDispEnv(PSX_ADDR(draw_env + 0x5Cu));
        PutDrawEnv(PSX_ADDR(draw_env));
#if !defined(W34N8_MUTANT_SKIP_DRAW)
        (void)wm_ot_draw_otag_guest(ot_guest + 0xFFCu);
#endif
#if defined(W34N8_MUTANT_WRONG_INTENSITY_STEP)
        intensity += intensity_step + 1;
#else
        intensity += intensity_step;
#endif
    }

    DrawSync(NULL);
    Vsync(0);
    PutDispEnv(PSX_ADDR(WM_72DB4_DISP_ENV_B));
#if defined(W34N8_MUTANT_SKIP_FINAL_FREE)
    (void)ft4;
    (void)g4;
    (void)tpage;
#else
    wm_72db4_free_allocations(ft4, g4, tpage);
#endif
    return 0;
}
