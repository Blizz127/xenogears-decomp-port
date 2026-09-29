/*
 * Retail base-world session teardown [0x8007299C, 0x80072BB0).
 *
 * The world overlay stores heap allocations as guest KSEG addresses.  The
 * scheduler slot at +0x4C and the two sound globals are deliberate native
 * pointer authorities and must not be rebased through PSX_ADDR.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_86124.h"
#include "world_map_teardown_7299c.h"

extern void* D_80062528;
extern void* D_8006259C;
extern void* g_GfxWorkBuffers;
extern s32 D_80059190;
extern u16 D_8006F954;
extern u8 D_8005A4E4[];

extern void func_8003A89C(void* manager, s32 level, s32 steps);
extern void func_80039FF8(void);
extern void func_8003852C(void* seds);
extern unsigned int HeapFree(void* ptr);
extern void func_800230A8(void* sprite);
extern void func_800346D4(void* window);
extern void func_8002CBBC(u8* model_data);
extern u32 func_8002C3D8(void);

#if defined(WM_7299C_TEST_HOOKS)
extern void wm_7299c_test_snapshot_write(u32 offset, u32 size);
#endif

#define WM_D7CC              0x8009D7CCu
#define WM_POOL_BE24         0x8009BE24u
#define WM_TRANSITION_FLAGS  0x8006F954u

static u32 td_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 td_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

#if defined(WM_7299C_MUTANT_GUEST_TRANSITION)
static u16 td_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

#endif

static void td_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

#if defined(WM_7299C_MUTANT_GUEST_TRANSITION)
static void td_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

#endif

/* World allocations are KSEG after the Rung-4 domain repairs.  Retain the
 * legacy low-native case for allocations owned by older compiled system TUs. */
static void* td_pointer_to_host(u32 value)
{
    if (value == 0u)
        return NULL;
    if (value >= 0x80000000u && value < 0x80200000u)
        return PSX_ADDR(value);
    return (void*)(uintptr_t)value;
}

static void td_free_value(u32 value)
{
    HeapFree(td_pointer_to_host(value));
}

static void td_snapshot_sw(u32 offset, u32 value)
{
#if defined(WM_7299C_TEST_HOOKS)
    wm_7299c_test_snapshot_write(offset, 4u);
#endif
#if defined(WM_7299C_MUTANT_GUEST_SNAPSHOT)
    memcpy(PSX_ADDR(0x8005A4E4u + offset), &value, sizeof(value));
#else
    memcpy(D_8005A4E4 + offset, &value, sizeof(value));
#endif
}

static void td_snapshot_copy_from_guest(u32 offset, u32 src, u32 size)
{
#if defined(WM_7299C_TEST_HOOKS)
    wm_7299c_test_snapshot_write(offset, size);
#endif
#if defined(WM_7299C_MUTANT_GUEST_SNAPSHOT)
    memcpy(PSX_ADDR(0x8005A4E4u + offset), PSX_ADDR(src), (size_t)size);
#else
    memcpy(D_8005A4E4 + offset, PSX_ADDR(src), (size_t)size);
#endif
}

/* Retail 0x80075460: preserve the base-world session state for D7CC == 1. */
static void wm_80075460(void)
{
    u32 pool = td_lw(WM_POOL_BE24);

    td_snapshot_copy_from_guest(0u, pool, 0x2000u);
    td_snapshot_sw(0x2000u, td_lw(0x8009D55Cu));
    td_snapshot_sw(0x2004u, td_lw(0x8009D560u));
    td_snapshot_sw(0x2008u, td_lw(0x8009D564u));
    td_snapshot_sw(0x2010u, (u32)(s32)td_lh(0x8009D52Cu));
    td_snapshot_sw(0x2014u, td_lw(0x8009BE40u));
    td_snapshot_sw(0x2018u, td_lw(0x8009BCC4u));
    td_snapshot_sw(0x201Cu, td_lw(0x8009D64Cu));
    td_snapshot_copy_from_guest(0x2020u, 0x8009C854u, 0x20u);
    td_snapshot_copy_from_guest(0x2040u, 0x8009CEC4u, 0x280u);
    td_snapshot_sw(0x22C0u, (u32)(s32)td_lh(0x8009D154u));
    td_snapshot_sw(0x22C4u, td_lw(0x8009BD38u));
    td_snapshot_sw(0x22C8u, td_lw(0x8009BD3Cu));
    td_snapshot_sw(0x22CCu, td_lw(0x8009D3F0u));
    td_snapshot_sw(0x22D0u, td_lw(0x8009BE0Cu));
#if defined(WM_7299C_MUTANT_SWAP_SNAPSHOT_TAIL)
    td_snapshot_sw(0x22D4u, td_lw(0x8009BBB4u));
    td_snapshot_sw(0x22D8u, td_lw(0x8009BBB8u));
    td_snapshot_sw(0x22DCu, td_lw(0x8009BBBCu));
#endif
    td_snapshot_sw(0x22E4u, td_lw(0x8009C838u));
    td_snapshot_sw(0x22E8u, td_lw(0x8009C83Cu));
#if !defined(WM_7299C_MUTANT_SWAP_SNAPSHOT_TAIL)
    td_snapshot_sw(0x22D4u, td_lw(0x8009BBB4u));
    td_snapshot_sw(0x22D8u, td_lw(0x8009BBB8u));
    td_snapshot_sw(0x22DCu, td_lw(0x8009BBBCu));
#endif
    td_snapshot_sw(0x22ECu, td_lw(0x8009BE28u));
    td_snapshot_sw(0x22F0u, td_lw(0x8009BE2Cu));
    td_snapshot_sw(0x22F4u, td_lw(0x8009BE30u));
#if defined(WM_7299C_MUTANT_ZERO_SNAPSHOT_HOLES)
    td_snapshot_sw(0x200Cu, 0u);
    td_snapshot_sw(0x22E0u, 0u);
    td_snapshot_sw(0x22F8u, 0u);
#endif
}

void wm_80084818(void)
{
    s32 count = (s32)td_lh(0x8009D7E0u);
    u32 base = td_lw(0x8009C620u);
    s32 i;

    for (i = 0; i < count; i++) {
        u32 entry = base + (u32)i * 0x54u;
        td_free_value(td_lw(entry + 0x48u));
        func_8002CBBC((u8*)td_pointer_to_host(td_lw(entry + 0x40u)));
    }
    td_free_value(base);
}

void wm_80097D64(void)
{
    u32 i;
    for (i = 0u; i < 256u; i++) {
        u32 value = td_lw(0x8009C184u + i * 4u);
        if (value != 0u)
            td_free_value(value);
    }
}

static void wm_800960BC(void)
{
    u32 first = func_8002C3D8();
    u32 second = func_8002C3D8();
    u32 selected = (first == 0u || second == UINT32_MAX)
                       ? td_lw(0x8009BE08u)
                       : td_lw(0x8009D3C0u);

    td_free_value(selected);
    td_free_value(td_lw(0x8009D7D4u));
}

/* The compiled native port previously generated an empty stub for this
 * system helper.  Retail is exactly HeapFree(g_GfxWorkBuffers) followed by
 * the work-list reset performed by func_8001D2A4 (D_80059190 = 0). */
void GfxFreeWorkBuffers(void)
{
    HeapFree(g_GfxWorkBuffers);
    D_80059190 = 0;
}

void wm_8007299C(void)
{
    u32 state = td_lw(WM_D7CC);
    u32 pool;
    u32 i;

#if defined(WM_7299C_MUTANT_AUDIO_ALWAYS)
    if (state != UINT32_MAX)
#else
    if (state == 0u)
#endif
        func_8003A89C(D_80062528, 0, 240);

    func_80039FF8();
    func_8003852C(D_8006259C);
    HeapFree(D_8006259C);

    pool = td_lw(WM_POOL_BE24);
#if defined(WM_7299C_MUTANT_POOL_63)
    for (i = 0u; i < 63u; i++) {
#else
    for (i = 0u; i < 64u; i++) {
#endif
        u32 slot = pool + i * 0x80u;
        u32 object = td_lw(slot + 0x4Cu);
        if (object != 0u) {
            func_800230A8((void*)(uintptr_t)object);
#if !defined(WM_7299C_MUTANT_NO_SLOT_CLEAR)
            td_sw(slot + 0x4Cu, 0u);
#endif
        }
    }

    if (state == 1u) {
#if !defined(WM_7299C_MUTANT_SKIP_SNAPSHOT)
        wm_80075460();
#endif
#if defined(WM_7299C_MUTANT_GUEST_TRANSITION)
        td_sh(WM_TRANSITION_FLAGS,
              (u16)(td_lhu(WM_TRANSITION_FLAGS) | 0x8000u));
#else
        /* Retail 80072A68..74 updates the game-state entrance halfword.
         * World initialization reads its native owner, not the guest mirror. */
        D_8006F954 = (u16)(D_8006F954 | 0x8000u);
#endif
    }

#if defined(WM_7299C_MUTANT_SWAP_WINDOW_ORDER)
    func_800346D4(PSX_ADDR(0x8009BD64u));
    func_800346D4(PSX_ADDR(0x8009D498u));
#else
    func_800346D4(PSX_ADDR(0x8009D498u));
    func_800346D4(PSX_ADDR(0x8009BD64u));
#endif
    wm_80084818();

    wm_80086124();
    GfxFreeWorkBuffers();
    td_free_value(td_lw(0x8009CEB4u));
    td_free_value(td_lw(0x8009D150u));
    wm_800866C8();
    td_free_value(td_lw(0x8009BE18u));
    td_free_value(td_lw(0x8009BE14u));
    td_free_value(td_lw(0x8009D30Cu));
    td_free_value(td_lw(0x8009D780u));
    td_free_value(td_lw(0x8009D7D0u));
    td_free_value(td_lw(0x8009BDF4u));
    wm_80089128();
    wm_80097D64();

    td_free_value(td_lw(0x8009BC38u));
    td_free_value(td_lw(0x8009BCB0u));
    td_free_value(td_lw(0x8009BC3Cu));
    td_free_value(td_lw(0x8009BCB4u));
    td_free_value(td_lw(0x8009C180u));

    for (i = 0u; i < 3u; i++) {
        u32 value = td_lw(0x8009CD34u + i * 4u);
        if (value != 0u)
            td_free_value(value);
#if !defined(WM_7299C_MUTANT_SKIP_SECONDARY_PAIR)
        value = td_lw(0x8009BDF8u + i * 4u);
        if (value != 0u)
            td_free_value(value);
#endif
    }

    td_free_value(td_lw(WM_POOL_BE24));
    wm_800960BC();
}
