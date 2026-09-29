/*
 * psyq_compat.c - PsyQ SDK compatibility shims for the Xenogears PC port.
 *
 * build_port.sh compiles every game TU EXCEPT the decompiled PsyQ tree
 * (src/slus_006.64/psyq/* is grep -v'd out), so the game's PsyQ calls resolve
 * to PsyCross instead of the on-hardware reimplementations. Where the game's
 * symbol name/signature doesn't line up 1:1 with a PsyCross export, this file
 * provides the thin forwarding shim. Defining a symbol here removes it from the
 * auto-generated no-op stub set.
 *
 * PsyCross's public entry points (PsyX_*) and PsyQ exports (VSync, ...) are all
 * extern "C" (unmangled T symbols in libpsycross.a), so they're callable from C.
 */

#include <errno.h>    /* strtoul validation for the scripted pad schedule below */
#include "gte_normalize_table.h"
#include "../include/xg_plat/mods.h"
#pragma weak xg_plat_mods_emit
#include <stdarg.h>
#include <stdint.h>
#include "system/controller_vblank.h"
/* The vblank service (controller_vblank_service.c) is always in the port
 * link; weak here so tests that link this TU without it still link (they
 * then get no deferred vblank delivery). */
#pragma weak PcPort_ServiceVblank
#pragma weak PcPort_GetServicedVblankCount
#pragma weak PcPort_MaskVblank
#pragma weak PcPort_UnmaskVblank
#define XENO_SERVICE_VBLANK() do { if (PcPort_ServiceVblank) PcPort_ServiceVblank(); } while (0)
#include <stdio.h>
#include <stdlib.h>   /* getenv/atoi for the headless test hook below */
#include <string.h>   /* memcpy for TIM parsing */
#include <libgte.h>   /* PsyCross: pull in before libgpu.h (it uses SVECTOR) */
#include <inline_c.h>
#include <libgpu.h>   /* PsyCross: POLY_F3, setPolyF3 macro (setlen/setcode) */

/* --- PsyCross internals / exports used below (extern "C") --- */
extern void PsyX_EndScene(void);  /* GR_EndScene + GR_StoreFrameBuffer + GR_SwapWindow (SDL_GL_SwapWindow) */
extern int  VSync(int mode);      /* PsyCross frame pacing; returns vblank count. Does NOT present. */
extern void DrawAllSplits(void);  /* flush queued primitives to the GL framebuffer; no-op when none */

/* CdFlush remains unresolved: retail CD_flush resets interrupt state. */

/* Retail soft-reset teardown (libapi 0x800408F4): stop pad communication,
 * remove the pad interrupt registration, and clear the active-pad flag. The
 * host has no PSX interrupt table to unregister from, so that middle step is
 * represented by the existing host lifecycle boundary. */
void func_800408F4(void)
{
    extern void PadStopCom(void);
    extern int32_t D_80056414;

    PadStopCom();
    D_80056414 = 0;
}

/* build_port.sh excludes src/slus_006.64/psyq, so an unqualified rand() would
 * otherwise bind to the host libc and produce a different range and sequence.
 * Retail rand (0x8003FA38) uses this 32-bit LCG and returns bits 16..30.  Keep
 * the seed explicitly 32-bit: PsyQ u_long is 32-bit, while host unsigned long
 * is 64-bit on the native port. */
uint32_t g_RandomSeed;

int rand(void)
{
    uint32_t next = g_RandomSeed * UINT32_C(0x41C64E6D) + UINT32_C(0x3039);
    g_RandomSeed = next;
    return (int)((next >> 16) & UINT32_C(0x7FFF));
}

/* Retail func_80048AB0 (asm 80048AB0) is SetFogNearFar: compute DQA/DQB from
 * near Z, far Z, and projection H. PsyCross exports SetFogNearFar under that
 * name; the game calls the Xenogears symbol, so forward here. Matching-tree
 * ownership of 80048AB0 is the pre-InitGeom asm blob (not libgte.yaml yet). */
void func_80048AB0(long a, long b, long h)
{
    SetFogNearFar(a, b, h);
}

/* PsyQ ReadGeomOffset: PsyCross implements SetGeomOffset (C2_OFX = ofx<<16)
 * but not the read-back. Used by the 0xBC sub-command 0x17 (screen-center
 * delta) in animation_scripts.c. */
#include <psx/gtereg.h>
void ReadGeomOffset(long* ofxp, long* ofyp)
{
    *ofxp = C2_OFX >> 16;
    *ofyp = C2_OFY >> 16;
}

/*
 * Field C (misc2.c) uses the PSY-Q gte_* names. Matching builds expand those
 * via gtemac.h; the port TU does not, so they compiled as calls. Auto-stubs
 * were `long gte_*(void)` and never wrote the output vectors. Intro then
 * VectorNormal'd garbage and DrawOTag walked a junk OT (SIGSEGV in
 * ParsePrimitivesLinkedList). Forward to PsyCross's real GTE.
 */
void OuterProduct12(VECTOR* v0, VECTOR* v1, VECTOR* v2);
void gte_OuterProduct12(VECTOR* v0, VECTOR* v1, VECTOR* v2)
{
    OuterProduct12(v0, v1, v2);
}

SVECTOR* gte_ApplyMatrixSV(MATRIX* m, SVECTOR* v0, SVECTOR* v1)
{
    return ApplyMatrixSV(m, v0, v1);
}

int gte_RotTransPers(SVECTOR* v0, int* sxy, long* p, long* flag, long* otz)
{
    int z = RotTransPers(v0, sxy, p, flag);
    if (otz)
        *otz = z;
    return z;
}

/*
 * OpenTIM / ReadTIM: PsyCross declares these in libgpu.h but provides NO
 * implementation — they fall through to auto-generated no-op stubs that return
 * 0/NULL. FieldLoadTIMWithClut calls OpenTIM(pTimData) then ReadTIM(&tim);
 * the stubbed ReadTIM returns NULL, so the `if (pTIM)` guard fails and
 * LoadImage is never called — no CLUT or pixel data reaches host VRAM (vram[]),
 * and StoreImage/GR_ReadVRAM reads zeros.
 *
 * Implementation follows PsyCross's own GetTimInfo (src/gpu/font.h) exactly:
 * TIM format is [0]=u32 header (ID=0x10, version=0x00), [1]=u32 mode,
 * then if mode&8: [2]=u32 blocklen, [3..4]=RECT16 clut rect, [5..]=clut pixels,
 * then [N]=u32 blocklen, [N+1..N+2]=RECT16 pixel rect, [N+3..]=pixel data.
 *
 * OpenTIM stores the data pointer; ReadTIM parses it into the caller's
 * TIM_IMAGE struct and returns a pointer to it (or NULL on bad ID/version).
 */
static u_long* s_pTimData = NULL;

int OpenTIM(u_long* addr)
{
    s_pTimData = addr;
    return 0;
}

TIM_IMAGE* ReadTIM(TIM_IMAGE* timimg)
{
    u_int* rtim = (u_int*)s_pTimData;

    if (!rtim) {
        printf("[psyq_compat] ReadTIM: no TIM data (OpenTIM not called)\n");
        return NULL;
    }

    /* Check ID */
    if ((rtim[0] & 0xff) != 0x10) {
        printf("[psyq_compat] ReadTIM: bad TIM ID 0x%02x (expected 0x10)\n",
               (unsigned)(rtim[0] & 0xff));
        return NULL;
    }

    /* Check version */
    if (((rtim[0] >> 8) & 0xff) != 0x00) {
        printf("[psyq_compat] ReadTIM: bad TIM version 0x%02x\n",
               (unsigned)((rtim[0] >> 8) & 0xff));
        return NULL;
    }

    timimg->mode = rtim[1];
    rtim += 2;

    /* Clut present? */
    if (timimg->mode & 0x8) {
        timimg->cRECT16 = (RECT16*)&rtim[1];
        timimg->caddr = (u_int*)&rtim[3];
        rtim += rtim[0] >> 2;  /* advance by block length (in words) */
    } else {
        timimg->caddr = 0;
        timimg->cRECT16 = 0;
    }

    timimg->pRECT16 = (RECT16*)&rtim[1];
    timimg->paddr = (u_int*)&rtim[3];

    return timimg;
}

/*
 * SetPolyF3: PsyCross declares it (libgpu.h) but ships only the setPolyF3 macro,
 * not the function -- so the game's call falls through to a no-op auto-stub and
 * the KernelMenu cursor (a flat triangle) never gets a valid primitive tag. This
 * is the exact body of the game's own decompiled SetPolyF3 (psyq/libgpu.c).
 */
void SetPolyF3(POLY_F3* p)
{
    setPolyF3(p);   /* setlen(p, 4), setcode(p, 0x20) */
}

VECTOR* Square0(VECTOR* v0, VECTOR* v1)
{
    /* Retail 8004A414: IR loads, SQR(sf=0), then MAC stores. This also
     * preserves GTE side effects and loads all inputs before in-place writes. */
    gte_ldlvl(v0);
    gte_sqr0();
    gte_stlvnl(v1);
    return v1;
}

MATRIX* ScaleMatrixL(MATRIX* m, VECTOR* v)
{
    m->m[0][0] = (m->m[0][0] * v->vx) >> 12;
    m->m[0][1] = (m->m[0][1] * v->vx) >> 12;
    m->m[0][2] = (m->m[0][2] * v->vx) >> 12;
    m->m[1][0] = (m->m[1][0] * v->vy) >> 12;
    m->m[1][1] = (m->m[1][1] * v->vy) >> 12;
    m->m[1][2] = (m->m[1][2] * v->vy) >> 12;
    m->m[2][0] = (m->m[2][0] * v->vz) >> 12;
    m->m[2][1] = (m->m[2][1] * v->vz) >> 12;
    m->m[2][2] = (m->m[2][2] * v->vz) >> 12;

    return m;
}

static int32_t GteClampS32(int64_t value)
{
    if (value > 0x7FFFFFFFLL)
        return 0x7FFFFFFF;
    if (value < -0x80000000LL)
        return (int32_t)0x80000000u;
    return (int32_t)value;
}

/*
 * OuterProduct0 — retail Psy-Q GTE OP with sf=0 (no 12-bit shift).
 * PsyCross does not export this; without a real body, build_port.sh stubs it
 * as a no-op that returns 0 and leaves *out untouched. Integer cross product
 * of two VECTOR records; pad is not written. Matches the XENO_PC_PORT body in
 * src/slus_006.64/psyq/libgte.c (excluded from the port compile).
 */
void OuterProduct0(VECTOR* v0, VECTOR* v1, VECTOR* out)
{
    out->vx = v0->vy * v1->vz - v0->vz * v1->vy;
    out->vy = v0->vz * v1->vx - v0->vx * v1->vz;
    out->vz = v0->vx * v1->vy - v0->vy * v1->vx;
}

void OuterProduct12(VECTOR* v0, VECTOR* v1, VECTOR* v2)
{
    int32_t x0 = (int16_t)v0->vx;
    int32_t y0 = (int16_t)v0->vy;
    int32_t z0 = (int16_t)v0->vz;
    int32_t x1 = (int16_t)v1->vx;
    int32_t y1 = (int16_t)v1->vy;
    int32_t z1 = (int16_t)v1->vz;

    v2->vx = GteClampS32(((int64_t)y0 * z1 - (int64_t)z0 * y1) >> 12);
    v2->vy = GteClampS32(((int64_t)z0 * x1 - (int64_t)x0 * z1) >> 12);
    v2->vz = GteClampS32(((int64_t)x0 * y1 - (int64_t)y0 * x1) >> 12);
}

static int16_t s_InvSqrtTable[XENO_GTE_NORM_TABLE_LEN];
__attribute__((constructor)) static void s_InvSqrtTable_Init(void)
{
    XenoGteNormTableFill(s_InvSqrtTable);
}

static int32_t VectorNormalWork(int32_t x, int32_t y, int32_t z, int32_t* outX, int32_t* outY, int32_t* outZ)
{
    int32_t sx = (int16_t)x;
    int32_t sy = (int16_t)y;
    int32_t sz = (int16_t)z;
    uint32_t squared = (uint32_t)(sx * sx + sy * sy + sz * sz);
    int lzc;
    int lzcEven;
    int shift;
    int index;
    int32_t scale;

    if (squared == 0) {
        *outX = 0;
        *outY = 0;
        *outZ = 0;
        return 0;
    }

    lzc = __builtin_clz(squared);
    lzcEven = lzc & ~1;
    shift = (31 - lzcEven) >> 1;

    if (lzcEven - 24 >= 0)
        index = (int)(squared << (lzcEven - 24));
    else
        index = (int)(squared >> (24 - lzcEven));

    index -= 0x40;
    if (index < 0)
        index = 0;
    if (index >= (int)(sizeof(s_InvSqrtTable) / sizeof(s_InvSqrtTable[0])))
        index = (sizeof(s_InvSqrtTable) / sizeof(s_InvSqrtTable[0])) - 1;

    scale = s_InvSqrtTable[index];
    *outX = GteClampS32(((int64_t)scale * sx) >> shift);
    *outY = GteClampS32(((int64_t)scale * sy) >> shift);
    *outZ = GteClampS32(((int64_t)scale * sz) >> shift);
    return (int32_t)squared;
}

/* Retail entry 80048D7C..48DA8 and shared core 80048DD8..48E94.
 * Load all three full words before any output, retaining partial aliasing
 * and SQR/GPF/LZCS state; only IR loads narrow the operands to signed16. */
long VectorNormal(VECTOR* v0, VECTOR* v1)
{
    int32_t x = v0->vx, y = v0->vy, z = v0->vz;
    uint32_t squared;
    int32_t even, shift, index, scale;

    MTC2((uint32_t)x, 9);
    MTC2((uint32_t)y, 10);
    MTC2((uint32_t)z, 11);
    doCOP2(0x00a00428); /* SQR0 */
    squared = MFC2(25) + MFC2(26) + MFC2(27);
    if (squared > INT32_MAX) {
        /* The retail signed ADD traps for this domain. The native port has
         * no PS1 exception handler; fail explicitly instead of normalizing
         * an invented clamped vector or indexing outside the retail table. */
        fputs("[xeno-port] VectorNormal: retail signed ADD overflow\n", stderr);
        abort();
    }
    MTC2(squared, 30);
    even = (int32_t)MFC2(31) & ~1;
    shift = ((31 - even) >> 1) & 31; /* MIPS SRAV masks the count */
    index = even >= 24 ? (int32_t)(squared << (even - 24))
                       : (int32_t)squared >> (24 - even);
    index -= 0x40;
    /* Zero takes index -64: the LH at 80048E50 reads 80056B14, before
     * D_80056B94. Retain its actual 0x1C6C value even though output is zero. */
    scale = squared == 0 ? 0x1c6c : s_InvSqrtTable[index];
    MTC2((uint32_t)scale, 8);
    MTC2((uint32_t)x, 9);
    MTC2((uint32_t)y, 10);
    MTC2((uint32_t)z, 11);
    doCOP2(0x0190003d); /* GPF0 */
    x = (int32_t)MFC2(25) >> shift;
    y = (int32_t)MFC2(26) >> shift;
    z = (int32_t)MFC2(27) >> shift;
    v1->vx = x;
    v1->vy = y;
    v1->vz = z;
    return (int32_t)squared;
}

long VectorNormalS(VECTOR* v0, SVECTOR* v1)
{
    int32_t x, y, z;
    int32_t squared = VectorNormalWork(v0->vx, v0->vy, v0->vz, &x, &y, &z);
    v1->vx = x;
    v1->vy = y;
    v1->vz = z;
    return squared;
}

/* Retail 80048DA8..80048E94. Preserve SQR/GPF register side effects,
 * signed halfword loads and the three halfword stores (including aliasing).
 * Do not use VectorNormalWork: its host arithmetic omits GTE state changes. */
long VectorNormalSS(SVECTOR* v0, SVECTOR* v1)
{
    int32_t x = v0->vx, y = v0->vy, z = v0->vz;
    uint32_t squared;
    int32_t even, shift, index, scale;

    MTC2((uint32_t)x, 9);
    MTC2((uint32_t)y, 10);
    MTC2((uint32_t)z, 11);
    doCOP2(0x00a00428); /* SQR0 */
    squared = MFC2(25) + MFC2(26) + MFC2(27);
    if (squared > INT32_MAX) {
        /* The retail signed ADD traps for this domain. The native port has
         * no PS1 exception handler; fail explicitly instead of normalizing
         * an invented clamped vector or indexing outside the retail table. */
        fputs("[xeno-port] VectorNormalSS: retail signed ADD overflow\n", stderr);
        abort();
    }
    MTC2(squared, 30);
    even = (int32_t)MFC2(31) & ~1;
    shift = ((31 - even) >> 1) & 31; /* MIPS SRAV masks the count */
    index = even >= 24 ? (int32_t)(squared << (even - 24))
                       : (int32_t)squared >> (24 - even);
    index -= 0x40;
    /* Zero takes index -64: the LH at 80048E50 reads 80056B14, before
     * D_80056B94. Retain its actual 0x1C6C value even though output is zero. */
    scale = squared == 0 ? 0x1c6c : s_InvSqrtTable[index];
    MTC2((uint32_t)scale, 8);
    MTC2((uint32_t)x, 9);
    MTC2((uint32_t)y, 10);
    MTC2((uint32_t)z, 11);
    doCOP2(0x0190003d); /* GPF0 */
    x = (int32_t)MFC2(25) >> shift;
    y = (int32_t)MFC2(26) >> shift;
    z = (int32_t)MFC2(27) >> shift;
    v1->vx = (int16_t)x;
    v1->vy = (int16_t)y;
    v1->vz = (int16_t)z;
    return (int32_t)squared;
}

long RotAverage4(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2, SVECTOR* v3,
                 long* sxy0, long* sxy1, long* sxy2, long* sxy3,
                 long* p, long* flag)
{
    long flag0 = 0;
    long flag1 = 0;
    long interpolation = 0;
    long otz = 0;

    gte_ldv3(v0, v1, v2);
    gte_rtpt();
    gte_stsxy3(sxy0, sxy1, sxy2);
    gte_stflg(&flag0);

    gte_ldv0(v3);
    gte_rtps();
    gte_stsxy(sxy3);
    gte_stflg(&flag1);
    gte_stdp(&interpolation);
    /* Retail 8004A81C/824 writes IR0 first, then both projection FLAGs.
     * Particle rendering aliases p/flag, so the final FLAG must win. These
     * are host long outputs; guest calls narrow them at the ABI boundary. */
    *p = (long)(int32_t)(uint32_t)interpolation;
    *flag = (long)(int32_t)((uint32_t)flag0 | (uint32_t)flag1);

    gte_avsz4();
    gte_stotz(&otz);

    /* Retail returns OTZ in v0; it does not overwrite either output. */
    return (long)(int32_t)(uint32_t)otz;
}

/* Mask deferred game-thread vblank delivery. This does not couple arbitrary
 * critical sections to the sound mutex: sound transfer writes retain their
 * dedicated gate. Other native interrupt sources still need separate audit. */
int EnterCriticalSection(void)
{
    return PcPort_MaskVblank ? PcPort_MaskVblank() : 1;
}

void ExitCriticalSection(void)
{
    if (PcPort_UnmaskVblank)
        PcPort_UnmaskVblank();
}

int CdDataSync(int mode)
{
    (void)mode;
    return 0;
}

/*
 * Sprintf (PsyQ): the game's variadic string formatter (KernelMenu builds its
 * menu text with it). Forward to libc vsprintf. Signature matches the game's
 * include/system/memory.h declaration.
 */
int Sprintf(char* dest, char* fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsprintf(dest, fmt, ap);
    va_end(ap);
    return n;
}

/*
 * Vsync (game spelling, lowercase 's'; ELF symbol `Vsync` @ 0x8004b54c) vs PsyQ
 * VSync. On PSX, VSync(0) waits for vblank, at which point the GPU has finished
 * and the displayed framebuffer flips. PsyCross's VSync only paces/returns the
 * count -- the actual GL backbuffer swap lives in PsyX_EndScene, which nothing
 * on the boot path calls per frame (only ResetGraph does, and the game state
 * loops don't re-enter it). So the screen never updates after the first frame.
 *
 * Presenting here makes Vsync the once-per-frame swap point, which is exactly
 * the PSX display-flip semantics and robust against game code that issues
 * multiple DrawOTag()s per frame (presenting in DrawOTag would swap mid-frame).
 * PsyX_EndScene() is a no-op when no scene is open (begin_scene_flag == 0), so
 * the early boot Vsync(2) before any DrawOTag is safe.
 */
/*
 * Headless test hook for the KernelMenu. Synthetic X11/SDL key events can't reach
 * the focused XWayland window in automated runs, so the only way to drive the menu
 * from a script is to inject the game's button state in code. With XENO_KERNEL_SEL
 * set to an option index (Field=0, Battle=1, Worldmap=2, Battling=3, Menu=4,
 * Movie=5) this presses Circle on that option once, XENO_KERNEL_DELAY frames (def
 * 60) after the KernelMenu becomes active -- exercising ChangeGameState ->
 * MainLoop's overlay load (LoadGameStateOverlay -> ArchiveReadFileToBuffer ->
 * LZSSDecompress) without a human at the keyboard.
 *
 * Normal (non-field-test) boots no longer use KernelMenu -- PcPort_BootMain owns
 * title/movie-skip/new-game and enters Field itself -- so this hook stays idle
 * unless XENO_FIELD_TEST=1 (or an explicit XENO_KERNEL_SEL) is set.
 */
static void PcPort_ForcedKernelSelect(void)
{
    extern int g_KernelMenuCurChoice;
    extern int g_KernelMenuIsRunning;
    extern unsigned short g_C1ButtonStateReleased;
    static int sel = -2, delay, frame;

    if (sel == -2) {  /* first call: read config */
        const char* e = getenv("XENO_KERNEL_SEL");
        const char* d = getenv("XENO_KERNEL_DELAY");
        const char* ft = getenv("XENO_FIELD_TEST");
        if (e && *e) {
            sel = atoi(e);        /* explicit menu drive (field-test or manual) */
        } else if (ft && ft[0] == '1') {
            sel = -1;             /* field-test w/o an explicit choice: leave the
                                   * debug KernelMenu up for interactive use */
        } else {
            sel = -1;             /* NORMAL BOOT: KernelMenu is not the boot state
                                   * (PcPort_BootMain handles Field entry). */
        }
        delay = (d && *d) ? atoi(d) : 60;
        frame = 0;
    }
    if (sel < 0 || !g_KernelMenuIsRunning)
        return;  /* disabled, or menu not up yet -- don't start the countdown */

    if (++frame == delay) {
        g_KernelMenuCurChoice = sel;
        g_C1ButtonStateReleased |= 0x20;  /* CTRL_BTN_CIRCLE */
        printf("[xeno-port][test] forcing KernelMenu select %d (Circle)\n", sel);
    }
}

/*
 * Headless RESOURCED main-menu driver (no-op unless XENO_MENU_FORCE=1).
 * XENO_KERNEL_SEL=4 dispatches the menu but never loads its resources
 * (g_MenuDebugEnabled=0, D_8005945C=NULL), so no textured content can build.
 * This instead forces the FIELD menu-opener path: once the field is up and
 * idle, set D_800ADB64=0x80 (main menu; 0x80 & 0x7F -> D_80059460=0), mimicking
 * the menu button (field main.c:624). The field loop's trigger (main.c:618)
 * then calls func_800799D4, which streams the menu resources (D_8005945C =
 * archive file 1) and runs MenuMain -- the same resourced path member_change
 * was validated on (the map005 repro). A code-side write of the real global is
 * reliable, unlike gdb symbol-writes (which hit a native-layout phantom view).
 */
static int s_xenoMenuForceFired = 0;

/* N2c-2b target-navigation flavors of the prompt test: 0 = plain prompt,
 * 1 = wraparound (DOWN x3 / UP x3 on the standard party), 2 = ineligible-skip
 * (DOWN x2 on the 2b-test-only two-member party).  Declared ahead of
 * PcPort_ForcedFieldMenu, whose seed consults it. */
static int s_xenoMenuPromptNavKind = 0;

static void PcPort_ForcedFieldMenu(void)
{
    extern int D_800ADB64;   /* menu request (0xFF = none); s32 in-game */
    extern int D_800ADB68;   /* playerCanRun -- field is up + idle; s32 in-game */
    static int armed = -2, frame = 0, delay = 0;
#define fired s_xenoMenuForceFired

    if (armed == -2) {  /* first call: read config */
        const char* e = getenv("XENO_MENU_FORCE");
        const char* d = getenv("XENO_MENU_FORCE_DELAY");
        armed = (e && e[0] == '1') ? 1 : 0;
        delay = (d && *d) ? atoi(d) : 90;
        frame = 0;
    }
    if (!armed || fired)
        return;
    if (++frame < delay)
        return;
    if (D_800ADB64 == 0xFF && D_800ADB68 == 1) {
        /* Equip ONLY (XENO_MENU_NAV_TEST=equip): XENO_FIELD_TEST=1 skips
         * func_8001BB50, leaving character stats at zero so func_801D8644's
         * bar scaler sees maxVal==0 and returns.  Load the retail new-game
         * template via func_8001B970 when Fei still has no HP.  TEST TOOLING
         * -- remove with the Equip acceptance harness. */
        {
            const char* nav = getenv("XENO_MENU_NAV_TEST");
            if (nav && strcmp(nav, "equip") == 0) {
                extern unsigned char g_GameState[];

                if (*(unsigned short*)&g_GameState[0x2B8] == 0) {
                    extern void func_8001B970(void);
                    extern unsigned char D_800594CC;

                    func_8001B970();
                    /* B970 sets D_800594CC=6; func_801C6AA0 restores that as
                     * menu1Choice, so 2x DOWN lands on Items (4) not Equip. */
                    D_800594CC = 0;
                    g_GameState[0x1D34] = 0;
                    g_GameState[0x1D35] = 0xFF;
                    g_GameState[0x1D36] = 0xFF;
                    *(unsigned short*)&g_GameState[0x1D30] = 1;
                    *(unsigned short*)&g_GameState[0x1D32] = 0xFFFF;
                    printf("[xeno-port][test] EQUIP SEED: func_8001B970 "
                           "(new-game template; FIELD_TEST skips BB50), "
                           "D_800594CC=0, Fei-only roster\n");
                    /* TEST TOOLING: dump retail-flat equip/inventory bytes. */
                    printf("[xeno-port][test] EQUIP SEED dump: "
                           "eq@2D6=");
                    {
                        int ei;
                        for (ei = 0; ei < 16; ei++)
                            printf("%u%s", g_GameState[0x2D6 + ei],
                                   ei == 15 ? "" : " ");
                    }
                    printf(" wpnIds=%u,%u,%u,%u wpnQty=%u,%u,%u,%u "
                           "accIds=%u,%u,%u,%u\n",
                           g_GameState[0x1D9C], g_GameState[0x1D9D],
                           g_GameState[0x1D9E], g_GameState[0x1D9F],
                           g_GameState[0x1D38], g_GameState[0x1D39],
                           g_GameState[0x1D3A], g_GameState[0x1D3B],
                           g_GameState[0x1EC8], g_GameState[0x1EC9],
                           g_GameState[0x1ECA], g_GameState[0x1ECB]);
                    /* Also dump HP/level to prove template landed. */
                    printf("[xeno-port][test] EQUIP SEED char0 "
                           "hp=%u/%u lv=%u at=%u df=%u\n",
                           *(unsigned short*)&g_GameState[0x2B8],
                           *(unsigned short*)&g_GameState[0x2BA],
                           g_GameState[0x2CE],
                           g_GameState[0x2C4], g_GameState[0x2C5]);
                    fflush(stdout);
                }
            }
        }
        /* HARNESS-ONLY availability setup.  Retail at the established Map1
         * party-init anchor has roster {0,2,FF}, mask 0x0005, FrMask 0xFFFF:
         * derive the mask from valid roster IDs so XENO_MENU_FORCE preserves
         * the bootstrap stand-in instead of replacing it.  The Fei-only
         * fallback remains solely for genuinely cold all-zero/all-FF rosters.
         * g_GameState offsets: 0x1D30 mask, 0x1D32 FrMask, 0x1D34 members[3]. */
        {
            extern unsigned char g_GameState[];  /* PSX-layout data blob */
            unsigned short* pMask = (unsigned short*)&g_GameState[0x1D30];
            unsigned short* pFrMask = (unsigned short*)&g_GameState[0x1D32];
            unsigned char* pMembers = &g_GameState[0x1D34];

            unsigned short rosterMask = 0;
            int coldRoster =
                ((pMembers[0] == 0 && pMembers[1] == 0 && pMembers[2] == 0) ||
                 (pMembers[0] == 0xFF && pMembers[1] == 0xFF &&
                  pMembers[2] == 0xFF));
            int i;

            if (coldRoster) {
                pMembers[0] = 0;      /* Fei */
                pMembers[1] = 0xFF;
                pMembers[2] = 0xFF;
            }
            for (i = 0; i < 3; i++) {
                if (pMembers[i] < 11)
                    rosterMask |= (unsigned short)(1U << pMembers[i]);
            }
            *pMask |= rosterMask;
            *pFrMask = 0xFFFF;
            printf("[xeno-port][test] XENO_MENU_FORCE: roster=%u/%u/%u "
                   "availability=%04x FrMask=%04x fallback=%s\n",
                   pMembers[0], pMembers[1], pMembers[2], *pMask, *pFrMask,
                   coldRoster ? "Fei-only" : "preserved");
        }
        /* The same cold boot also bypasses the intro/save inventory setup.
         * Seed a single scrollable, retail-valid Items list only when every
         * item slot is empty.  IDs still resolve through the loaded game name
         * bank and quantities still use the real two-glyph renderer; this only
         * supplies the state a reachable retail menu would already have.
         * g_GameState offsets: 0x1F90 quantities[150], 0x2026 IDs[150]. */
        {
            extern unsigned char g_GameState[];  /* PSX-layout data blob */
            unsigned char* pQuantities = &g_GameState[0x1F90];
            unsigned char* pItemIds = &g_GameState[0x2026];
            int hasItem = 0;
            int i;

            for (i = 0; i < 150; i++) {
                if (pItemIds[i] != 0) {
                    hasItem = 1;
                    break;
                }
            }
            if (!hasItem) {
                for (i = 0; i < 18; i++) {
                    pItemIds[i] = (unsigned char)(i + 1);
                    pQuantities[i] = (unsigned char)(i + 11);
                }
                printf("[xeno-port][test] XENO_MENU_FORCE: seeded harness "
                       "inventory (18 real item IDs, quantities 11..28) -- "
                       "cold boot had no save inventory\n");
            }
        }
        /* N2c-3 ONLY (XENO_MENU_NAV_TEST=item-use): give the effect proof an
         * observable target and a depletion case.  Fei is damaged (hp 20 of
         * maxHp 50) so the seeded row-0 item (ID 1, an ordinary HP restore,
         * +mag*50 clamped to maxHp) produces a visible 20 -> 50 delta AND
         * exercises the clamp; quantity is forced to 1 so one use depletes
         * the slot (DB920 decrements, clears the ID at zero, and the prompt
         * auto-closes on the depletion exit).  GameCharacter offsets: char 0
         * hp @ 0x26C+0x4C = 0x2B8, maxHp @ 0x2BA.  Isolated to this mode. */
        if (s_xenoMenuPromptNavKind == 3) {
            extern unsigned char g_GameState[];

            *(unsigned short*)&g_GameState[0x2B8] = 20;  /* Fei hp */
            *(unsigned short*)&g_GameState[0x2BA] = 50;  /* Fei maxHp */
            g_GameState[0x1F90] = 1;                     /* quantity[0] = 1 */
            printf("[xeno-port][test] N2C3 SEED: Fei hp=20/50, row-0 item "
                   "(ID %d) quantity=1\n", (int)g_GameState[0x2026]);
            fflush(stdout);
        }
        /* N2c-4 ONLY (XENO_MENU_NAV_TEST=item-special): row 0 becomes the
         * MAGNITUDE-1 bulk special (item ID 33, verified mag=1 via the DB
         * probe -- ID 34 is mag=2 and would route to the PARKED E5178) at
         * quantity 1.  The run is a disposable cold boot; nothing persists.
         * Isolated to this mode. */
        if (s_xenoMenuPromptNavKind == 4) {
            extern unsigned char g_GameState[];

            g_GameState[0x2026] = 33;   /* itemIDs[0] = the bulk special */
            g_GameState[0x1F90] = 1;    /* quantity 1 -> depletion path */
            printf("[xeno-port][test] N2C4 SEED: row-0 item = ID 33 "
                   "(magnitude-1 special), quantity 1\n");
            fflush(stdout);
        }
        /* N3a-A1b-1 ONLY (XENO_MENU_NAV_TEST=abilities): the cold-boot
         * harness state has Fei with NO known Ether abilities and zeroed MP
         * (new-game init never runs on this boot), which would leave the
         * func_801DC3D8 category-0 arm unexercised -- every row absent.
         * Seed the per-character ability-known bitfield (GameState+0x16C2,
         * charId * 0x20) and Fei's MP so six rows build for real.  The row
         * strings and MP costs remain genuine game data (string bundle +
         * C72BC mode-2 blob); only the known-mask and MP are harness
         * conveniences.  Fires before the STATE-BEFORE hash, so the
         * read-only assertion is unaffected.  Isolated to this mode. */
        if (s_xenoMenuPromptNavKind == 5) {
            extern unsigned char g_GameState[];

            *(unsigned short*)&g_GameState[0x16C2] = 0xFC00; /* rows 0..5 */
            *(unsigned short*)&g_GameState[0x2BC] = 8;       /* Fei mp */
            *(unsigned short*)&g_GameState[0x2BE] = 12;      /* Fei maxMp */
            printf("[xeno-port][test] N3A SEED: Fei ability-known mask "
                   "0xFC00 (D_801E96C8[i] = 0x8000>>i, so bits 15..10 = "
                   "rows 0..5), mp=8, maxMp=12 (cold boot has none/zero)\n");
            fflush(stdout);
        }
        D_800ADB64 = 0x80;   /* request the field main menu via the opener */
        fired = 1;
        printf("[xeno-port][test] XENO_MENU_FORCE: requesting field main menu "
               "(D_800ADB64=0x80) at frame %d\n", frame);
    }
#undef fired
}

/* DIAGNOSTIC / TEST TOOLING -- controller-level scripted input.
 *
 * XENO_PAD_TEST_INPUT="frame:value,frame:value,..." -- same schedule syntax as
 * XENO_FIELD_TEST_INPUT / XENO_WORLD_TEST_INPUT (pc_port/src/test_input.c):
 * first frame must be 0, boundaries strictly increase, `value` is a held-button
 * mask that stays in effect until the next boundary.  Values are in the game's
 * CTRL_BTN_* space (include/system/controller.h): 0x40 Cross, 0x20 Circle,
 * 0x10 Triangle, 0x80 Square, 0x800 Start, 0x1000/0x2000/0x4000/0x8000
 * Up/Right/Down/Left.  Accepts 0x-prefixed or decimal values.
 *
 * Why this exists instead of reusing the field/world schedules: those merge at
 * the field's and world's per-frame accumulator seams, which only tick from
 * their own main loops.  MenuMain() is called SYNCHRONOUSLY from inside the
 * field menu opener func_800799D4 (src/field/main/misc4.c), so while a menu is
 * up neither loop runs and neither schedule advances -- there was no headless
 * way to CLOSE a menu.  This one injects at the RAW BIOS pad buffer
 * (g_C1Buffer), which every input path in the game ultimately reads through
 * ControllerPoll(), so it works in the field, the world map, the menus, and
 * anywhere else the Vsync shim below is reached.
 *
 * The pad buffer is ACTIVE LOW (idle = 0xFF); ControllerGetButtonState()
 * returns (~buf[3] & 0xFF) | ((buf[2] << 8) ^ 0xFF00), so game bits 0x00FF live
 * in buf[CONTROLLER_BUTTONS_2] and bits 0xFF00 in buf[CONTROLLER_BUTTONS_1].
 * Pressing == clearing the bit, which is exactly what a real key does, so the
 * synthetic hold flows through ControllerPoll -> the derived edge globals ->
 * ControllerPushState -> the queue drained by the menu/field readers.  Because
 * we only CLEAR bits, a real keypress on the same frame still registers.
 *
 * The frame clock is Vsync-shim calls, i.e. presented frames -- deliberately
 * NOT the field frame counter, so a schedule keeps advancing across the
 * open/close phases that reset the field's own cadence.
 *
 * Frame numbers are logged on every step transition so a capture can be lined
 * up against the schedule. */
/* Same capacity as the field schedule (FIELD_TEST_INPUT_MAX_STEPS): the
 * title -> New Game -> prologue smoke needs several hundred Circle pulses
 * to page through the Map 4 narration, which does not fit in 256 steps. */
#define PAD_TEST_INPUT_MAX_STEPS 4096

static struct { unsigned int frame; unsigned short value; }
    s_padTestSteps[PAD_TEST_INPUT_MAX_STEPS];
static int s_padTestStepCount;
static int s_padTestCurrentStep;
static unsigned int s_padTestFrame;
static int s_padTestState = -1;   /* -1 unparsed, 0 disabled, 1 enabled */
/* TEST TOOLING: PadOnControl sets this so a lingering title/prologue Circle
 * mash cannot fight Triangle/Equip nav after free-roam unlock. Remove with
 * XENO_PAD_ON_CONTROL. */
static int s_padTestSuppress;

static int pad_test_input_error(const char* reason)
{
    fprintf(stderr, "[pad-test-input] invalid XENO_PAD_TEST_INPUT: %s\n",
            reason);
    s_padTestStepCount = 0;
    s_padTestState = 0;
    return -1;
}

static int PcPort_PadTestInputInit(void)
{
    const char* schedule;
    const char* cursor;
    unsigned long previousFrame = 0;

    s_padTestState = 0;
    schedule = getenv("XENO_PAD_TEST_INPUT");
    if (schedule == NULL)
        return 0;
    if (*schedule == '\0')
        return pad_test_input_error("empty schedule");

    cursor = schedule;
    while (*cursor != '\0') {
        char* end;
        unsigned long frame;
        unsigned long value;

        if (s_padTestStepCount == PAD_TEST_INPUT_MAX_STEPS)
            return pad_test_input_error("too many frame/value pairs");
        errno = 0;
        frame = strtoul(cursor, &end, 10);
        if (errno != 0 || end == cursor || *end != ':')
            return pad_test_input_error("invalid frame boundary");
        cursor = end + 1;
        errno = 0;
        value = strtoul(cursor, &end, 0);
        if (errno != 0 || end == cursor || value > 0xFFFFu)
            return pad_test_input_error("invalid input value");
        if (*end != '\0' && *end != ',')
            return pad_test_input_error("expected comma between pairs");
        if (s_padTestStepCount == 0 && frame != 0)
            return pad_test_input_error("first frame must be zero");
        if (s_padTestStepCount != 0 && frame <= previousFrame)
            return pad_test_input_error("frame boundaries must increase");

        s_padTestSteps[s_padTestStepCount].frame = (unsigned int)frame;
        s_padTestSteps[s_padTestStepCount].value = (unsigned short)value;
        s_padTestStepCount++;
        previousFrame = frame;
        if (*end == '\0')
            break;
        cursor = end + 1;
        if (*cursor == '\0')
            return pad_test_input_error("trailing comma");
    }

    s_padTestState = 1;
    fprintf(stderr, "[pad-test-input] enabled steps=%d\n", s_padTestStepCount);
    return 0;
}

/* Weak: standalone-TU tests link this file without the game tables; then
 * there is no remap to undo (PcPort_PadTestGameToPhysical returns the value). */
extern unsigned char g_ControllerButtonMappings[8];
extern unsigned short g_ControllerButtonMasks[8];
#pragma weak g_ControllerButtonMappings
#pragma weak g_ControllerButtonMasks

static unsigned short PcPort_PadTestGameToPhysical(unsigned short game)
{
    unsigned short remapped = 0, physical;
    int i;

    if (g_ControllerButtonMappings == NULL || g_ControllerButtonMasks == NULL)
        return game;

    for (i = 0; i < 8; i++)
        remapped |= g_ControllerButtonMasks[i];
    physical = game & (unsigned short)~remapped;
    for (i = 0; i < 8; i++)
        if (game & g_ControllerButtonMasks[g_ControllerButtonMappings[i] & 7])
            physical |= g_ControllerButtonMasks[i];
    return physical;
}

/* Must run AFTER PsyX_UpdateInput() refreshes g_C1Buffer from SDL and BEFORE
 * ControllerPoll() derives the edge state from it. */
static void PcPort_PadTestInputInject(void)
{
    extern unsigned char g_C1Buffer[];
    unsigned short value;
    int previousStep;

    if (s_padTestState < 0)
        PcPort_PadTestInputInit();
    if (s_padTestState != 1)
        return;

    /* XENO_PAD_TEST_STOP_FIELD=<map>: retire the schedule for good once that
     * field is loaded.
     *
     * The boot schedules end with a Circle pulse train thousands of frames
     * long (the Lahan one mashes Circle every 2 frames from frame 3800 to
     * 11962) because paging the prologue narration needs it.  That train does
     * not stop when the prologue does, and in a FIELD Circle is talk/confirm:
     * it re-triggers interaction continuously and the player cannot walk.
     * Measured in field 14 -- the player actor sat parked on the free-control
     * opcode 0x0C for 4628 dispatches while all 32 direction trials moved him
     * exactly 0 units.  A driver that wants to walk has to be able to take the
     * pad back, and only the schedule's owner knows when. */
    {
        static int stopField = -2;
        extern int g_GameSceneMapNum;

        if (stopField == -2) {
            const char* e = getenv("XENO_PAD_TEST_STOP_FIELD");
            stopField = (e != NULL && e[0] != '\0') ? atoi(e) : -1;
        }
        if (stopField >= 0 && !s_padTestSuppress &&
            (g_GameSceneMapNum & 0xFFF) == stopField) {
            s_padTestSuppress = 1;
            printf("[xeno-port][test] XENO_PAD_TEST_STOP_FIELD: field %d "
                   "reached at frame %u; schedule retired\n",
                   stopField, s_padTestFrame);
            fflush(stdout);
        }
    }

    previousStep = s_padTestCurrentStep;
    while (s_padTestCurrentStep + 1 < s_padTestStepCount &&
           s_padTestFrame >= s_padTestSteps[s_padTestCurrentStep + 1].frame)
        s_padTestCurrentStep++;
    value = s_padTestSuppress ? 0 : s_padTestSteps[s_padTestCurrentStep].value;

#if defined(XENO_PAD_TEST_INPUT_MUTANT_NO_INJECT)
    /* Deliberate mutant control: parse and log the schedule but never touch
     * the pad buffer, so nothing the schedule asks for can happen. */
    value = 0;
#endif

    /* Schedule values are game (post-remap) CTRL_BTN_* bits, as documented
     * above, but the buffer holds the PHYSICAL pad.  ControllerInit's retail
     * remap (g_ControllerButtonMappings; the USA layout swaps Circle/Cross
     * and Triangle/Square) turns physical button i into game button
     * masks[map[i]], so press the physical button that the active layout
     * maps to each requested game button. */
    value = PcPort_PadTestGameToPhysical(value);

    /* CONTROLLER_BUTTONS_1 == 0x2 carries game bits 0xFF00,
     * CONTROLLER_BUTTONS_2 == 0x3 carries game bits 0x00FF, both active low. */
    g_C1Buffer[0x2] &= (unsigned char)~(unsigned char)(value >> 8);
    g_C1Buffer[0x3] &= (unsigned char)~(unsigned char)(value & 0xFF);

    if (s_padTestFrame == 0 || s_padTestCurrentStep != previousStep) {
        printf("[xeno-port][test] XENO_PAD_TEST_INPUT: frame=%u held=0x%04x "
               "(pad buf[2]=%02x buf[3]=%02x)\n",
               s_padTestFrame, (unsigned int)value,
               (unsigned int)g_C1Buffer[0x2], (unsigned int)g_C1Buffer[0x3]);
        fflush(stdout);
    }
    s_padTestFrame++;
}

/* TEST TOOLING -- set by func_800799D4 just before MenuMain when the field
 * menu actually opens (not when D_800B21D0 blocks the opener).  PadOnControl
 * waits for this instead of trusting ADB64, which main.c clears even on the
 * early-return path.  Remove with XENO_PAD_ON_CONTROL. */
int g_PcPortFieldMenuOpened;


/* Guest busy-waits (including battle pause) may never call Vsync. Use the
 * same counter epoch as the presentation path, not a second input producer. */
void PcPort_PadVblankPump(void)
{
    XENO_SERVICE_VBLANK();
}

/* TEST TOOLING -- natural field-menu driver after free-roam unlock.
 *
 * XENO_PAD_ON_CONTROL=equip: post-opening-battle, wait for ADB68==1 with
 * dialog gate D_800B21D0[0]==0 (misc4 early-returns and main clears ADB64
 * when a dialog is busy).  Then store ADB64=0x80 (retail Triangle path) and
 * wait for g_PcPortFieldMenuOpened before Equip nav.  While dialogs block,
 * pulse Circle.  Remove with the natural Equip acceptance harness. */
static void PcPort_PadOnControl(void)
{
    extern unsigned char g_C1Buffer[];
    extern int D_800ADB64;
    extern int D_800ADB68;
    extern unsigned char D_800B21D0;
    extern int g_PcPortFieldMenuOpened;
    extern int g_GameSceneMapNum;
    static int armed = -2;
    static int prev68 = -1;
    static int phase = 0; /* 0 hunt menu open, 1 Equip nav timeline */
    static int tick = 0;
    static int tri_pulses = 0;
    static int seen_run = 0;
    static int clearTick = 0;
    unsigned short hold = 0;
    int mapId;

    if (armed == -2) {
        const char* e = getenv("XENO_PAD_ON_CONTROL");
        armed = (e && strcmp(e, "equip") == 0) ? 1 : 0;
    }
    if (!armed)
        return;

    mapId = g_GameSceneMapNum & 0xFFF;
    if (D_800ADB68 != prev68) {
        printf("[xeno-port][test] XENO_PAD_ON_CONTROL: D_800ADB68 %d -> %d "
               "ADB64=%d B21D0=%u map=%d suppress=%d phase=%d\n",
               prev68, D_800ADB68, D_800ADB64, (unsigned)D_800B21D0, mapId,
               s_padTestSuppress, phase);
        fflush(stdout);
        prev68 = D_800ADB68;
    }

    if (phase == 0) {
        if (g_PcPortFieldMenuOpened) {
            s_padTestSuppress = 1;
            phase = 1;
            tick = 0;
            printf("[xeno-port][test] XENO_PAD_ON_CONTROL: field MenuMain "
                   "opened — Equip nav\n");
            fflush(stdout);
            return;
        }
        /* Natural Lahan Equip only: post-opening-battle field 14. FIELD_TEST
         * direct-boots field 0 with ADB68==1 (run13 false arm). */
        if (mapId != 14)
            return;
        if (D_800ADB68 == 1)
            seen_run = 1;
        if (seen_run && !s_padTestSuppress) {
            s_padTestSuppress = 1;
            clearTick = 0;
            printf("[xeno-port][test] XENO_PAD_ON_CONTROL: suppressing pad "
                   "mash (post-run-control)\n");
            fflush(stdout);
        }
        if (D_800ADB68 == 1 && D_800B21D0 == 0) {
            /* Retail gates for Triangle→menu; also survive Vsync/field-body
             * sampling order by writing the same ADB64 store as main.c:662.
             * Do NOT request while D_800B21D0!=0: misc4 early-returns and
             * main.c still clears ADB64 to 0xFF. */
            hold = 0x10;
            D_800ADB64 = 0x80;
            tri_pulses++;
            if (tri_pulses <= 5 || (tri_pulses % 30) == 0) {
                printf("[xeno-port][test] XENO_PAD_ON_CONTROL: "
                       "Triangle+ADB64=0x80 pulse #%d (B21D0 clear)\n",
                       tri_pulses);
                fflush(stdout);
            }
        } else if (s_padTestSuppress) {
            clearTick++;
            if ((clearTick % 45) < 2)
                hold = 0x20; /* Circle — clear dialogs blocking B21D0/FE54 */
            if (hold && (clearTick % 45) == 1) {
                printf("[xeno-port][test] XENO_PAD_ON_CONTROL: dialog Circle "
                       "clearTick=%d B21D0=%u ADB68=%d\n",
                       clearTick, (unsigned)D_800B21D0, D_800ADB68);
                fflush(stdout);
            }
        }
        if (hold) {
            g_C1Buffer[0x2] &= (unsigned char)~(unsigned char)(hold >> 8);
            g_C1Buffer[0x3] &= (unsigned char)~(unsigned char)(hold & 0xFF);
        }
        return;
    }

    tick++;
    /* Timeline in Vsync-shim frames after MenuMain open.
     * Field main list: Status, Equip, Items, ... — cursor starts on Status.
     * One Down lands Equip; a second Down overshoots to Items (run11). */
    if (tick >= 40 && tick < 44)
        hold = 0x4000; /* Down → Equip */
    else if (tick >= 70 && tick < 74)
        hold = 0x20; /* Circle — open Equip */
    else if (tick >= 250 && tick < 254)
        hold = 0x40; /* Cross — leave Equip to main */
    else if (tick >= 310 && tick < 314)
        hold = 0x40; /* Cross — close field MenuMain */
    else if (tick >= 360)
        armed = 0; /* one-shot done */

    if (hold) {
        g_C1Buffer[0x2] &= (unsigned char)~(unsigned char)(hold >> 8);
        g_C1Buffer[0x3] &= (unsigned char)~(unsigned char)(hold & 0xFF);
        if (tick == 40 || tick == 70 || tick == 250 || tick == 310) {
            printf("[xeno-port][test] XENO_PAD_ON_CONTROL: hold=0x%04x at "
                   "tick %d\n",
                   (unsigned)hold, tick);
            fflush(stdout);
        }
    }
}

static int s_xenoMenuNavActions = 0;
static int s_xenoMenuNavDowns = 0;
static int s_xenoMenuReorderActions = 0;
static int s_xenoMenuPromptActions = 0;
static int s_xenoMenuEquipActions = 0; /* TEST TOOLING: NAV_TEST=equip */
/* TEST TOOLING: Accessories category DOWNs. Template Fei 0x2E0={1,16,92};
 * id 92 has nonempty desc bank text; 1/16 are empty stubs. */
static int s_xenoMenuEquipAccDowns = 0;
static int s_xenoMenuEquipAccDownBase = -1;
static int s_xenoMenuItemsOpenTick = -1;

/* Synthetic menu-nav edges (XENO_MENU_NAV_TEST=N): once the forced menu has
 * had time to open, hold DPAD-DOWN for a single frame, N times, ~30 hook-calls
 * apart, so an automated capture can verify the cursor moves.  The special
 * value XENO_MENU_NAV_TEST=items performs three DOWN taps (Exit -> Items), then
 * injects Circle/Cross release edges around a stable open-window interval.
 * XENO_MENU_NAV_TEST=items-reorder follows the same path, then selects row 0,
 * moves right to row 1, confirms the reorder, and closes the screen.
 * XENO_MENU_NAV_TEST=items-prompt confirms row 0 twice, holds the initial
 * target prompt without any direction input, then cancels prompt and Items.
 *
 * The inject is at the RAW BIOS pad buffer (g_C1Buffer), exactly where a real
 * keypress lands: PsyX_UpdateInput() refreshes the buffer from SDL each frame
 * (idle = 0xFF, active-low), then ControllerPoll() derives the pressed/edge/
 * repeat state the menu reads.  DPAD-DOWN is bit 0x40 of buttons byte
 * g_C1Buffer[CONTROLLER_BUTTONS_1] (== PsyX pad->buttons[0], `ret &= ~0x40`).
 * So we MUST run after PsyX_UpdateInput and before ControllerPoll; clearing the
 * bit for one frame makes ControllerPoll compute a genuine rising edge that
 * flows through ControllerPushState → the queue → the menu reader, identical to
 * a physical DOWN tap.  (An earlier attempt OR-ing the derived edge var after
 * ControllerPoll failed: the reader drains the queue, not the live var.) */
static void PcPort_ForcedMenuNav(void)
{
    extern unsigned char g_C1Buffer[];
    extern int g_XenoMenuNavReaderTicks;   /* menu reader executions (misc.c) */
    static int armed = -2, injected = 0;

    if (armed == -2) {
        const char* e = getenv("XENO_MENU_NAV_TEST");
        if (e && (strcmp(e, "abilities") == 0 ||
                  strcmp(e, "equip") == 0 ||
                  strcmp(e, "items") == 0 ||
                  strcmp(e, "items-reorder") == 0 ||
                  strcmp(e, "items-prompt") == 0 ||
                  strcmp(e, "prompt-nav") == 0 ||
                  strcmp(e, "prompt-nav-skip") == 0 ||
                  strcmp(e, "item-use") == 0 ||
                  strcmp(e, "item-special") == 0)) {
            /* N3a-A1a: DOWN DECREMENTS menu1Choice (wrapping 0..6), so the
             * established 3 taps land on Items (entry 4); Abilities is entry 3,
             * one further along the same direction -> 4 taps.  Measured, not
             * assumed: 2 taps routed to func_801E0F78 (Equip, entry 5).
             * TEST TOOLING: XENO_MENU_NAV_TEST=equip is pad-buffer only. */
            if (strcmp(e, "equip") == 0) {
                armed = 2;
                s_xenoMenuEquipActions = 1;
            } else {
                armed = (strcmp(e, "abilities") == 0) ? 4 : 3;
            }
            s_xenoMenuNavActions = 1;
            s_xenoMenuReorderActions = strcmp(e, "items-reorder") == 0;
            s_xenoMenuPromptActions = strcmp(e, "abilities") != 0 &&
                                      strcmp(e, "equip") != 0 &&
                                      (strcmp(e, "items-prompt") == 0 ||
                                      strncmp(e, "prompt-nav", 10) == 0 ||
                                      strcmp(e, "item-use") == 0 ||
                                      strcmp(e, "item-special") == 0);
            if (strcmp(e, "prompt-nav") == 0) {
                s_xenoMenuPromptNavKind = 1;
            } else if (strcmp(e, "prompt-nav-skip") == 0) {
                s_xenoMenuPromptNavKind = 2;
            } else if (strcmp(e, "item-use") == 0) {
                s_xenoMenuPromptNavKind = 3;
            } else if (strcmp(e, "item-special") == 0) {
                s_xenoMenuPromptNavKind = 4;
            } else if (strcmp(e, "abilities") == 0) {
                s_xenoMenuPromptNavKind = 5;
            }
        } else {
            armed = (e && *e) ? atoi(e) : 0;
        }
        s_xenoMenuNavDowns = armed;
    }
    if (armed <= 0)
        return;
    /* Clock the injection off READER TICKS, not Vsync frames: the field's
     * open/close phases reset the pad queue at a different cadence, so frame
     * counting races it.  A tick == one execution of the menu's interactive
     * input reader, so a hold spanning >=4 ticks is guaranteed to be polled,
     * edged, queued, and drained while the menu is actually listening.
     * Schedule: settle 40 ticks, then press k = hold 4 ticks / release 10. */
    {
        int t = g_XenoMenuNavReaderTicks;
        int k, ph;
        if (t < 40)
            return;
        k = (t - 40) / 14;
        ph = (t - 40) % 14;
        if (k < armed && ph < 4) {
            g_C1Buffer[0x2] &= (unsigned char)~0x40;  /* hold DPAD-DOWN */
            if (k + 1 > injected) {
                injected = k + 1;
                printf("[xeno-port][test] XENO_MENU_NAV_TEST: press DOWN %d/%d "
                       "(reader tick %d)\n", injected, armed, t);
                fflush(stdout);
            }
        }

        /* TEST TOOLING: Equip category DOWNs (pad-buffer PressedOnce). */
        if (s_xenoMenuEquipAccDowns > 0 && s_xenoMenuEquipAccDownBase >= 0 &&
            t >= s_xenoMenuEquipAccDownBase) {
            int k = (t - s_xenoMenuEquipAccDownBase) / 14;
            int ph = (t - s_xenoMenuEquipAccDownBase) % 14;
            if (k < s_xenoMenuEquipAccDowns && ph < 4) {
                g_C1Buffer[0x2] &= (unsigned char)~0x40;
                if (ph == 0) {
                    printf("[xeno-port][test] XENO_MENU_NAV_TEST=equip: DOWN "
                           "category %d/%d at reader tick %d\n",
                           k + 1, s_xenoMenuEquipAccDowns, t);
                    fflush(stdout);
                }
            }
        }

        /* N2c-1 proof: after Circle selects the first item, inject a genuine
         * RIGHT pad hold so the retail Items reader advances from row 0 to
         * row 1 before the second Circle confirms the exchange. */
        if (s_xenoMenuReorderActions && s_xenoMenuItemsOpenTick >= 0 &&
            t >= s_xenoMenuItemsOpenTick + 18 &&
            t < s_xenoMenuItemsOpenTick + 22) {
            g_C1Buffer[0x2] &= (unsigned char)~0x20;  /* hold DPAD-RIGHT */
            if (t == s_xenoMenuItemsOpenTick + 18) {
                printf("[xeno-port][test] N2C1 REORDER: press RIGHT "
                       "row 0 -> row 1 at reader tick %d\n", t);
                fflush(stdout);
            }
        }

        /* N2c-2b proof: while the target prompt is open (phase 1), drive the
         * navigator with genuine pad holds on the established 4-tick-hold /
         * 14-tick-spacing cadence.  The retail {0,2,FF} roster has two
         * eligible slots.  wrap (kind 1): DOWN x2 then UP x2
         * (0->1->0 forward, then 0->1->0 backward).  skip (kind 2):
         * DOWN x2 (0->1, then 1 -> skip absent 2 -> wrap -> 0). */
        if (s_xenoMenuPromptNavKind == 1 || s_xenoMenuPromptNavKind == 2) {
            extern int g_XenoMenuN2c2Phase;
            static int navTick = -1, navLogged = 0;
            if (g_XenoMenuN2c2Phase == 1 && navTick < 0)
                navTick = t;
            if (navTick >= 0 && g_XenoMenuN2c2Phase == 1 && t - navTick >= 8) {
                int rel = t - navTick - 8;
                int presses = (s_xenoMenuPromptNavKind == 1) ? 4 : 2;
                int idx = rel / 14;
                int phn = rel % 14;
                if (idx < presses && phn < 4) {
                    int isUp = (s_xenoMenuPromptNavKind == 1) && (idx >= 2);
                    g_C1Buffer[0x2] &= (unsigned char)~(isUp ? 0x10 : 0x40);
                    if (idx + 1 > navLogged) {
                        navLogged = idx + 1;
                        printf("[xeno-port][test] N2C2B NAV: press %s %d/%d "
                               "at reader tick %d\n", isUp ? "UP" : "DOWN",
                               idx + 1, presses, t);
                        fflush(stdout);
                    }
                }
            }
        }

        /* After the Items common-exit tail has rebuilt the main highlight,
         * issue one more genuine DOWN tap.  This is the nav-alive proof. */
        if (s_xenoMenuNavActions) {
            extern int g_XenoMenuN2Phase;
            extern int g_XenoMenuN3aPhase;
            static int closedTick = -1, finalLogged = 0;
            static int n3aClosedTick = -1, n3aFinalLogged = 0;
            if (g_XenoMenuN2Phase >= 2 && closedTick < 0)
                closedTick = t;
            if (closedTick >= 0 && t >= closedTick + 20 && t < closedTick + 24) {
                g_C1Buffer[0x2] &= (unsigned char)~0x40;
                if (!finalLogged) {
                    finalLogged = 1;
                    printf("[xeno-port][test] XENO_MENU_NAV_TEST=items: "
                           "post-close DOWN (nav-alive) at reader tick %d\n", t);
                    fflush(stdout);
                }
            }
            /* N3a-A1b-1: the same nav-alive proof after the Abilities close. */
            if (s_xenoMenuPromptNavKind == 5 && g_XenoMenuN3aPhase == 2 &&
                n3aClosedTick < 0)
                n3aClosedTick = t;
            if (n3aClosedTick >= 0 && t >= n3aClosedTick + 20 &&
                t < n3aClosedTick + 24) {
                g_C1Buffer[0x2] &= (unsigned char)~0x40;
                if (!n3aFinalLogged) {
                    n3aFinalLogged = 1;
                    printf("[xeno-port][test] XENO_MENU_NAV_TEST=abilities: "
                           "post-close DOWN (nav-alive) at reader tick %d\n", t);
                    fflush(stdout);
                }
            }
        }
    }
}

/* Circle/Cross must be inserted after ControllerPoll recomputes the derived
 * edge globals but before ControllerPushState snapshots them for the queue
 * drained by func_801C7D78. */

/* N2c-4: per-family inventory digests for the bulk-special proof.  One FNV
 * per family region plus slot spot-checks and bound-edge pairs, with
 * containment sentinels on both neighbors of the inventory block.  The item
 * family's writes are slot-shifted by +2 (func_801E5058 preserves slots
 * 0-2), hence the per-family shift column. */
static void PcPort_N2c4PrintFamilies(const char* tag)
{
    extern unsigned char g_GameState[];
    static const struct {
        const char* name;
        unsigned qty, ids, size, bound, shift;
    } fam[5] = {
        { "weapon",    0x1D38, 0x1D9C, 0x64, 0x48, 0 },
        { "accessory", 0x1E00, 0x1EC8, 0xC8, 0x96, 0 },
        { "item",      0x1F90, 0x2026, 0x96, 0x4C, 2 },
        { "unk20BC",   0x20BC, 0x2120, 0x64, 0x48, 0 },
        { "unk2184",   0x2184, 0x221A, 0x96, 0x69, 0 },
    };
    int f;

    for (f = 0; f < 5; f++) {
        unsigned int h = 2166136261u;
        unsigned int i;
        unsigned int lastw = fam[f].shift + fam[f].bound - 1;

        for (i = 0; i < fam[f].size; i++) {
            h ^= g_GameState[fam[f].qty + i]; h *= 16777619u;
            h ^= g_GameState[fam[f].ids + i]; h *= 16777619u;
        }
        printf("[xeno-port][test] N2C4 %s %-9s fnv=%08x slot0=(id%u,q%u) "
               "slot1=(id%u,q%u) lastw=(id%u,q%u) beyond=(id%u,q%u)\n",
               tag, fam[f].name, h,
               g_GameState[fam[f].ids], g_GameState[fam[f].qty],
               g_GameState[fam[f].ids + 1], g_GameState[fam[f].qty + 1],
               g_GameState[fam[f].ids + lastw], g_GameState[fam[f].qty + lastw],
               g_GameState[fam[f].ids + lastw + 1],
               g_GameState[fam[f].qty + lastw + 1]);
    }
    printf("[xeno-port][test] N2C4 %s sentinels party=%02x%02x%02x%02x "
           "post-block=%02x%02x%02x itemSlot2=(id%u,q%u)\n",
           tag,
           g_GameState[0x1D34], g_GameState[0x1D35],
           g_GameState[0x1D36], g_GameState[0x1D37],
           g_GameState[0x22B0], g_GameState[0x22B1], g_GameState[0x22B2],
           g_GameState[0x2028], g_GameState[0x1F92]);
    fflush(stdout);
}

static void PcPort_ForcedMenuActionEdges(void)
{
    extern unsigned short g_C1ButtonStateReleased;
    extern int g_XenoMenuNavReaderTicks;
    extern int g_XenoMenuN2Phase;
    extern int g_XenoMenuN2c2Phase;
    extern unsigned int PcPort_N2c1ReadItemPair(void);
    static int confirmInjected = 0, selectInjected = 0;
    static int reorderInjected = 0, cancelInjected = 0;
    static int promptInjected = 0, promptCancelInjected = 0;
    static int promptObserved = 0, promptTick = -1;
    static int reorderObserved = 0;
    static unsigned int inventoryHashBefore = 0;
    static unsigned int characterHashBefore = 0;
    unsigned int itemPair;
    int confirmTick;
    int t;

    /* FNV-1a over the actual save-backed regions touched by item effects. */
    #define HASH_REGION(dst, start, length) do { \
        extern unsigned char g_GameState[]; \
        unsigned int _h = 2166136261U; \
        int _i; \
        for (_i = 0; _i < (length); _i++) { \
            _h ^= g_GameState[(start) + _i]; \
            _h *= 16777619U; \
        } \
        (dst) = _h; \
    } while (0)

    /* Root System Menu (XENO_MENU_FORCE without a nav script): after the
     * input reader is live, inject Cross so func_801C55A0 takes MENU_INPUT_BACK
     * and MenuMain returns. */
    if (!s_xenoMenuNavActions) {
        static int rootCancelInjected = 0;
        if (s_xenoMenuForceFired && !rootCancelInjected &&
            g_XenoMenuNavReaderTicks >= 120) {
            rootCancelInjected = 1;
            g_C1ButtonStateReleased |= 0x40; /* CTRL_BTN_CROSS */
            printf("[xeno-port][test] XENO_MENU_FORCE: Cross cancel "
                   "at reader tick %d\n", g_XenoMenuNavReaderTicks);
            fflush(stdout);
        }
        return;
    }

    t = g_XenoMenuNavReaderTicks;
    confirmTick = 40 + 14 * s_xenoMenuNavDowns + 14;
    if (!confirmInjected && t >= confirmTick) {
        confirmInjected = 1;
        g_C1ButtonStateReleased |= 0x20;  /* CTRL_BTN_CIRCLE */
        printf("[xeno-port][test] XENO_MENU_NAV_TEST=%s: Circle confirm "
               "at reader tick %d\n",
               s_xenoMenuEquipActions ? "equip" : "items", t);
        fflush(stdout);
    }

    /* TEST TOOLING: Equip — three DOWNs to accessory id 92 (Stamina Ring). */
    if (s_xenoMenuEquipActions && confirmInjected && !cancelInjected) {
        static int equipDownArmed = 0;
        if (!equipDownArmed && t >= confirmTick + 50) {
            equipDownArmed = 1;
            s_xenoMenuEquipAccDowns = 3; /* cat 0→1→2→3 → 0x2E2=92 */
            s_xenoMenuEquipAccDownBase = t;
        }
        if (t >= confirmTick + 220) {
            cancelInjected = 1;
            g_C1ButtonStateReleased |= 0x40; /* CTRL_BTN_CROSS */
            printf("[xeno-port][test] XENO_MENU_NAV_TEST=equip: Cross cancel "
                   "at reader tick %d\n", t);
            fflush(stdout);
        }
    }

    /* N3a-A1a (kind 5): Abilities lifecycle.  Snapshot state at the confirm
     * that opens the screen, wait for the windows + open animation, cancel,
     * then re-hash.  A1a is READ-ONLY by construction: the character-stat
     * writes live in func_801DD790 (A2), which the guard keeps unreached, so
     * both hashes must come back identical.
     *
     * A1b-1 extension: func_801DC3D8 is now real, so rows are flagged for
     * the first time (0x81 confirmable, 0x01 populated-not-confirmable,
     * 0x00 absent) and the boundary is BEHAVIOURAL, not structural.  Dump
     * the flags at open, drive the cursor to the first rejecting row,
     * confirm there, and show the guard rejects (func_801DD790 must stay at
     * zero hits -- checked via the absence of its [stub] line in the log). */
    if (s_xenoMenuPromptNavKind == 5) {
        extern int g_XenoMenuN3aPhase;
        extern unsigned char g_XenoMenuN3aRowFlags[];
        extern unsigned char g_C1Buffer[];
        static int n3aOpenTick = -1, n3aCancel = 0, n3aDone = 0;
        static unsigned int n3aCharBefore, n3aInvBefore;
        static int n3aHashed = 0;
        static int n3aTarget = -2, n3aNavPressed = 0, n3aRowConfirm = 0;

        if (confirmInjected && !n3aHashed) {
            n3aHashed = 1;
            HASH_REGION(n3aCharBefore, 0x26C, 0x70C);
            HASH_REGION(n3aInvBefore, 0x1D38, 0x578);
            printf("[xeno-port][test] N3A STATE-BEFORE: characters=%08x "
                   "inventory=%08x\n", n3aCharBefore, n3aInvBefore);
            fflush(stdout);
        }
        if (g_XenoMenuN3aPhase == 1 && n3aOpenTick < 0) {
            int i;

            n3aOpenTick = t;
            printf("[xeno-port][test] N3A OPEN: Abilities settled open at "
                   "reader tick %d\n", t);
            printf("[xeno-port][test] N3A ROWFLAGS:");
            for (i = 0; i < 0xE; i++) {
                printf(" %02x", g_XenoMenuN3aRowFlags[i]);
            }
            printf("\n");
            /* The boundary row: first cursor-reachable row (0..0xB) that is
             * not 0x81 -- 0x00 (empty slot) or 0x01 (cost > MP). */
            n3aTarget = -1;
            for (i = 0; i < 0xC; i++) {
                if (g_XenoMenuN3aRowFlags[i] != 0x81) {
                    n3aTarget = i;
                    break;
                }
            }
            if (n3aTarget >= 0) {
                printf("[xeno-port][test] N3A BOUNDARY: rejecting row %d "
                       "(flags %02x) chosen; driving cursor there\n",
                       n3aTarget, g_XenoMenuN3aRowFlags[n3aTarget]);
            } else {
                /* No rejecting row: prove the guard arithmetically rather
                 * than reporting a zero that was never exercised. */
                printf("[xeno-port][test] N3A BOUNDARY: every row is 0x81, "
                       "no rejecting row exists; asserting the guard "
                       "condition directly: 0x00&0x80=%d 0x01&0x80=%d "
                       "(both reject, func_801DD790 gated)\n",
                       0x00 & 0x80, 0x01 & 0x80);
            }
            fflush(stdout);
        }
        /* Drive the cursor 0 -> n3aTarget with genuine RIGHT holds (cursor+1
         * per press, DDF24 case 0), on the 4-tick-hold / 14-tick cadence. */
        if (n3aOpenTick >= 0 && n3aTarget > 0 && n3aNavPressed < n3aTarget) {
            int rel = t - (n3aOpenTick + 8) - n3aNavPressed * 14;

            if (rel >= 0 && rel < 4) {
                g_C1Buffer[0x2] &= (unsigned char)~0x20;  /* DPAD-RIGHT */
                if (rel == 0) {
                    printf("[xeno-port][test] N3A BOUNDARY: press RIGHT %d/%d "
                           "at reader tick %d\n",
                           n3aNavPressed + 1, n3aTarget, t);
                    fflush(stdout);
                }
            }
            if (rel == 4) {
                n3aNavPressed++;
            }
        }
        /* Confirm on the rejecting row: the rowFlags bit-7 guard in DDF24
         * case 4 must reject, so func_801DD790 must NOT fire. */
        if (n3aOpenTick >= 0 && n3aTarget >= 0 && !n3aRowConfirm &&
            n3aNavPressed >= n3aTarget &&
            t >= n3aOpenTick + 8 + n3aTarget * 14 + 10) {
            n3aRowConfirm = 1;
            g_C1ButtonStateReleased |= 0x20;  /* CTRL_BTN_CIRCLE */
            printf("[xeno-port][test] N3A BOUNDARY: Circle confirm on row %d "
                   "(flags %02x, bit7=%d) at reader tick %d -- guard must "
                   "reject, func_801DD790 must NOT fire\n",
                   n3aTarget, g_XenoMenuN3aRowFlags[n3aTarget],
                   (g_XenoMenuN3aRowFlags[n3aTarget] & 0x80) ? 1 : 0, t);
            fflush(stdout);
        }
        if (n3aOpenTick >= 0 && !n3aCancel &&
            ((n3aRowConfirm &&
              t >= n3aOpenTick + 8 + n3aTarget * 14 + 40) ||
             (n3aTarget < 0 && t >= n3aOpenTick + 30))) {
            n3aCancel = 1;
            g_C1ButtonStateReleased |= 0x40;  /* CTRL_BTN_CROSS */
            printf("[xeno-port][test] N3A CANCEL: Cross at reader tick %d\n", t);
            fflush(stdout);
        }
        if (g_XenoMenuN3aPhase == 2 && !n3aDone) {
            unsigned int charAfter, invAfter;

            n3aDone = 1;
            HASH_REGION(charAfter, 0x26C, 0x70C);
            HASH_REGION(invAfter, 0x1D38, 0x578);
            printf("[xeno-port][test] N3A STATE-AFTER: characters=%08x "
                   "inventory=%08x unchanged=%s\n",
                   charAfter, invAfter,
                   (charAfter == n3aCharBefore && invAfter == n3aInvBefore)
                       ? "yes" : "NO");
            fflush(stdout);
        }
    }

    if (g_XenoMenuN2Phase == 1 && s_xenoMenuItemsOpenTick < 0) {
        s_xenoMenuItemsOpenTick = t;
        if (s_xenoMenuReorderActions) {
            itemPair = PcPort_N2c1ReadItemPair();
            printf("[xeno-port][test] N2C1 STATE-BEFORE: "
                   "row0=(ID%u,qty%u) row1=(ID%u,qty%u)\n",
                   itemPair & 0xFF, (itemPair >> 8) & 0xFF,
                   (itemPair >> 16) & 0xFF, (itemPair >> 24) & 0xFF);
            fflush(stdout);
        }
        if (s_xenoMenuPromptActions) {
            if (s_xenoMenuPromptNavKind == 4) {
                PcPort_N2c4PrintFamilies("BEFORE");
            }
            HASH_REGION(characterHashBefore, 0x26C, 0x70C);
            HASH_REGION(inventoryHashBefore, 0x1F90, 0x12C);
            printf("[xeno-port][test] N2C2A STATE-BEFORE: "
                   "inventory=%08x characters=%08x\n",
                   inventoryHashBefore, characterHashBefore);
            fflush(stdout);
        }
    }

    if (s_xenoMenuReorderActions && s_xenoMenuItemsOpenTick >= 0) {
        if (!selectInjected && t >= s_xenoMenuItemsOpenTick + 8) {
            selectInjected = 1;
            g_C1ButtonStateReleased |= 0x20;  /* CTRL_BTN_CIRCLE */
            printf("[xeno-port][test] N2C1 REORDER: Circle select row 0 "
                   "at reader tick %d\n", t);
            fflush(stdout);
        }
        if (!reorderInjected && t >= s_xenoMenuItemsOpenTick + 32) {
            reorderInjected = 1;
            g_C1ButtonStateReleased |= 0x20;  /* CTRL_BTN_CIRCLE */
            printf("[xeno-port][test] N2C1 REORDER: Circle confirm row 1 "
                   "at reader tick %d\n", t);
            fflush(stdout);
        }
        if (reorderInjected && !reorderObserved) {
            itemPair = PcPort_N2c1ReadItemPair();
            if (itemPair == 0x0B010C02U) {
                reorderObserved = 1;
                printf("[xeno-port][test] N2C1 STATE-AFTER: "
                       "row0=(ID%u,qty%u) row1=(ID%u,qty%u)\n",
                       itemPair & 0xFF, (itemPair >> 8) & 0xFF,
                       (itemPair >> 16) & 0xFF,
                       (itemPair >> 24) & 0xFF);
                fflush(stdout);
            }
        }
    }

    if (s_xenoMenuPromptActions && s_xenoMenuItemsOpenTick >= 0 &&
        !selectInjected && t >= s_xenoMenuItemsOpenTick + 8) {
        selectInjected = 1;
        g_C1ButtonStateReleased |= 0x20;
        printf("[xeno-port][test] N2C2A PROMPT: Circle select row 0 "
               "at reader tick %d\n", t);
        fflush(stdout);
    }
    if (s_xenoMenuPromptActions && selectInjected && !promptInjected &&
        t >= s_xenoMenuItemsOpenTick + 24) {
        promptInjected = 1;
        g_C1ButtonStateReleased |= 0x20;
        printf("[xeno-port][test] N2C2A PROMPT: Circle confirm same row "
               "at reader tick %d\n", t);
        fflush(stdout);
    }
    if (s_xenoMenuPromptActions && g_XenoMenuN2c2Phase == 1 &&
        promptTick < 0) {
        promptTick = t;
        printf("[xeno-port][test] N2C2A PROMPT-OPEN: default target stable "
               "at reader tick %d\n", t);
        fflush(stdout);
    }
    /* N2c-3 (kind 3): confirm the default target once; the effect applies,
     * the quantity hits zero, and DB920's depletion exit auto-closes the
     * prompt -- no Cross needed at prompt level. */
    if ((s_xenoMenuPromptNavKind == 3 || s_xenoMenuPromptNavKind == 4) &&
        promptTick >= 0 &&
        !promptCancelInjected && t >= promptTick + 10) {
        extern unsigned char g_GameState[];

        promptCancelInjected = 1;   /* reuse: one prompt-level action only */
        g_C1ButtonStateReleased |= 0x20;
        printf("[xeno-port][test] N2C%d USE: Circle target-confirm at reader "
               "tick %d (before: hp=%d/%d qty=%d id=%d)\n",
               s_xenoMenuPromptNavKind == 4 ? 4 : 3, t,
               (int)*(unsigned short*)&g_GameState[0x2B8],
               (int)*(unsigned short*)&g_GameState[0x2BA],
               (int)g_GameState[0x1F90], (int)g_GameState[0x2026]);
        fflush(stdout);
    }
    if (s_xenoMenuPromptActions && s_xenoMenuPromptNavKind < 3 &&
        promptTick >= 0 && !promptCancelInjected &&
        t >= promptTick + (s_xenoMenuPromptNavKind == 1 ? 115 :
                           (s_xenoMenuPromptNavKind == 2 ? 55 : 45))) {
        promptCancelInjected = 1;
        g_C1ButtonStateReleased |= 0x40;
        printf("[xeno-port][test] N2C2A PROMPT: Cross cancel "
               "at reader tick %d\n", t);
        fflush(stdout);
    }
    if (s_xenoMenuPromptActions && g_XenoMenuN2c2Phase == 2 &&
        !promptObserved) {
        unsigned int inventoryHashAfter;
        unsigned int characterHashAfter;
        HASH_REGION(characterHashAfter, 0x26C, 0x70C);
        HASH_REGION(inventoryHashAfter, 0x1F90, 0x12C);
        promptObserved = 1;
        printf("[xeno-port][test] N2C2A STATE-AFTER: "
               "inventory=%08x characters=%08x unchanged=%s\n",
               inventoryHashAfter, characterHashAfter,
               (inventoryHashAfter == inventoryHashBefore &&
                characterHashAfter == characterHashBefore) ? "yes" : "NO");
        fflush(stdout);
        /* N2c-3: for the item-use mode the hashes are EXPECTED to change --
         * print the explicit save-state values the proof asserts on. */
        if (s_xenoMenuPromptNavKind == 4) {
            extern int g_XenoMenuN2c3SpecialHits;

            PcPort_N2c4PrintFamilies("AFTER");
            printf("[xeno-port][test] N2C4 AFTER: mag2SpecialHits=%d\n",
                   g_XenoMenuN2c3SpecialHits);
            fflush(stdout);
        }
        if (s_xenoMenuPromptNavKind == 3) {
            extern unsigned char g_GameState[];
            extern int g_XenoMenuN2c3SpecialHits;

            printf("[xeno-port][test] N2C3 AFTER: hp=%d/%d qty=%d id=%d "
                   "specialDispatchHits=%d\n",
                   (int)*(unsigned short*)&g_GameState[0x2B8],
                   (int)*(unsigned short*)&g_GameState[0x2BA],
                   (int)g_GameState[0x1F90], (int)g_GameState[0x2026],
                   g_XenoMenuN2c3SpecialHits);
            fflush(stdout);
        }
    }

    if (!cancelInjected && s_xenoMenuItemsOpenTick >= 0 &&
        ((!s_xenoMenuReorderActions && !s_xenoMenuPromptActions &&
          t >= s_xenoMenuItemsOpenTick + 45) ||
         (s_xenoMenuReorderActions && reorderObserved &&
          t >= s_xenoMenuItemsOpenTick + 60) ||
         (s_xenoMenuPromptActions && promptObserved &&
          t >= promptTick + (s_xenoMenuPromptNavKind == 1 ? 145 :
                             (s_xenoMenuPromptNavKind == 2 ? 85 : 75))))) {
        cancelInjected = 1;
        g_C1ButtonStateReleased |= 0x40;  /* CTRL_BTN_CROSS */
        printf("[xeno-port][test] XENO_MENU_NAV_TEST=%s: Cross cancel "
               "at reader tick %d\n",
               s_xenoMenuReorderActions ? "items-reorder" :
               (s_xenoMenuPromptActions ? "items-prompt" : "items"), t);
        fflush(stdout);
    }
    #undef HASH_REGION
}

/* Called once per dispatched game vblank, not once per timing query. */
void PcPort_PrepareControllerPoll(void)
{
    extern void PsyX_UpdateInput(void);
    PsyX_UpdateInput();
    PcPort_PadTestInputInject();
    PcPort_PadOnControl();
    PcPort_ForcedMenuNav();
}

void PcPort_BeforeControllerPush(void)
{
    PcPort_ForcedMenuActionEdges();
}

int Vsync(int mode)
{
    int count;
    /* Retail Vsync(1)/Vsync(<0) are timing queries: they must not flush or
     * present. The deferred IRQ service may consume newly elapsed ticks, but
     * repeated queries at the same count never add controller states. The
     * field starts each iteration with Vsync(1) while the prior DrawOTag scene
     * is still open and reaches the blocking Vsync(0) later in the same
     * iteration. Presenting at the query would close the scene there and then
     * make the Vsync(0) take the VRAM fallback, so one rendered frame swaps
     * twice (PsyX_EndScene + VRAM fallback) and flickers. Keep presentation
     * in the blocking path. */
    if (mode == 1 || mode < 0) {
        count = VSync(mode);
        XENO_SERVICE_VBLANK();
        return (mode < 0 && PcPort_GetServicedVblankCount) ? PcPort_GetServicedVblankCount() : count;
    }

    /* Flush any primitives queued this frame before presenting. DrawOTag flushes
     * its own ordering table via DrawAllSplits, but immediate-mode DrawPrim (used
     * by GameShowSplashScreen's fade loops) does NOT -- it only queues into the
     * split list, relying on a later DrawSync/DrawOTag to flush. The splash issues
     * DrawPrim + Vsync with no such flush in between, so its sprite would never
     * reach the framebuffer. Flushing here (idempotent when empty, like DrawSync)
     * makes the present show everything drawn since the last frame. */
    {
        /* PSX display semantics: at vblank the screen shows the DISPENV area of
         * VRAM whether or not anything was drawn.  PsyCross only presents
         * rendered primitives, so a frame built purely by LoadImage into the
         * display buffer (the STR movie player) never reaches the window.
         * When no scene is open at a blocking Vsync(0), present the display
         * area from the VRAM mirror instead (patches/psycross_display_present). */
        extern int PsyX_IsSceneOpen(void);
        extern int PsyX_PresentDisplayFromVRAM(void);
        int sceneOpen = PsyX_IsSceneOpen();

        DrawAllSplits();

        PsyX_EndScene();      /* present the frame the game just finished building */

        /* Dev-harness field capture (no-op unless XENO_FIELD_CAPTURE_DIR). */
        { extern void PcPort_FieldCaptureOnVsync(void) __attribute__((weak)); if (PcPort_FieldCaptureOnVsync) PcPort_FieldCaptureOnVsync(); }

        if (!sceneOpen && mode == 0)
            PsyX_PresentDisplayFromVRAM();
    }

    /* TEST TOOLING hooks below are weak: tests that link this TU without the
     * tooling files still link.  Inert unless their XENO_* switch is set. */
    /* TEST TOOLING: headless walk telemetry (XENO_FIELD_POS_DIAG). */
    { extern void PcPort_FieldPosDiag(void) __attribute__((weak)); if (PcPort_FieldPosDiag) PcPort_FieldPosDiag(); }
    /* One frame_tick per presented frame (xg_plat/mods.h).  Its first
     * subscribers are the cheat services that used to be called right here,
     * in the same order: field warp (XENO_FIELD_WARP / `warp`), battle warp
     * (XENO_BATTLE_WARP_FILE / `battle`), then queued cheat-console commands
     * (cheat_console.c), then mods.  Weak: no-op without the layer. */
    if (xg_plat_mods_emit)
        (void)xg_plat_mods_emit(XG_PLAT_EVENT_FRAME_TICK, 0, 0);
    /* TEST TOOLING: dump the loaded walkmesh (XENO_WALKMESH_DUMP); inert unset. */
    { extern void PcPort_WalkmeshDump(void) __attribute__((weak)); if (PcPort_WalkmeshDump) PcPort_WalkmeshDump(); }
    /* Pace first so the newly elapsed tick is serviced before returning.
     * All entry points share one counter cursor, including the guest pump. */
    count = VSync(mode);
    XENO_SERVICE_VBLANK();

    /* Headless menu driver (no-op unless XENO_KERNEL_SEL is set). Must run after
     * ControllerPoll, which recomputes g_C1ButtonState* each frame -- we OR the
     * synthetic Circle in afterwards so it survives to the next KernelMenuUpdate. */
    PcPort_ForcedKernelSelect();

    /* Headless resourced main-menu driver (no-op unless XENO_MENU_FORCE=1). */
    PcPort_ForcedFieldMenu();

    /* Temporary interactive camera+cull logger (XENO_CULL_CAM_LOG=1). */
    { extern void PcPort_CullCamLogOnVsync(void); PcPort_CullCamLogOnVsync(); }

    return count;
}
