/* Retail differential, src/field/camera/camera_movement.c hand-written arm:
 * func_8008FC4C.  The production TU is linked unmodified; see
 * lib/field_retail_core.h.  g_FieldCameraMode, g_CamInterpolation and g_Scene
 * are distinct symbols on retail and PC; each is placed at its own retail
 * address (the arm reaches each only through its own name). */
#define TESTNAME "FIELD F2 MISCCAM"
enum { S_GETARG = 1 };
#include "field_retail_core.h"

#define CURACT 0x80130000u
#define RA_BASE 0x800AF930u
#define RA_SIZE 0x1A0u
#define RB_BASE 0x800B0070u
#define RB_SIZE 0x10u
#define RC_BASE 0x800B21D0u
#define RC_SIZE 0x10u
REGION(RA); REGION(RB); REGION(RC);
ALIAS(g_FieldCameraMode, RA, 0x800AF934); ALIAS(g_CamInterpolation, RA, 0x800AF984); ALIAS(g_Scene, RA, 0x800AF990);
ALIAS(D_800B21D8, RC, 0x800B21D8);
uintptr_t g_FieldScriptVMCurActor;
const Region regions[] = { { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB }, { RC_BASE, RC_SIZE, RC } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0x800B0078u, &g_FieldScriptVMCurActor } };
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

SPY(FieldScriptVMGetArgument, S_GETARG)
const Target targets[] = { { 0x800ACDEC, S_GETARG, 1, 1, 0, 0 } };
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

extern void func_8008FC4C(void);
static uint32_t n_FC4C(Args a) { (void)a; func_8008FC4C(); return 0; }
static void g_FC4C(Args* a) {
    static const uint16_t modes[] = { 0, 1, 1, 2, 3, 0xFFFF, 0x8000, 0x101 };
    (void)a;
    w32(0x800B0078u, CURACT);
    w16(CURACT + 0xCC, (uint16_t)rnd());
    w16(0x800AF934u, (rnd() % 4) ? modes[rnd_range(8)] : (uint16_t)rnd());
    if (rnd() & 1) feed[0] = 0;
}
Fn fns[] = { { "func_8008FC4C", 0x8008FC4C, 61, n_FC4C, g_FC4C, 1500, 0 } };
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
