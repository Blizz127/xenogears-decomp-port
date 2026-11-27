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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* The port's checkpoints, scripts and guest bridge share this 32-bit word. */
uint32_t g_RandomSeed;

/* The field-step vblank log consumes these counters and marks its game thread. */
static uint32_t s_rand_calls, s_rand_calls_off_main;
static void *s_rand_last_off_main;
static _Thread_local int s_rand_on_main;
extern int g_GameSceneMapNum __attribute__((weak));
extern int PcPort_FieldStepPadArmedStep(unsigned *step) __attribute__((weak));

static void record_random_draw(void *caller)
{
    static FILE *trace;
    static int initialized;

    ++s_rand_calls;
    if (!s_rand_on_main) {
        ++s_rand_calls_off_main;
        s_rand_last_off_main = caller;
    }

    if (!initialized) {
        initialized = 1;
        const char *path = getenv("XENO_RAND_TRACE");
        if (path && *path && &g_GameSceneMapNum && PcPort_FieldStepPadArmedStep)
            trace = fopen(path, "w");
    }
    if (trace) {
        unsigned step;
        if (PcPort_FieldStepPadArmedStep(&step)) {
            fclose(trace);
            trace = NULL;
        } else {
            fprintf(trace, "%u %d %p\n", (unsigned)s_rand_calls, g_GameSceneMapNum, caller);
        }
    }
}

int rand(void)
{
    /* The C11 7.22.2.2 example's LCG parameters match the recorded game draws.
     * Unsigned arithmetic retains all 32 state bits across checkpoint restores. */
    uint32_t state = UINT32_C(1103515245) * g_RandomSeed + UINT32_C(12345);
    record_random_draw(__builtin_return_address(0));
    g_RandomSeed = state;
    return (int)((state >> 16) & UINT32_C(0x7fff));
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
#define xeno_cleanroom_cfc2 CFC2
#include "cleanroom/gte_readgeom_cleanroom.h"

/* floor(int32(raw) / 65536) without relying on right shift of a negative value */
static int32_t offset_value(uint32_t raw)
{
    uint32_t hi = raw >> 16;
    return hi >= 0x8000u ? (int32_t)hi - 65536 : (int32_t)hi;
}

void ReadGeomOffset(long *ofx, long *ofy)
{
    uint32_t raw_x = xeno_cleanroom_cfc2(24);
    uint32_t raw_y = xeno_cleanroom_cfc2(25);
    int32_t x = offset_value(raw_x);
    int32_t y = offset_value(raw_y);

    /* both reads precede both stores; ofx first, ofy second (second store wins on alias) */
    *ofx = (long)x;
    *ofy = (long)y;
}

#undef xeno_cleanroom_cfc2

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

/* PsyCross e56e4cde, include/psx/libgpu.h (MIT). */
void SetPolyF3(POLY_F3 *packet)
{
    setPolyF3(packet);
}

/* Independently written against the public PsyCross GTE interface at
 * e56e4cde1c2b8a15e0d4e38b26cdd9202e0d17e6. See include/psx/gtemac.h
 * (gte_OuterProduct0/12, gte_Square0) and include/psx/inline_c.h for the
 * operand, accumulator and command interfaces. */
static void sdk_vector_load_ir(const VECTOR *vector)
{
    uint32_t xyz[3];
    memcpy(xyz, vector, sizeof(xyz));
    MTC2(xyz[0], 9);
    MTC2(xyz[1], 10);
    MTC2(xyz[2], 11);
}

static void sdk_vector_store_mac(VECTOR *result)
{
    uint32_t xyz[3] = { MFC2(25), MFC2(26), MFC2(27) };
    memcpy(result, xyz, sizeof(xyz));
}

VECTOR *Square0(VECTOR *input, VECTOR *result)
{
    sdk_vector_load_ir(input);
    gte_sqr0();
    sdk_vector_store_mac(result);
    return result;
}

static void sdk_vector_cross(const VECTOR *left, const VECTOR *right,
                             VECTOR *result, int shift12)
{
    /* OP uses the rotation diagonal as one of its input vectors. The
     * callable interface preserves these three packed matrix registers. */
    unsigned int rot0 = CFC2(0), rot2 = CFC2(2), rot4 = CFC2(4);
    uint32_t xyz[3];
    memcpy(xyz, left, sizeof(xyz));
    CTC2(xyz[0], 0);
    CTC2(xyz[1], 2);
    CTC2(xyz[2], 4);
    sdk_vector_load_ir(right);
    if (shift12) {
        gte_op12();
    } else {
        gte_op0();
    }
    sdk_vector_store_mac(result);
    CTC2(rot0, 0);
    CTC2(rot2, 2);
    CTC2(rot4, 4);
}

void OuterProduct0(VECTOR *left, VECTOR *right, VECTOR *result)
{
    sdk_vector_cross(left, right, result, 0);
}

void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *result)
{
    sdk_vector_cross(left, right, result, 1);
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
    uint32_t squared = (uint32_t)sx * (uint32_t)sx + (uint32_t)sy * (uint32_t)sy + (uint32_t)sz * (uint32_t)sz; /* wraps like the 32-bit GTE read-back, no signed-overflow UB */
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

#include <stdint.h>
#include <string.h>
#include <psx/libgte.h>
#include <psx/inline_c.h>

static void load_projection_vertex(const SVECTOR *vertex, int slot)
{
    uint32_t words[2];
    memcpy(words, vertex, sizeof(words));
    MTC2(words[0], slot * 2);
    MTC2(words[1], slot * 2 + 1);
}

long RotAverage4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                 long *sxy0, long *sxy1, long *sxy2, long *sxy3,
                 long *p, long *flag)
{
    load_projection_vertex(v0, 0);
    load_projection_vertex(v1, 1);
    load_projection_vertex(v2, 2);
    gte_rtpt();
    uint32_t projection_flags = CFC2(31);
    uint32_t screen[3] = {MFC2(12), MFC2(13), MFC2(14)};

    /* GPU packets contain packed XY pairs even when host longs are wider. */
    memcpy((unsigned char *)sxy0, &screen[0], sizeof(screen[0]));
    memcpy((unsigned char *)sxy1, &screen[1], sizeof(screen[1]));
    memcpy((unsigned char *)sxy2, &screen[2], sizeof(screen[2]));

    load_projection_vertex(v3, 0);
    gte_rtps();
    projection_flags |= CFC2(31);
    uint32_t last_screen = MFC2(14);
    memcpy((unsigned char *)sxy3, &last_screen, sizeof(last_screen));
    *p = (int32_t)MFC2(8);
    *flag = (int32_t)projection_flags;

    gte_avsz4();
    return MFC2(7);
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

/* Runtime-held buttons (game CTRL_BTN_* bits), set by the harness command
 * `dev pad <hex>` (port_dev_menu_actions.c) so a test can press a button from
 * the moment a quick-load finishes instead of guessing a Vsync frame.  Applied
 * only while XENO_PAD_TEST_INPUT is active, like the schedule itself. */
static volatile unsigned short s_padTestRuntimeHeld;
void PcPort_PadTestInputHold(unsigned held) { s_padTestRuntimeHeld = (unsigned short)held; }

/* TEST TOOLING: the cheat console's `pad <mask> [frames]` (headless drivers,
 * pc_port/tests/*_easel_*). Holds GAME buttons (same CTRL_BTN_* space and
 * physical remap as XENO_PAD_TEST_INPUT) for that many Vsync-shim frames.
 * Inert unless the console queues a hold. */
static volatile unsigned short s_padConsoleMask;
static volatile int s_padConsoleFrames;

void PcPort_PadConsoleHold(unsigned short gameMask, int frames)
{
    s_padConsoleMask = gameMask;
    s_padConsoleFrames = frames > 0 ? frames : 0;
}

static void PcPort_PadConsoleInject(void)
{
    extern unsigned char g_C1Buffer[];
    unsigned short value;

    if (s_padConsoleFrames <= 0)
        return;
    value = PcPort_PadTestGameToPhysical(s_padConsoleMask);
    g_C1Buffer[0x2] &= (unsigned char)~(unsigned char)(value >> 8);
    g_C1Buffer[0x3] &= (unsigned char)~(unsigned char)(value & 0xFF);
    if (--s_padConsoleFrames == 0) {
        printf("[xeno-port][test] console pad 0x%04x released\n",
               (unsigned)s_padConsoleMask);
        fflush(stdout);
    }
}

/* Must run AFTER PsyX_UpdateInput() refreshes g_C1Buffer from SDL and BEFORE
 * ControllerPoll() derives the edge state from it. */
static void PcPort_PadTestInputInject(void)
{
    extern unsigned char g_C1Buffer[];
    unsigned short value;
    int previousStep;

    PcPort_PadConsoleInject();
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

    value |= s_padTestRuntimeHeld;

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

/* TEST TOOLING: the XENO_PAD_TEST_INPUT schedule clock (-1 when no schedule
 * is active), read by the XENO_PRESENT_HASH_LOG present hash
 * (psycross_hd2d_hooks.patch) so a run can stop at a schedule frame. */
int PcPort_PadTestInputFrame(void)
{
    return s_padTestState == 1 ? (int)s_padTestFrame : -1;
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

/* TEST TOOLING for the field-step slice (run_field14_step_slice.sh), all
 * keyed to XENO_FIELD_STEP_PAD_INPUT's field-step clock and inert unless set:
 *   XENO_FIELD_STEP_SPU_LOG=<file>  one line per vblank once armed: vblank
 *       index since arming, step, FNV-1a of the 0x200-byte SPU register page
 *       the sound driver writes (g_pSoundSpuRegisters).  That page is the
 *       sound driver's output as retail hardware would receive it; the port
 *       does not yet render it to an audio device (sound.c, WDS/B5).
 *   XENO_FIELD_STEP_SAVE_AT=<step> / XENO_FIELD_STEP_LOAD_AT=<step>  queue
 *       the ordinary F11 quick save / F12 quick load once the step clock
 *       reaches that step (retried per vblank until queued). */
/* DIAGNOSTIC: XENO_FIELD_VAR_WATCH=<byte offset> -- called by the field
 * VM's script-memory writer for every write (src/field/scripts/
 * virtual_machine.c); logs writes to the watched offset with the writing
 * actor and instruction pointer, the field-step clock and the game, serviced
 * and host vblank counts.  Logging only. */
void PcPort_FieldVarWatch(int address, unsigned short old_value, unsigned short new_value,
                          int actor, int ip)
{
    extern int PcPort_FieldStepPadArmedStep(unsigned *step);
    extern int D_80059488;
    extern int PsyX_Sys_GetVBlankCount(void);
    static int watch = -2;
    unsigned step = 0;
    int armed;

    if (watch == -2) {
        const char* e = getenv("XENO_FIELD_VAR_WATCH");
        watch = (e != NULL && e[0] != '\0') ? (int)strtol(e, NULL, 0) : -1;
    }
    if (watch < 0 || ((address ^ watch) & ~1) != 0)
        return;
    armed = PcPort_FieldStepPadArmedStep(&step);
    fprintf(stderr, "[var-watch] off=0x%x %u->%u actor=%d ip=%d armed=%d step=%u game_vblank=%d "
            "serviced=%d host_vblank=%d\n", address, old_value, new_value, actor, ip, armed, step,
            D_80059488, PcPort_GetServicedVblankCount ? PcPort_GetServicedVblankCount() : -1,
            PsyX_Sys_GetVBlankCount());
}

int PcPort_DeterministicVblankLogGate(void);

static void PcPort_FieldStepEvidenceOnVblank(void)
{
    extern int PcPort_FieldStepPadArmedStep(unsigned *step);
    extern void* g_pSoundSpuRegisters;
    extern void PcPort_QuickCheckpointRequestSave(void);
    extern void PcPort_QuickCheckpointRequestLoad(void);
    extern int PcPort_QuickCheckpointGetUiState(void);
    static int init;
    static FILE* spu;
    static FILE* rng;
    static long save_at = -1, load_at = -1;
    static int saved, loaded;
    static unsigned vblanks;
    unsigned step;

    if (!init) {
        const char* e;
        init = 1;
        e = getenv("XENO_FIELD_STEP_SPU_LOG");
        if (e != NULL && e[0] != '\0')
            spu = fopen(e, "w");
        /* XENO_FIELD_STEP_RNG_LOG=<path>: per armed vblank, the retail rand
         * seed and call counters (all / off the game thread, last off-thread
         * caller). The game thread is the one running this hook. */
        e = getenv("XENO_FIELD_STEP_RNG_LOG");
        if (e != NULL && e[0] != '\0')
            rng = fopen(e, "w");
        e = getenv("XENO_FIELD_STEP_SAVE_AT");
        if (e != NULL && e[0] != '\0')
            save_at = strtol(e, NULL, 10);
        e = getenv("XENO_FIELD_STEP_LOAD_AT");
        if (e != NULL && e[0] != '\0')
            load_at = strtol(e, NULL, 10);
    }
    s_rand_on_main = 1;
    if (!PcPort_FieldStepPadArmedStep(&step))
        return;
    if (rng != NULL) {
        fprintf(rng, "%u %u %08x %u %u %p\n", vblanks, step, (unsigned)g_RandomSeed,
                (unsigned)s_rand_calls, (unsigned)s_rand_calls_off_main, s_rand_last_off_main);
        fflush(rng);
    }
    /* XENO_FIELD_STEP_EXIT_AT=<step>: end the run by itself on its frame cap
     * (exit(0), so atexit receipts are written); the driver's timeout is
     * then only a hang detector. */
    {
        static long exit_at = -2;
        if (exit_at == -2) {
            const char* e = getenv("XENO_FIELD_STEP_EXIT_AT");
            exit_at = (e != NULL && e[0] != '\0') ? strtol(e, NULL, 10) : -1;
        }
        if (exit_at >= 0 && step >= (unsigned long)exit_at) {
            fprintf(stderr, "[field-step-pad] frame cap: exiting at step=%u\n", step);
            fflush(stdout);
            fflush(stderr);
            exit(0);
        }
    }
    if (spu != NULL && g_pSoundSpuRegisters != NULL && PcPort_DeterministicVblankLogGate()) {
        const unsigned char* page = (const unsigned char*)g_pSoundSpuRegisters;
        unsigned int h = 2166136261u;
        unsigned i;
        for (i = 0; i < 0x200u; i++) {
            h ^= page[i];
            h *= 16777619u;
        }
        fprintf(spu, "%u %u %08x\n", vblanks, step, h);
        fflush(spu);
    }
    vblanks++;
    /* Queued == UI state SAVE_PENDING (1) / LOAD_PENDING (2); an ignored
     * request (not in a field) is retried about once a second, like a key
     * pressed again. */
    if (save_at >= 0 && !saved && step >= (unsigned long)save_at && vblanks % 60u == 1u) {
        PcPort_QuickCheckpointRequestSave();
        saved = PcPort_QuickCheckpointGetUiState() == 1;
        if (saved)
            fprintf(stderr, "[field-step-pad] F11 quick save requested at step=%u\n", step);
    }
    if (load_at >= 0 && !loaded && step >= (unsigned long)load_at && vblanks % 60u == 1u) {
        PcPort_QuickCheckpointRequestLoad();
        loaded = PcPort_QuickCheckpointGetUiState() == 2;
        if (loaded)
            fprintf(stderr, "[field-step-pad] F12 quick load requested at step=%u\n", step);
    }
}

/* Called once per dispatched game vblank, not once per timing query. */
void PcPort_PrepareControllerPoll(void)
{
    extern void PsyX_UpdateInput(void);
    PsyX_UpdateInput();
    PcPort_PadTestInputInject();
    {
        /* XENO_FIELD_STEP_PAD_INPUT (test_input.c): field-step-clocked
         * presses on the physical pad, so every control mask still applies.
         * An invalid schedule stops the run instead of running without it. */
        extern int PcPort_FieldStepPadApply(unsigned char *,
                                            unsigned short (*)(unsigned short), int);
        extern unsigned char g_C1Buffer[];
        extern int g_GameSceneMapNum;

        if (PcPort_FieldStepPadApply(g_C1Buffer, PcPort_PadTestGameToPhysical,
                                     g_GameSceneMapNum) != 0)
            exit(EXIT_FAILURE);
    }
    PcPort_FieldStepEvidenceOnVblank();
    PcPort_PadOnControl();
}

void PcPort_BeforeControllerPush(void)
{
    /* The preview offers no inventory or card screen for scripted test input. */
}

/* TEST TOOLING: XENO_DETERMINISTIC_VBLANK=<K> -- a jitter-free
 * test-determinism clock, NOT a retail-timing claim.  The port's game vblank
 * count normally follows host wall time, so how many vblanks one field
 * iteration spans varies with host speed (usually 2, sometimes 3), and
 * everything clocked by vblanks (pad pushes, Vsync(1) frame deltas,
 * vblank-driven script counters, the lock-step sound tick) varies with it.
 * From XENO_FIELD_STEP_PAD_INPUT arming (boot and title keep the host clock),
 * or with XENO_DETERMINISTIC_VBLANK_FROM_FIELD=<map> from the first vblank in
 * that field (e.g. 490, the title map: the title's frame count is otherwise
 * host-paced, and its per-frame effects consume a varying number of rand()
 * calls before a later slice arms), each blocking Vsync in the live field loop (frozen while loading)
 * advances a virtual count by exactly K (Vsync(n>=2): n), Vsync(1) returns it,
 * and the vblank service (controller_vblank_service.c) follows it instead of
 * the host counter.  Presentation is still paced by the host.  Retail's own
 * per-frame vblank cost, and its slowdowns, are not modelled: K is a fixed
 * cadence (2 for the field, the port's usual no-jitter value). */
static int s_detVblankK = -1;
static int s_detVblankOn;         /* armed: the test clock is in charge of Vsync(1) */
static int s_detVblankLive;       /* the field loop is running (a drain since the last load) */
static uint32_t s_detVblankNow;   /* service clock while live (service cursor domain) */
static uint32_t s_detQuery;       /* game-visible clock returned by Vsync(1) */
static int s_detLastQuery;        /* last host Vsync(1) value before arming */
static unsigned s_detLoadStep;    /* drain count at the last load */
static unsigned s_detDrains;      /* FieldPollControllers drains since boot */
static int s_detFromField = -2;   /* XENO_DETERMINISTIC_VBLANK_FROM_FIELD, -1 unset */
/* Busy-wait polls: the field's frame wait (func_8007554C) spins on Vsync(-1)
 * until s1 + D_800B217C + 2 -- the script frame rate (0/1/2 = 30/20/15 fps).
 * The test clock adds K per blocking Vsync only, so at 20/15 fps the target
 * was never reached (follow-up d, the map-2 hang). A Vsync(-1) whose
 * previous Vsync call was also a Vsync(-1) returning the same count is a
 * spin: it advances the test clock by one vblank, as real vblanks keep
 * arriving on hardware while the game spins. Any Vsync(1) or blocking Vsync
 * ends the spin, so the per-frame s1 read and the first loop check never
 * advance; at 30 fps nothing changes. */
static int s_detPollPending;
static uint32_t s_detPollLast;
static uint32_t s_detSpinVblanks;

/* Every FieldPollControllers drain, armed or not (test_input.c). */
void PcPort_DetVblankNoteDrain(void)
{
    s_detDrains++;
}
static int s_detPrevLatch;

static int PcPort_DetVblankK(void)
{
    if (s_detVblankK < 0) {
        const char* e = getenv("XENO_DETERMINISTIC_VBLANK");
        s_detVblankK = (e != NULL && e[0] != '\0') ? atoi(e) : 0;
        if (s_detVblankK < 0 || s_detVblankK > 8)
            s_detVblankK = 0;
    }
    return s_detVblankK;
}

static int PcPort_DetVblankActive(void)
{
    return s_detVblankLive;
}

/* TEST TOOLING: XENO_DETERMINISTIC_VBLANK_BATTLE=1 (needs the armed test
 * clock above; not a retail-timing claim).  Without it the battle runs
 * host-paced: battle.bin is entered from a field teardown (a load) and has
 * no field-step drain, so the clock only goes live at the next field.  The
 * retail battle frame (func_800BE790) turns elapsed Vsync(-1) vblanks into
 * D_80059494 catch-up ticks (cap 4), so a faster host then gets more battle
 * frames for the same game time.  With the switch, battle entry
 * (func_80070F40) puts the service on the field's rule -- +K per blocking
 * Vsync (Vsync(n>=2): n), the Vsync(-1) spin rule -- seeded from the
 * service cursor, so elapsed vblanks per battle frame follow the game's own
 * calls.  Battle-internal loads (HeapAlloc, CD reads) are synchronous host
 * calls here, so they need no host-paced span.  A guest wait that makes
 * no Vsync call (a loop on a vblank-callback counter) would never see the
 * clock move, so a stall guard: after DET_BATTLE_STALL_VBLANKS host vblanks
 * (10 s) without any test-clock advance (blocking Vsync or Vsync(-1) spin)
 * the service follows the host, one vblank per host vblank, until the next
 * Vsync advance.  It never fires in a battle whose frames keep calling
 * Vsync; when it does fire it is logged and counted at return (that battle
 * is not frame-comparable).  Service queries are not counted: the battle
 * bridge pumps the service before every interpreted instruction, so query
 * counts differ between native and interpreted runs.  Battle return restores the saved live/load state and
 * keeps the service continuous: until the next field goes live it runs
 * host-paced from the battle's final cursor (host + offset), instead of
 * stalling until the host count catches up with a cursor that ran ahead.
 * The switch is read once, at the first battle entry. */
#define DET_BATTLE_STALL_VBLANKS 600u
static int s_detBattleEnv = -1;   /* XENO_DETERMINISTIC_VBLANK_BATTLE, read once */
static int s_detBattle;           /* inside a battle on the deterministic rule */
static int s_detBattleSavedLive;
static unsigned s_detBattleSavedLoadStep;
static uint32_t s_detBattleMark;      /* host vblank count at the last test-clock advance */
static int s_detBattleStalled;        /* stall guard: following the host until a Vsync */
static unsigned s_detBattleStalls;    /* stall-guard episodes this battle */
static uint32_t s_detBattleStallVblanks;  /* vblanks the guard added this battle */
static unsigned s_detBattleCount;
static int s_detHostOffsetOn;         /* after a deterministic battle: host + offset */
static uint32_t s_detHostOffset;

void PcPort_DetVblankBattleEnter(void)
{
    if (s_detBattleEnv < 0) {
        const char* e = getenv("XENO_DETERMINISTIC_VBLANK_BATTLE");
        s_detBattleEnv = (e != NULL && e[0] != '\0' && atoi(e) != 0) ? 1 : 0;
    }
    if (!s_detBattleEnv)
        return;
    s_detBattleCount++;
    if (PcPort_DetVblankK() == 0 || !s_detVblankOn) {
        fprintf(stderr, "[det-vblank] battle %u enter: XENO_DETERMINISTIC_VBLANK_BATTLE set but "
                "the test clock is not armed (K=%d armed=%d): battle stays host-paced\n",
                s_detBattleCount, PcPort_DetVblankK(), s_detVblankOn);
        return;
    }
    s_detBattle = 1;
    s_detBattleSavedLive = s_detVblankLive;
    s_detBattleSavedLoadStep = s_detLoadStep;
    s_detBattleStalled = 0;
    s_detBattleStalls = 0;
    s_detBattleStallVblanks = 0;
    s_detPollPending = 0;
    /* Not live (entered from a field teardown): the next service query seeds
     * s_detVblankNow from the service cursor, exactly as the field does. */
    fprintf(stderr, "[det-vblank] battle %u enter: deterministic battle clock K=%d "
            "(saved live=%d, drain %u; not a retail-timing claim)\n",
            s_detBattleCount, s_detVblankK, s_detBattleSavedLive, s_detDrains);
}

void PcPort_DetVblankBattleReturn(void)
{
    extern int PsyX_Sys_GetVBlankCount(void);
    if (!s_detBattle)
        return;
    s_detBattle = 0;
    s_detPollPending = 0;
    s_detVblankLive = s_detBattleSavedLive;
    s_detLoadStep = s_detBattleSavedLoadStep;
    if (!s_detVblankLive) {
        s_detHostOffset = s_detVblankNow - (uint32_t)PsyX_Sys_GetVBlankCount();
        s_detHostOffsetOn = 1;
    }
    fprintf(stderr, "[det-vblank] battle %u return: restored live=%d, cursor %u, "
            "stall-guard episodes %u (+%u vblanks)\n", s_detBattleCount, s_detVblankLive,
            s_detVblankNow, s_detBattleStalls, s_detBattleStallVblanks);
}

/* A test-clock advance by the game's own Vsync call: ends a stall span. */
static void PcPort_DetBattleNoteAdvance(void)
{
    extern int PsyX_Sys_GetVBlankCount(void);
    s_detBattleMark = (uint32_t)PsyX_Sys_GetVBlankCount();
    s_detBattleStalled = 0;
}

/* Two clocks.  The SERVICE clock (pad pushes, sound ticks, the callbacks a
 * field load waits on) stays host-paced while loading, so a load cannot
 * stall; it becomes deterministic (+K per blocking Vsync, seeded from the
 * service's own cursor, so no jump) once the field loop is live: a
 * field-step drain has happened since the last load (arming, or the field
 * teardown of an F12 reload).  The GAME-VISIBLE clock (Vsync(1), which the
 * field turns into frame deltas) advances only while live, so a load's
 * duration never becomes field time.  Hook for controller_vblank_service.c
 * (weak there); *now carries the service cursor in. */
int PcPort_DeterministicVblank(uint32_t* now)
{
    extern int PcPort_FieldStepPadArmedStep(unsigned *step);
    extern int PcPort_QuickCheckpointFieldIsActive(void);
    unsigned step;
    int latch;

    if (PcPort_DetVblankK() == 0)
        return 0;
    if (s_detFromField == -2) {
        const char* e = getenv("XENO_DETERMINISTIC_VBLANK_FROM_FIELD");
        s_detFromField = (e != NULL && e[0] != '\0') ? (atoi(e) & 0xFFF) : -1;
    }
    if (!PcPort_FieldStepPadArmedStep(&step)) {
        extern int g_GameSceneMapNum;
        step = 0;
        if (!s_detVblankOn && (s_detFromField < 0 || (g_GameSceneMapNum & 0xFFF) != s_detFromField))
            return 0;
    }
    latch = PcPort_QuickCheckpointFieldIsActive();
    if (!s_detVblankOn) {
        s_detVblankOn = 1;
        s_detLoadStep = s_detDrains;
        s_detQuery = (uint32_t)s_detLastQuery;
        fprintf(stderr, "[det-vblank] jitter-free test clock armed: K=%d at step %u drain %u "
                "(not a retail-timing claim)\n", s_detVblankK, step, s_detDrains);
    }
    if (s_detPrevLatch && !latch && !s_detBattle) {  /* field teardown: a load begins */
        s_detVblankLive = 0;
        s_detLoadStep = s_detDrains;
        fprintf(stderr, "[det-vblank] load: host-paced service from step %u drain %u\n", step,
                s_detDrains);
    }
    s_detPrevLatch = latch;
    if (s_detBattle) {                        /* XENO_DETERMINISTIC_VBLANK_BATTLE */
        extern int PsyX_Sys_GetVBlankCount(void);
        uint32_t h = (uint32_t)PsyX_Sys_GetVBlankCount();
        if (!s_detVblankLive) {
            s_detVblankLive = 1;
            s_detVblankNow = *now;
            s_detBattleMark = h;
            s_detBattleStalled = 0;
            fprintf(stderr, "[det-vblank] battle %u live: deterministic service, cursor %u\n",
                    s_detBattleCount, *now);
        }
        if (!s_detBattleStalled && h - s_detBattleMark >= DET_BATTLE_STALL_VBLANKS &&
            (int32_t)(h - s_detBattleMark) > 0) {
            s_detBattleStalled = 1;
            s_detBattleMark = h;
            if (s_detBattleStalls++ == 0)
                fprintf(stderr, "[det-vblank] battle %u stall guard: %u host vblanks without a "
                        "Vsync advance; following the host until the next Vsync (this battle "
                        "is not frame-comparable)\n", s_detBattleCount, DET_BATTLE_STALL_VBLANKS);
        }
        if (s_detBattleStalled && (int32_t)(h - s_detBattleMark) > 0) {
            uint32_t d = h - s_detBattleMark;
            s_detBattleMark = h;
            s_detVblankNow += d;
            s_detQuery += d;
            s_detBattleStallVblanks += d;
        }
        *now = s_detVblankNow;
        return 1;
    }
    if (!s_detVblankLive) {
        if (s_detDrains <= s_detLoadStep) {
            /* still loading: host-paced service */
            if (s_detHostOffsetOn) {
                /* after a deterministic battle: continue from its cursor */
                extern int PsyX_Sys_GetVBlankCount(void);
                uint32_t h = (uint32_t)PsyX_Sys_GetVBlankCount() + s_detHostOffset;
                if ((int32_t)(h - *now) > 0)
                    *now = h;
                return 1;
            }
            return 0;
        }
        s_detVblankLive = 1;
        s_detVblankNow = *now;
        fprintf(stderr, "[det-vblank] live: deterministic service from step %u, cursor %u\n",
                step, *now);
    }
    *now = s_detVblankNow;
    return 1;
}

int PcPort_DeterministicVblankLogGate(void)
{
    /* Evidence logs keyed to game time cover only the deterministic spans. */
    return PcPort_DetVblankK() == 0 || s_detVblankLive;
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
        if (mode < 0 && PcPort_DetVblankActive()) {
            if (s_detPollPending && s_detPollLast == s_detVblankNow) {
                s_detVblankNow++;
                s_detQuery++;
                if (s_detBattle)
                    PcPort_DetBattleNoteAdvance();
                if (s_detSpinVblanks++ == 0)
                    fprintf(stderr, "[det-vblank] busy-wait Vsync(-1) poll advances the test "
                            "clock by one vblank per spin (not a retail-timing claim)\n");
            }
            s_detPollPending = 1;
            s_detPollLast = s_detVblankNow;
        } else {
            s_detPollPending = 0;
        }
        count = VSync(mode);
        XENO_SERVICE_VBLANK();
        if (mode == 1) {
            /* Test clock: the game-visible count, frozen while loading. */
            if (s_detVblankOn)
                count = (int)s_detQuery;
            else
                s_detLastQuery = count;
        }
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
        extern int PsyX_GpuTraceOn(void);
        extern int g_xeno_swap_index;
        int sceneOpen = PsyX_IsSceneOpen();

        /* Diagnostic (XENO_GPU_TRACE, psycross_gpu_trace.patch). */
        if (PsyX_GpuTraceOn()) {
            printf("[gputrace] s=%d Vsync(%d) scene=%d\n", g_xeno_swap_index,
                   mode, sceneOpen);
            fflush(stdout);
        }

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
    /* XENO_DETERMINISTIC_VBLANK: this wait consumes exactly K (or n) game
     * vblanks; VSync below still paces presentation to the host. */
    /* Frozen while loading (field latch clear), so a host-timed load --
     * the fixture load or an F12 reload -- adds no game vblanks, pad
     * samples or sound ticks.  Test-clock behaviour, not retail timing. */
    s_detPollPending = 0;
    if (PcPort_DetVblankActive()) {
        uint32_t step_vblanks = (uint32_t)(mode >= 2 ? mode : s_detVblankK);
        s_detVblankNow += step_vblanks;
        s_detQuery += step_vblanks;
        if (s_detBattle)
            PcPort_DetBattleNoteAdvance();
    }
    count = VSync(mode);
    XENO_SERVICE_VBLANK();
    if (s_detVblankOn)
        count = (int)s_detQuery;

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
