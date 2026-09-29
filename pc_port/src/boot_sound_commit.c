#include "common.h"
#include "system/sound.h"

#include "boot_sound_commit.h"

extern int DisableEvent(unsigned int event);
extern int EnableEvent(unsigned int event);

void PcPort_CommitPendingBootSoundCommonAttr(void)
{
    unsigned int event = (unsigned int)g_unk_SoundEvent;

    DisableEvent(event);
    if (g_SoundVolumeController.commonAttr.mask != 0) {
        /* This is the exact pending-common-attribute tail of the translated
         * retail 240 Hz sound tick (func_8003C020).  On a fast host the boot
         * reset can otherwise clear the mirror before that tick runs. */
        SpuSetCommonAttr(&g_SoundVolumeController.commonAttr);
        g_SoundVolumeController.commonAttr.mask = 0;
    }
    EnableEvent(event);
}
