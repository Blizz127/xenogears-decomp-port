#ifndef XENO_PC_SPU_REGISTER_FLUSH_H
#define XENO_PC_SPU_REGISTER_FLUSH_H

/* Register after SoundInitialize so the host consumes each completed game
 * sound tick. The caller owns the event until the audio backend shuts down. */
int PcPort_OpenSpuRegisterFlush(void);
void PcPort_CloseSpuRegisterFlush(int event);

#endif
