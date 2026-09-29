#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/world_map_helper_72db4.h"
#include "../src/psx_memory.h"

#define DRAW_ENV_A 0x8009BBC8u
#define DRAW_ENV_B 0x8009BC40u
#define DISP_ENV_B 0x8009BC9Cu
#define OT_A 0x80150000u
#define OT_B 0x80151000u
#define HEAP_BASE 0x00180000u
#define LINK_MASK 0x00FFFFFFu

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static void* allocations[3];
static u32 allocation_sizes[3];
static u32 allocation_flags[3];
static int allocation_count;
static void* frees[3];
static int free_count;
static int draw_sync_count;
static int vsync_count;
static u32 draw_env_calls[65];
static int draw_env_count;
static u32 disp_env_calls[66];
static int disp_env_count;
static int clear_count;
static u32 clear_roots[64];
static int draw_count;
static int packet_count;
static int get_tpage_count;
static int get_tpage_args[4][4];
static int failure_count;

static u32 read_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void write_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 read_u16_host(const u8* p)
{
    u16 value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void fail_named(const char* name, long long got, long long expected)
{
    fprintf(stderr, "ASSERT_%s got=%lld expected=%lld\n", name, got, expected);
    failure_count++;
}

#define CHECK_EQ(name, got_expr, expected_expr) do { \
    long long check_got = (long long)(got_expr); \
    long long check_expected = (long long)(expected_expr); \
    if (check_got != check_expected) \
        fail_named((name), check_got, check_expected); \
} while (0)

void* HeapAlloc(u32 size, u32 flags)
{
    u32 offset = HEAP_BASE;
    int index = allocation_count;
    if (index > 0)
        offset += 0x100u * (u32)index;
    allocations[index] = g_PsxRam + offset;
    allocation_sizes[index] = size;
    allocation_flags[index] = flags;
    allocation_count++;
    return allocations[index];
}

u32 HeapFree(void* ptr)
{
    if (free_count < 3)
        frees[free_count] = ptr;
    free_count++;
    return 0u;
}

u16 GetTPage(int tp, int abr, int x, int y)
{
    int index = get_tpage_count;
    get_tpage_args[index][0] = tp;
    get_tpage_args[index][1] = abr;
    get_tpage_args[index][2] = x;
    get_tpage_args[index][3] = y;
    get_tpage_count++;
    return (u16)(0x0100u + (u16)(index * 0x11));
}

void SetSemiTrans(void* primitive, int enabled)
{
    u8* p = (u8*)primitive;
    if (enabled != 0)
        p[7] = (u8)(p[7] | 2u);
    else
        p[7] = (u8)(p[7] & (u8)~2u);
}

void SetDrawTPage(void* packet, int dfe, int dtd, int tpage)
{
    u8* p = (u8*)packet;
    u32 command = 0xE1000000u | (u32)(tpage & 0xFFFF) |
                  ((dfe != 0) ? 0x00000200u : 0u) |
                  ((dtd != 0) ? 0x00000400u : 0u);
    p[3] = 1u;
    memcpy(p + 4, &command, sizeof(command));
}

void DrawSync(void (*func)(unsigned long))
{
    (void)func;
    draw_sync_count++;
}

void Vsync(long mode)
{
    CHECK_EQ("VSYNC_MODE", mode, 0);
    vsync_count++;
}

void PutDrawEnv(void* env)
{
    if (draw_env_count < 65)
        draw_env_calls[draw_env_count] = PsxMemory_GuestAddr(env);
    draw_env_count++;
}

void PutDispEnv(void* env)
{
    if (disp_env_count < 66)
        disp_env_calls[disp_env_count] = PsxMemory_GuestAddr(env);
    disp_env_count++;
}

void wm_ot_clear_r_guest(u32 ot_guest, u32 count)
{
    u32 index;
    CHECK_EQ("OT_CLEAR_WORDS", count, 0x400u);
    if (clear_count < 64)
        clear_roots[clear_count] = ot_guest;
    clear_count++;
    write_u32(ot_guest, 0x00FFFFFFu);
    for (index = 1u; index < count; index++)
        write_u32(ot_guest + index * 4u,
                  (ot_guest + (index - 1u) * 4u) & LINK_MASK);
}

static void check_ft4(const u8* p, int strip)
{
    static const s16 expected_x[3][4] = {
        {0, 128, 0, 128}, {128, 256, 128, 256}, {256, 320, 256, 320}
    };
    static const s16 expected_y[4] = {0, 0, 239, 239};
    static const u8 expected_u[3][4] = {
        {0, 128, 0, 128}, {0, 128, 0, 128}, {0, 64, 0, 64}
    };
    int vertex;
    CHECK_EQ("FT4_LEN", p[3], 9);
    CHECK_EQ("FT4_CODE", p[7], 0x2D);
    for (vertex = 0; vertex < 4; vertex++) {
        u32 xy = 8u + (u32)vertex * 8u;
        u32 uv = 12u + (u32)vertex * 8u;
        CHECK_EQ("FT4_X", (s16)read_u16_host(p + xy), expected_x[strip][vertex]);
        CHECK_EQ("FT4_Y", (s16)read_u16_host(p + xy + 2u), expected_y[vertex]);
        CHECK_EQ("FT4_U", p[uv], expected_u[strip][vertex]);
        CHECK_EQ("FT4_V", p[uv + 1u], expected_y[vertex]);
    }
    CHECK_EQ("FT4_TPAGE", read_u16_host(p + 22u), 0x0100 + strip * 0x11);
}

int wm_ot_draw_otag_guest(u32 entry_guest)
{
    u32 current = entry_guest;
    u32 seen = 0u;
    int local_packets = 0;
    int local_ft4 = 0;
    int local_g4 = 0;
    int local_tpage = 0;

    while (seen++ < 1100u) {
        u32 tag = read_u32(current);
        u32 len = tag >> 24;
        u32 next = tag & LINK_MASK;
        if (len != 0u) {
            u8* packet = (u8*)PSX_ADDR(current);
            u8 code = packet[7];
            local_packets++;
            packet_count++;
            if (local_packets <= 3)
                CHECK_EQ("PACKET_ORDER", code & 0xFCu, 0x2Cu);
            else if (local_packets == 4)
                CHECK_EQ("PACKET_ORDER", code, 0xE1u);
            else if (local_packets == 5)
                CHECK_EQ("PACKET_ORDER", code & 0xFCu, 0x38u);
            if ((code & 0xFCu) == 0x2Cu) {
                u32 base = PsxMemory_GuestAddr(allocations[0]);
                int strip = (int)((current - base) / 40u);
                CHECK_EQ("PACKET_FT4_RANGE", current >= base && current < base + 120u, 1);
                CHECK_EQ("PACKET_FT4_STRIDE", (current - base) % 40u, 0);
                check_ft4(packet, strip);
                local_ft4++;
            } else if ((code & 0xFCu) == 0x38u) {
                u32 base = PsxMemory_GuestAddr(allocations[1]);
                u32 expected_offset = ((draw_count & 1) == 0) ? 36u : 0u;
                int color;
                CHECK_EQ("PACKET_G4_ADDRESS", current, base + expected_offset);
                for (color = 0; color < 12; color++) {
                    static const u8 offsets[12] = {
                        4u,5u,6u,12u,13u,14u,20u,21u,22u,28u,29u,30u
                    };
                    CHECK_EQ("G4_INTENSITY", packet[offsets[color]], draw_count * 4);
                }
                local_g4++;
            } else if (code == 0xE1u) {
                CHECK_EQ("PACKET_TPAGE_ADDRESS", current,
                         PsxMemory_GuestAddr(allocations[2]));
                local_tpage++;
            } else {
                fail_named("PACKET_OPCODE", code, 0x2D);
            }
        }
        if (next == LINK_MASK)
            break;
        current = 0x80000000u | next;
    }
    CHECK_EQ("PACKET_ORDER_COUNT", local_packets, 5);
    CHECK_EQ("PACKET_FT4_COUNT", local_ft4, 3);
    CHECK_EQ("PACKET_G4_COUNT", local_g4, 1);
    CHECK_EQ("PACKET_TPAGE_COUNT", local_tpage, 1);
    draw_count++;
    return 1;
}

static void check_results(void)
{
    int i;
    CHECK_EQ("ALLOC_COUNT", allocation_count, 3);
    CHECK_EQ("ALLOC_FT4_SIZE", allocation_sizes[0], 120);
    CHECK_EQ("ALLOC_G4_SIZE", allocation_sizes[1], 72);
    CHECK_EQ("ALLOC_TPAGE_SIZE", allocation_sizes[2], 8);
    for (i = 0; i < 3; i++)
        CHECK_EQ("ALLOC_FLAG", allocation_flags[i], 1);
    CHECK_EQ("GET_TPAGE_COUNT", get_tpage_count, 4);
    CHECK_EQ("GET_TPAGE_0_TP", get_tpage_args[0][0], 2);
    CHECK_EQ("GET_TPAGE_0_X", get_tpage_args[0][2], 704);
    CHECK_EQ("GET_TPAGE_1_X", get_tpage_args[1][2], 832);
    CHECK_EQ("GET_TPAGE_2_X", get_tpage_args[2][2], 960);
    CHECK_EQ("GET_TPAGE_3_TP", get_tpage_args[3][0], 0);
    CHECK_EQ("GET_TPAGE_3_ABR", get_tpage_args[3][1], 2);
    CHECK_EQ("GET_TPAGE_3_X", get_tpage_args[3][2], 0);
    CHECK_EQ("GET_TPAGE_3_Y", get_tpage_args[3][3], 0);
    CHECK_EQ("DRAW_COUNT", draw_count, 64);
    CHECK_EQ("PACKET_TOTAL", packet_count, 320);
    CHECK_EQ("CLEAR_COUNT", clear_count, 64);
    CHECK_EQ("DRAW_SYNC_COUNT", draw_sync_count, 66);
    CHECK_EQ("VSYNC_COUNT", vsync_count, 66);
    CHECK_EQ("DRAW_ENV_COUNT", draw_env_count, 65);
    CHECK_EQ("DISP_ENV_COUNT", disp_env_count, 66);
    CHECK_EQ("DRAW_ENV_INITIAL", draw_env_calls[0], DRAW_ENV_A);
    CHECK_EQ("DISP_ENV_INITIAL", disp_env_calls[0], DISP_ENV_B);
    CHECK_EQ("DISP_ENV_FINAL", disp_env_calls[65], DISP_ENV_B);
    for (i = 0; i < 64; i++) {
        u32 expected_env = ((i & 1) == 0) ? DRAW_ENV_B : DRAW_ENV_A;
        u32 expected_ot = ((i & 1) == 0) ? OT_B : OT_A;
        CHECK_EQ("DRAW_ENV_ALTERNATION", draw_env_calls[i + 1], expected_env);
        CHECK_EQ("OT_ROOT_ALTERNATION", clear_roots[i], expected_ot);
    }
    CHECK_EQ("FREE_COUNT", free_count, 3);
    for (i = 0; i < 3; i++)
        CHECK_EQ("FREE_IDENTITY", frees[i] == allocations[i], 1);
}

int main(void)
{
    int result;
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    write_u32(DRAW_ENV_A + 0x70u, OT_A);
    write_u32(DRAW_ENV_B + 0x70u, OT_B);
    result = wm_80072DB4(64, 0u, 4, 2);
    CHECK_EQ("RETURN", result, 0);
    check_results();
    if (failure_count != 0) {
        fprintf(stderr, "W34N8 72DB4 CERTIFICATE FAIL assertions=%d\n", failure_count);
        return 1;
    }
    puts("W34N8 72DB4 CERTIFICATE PASS");
    return 0;
}
