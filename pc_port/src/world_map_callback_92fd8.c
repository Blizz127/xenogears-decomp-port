/*
 * World-map scheduler callback 0x80092FD8 (slot-13 cb1).
 * Similar to 92C70 but reads D_8009CE68, different UI object,
 * and has palette color blending.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_92fd8.h"

extern void func_80034614(void* object);
extern void* GetStringEntry(void* table, s32 index);
extern void func_80034714(void* object, void* entry);
extern void func_80034888(void* object, void* ot, s32 render_context);

#define POOL_PTR   0x8009BE24u
#define CE68       0x8009CE68u  /* lh -0x3198 */
#define UI_OBJECT  0x8009BD64u  /* -0x429C */
#define D_8009D784 0x8009D784u
#define D_8009D7F0 0x8009D7F0u
#define D_8009BE3C 0x8009BE3Cu

static s16 fd_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void fd_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static s32 fd_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void fd_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }

static void* fd_string_table_pointer(u32 address)
{
#if defined(WM_UI_MUTANT_RAW_STRING_TABLE)
    return (void*)(uintptr_t)address;
#else
    return PSX_ADDR(address);
#endif
}

static void* fd_ot_pointer(u32 address)
{
#if defined(WM_UI_MUTANT_RAW_OT)
    return (void*)(uintptr_t)address;
#else
    return PSX_ADDR(address);
#endif
}

static void* fd_string_entry(u32 table, s32 index)
{
#if defined(WM_UI_MUTANT_SKIP_STRING_LOOKUP)
    (void)fd_string_table_pointer(table);
    (void)index;
    return NULL;
#else
    return GetStringEntry(fd_string_table_pointer(table), index);
#endif
}

s32 wm_80092FD8(s32 slot_idx)
{
    u32 pool_ptr = (u32)fd_lw(POOL_PTR);
    u32 slot = pool_ptr + (u32)(slot_idx << 7);

    /* Main dispatch on slot[+0x20] */
    s16 state = fd_lh(slot + 0x20);

    if (state == 0) {
        /* State 0: init */
        s16 ce68 = fd_lh(CE68);
        if (ce68 == -1) {
            goto palette_update;
        }
        func_80034614(PSX_ADDR(UI_OBJECT) /* W34B25: host ptr into g_PsxRam */);
        {
            u32 table = (u32)fd_lw(D_8009D784);
            void* entry = fd_string_entry(table, (s32)ce68);
            func_80034714(PSX_ADDR(UI_OBJECT) /* W34B25: host ptr into g_PsxRam */, entry);
        }
        fd_sw(slot + 0x50, (u32)(s32)ce68);
        fd_sh(slot + 0x20, 1);

    } else if (state == 1) {
        /* State 1: active */
        s16 ce68 = fd_lh(CE68);
        s32 stored = fd_lw(slot + 0x50);

        if (ce68 == -1) {
            func_80034614(PSX_ADDR(UI_OBJECT) /* W34B25: host ptr into g_PsxRam */);
            fd_sh(slot + 0x20, 0);
        } else if ((s32)ce68 != stored) {
            func_80034614(PSX_ADDR(UI_OBJECT) /* W34B25: host ptr into g_PsxRam */);
            {
                u32 table = (u32)fd_lw(D_8009D784);
                void* entry = fd_string_entry(table, (s32)ce68);
                func_80034714(PSX_ADDR(UI_OBJECT) /* W34B25: host ptr into g_PsxRam */, entry);
            }
            fd_sw(slot + 0x50, (u32)(s32)ce68);
        }
    }

palette_update:
    /* Palette update */
    {
        u32 src = (u32)fd_lw(D_8009BE3C);
        u32 pal_data = (u32)fd_lw(src + 0x70);
        s32 pal_table = fd_lw(D_8009D7F0);
        func_80034888(PSX_ADDR(UI_OBJECT) /* W34B25: host ptr into g_PsxRam */,
                      fd_ot_pointer(pal_data), pal_table);
    }

    /* Palette color blending (state==1 only) */
    if (fd_lh(slot + 0x20) == 1) {
        u32 pal_idx = (u32)fd_lw(D_8009D7F0);
        u32 tbl_base = 0x8009D2B8u; /* -0x2D48 */
        u32 src = (u32)fd_lw(D_8009BE3C);
        u32 pal_entry = (u32)fd_lw(src + 0x70);

        /* Blend: tbl[pal_idx] = (tbl[pal_idx] & 0xFF000000) | (*pal_entry & 0x00FFFFFF) */
        u32 tbl_addr = tbl_base + pal_idx * 40; /* stride = 5*8 = 40 */
        u32 tbl_val = (u32)fd_lw(tbl_addr);
        u32 pal_val = (u32)fd_lw(pal_entry);
        fd_sw(tbl_addr, (tbl_val & 0xFF000000u) | (pal_val & 0x00FFFFFFu));

        /* Reverse blend: *pal_entry = (*pal_entry & 0xFF000000) | (tbl_addr & 0x00FFFFFF) */
        pal_val = (u32)fd_lw(pal_entry);
        fd_sw(pal_entry, (pal_val & 0xFF000000u) | (tbl_addr & 0x00FFFFFFu));
    }

    return 1;
}
