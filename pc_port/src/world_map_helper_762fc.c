/* Retail world-map helper [0x800762FC, 0x8007634C). */
#include "world_map_helper_762fc.h"

extern int DrawSync(int mode);
extern int VSync(int mode);
extern void EnterCriticalSection(void);
extern void FlushCache(void);
extern void ExitCriticalSection(void);

void wm_800762FC(void)
{
#if !defined(W34N13_MUTANT_SKIP_FIRST_DRAW_SYNC)
    (void)DrawSync(0);
#endif
#if defined(W34N13_MUTANT_WRONG_FIRST_VSYNC_ARG)
    (void)VSync(1);
#else
    (void)VSync(0);
#endif
#if !defined(W34N13_MUTANT_SKIP_ENTER_CRITICAL)
    EnterCriticalSection();
#endif
#if defined(W34N13_MUTANT_SWAP_SECOND_SYNCS)
    (void)VSync(0);
    (void)DrawSync(0);
#else
    (void)DrawSync(0);
    (void)VSync(0);
#endif
#if !defined(W34N13_MUTANT_SKIP_FLUSH_CACHE)
    FlushCache();
#endif
#if !defined(W34N13_MUTANT_SKIP_EXIT_CRITICAL)
    ExitCriticalSection();
#endif
}
