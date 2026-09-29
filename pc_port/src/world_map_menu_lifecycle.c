/* Retail world-map menu lifecycle [0x800758C0,0x80075D4C). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"
#include "world_map_framebuffer_init.h"
#include "world_map_gpu_asset_8440c.h"
#include "world_map_helper_71fec.h"
#include "world_map_helper_72db4.h"
#include "world_map_helper_75d4c.h"
#include "world_map_helper_86124.h"
#include "world_map_helper_93f18.h"
#include "world_map_helper_96130.h"
#include "world_map_menu_lifecycle.h"
#include "world_map_ot_adapter.h"

typedef struct WmMenuRect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} WmMenuRect;

extern u8 D_80059179;
extern void* D_8005945C;

extern void HeapChangeCurrentUser(u_int user_tag, char** content_types);
extern int ArchiveSetIndex(int directory_index, int entry_index);
extern int ArchiveDecodeAlignedSize(unsigned int entry_index);
extern int ArchiveReadFileToBuffer(int entry_index, void* destination,
                                   int arg2, int flags);
extern int ArchiveDataSync(void);
extern void ArchiveCdDataSync(int mode);
extern void* HeapAlloc(u_int size, u_int flags);
extern unsigned int HeapFree(void* ptr);
extern void* LZSSDecompress(void* source, void* destination);
extern int StoreImage(WmMenuRect* rect, u32* data);
extern int LoadImage(WmMenuRect* rect, u32* data);
extern int MoveImage(WmMenuRect* rect, int x, int y);
extern int DrawSync(int mode);
extern int VSync(int mode);
extern void SystemTransferPaletteToVRAM(short x, short y);
extern void ControllerResetState(void);

#define WM_BD3A 0x8009BD3Au
#define WM_D55C 0x8009D55Cu
#define WM_D14C 0x8009D14Cu
#define WM_BC3C 0x8009BC3Cu
#define WM_BCB4 0x8009BCB4u
#define WM_C7E4 0x8009C7E4u
#define WM_C174 0x8009C174u
#define WM_BD20 0x8009BD20u
#define WM_D528 0x8009D528u
#define WM_C800 0x8009C800u
#define WM_C890 0x8009C890u
#define WM_D7F0 0x8009D7F0u
#define WM_BE3C 0x8009BE3Cu
#define WM_D804 0x8009D804u
#define WM_EE6A 0x8006EE6Au
#define WM_F8E5 0x8006F8E5u
#define WM_F950 0x8006F950u

static u32 wm_menu_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 wm_menu_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u8 wm_menu_lbu(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_menu_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_menu_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void* wm_menu_pointer_to_host(u32 value)
{
    if (value == 0u)
        return NULL;
    if (value >= 0x80000000u && value < 0x80200000u)
        return PSX_ADDR(value);
    return (void*)(uintptr_t)value;
}

static void wm_menu_free_guest_value(u32 value)
{
    (void)HeapFree(wm_menu_pointer_to_host(value));
}

/* Retail [0x800758C0,0x80075B58): enter menu and preserve world display. */
void wm_800758C0(void)
{
    WmMenuRect rect;
    void* probe;
    void* compressed;
    void* screen_a;
    void* screen_b;
    u32 compressed_guest;
    u32 scratch_size;
    int sync;
    u32 i;

    wm_menu_sh(WM_EE6A, 1u);
    wm_menu_sh(WM_F950, (u16)((wm_menu_lhu(WM_BD3A) + 0x2000u) & 0x3FFFu));
    for (i = 0u; i < 3u; i++)
        wm_menu_sh(WM_EE6A + 6u + i * 2u, wm_menu_lbu(WM_F8E5 + i));

    wm_menu_sw(WM_D14C, (u32)D_80059179);
    if ((s16)wm_80093F18(WM_D55C) == 4)
        D_80059179 = 1u;

    wm_80096694();
#if defined(W34N17_MUTANT_SWAP_ENTRY_FREE_ORDER)
    wm_800866C8();
    wm_80086124();
#else
    wm_80086124();
    wm_800866C8();
#endif
    wm_80089128();
    wm_menu_free_guest_value(wm_menu_lw(WM_BC3C));
    wm_menu_free_guest_value(wm_menu_lw(WM_BCB4));

    probe = HeapAlloc(4u, 1u);
    (void)HeapFree(probe);
#if defined(W34N17_MUTANT_WRONG_SCRATCH_SIZE)
    scratch_size = (PsxMemory_GuestAddr(probe) & 0x00FFFFFFu) + 0xFFE3B000u;
#else
    scratch_size = (PsxMemory_GuestAddr(probe) & 0x00FFFFFFu) + 0xFFE3B004u;
#endif
    compressed = HeapAlloc(scratch_size, 1u);
    compressed_guest = PsxMemory_GuestAddr(compressed);
    wm_menu_sw(WM_C7E4, compressed_guest);

    wm_80071FEC();

    screen_a = HeapAlloc(0x10000u, 0u);
    screen_b = HeapAlloc(0xC800u, 0u);
    wm_menu_sw(WM_C800, PsxMemory_GuestAddr(screen_a));
    wm_menu_sw(WM_C890, PsxMemory_GuestAddr(screen_b));

    rect.x = 384;
    rect.y = 256;
    rect.w = 128;
    rect.h = 256;
    (void)StoreImage(&rect, (u32*)screen_a);

    rect.x = 0;
    rect.y = 432;
    rect.w = 320;
    rect.h = 80;
    (void)StoreImage(&rect, (u32*)screen_b);

    if (wm_menu_lw(WM_D7F0) == 0u) {
#if !defined(W34N17_MUTANT_SKIP_CONDITIONAL_MOVE)
        rect.x = 0;
        rect.y = 216;
        rect.w = 320;
        rect.h = 216;
        (void)MoveImage(&rect, 0, 0);
#endif
    }

    rect.x = 0;
    rect.y = 0;
    rect.w = 320;
    rect.h = 216;
    (void)MoveImage(&rect, 704, 256);
    (void)DrawSync(0);

    (void)wm_80072DB4(16, 0, 8, 2);
    do {
        sync = ArchiveDataSync();
#if defined(W34N17_MUTANT_WRONG_SYNC_PREDICATE)
    } while (sync > 2);
#else
    } while (sync >= 2);
#endif

    (void)LZSSDecompress(wm_menu_pointer_to_host(wm_menu_lw(WM_D528)),
                         compressed);
    wm_menu_free_guest_value(wm_menu_lw(WM_D528));

    rect.x = 0;
    rect.y = 0;
    rect.w = 320;
    rect.h = 216;
    (void)MoveImage(&rect, 0, 224);
    (void)DrawSync(0);
    ArchiveCdDataSync(0);
}

/* Retail [0x80075B58,0x80075D4C): restore world after MenuMain. */
void wm_80075B58(void)
{
    WmMenuRect rect;
    void* bd20;
    u32 env;
    u32 ot;

    HeapChangeCurrentUser(3u, NULL);
    (void)ArchiveSetIndex(0x24, 0);
    (void)HeapFree(D_8005945C);
    wm_menu_free_guest_value(wm_menu_lw(WM_C7E4));
    wm_80072BB0();

    bd20 = HeapAlloc((u32)ArchiveDecodeAlignedSize(wm_menu_lw(WM_C174)), 1u);
    wm_menu_sw(WM_BD20, PsxMemory_GuestAddr(bd20));
    (void)ArchiveReadFileToBuffer((int)wm_menu_lw(WM_C174), bd20, 0, 0);
    (void)wm_80072DB4(16, 128, -8, 2);

    rect.x = 704;
    rect.y = 256;
    rect.w = 320;
    rect.h = 216;
    (void)MoveImage(&rect, 0, 0);
    (void)MoveImage(&rect, 0, 216);

    env = wm_menu_lw(WM_BE3C);
    ot = wm_menu_lw(env + 0x70u);
#if defined(W34N17_MUTANT_WRONG_OT_COUNT)
    wm_ot_clear_r_guest(ot, 0x200u);
#else
    wm_ot_clear_r_guest(ot, 0x400u);
#endif
    ArchiveCdDataSync(0);
#if !defined(W34N17_MUTANT_SKIP_8440C)
    (void)wm_8008440C();
#endif

    rect.x = 384;
    rect.y = 256;
    rect.w = 128;
    rect.h = 256;
    (void)LoadImage(&rect, (u32*)wm_menu_pointer_to_host(wm_menu_lw(WM_C800)));

    rect.x = 0;
    rect.y = 432;
    rect.w = 320;
    rect.h = 80;
    (void)LoadImage(&rect, (u32*)wm_menu_pointer_to_host(wm_menu_lw(WM_C890)));
    (void)DrawSync(0);
    wm_menu_free_guest_value(wm_menu_lw(WM_C800));
    wm_menu_free_guest_value(wm_menu_lw(WM_C890));

#if defined(W34N17_MUTANT_SWAP_REBUILD_ORDER)
    wm_8008901C();
    wm_800978FC();
#else
    wm_800978FC();
    wm_8008901C();
#endif
    wm_800865A0();
    wm_80085FE0();
    SystemTransferPaletteToVRAM(304, 480);
    (void)VSync(0);
    ControllerResetState();

#if !defined(W34N17_MUTANT_SKIP_FINAL_RESTORE)
    wm_menu_sw(WM_D804, 0u);
    wm_menu_sh(WM_EE6A, 0u);
    D_80059179 = (u8)wm_menu_lw(WM_D14C);
#endif
    wm_80075D4C();
}
