/* Retail pause support plus world-map waits:
 *   GraphicsDrawPauseLetters [0x8001FAB4,0x8001FB30)
 *   SoundMuteAllSpuChannels  [0x80037EE4,0x80037F44)
 *   wm_8007634C              [0x8007634C,0x80076594)
 *   wm_80076594              [0x80076594,0x800767D4)
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "system/controller.h"
#include "world_map_pause.h"

typedef struct WmPauseRect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} WmPauseRect;

extern volatile u16 *g_pSoundSpuRegisters;
extern short g_SoundControlFlags;
extern int D_80059488;

extern void *LZSSHeapDecompress(void *source, int flags);
extern s32 func_8002DDE4(void *image, s32 texture_mode, s32 texture_x,
                         s32 texture_y, s32 clut_mode, s32 clut_x,
                         s32 clut_y);
extern unsigned int HeapFree(void *pointer);
extern int DrawSync(int mode);
extern int Vsync(int mode);
extern int MoveImage(WmPauseRect *rect, int x, int y);
extern void PutDispEnv(void *env);
extern void PutDrawEnv(void *env);
extern int ControllerPopState(void);
extern int ControllerGetType(int port);
extern void SoundEnableAllSpuChannels(void);

#define WM_PAUSE_D7F0  UINT32_C(0x8009D7F0)
#define WM_PAUSE_BD1C  UINT32_C(0x8009BD1C)
#define WM_PAUSE_BD14  UINT32_C(0x8009BD14)
#define WM_PAUSE_CD50  UINT32_C(0x8009CD50)
#define WM_PAUSE_BD18  UINT32_C(0x8009BD18)
#define WM_PAUSE_BD10  UINT32_C(0x8009BD10)
#define WM_PAUSE_CD4C  UINT32_C(0x8009CD4C)

static u32 wm_pause_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 wm_pause_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_pause_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void wm_pause_store_native_u16(volatile u16 *base, u32 offset,
                                      u16 value)
{
    base[offset] = value;
}

void GraphicsDrawPauseLetters(int x, int y)
{
#if defined(W34N20_MUTANT_GRAPHICS_NATIVE_TWIN)
    extern u8 g_GfxPauseLettersCompressed[];
    void *source = g_GfxPauseLettersCompressed;
#else
    void *source = PSX_ADDR(UINT32_C(0x8004FBD8));
#endif
    void *decoded = LZSSHeapDecompress(source, 0);

#if defined(W34N20_MUTANT_GRAPHICS_WRONG_UPLOAD_ARGS)
    (void)func_8002DDE4(decoded, 0, x, y, 0, 0, 0);
#else
    (void)func_8002DDE4(decoded, 1, x, y, 0, 0, 0);
#endif
    (void)DrawSync(0);
#if !defined(W34N20_MUTANT_GRAPHICS_SKIP_FREE)
    (void)HeapFree(decoded);
#endif
}

void SoundMuteAllSpuChannels(void)
{
    volatile u16 *registers = g_pSoundSpuRegisters;
    u32 voice;

#if !defined(W34N20_MUTANT_MUTE_SKIP_FLAG)
    g_SoundControlFlags =
        (short)((u16)g_SoundControlFlags | UINT16_C(0x0040));
#endif
#if defined(W34N20_MUTANT_MUTE_23_VOICES)
    for (voice = 0u; voice < 23u; voice++) {
#else
    for (voice = 0u; voice < 24u; voice++) {
#endif
        u32 base = voice * 8u;
        u8 old_adsr1_low;

        /* Preserve retail's observable MMIO write/read order. */
        wm_pause_store_native_u16(registers, base, 0u);
        old_adsr1_low = (u8)registers[base + 4u];
        wm_pause_store_native_u16(registers, base + 1u, 0u);
        wm_pause_store_native_u16(registers, base + 2u, 0u);
        wm_pause_store_native_u16(registers, base + 5u,
                                  UINT16_C(0x1FDF));
#if defined(W34N20_MUTANT_MUTE_ZERO_ADSR1)
        (void)old_adsr1_low;
        wm_pause_store_native_u16(registers, base + 4u, UINT16_C(0x7F00));
#else
        wm_pause_store_native_u16(registers, base + 4u,
                                  (u16)(UINT16_C(0x7F00) + old_adsr1_low));
#endif
    }
}

static void wm_pause_clear_and_drain_inputs(void)
{
    int popped;

#if !defined(W34N20_MUTANT_WAIT_SKIP_CLEAR)
    wm_pause_sh(WM_PAUSE_BD1C, 0u);
    wm_pause_sh(WM_PAUSE_BD14, 0u);
    wm_pause_sh(WM_PAUSE_CD50, 0u);
    wm_pause_sh(WM_PAUSE_BD18, 0u);
    wm_pause_sh(WM_PAUSE_BD10, 0u);
    wm_pause_sh(WM_PAUSE_CD4C, 0u);
#endif
    do {
        popped = ControllerPopState();
        if (popped != 0) {
            wm_pause_sh(WM_PAUSE_CD4C,
                        (u16)(wm_pause_lhu(WM_PAUSE_CD4C) |
                              (u16)g_C1ButtonState));
            wm_pause_sh(WM_PAUSE_CD50,
                        (u16)(wm_pause_lhu(WM_PAUSE_CD50) |
                              (u16)g_C2ButtonState));
#if defined(W34N20_MUTANT_WAIT_DROP_RELEASED_SOURCE)
            wm_pause_sh(WM_PAUSE_BD10, wm_pause_lhu(WM_PAUSE_BD10));
#else
            wm_pause_sh(WM_PAUSE_BD10,
                        (u16)(wm_pause_lhu(WM_PAUSE_BD10) |
                              (u16)g_C1ButtonStateReleased));
#endif
            wm_pause_sh(WM_PAUSE_BD14,
                        (u16)(wm_pause_lhu(WM_PAUSE_BD14) |
                              (u16)g_C2ButtonStateReleased));
            wm_pause_sh(WM_PAUSE_BD18,
                        (u16)(wm_pause_lhu(WM_PAUSE_BD18) |
                              (u16)g_C1ButtonStatePressedOnce));
            wm_pause_sh(WM_PAUSE_BD1C,
                        (u16)(wm_pause_lhu(WM_PAUSE_BD1C) |
                              (u16)g_C2ButtonStatePressedOnce));
        }
    } while (popped != 0);
}

static void wm_pause_wait(int wait_for_start)
{
    WmPauseRect rect;
    int saved_timer = D_80059488;

    (void)DrawSync(0);
    (void)Vsync(0);
    if (wm_pause_lw(WM_PAUSE_D7F0) == 0u) {
        rect.x = 0;
        rect.y = 216;
        rect.w = 320;
        rect.h = 216;
#if defined(W34N20_MUTANT_WAIT_WRONG_ENTRY_COPY)
        (void)MoveImage(&rect, 0, 216);
#else
        (void)MoveImage(&rect, 0, 0);
#endif
    }

    PutDispEnv(PSX_ADDR(UINT32_C(0x8009BC9C)));
    PutDrawEnv(PSX_ADDR(UINT32_C(0x8009BC40)));
    SoundMuteAllSpuChannels();

    for (;;) {
        (void)DrawSync(0);
        (void)Vsync(0);
        GraphicsDrawPauseLetters(136, 100);
        wm_pause_clear_and_drain_inputs();
        if (wait_for_start != 0) {
#if defined(W34N20_MUTANT_WAIT_WRONG_START_MASK)
            if ((wm_pause_lhu(WM_PAUSE_BD10) & 0x0100u) != 0u)
#else
            if ((wm_pause_lhu(WM_PAUSE_BD10) & 0x0800u) != 0u)
#endif
                break;
        } else if (ControllerGetType(0) != 0) {
            break;
        }
    }

#if !defined(W34N20_MUTANT_WAIT_SKIP_ENABLE)
    SoundEnableAllSpuChannels();
#endif
    (void)DrawSync(0);
    (void)Vsync(0);
    rect.x = 0;
    rect.y = 0;
    rect.w = 320;
    rect.h = 216;
    (void)MoveImage(&rect, 0, 216);

    {
        u32 index = wm_pause_lw(WM_PAUSE_D7F0);
#if defined(W34N20_MUTANT_WAIT_WRONG_ENV_STRIDE)
        PutDispEnv(PSX_ADDR(UINT32_C(0x8009BC24) + index * 0x40u));
#else
        PutDispEnv(PSX_ADDR(UINT32_C(0x8009BC24) + index * 0x78u));
#endif
    }
    {
        u32 index = wm_pause_lw(WM_PAUSE_D7F0);
#if defined(W34N20_MUTANT_WAIT_WRONG_ENV_STRIDE)
        PutDrawEnv(PSX_ADDR(UINT32_C(0x8009BBC8) + index * 0x40u));
#else
        PutDrawEnv(PSX_ADDR(UINT32_C(0x8009BBC8) + index * 0x78u));
#endif
    }
    D_80059488 = saved_timer;
}

void wm_8007634C(void)
{
    wm_pause_wait(1);
}

void wm_80076594(void)
{
    wm_pause_wait(0);
}
