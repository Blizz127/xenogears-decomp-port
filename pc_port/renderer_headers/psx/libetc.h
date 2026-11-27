#ifndef XENO_RENDERER_LIBETC_H
#define XENO_RENDERER_LIBETC_H

#include "psx/types.h"

enum { MODE_NTSC = 0 };

#ifdef __cplusplus
extern "C" {
#endif

int GetVideoMode(void);
int SetVideoMode(int mode);
int VSync(int intervals);
int VSyncCallback(void (*callback)(void));
int ResetCallback(void);
int StopCallback(void);

void PadInit(int mode);
unsigned int PadRead(int controller);
void PadStop(void);

#ifdef __cplusplus
}
#endif
#endif
