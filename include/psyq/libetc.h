#ifndef XENO_VIDEO_TIMING_H
#define XENO_VIDEO_TIMING_H

/* Interfaces used by main.c, the field renderer and the matching libgpu.c.
 * The desktop implementations are in PsyCross src/psx/LIBETC.C. */
#ifdef __cplusplus
extern "C" {
#endif

#define MODE_NTSC 0

int Vsync(int mode);

#ifdef XENO_PC_PORT
int ResetCallback(void);
int SetVideoMode(int mode);
int GetVideoMode(void);
#else
void *ResetCallback(void);
long SetVideoMode(long mode);
long GetVideoMode(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
