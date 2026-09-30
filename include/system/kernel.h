#ifndef _XENO_KERNEL_H
#define _XENO_KERNEL_H

#include "psyq/libgpu.h"

#define TRUE 1
#define FALSE 0

typedef struct {
    DRAWENV drawEnv;
    DISPENV dispEnv;
    void* ot;
    POLY_F3 cursor;
} KernelMenuRenderEnvironment;

extern int g_KernelMenuCurChoice;
extern int g_KernelMenuIsRunning;
extern KernelMenuRenderEnvironment* g_KernelMenuCurRenderEnvironment;
extern KernelMenuRenderEnvironment g_KernelMenuRenderEnvironments[2];

// Version?
extern char D_80059484;
extern char D_80059420;
extern char D_80059418;

// Is Debug?
extern int D_80010000;

extern int D_800592C4;
extern int D_800592C8; // current menu render context

#endif