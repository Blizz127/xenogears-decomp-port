#include "psx_memory.h"
#include "types.h"
#include "system/sound.h"

#if __has_include("cleanroom/sound_host_abi.h")
#include "cleanroom/sound_host_abi.h"
#else
extern void *g_pSoundSpuRegisters;
#endif

extern void *LZSSHeapDecompress(void *source, int flags);
extern s32 func_8002DDE4(void *image, s32 texture_mode, s32 x, s32 y,
                         s32 clut_mode, s32 clut_x, s32 clut_y);
extern int DrawSync(int mode);
extern unsigned int HeapFree(void *allocation);

void GraphicsDrawPauseLetters(int x, int y)
{
    void *image = LZSSHeapDecompress(PSX_ADDR(UINT32_C(0x8004FBD8)), 0);
    if (image == NULL)
        return;

    func_8002DDE4(image, 1, x, y, 0, 0, 0);
    DrawSync(0);
    HeapFree(image);
}

void SoundMuteAllSpuChannels(void)
{
    enum { words_per_voice = 8 };
    g_SoundControlFlags |= 0x40;
    volatile u16 *registers = (volatile u16 *)(void *)g_pSoundSpuRegisters;
    if (registers == NULL)
        return;

    for (unsigned channel = 0; channel < NUM_VOICES; ++channel) {
        volatile u16 *voice = &registers[channel * words_per_voice];
        voice[0] = 0; /* left volume */
        u16 attack_decay = voice[4];
        voice[1] = 0; /* right volume */
        voice[2] = 0; /* pitch */
        voice[5] = 0x1fdfu;
        voice[4] = (u16)((attack_decay & 0xffu) | 0x7f00u);
    }
}
