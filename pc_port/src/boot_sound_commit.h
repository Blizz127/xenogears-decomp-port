#ifndef XENO_PC_BOOT_SOUND_COMMIT_H
#define XENO_PC_BOOT_SOUND_COMMIT_H

/* Commit the retail boot-time pending SPU common attributes before the
 * native host outruns the first 240 Hz sound tick. */
void PcPort_CommitPendingBootSoundCommonAttr(void);

#endif
