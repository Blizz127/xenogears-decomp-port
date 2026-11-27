#ifndef XENO_RENDERER_LIBPAD_H
#define XENO_RENDERER_LIBPAD_H

/* Host SDL input writes this packet directly into the game's pad buffer. */
typedef struct PADRAW {
    unsigned char status, id;
    unsigned char buttons[2], analog[4];
} PADRAW;
typedef PADRAW *LPPADRAW;

enum { MAX_CONTROLLERS = 2 };
enum { PadStateDiscon = 0, PadStateStable = 6 };

#ifdef __cplusplus
extern "C" {
#endif

void PadInitDirect(unsigned char *first, unsigned char *second);
void PadInitMtap(unsigned char *first, unsigned char *second);
void PadStartCom(void);
void PadStopCom(void);
unsigned int PadEnableCom(unsigned int mode);
int PadChkVsync(void);
int PadGetState(int port);

void PadInitGun(unsigned char *buffer, int mode);
void PadEnableGun(unsigned char mode);
void PadRemoveGun(void);

int PadInfoAct(int port, int actuator, int query);
int PadInfoComb(int port, int combination, int query);
int PadInfoMode(int port, int query, int offset);
int PadSetMainMode(int port, int offset, int lock);
int PadSetActAlign(int port, unsigned char *alignment);
void PadSetAct(int port, unsigned char *values, int count);

#ifdef __cplusplus
}
#endif
#endif
