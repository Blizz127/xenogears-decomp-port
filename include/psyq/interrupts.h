#ifndef XENO_MATCHING_INTERRUPTS_H
#define XENO_MATCHING_INTERRUPTS_H

/* Data views used by src/slus_006.64/psyq/libetc. See the interrupt symbols
 * in config/symbol_addrs.slus_006.64.txt and the interior aliases in Makefile. */
#include "kernel.h"

typedef void (*VoidCallback_t)(void);
void InterruptCallback(unsigned int irq, VoidCallback_t callback);

/* Both callback lists are provided by the linker. The registration and
 * dispatch loops in intr_vsync.c and intr_dma.c supply their bounds. */
enum { MAX_INTERRUPT_CALLBACKS = 8 };
typedef VoidCallback_t XenoInterruptCallbackTable[MAX_INTERRUPT_CALLBACKS];
extern XenoInterruptCallbackTable g_DmaInterruptCallbacks, g_VsyncInterruptCallbacks;

/* trapIntrVSync advances this count asynchronously. */
extern volatile int g_VsyncInterruptCount;

/* The flag at +2 and saved state at +0x30 have their own linker symbols.
 * The exception record at +0x38 is the BIOS setjmp context (SP at word 1).
 * https://psx-spx.consoledev.net/kernelbios/#a13h-setjmpbuf */
enum { JB_SP = 1 };
extern struct XenoInterruptEnvironment {
    unsigned short interruptsInitialized;
    VoidCallback_t handlers[11];
    unsigned char savedInterruptState[8];
    int buf[12];
    int stack[1024];
} g_InterruptEnvironment;

/* The 0x20-byte dispatch object has six callable slots followed by the
 * environment pointer (see the table initializer recorded in intr.c). */
typedef int (*SetVsyncIntrCallback_t)(unsigned int slot, VoidCallback_t callback);
extern struct {
    const char *revision;
    VoidCallback_t (*setDMAIntrCallback)(int channel, VoidCallback_t callback);
    VoidCallback_t (*setIntrHandler)(int irq, VoidCallback_t callback);
    void *(*intrResetHandler)(void);
    void *(*intrStopHandler)(void);
    SetVsyncIntrCallback_t setVsyncIntrCallback;
    void *(*intrRestartHandler)(void);
    struct XenoInterruptEnvironment *context;
} *g_pInterruptControl;

extern volatile unsigned short *g_pI_STAT, *g_pI_MASK;
extern volatile int *g_pDMA_DPCR, *g_pDMA_DICR;
extern int D_8005893C;

/* Vertical retrace samples the GPU status, H-blank timer and its counters. */
extern volatile int *g_pGPUSTAT;
extern volatile int *g_pTMR_HRETRACE_VAL, *g_pTMR_HRETRACE_MODE;
/* Vsync saves both samples after observing the next field. */
extern int g_HsyncInterruptCount, g_VsyncPrevInterruptCount;
extern int g_VideoMode;

void ChangeClearPAD(int enable);
void *ResetEntryInt(void);
void HookEntryInt(unsigned short *context);
void ReturnFromException(void);
void trapIntr(void);
void v_wait(int count, int max);

#endif
