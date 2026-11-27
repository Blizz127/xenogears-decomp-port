#ifndef XG_RENDERER_LIBGPU_H
#define XG_RENDERER_LIBGPU_H

#include <limits.h>
#include <stdint.h>
#include "psx/types.h"
#include "PsyX/common/pgxp_defs.h"

/* Compact packets used by the Xenogears ports of PsyCross e56e4cde.
 * GP0 payloads follow the project's independently authored console GPU ABI.
 * Native callers keep full pointers in TIM views and unsigned long OT slots.
 * Only the first four bytes of an OT slot participate in DMA linkage. */
#if USE_EXTENDED_PRIM_POINTERS || USE_PGXP
#error "The Xenogears renderer requires its compact primitive configuration"
#endif

#define P_LEN 1
#pragma pack(push, 1)

typedef struct _RECT16 {
    int16_t x, y, w, h;
} RECT16;

typedef struct {
    uint32_t mode;
    RECT16 *cRECT16;
    uint32_t *caddr;
    RECT16 *pRECT16;
    uint32_t *paddr;
} TIM_IMAGE;

typedef struct {
    unsigned int addr : 24;
    unsigned int len : 8;
#if ULONG_MAX > UINT32_MAX
    uint32_t slotPadding;
#endif
} OT_TAG;

typedef struct {
    unsigned int addr : 24;
    unsigned int len : 8;
    uint8_t pad0, pad1, pad2, code;
} P_TAG;

#define XG_GP0_HEADER \
    unsigned int tag; \
    unsigned char r0, g0, b0, code
#define XG_GP0_XY(i) short x##i, y##i
#define XG_GP0_RGB(i, reserved) unsigned char r##i, g##i, b##i, reserved##i
#define XG_GP0_UV(i, attribute) \
    unsigned char u##i, v##i; \
    unsigned short attribute

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_XY(1);
    XG_GP0_XY(2);
} POLY_F3;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_XY(1);
    XG_GP0_XY(2);
    XG_GP0_XY(3);
} POLY_F4;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_UV(0, clut);
    XG_GP0_XY(1);
    XG_GP0_UV(1, tpage);
    XG_GP0_XY(2);
    XG_GP0_UV(2, pad1);
} POLY_FT3;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_UV(0, clut);
    XG_GP0_XY(1);
    XG_GP0_UV(1, tpage);
    XG_GP0_XY(2);
    XG_GP0_UV(2, pad1);
    XG_GP0_XY(3);
    XG_GP0_UV(3, pad2);
} POLY_FT4;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_RGB(1, pad); XG_GP0_XY(1);
    XG_GP0_RGB(2, pad); XG_GP0_XY(2);
} POLY_G3;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_RGB(1, pad); XG_GP0_XY(1);
    XG_GP0_RGB(2, pad); XG_GP0_XY(2);
    XG_GP0_RGB(3, pad); XG_GP0_XY(3);
} POLY_G4;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0); XG_GP0_UV(0, clut);
    XG_GP0_RGB(1, p); XG_GP0_XY(1); XG_GP0_UV(1, tpage);
    XG_GP0_RGB(2, p); XG_GP0_XY(2); XG_GP0_UV(2, pad2);
} POLY_GT3;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0); XG_GP0_UV(0, clut);
    XG_GP0_RGB(1, p); XG_GP0_XY(1); XG_GP0_UV(1, tpage);
    XG_GP0_RGB(2, p); XG_GP0_XY(2); XG_GP0_UV(2, pad2);
    XG_GP0_RGB(3, p); XG_GP0_XY(3); XG_GP0_UV(3, pad3);
} POLY_GT4;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_XY(1);
} LINE_F2;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_XY(1);
    XG_GP0_XY(2);
    unsigned int pad;
} LINE_F3;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_XY(1);
    XG_GP0_XY(2);
    XG_GP0_XY(3);
    unsigned int pad;
} LINE_F4;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_RGB(1, p); XG_GP0_XY(1);
} LINE_G2;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    short w, h;
} TILE;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_UV(0, clut);
    short w, h;
} SPRT;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_UV(0, clut);
} SPRT_16;

typedef SPRT_16 SPRT_8;
typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
} TILE_1;
typedef TILE_1 TILE_8;
typedef TILE_1 TILE_16;

#undef XG_GP0_UV
#undef XG_GP0_RGB
#undef XG_GP0_XY
#undef XG_GP0_HEADER

typedef struct { unsigned int tag, code[1]; } DR_TPAGE;
typedef struct { unsigned int tag, code[2]; } DR_MODE;
typedef struct { unsigned int tag, code[5]; } DR_MOVE;
typedef struct { unsigned int tag, code[15]; } DR_ENV;

typedef struct {
    RECT16 clip;
    short ofs[2];
    RECT16 tw;
    unsigned short tpage;
    unsigned char dtd, dfe;
    unsigned char isbg, r0, g0, b0;
    DR_ENV dr_env;
} DRAWENV;

typedef struct {
    RECT16 disp, screen;
    unsigned char isinter, isrgb24, pad0, pad1;
} DISPENV;

typedef struct { uint32_t tag, code[2]; } DR_AREA;
typedef DR_AREA DR_PSYX_TEX;
typedef struct { uint32_t tag, code[3], p[13]; } DR_LOAD;
typedef struct { uint32_t tag, code; const char *text; } DR_PSYX_DBGMARKER;

#pragma pack(pop)

/* Read and write bytes so callers may link packed packets and unaligned
 * emulated-memory views without aliasing through a host-sized pointer. */
static inline uint32_t xgGpuReadAddress(const unsigned char *link)
{
    return link[0] | ((uint32_t)link[1] << 8) | ((uint32_t)link[2] << 16);
}

static inline void xgGpuWriteAddress(unsigned char *link, uintptr_t address)
{
    link[0] = (uint8_t)address;
    link[1] = (uint8_t)(address >> 8);
    link[2] = (uint8_t)(address >> 16);
}

#define XG_GPU_BYTES(p) ((unsigned char *)(uintptr_t)(p))
#define getaddr(p) xgGpuReadAddress(XG_GPU_BYTES(p))
#define setaddr(p, to) xgGpuWriteAddress(XG_GPU_BYTES(p), (uintptr_t)(to))
#define getlen(p) (XG_GPU_BYTES(p)[3])
#define setlen(p, n) (getlen(p) = (uint8_t)(n))
#define getcode(p) (XG_GPU_BYTES(p)[7])
#define setcode(p, value) (getcode(p) = (uint8_t)(value))
#define nextPrim(p) ((void *)(uintptr_t)getaddr(p))
#define isendprim(p) (getaddr(p) == 0xffffffu)
#define termPrim(p) setaddr(p, 0xffffffu)
#define catPrim(first, next) setaddr(first, next)

static inline void xgGpuPrepend(void *head, void *first, void *last)
{
    setaddr(last, getaddr(head));
    setaddr(head, first);
}
#define addPrims(head, first, last) xgGpuPrepend((head), (first), (last))
static inline void addPrim(void *head, void *packet)
{
    xgGpuPrepend(head, packet, packet);
}

#define getTPage(depth, blend, x, y) \
    ((((unsigned int)(x) >> 6) & 15u) | (((unsigned int)(y) >> 4) & 16u) | \
     (((unsigned int)(blend) & 3u) << 5) | (((unsigned int)(depth) & 3u) << 7) | \
     (((unsigned int)(y) & 512u) << 2))
#define getClut(x, y) \
    ((((unsigned int)(x) >> 4) & 63u) | (((unsigned int)(y) & 1023u) << 6))
#define setTPage(packet, depth, blend, x, y) \
    ((packet)->tpage = GetTPage(depth, blend, x, y))

static inline void xgGpuCommandFlag(void *packet, uint8_t mask, int enabled)
{
    uint8_t code = getcode(packet);
    setcode(packet, enabled ? code | mask : code & ~mask);
}
#define setSemiTrans(packet, enabled) xgGpuCommandFlag(packet, 2, enabled)
#define setShadeTex(packet, raw) xgGpuCommandFlag(packet, 1, raw)

static inline void xgGpuInit(void *packet, uint8_t words, uint8_t command)
{
    setlen(packet, words);
    setcode(packet, command);
}

/* Fixed-size textured sprites carry three GP0 words: color/opcode, XY and UV/CLUT. */
#define setTile(p) xgGpuInit(p, 3, 0x60)
#define setSprt(p) xgGpuInit(p, 4, 0x64)
#define setSprt8(p) xgGpuInit(p, 3, 0x74)
#define setSprt16(p) xgGpuInit(p, 3, 0x7c)
#define setLineG2(p) xgGpuInit(p, 4, 0x50)
#define setPolyF3(p) xgGpuInit(p, 4, 0x20)
#define setPolyF4(p) xgGpuInit(p, 5, 0x28)
#define setPolyG3(p) xgGpuInit(p, 6, 0x30)
#define setPolyG4(p) xgGpuInit(p, 8, 0x38)
#define setPolyFT3(p) xgGpuInit(p, 7, 0x24)
#define setPolyFT4(p) xgGpuInit(p, 9, 0x2c)
#define setPolyGT3(p) xgGpuInit(p, 9, 0x34)
#define setPolyGT4(p) xgGpuInit(p, 12, 0x3c)

static inline void xgGpuLine(void *packet, uint8_t words, uint8_t command)
{
    xgGpuInit(packet, words, command);
    /* The final GP0 word terminates the polyline. */
    uint8_t *end = XG_GPU_BYTES(packet) + 4u * words;
    end[0] = end[1] = end[2] = end[3] = 0x55;
}
#define setLineF3(p) xgGpuLine(p, 5, 0x48)

#define XG_GPU_COLOR(p, i, red, green, blue) \
    ((p)->r##i = (red), (p)->g##i = (green), (p)->b##i = (blue))
#define setRGB0(p, r, g, b) XG_GPU_COLOR(p, 0, r, g, b)
#define setRGB1(p, r, g, b) XG_GPU_COLOR(p, 1, r, g, b)
#define setRGB2(p, r, g, b) XG_GPU_COLOR(p, 2, r, g, b)
#define setRGB3(p, r, g, b) XG_GPU_COLOR(p, 3, r, g, b)

#define XG_GPU_XY(p, i, xval, yval) ((p)->x##i = (xval), (p)->y##i = (yval))
#define XG_GPU_UV(p, i, uval, vval) ((p)->u##i = (uval), (p)->v##i = (vval))
#define setXY0(p, a, b) XG_GPU_XY(p, 0, a, b)
#define setXY2(p, a, b, c, d) (setXY0(p, a, b), XG_GPU_XY(p, 1, c, d))
#define setXY3(p, a, b, c, d, e, f) (setXY2(p, a, b, c, d), XG_GPU_XY(p, 2, e, f))
#define setXY4(p, a, b, c, d, e, f, g, h) \
    (setXY3(p, a, b, c, d, e, f), XG_GPU_XY(p, 3, g, h))
#define setUV0(p, a, b) XG_GPU_UV(p, 0, a, b)
#define setUV3(p, a, b, c, d, e, f) \
    (setUV0(p, a, b), XG_GPU_UV(p, 1, c, d), XG_GPU_UV(p, 2, e, f))
#define setUV4(p, a, b, c, d, e, f, g, h) \
    (setUV3(p, a, b, c, d, e, f), XG_GPU_UV(p, 3, g, h))
#define setWH(p, width, height) ((p)->w = (width), (p)->h = (height))
#define setRECT(p, left, top, width, height) \
    ((p)->x = (left), (p)->y = (top), setWH(p, width, height))
#define setXYWH(p, x, y, w, h) \
    setXY4(p, x, y, (x)+(w), y, x, (y)+(h), (x)+(w), (y)+(h))
#define setUVWH(p, u, v, w, h) \
    setUV4(p, u, v, (u)+(w), v, u, (v)+(h), (u)+(w), (v)+(h))

static inline uint32_t xgGpuDrawPage(int display, int dither, int page)
{
    return 0xe1000000u | ((uint32_t)page & 0x9ffu)
           | ((display != 0) << 10) | ((dither != 0) << 9);
}

static inline void setDrawTPage(DR_TPAGE *packet, int display, int dither, int page)
{
    setlen(packet, 1);
    packet->code[0] = xgGpuDrawPage(display, dither, page);
}

/* E2 selects five-bit fields in units of eight texels. */
static inline uint32_t xgGpuWindowField(int value)
{
    return ((uint32_t)value >> 3) & 31u;
}

static inline void setDrawMode(DR_MODE *packet, int display, int dither, int page,
                               const RECT16 *window)
{
    setlen(packet, 2);
    packet->code[0] = xgGpuDrawPage(display, dither, page);
    packet->code[1] = window ? 0xe2000000u | xgGpuWindowField(-window->w)
                             | (xgGpuWindowField(-window->h) << 5)
                             | (xgGpuWindowField(window->x) << 10)
                             | (xgGpuWindowField(window->y) << 15) : 0;
}

static inline uint32_t xgGpuPair(int16_t lo, int16_t hi)
{
    return (uint16_t)lo | ((uint32_t)(uint16_t)hi << 16);
}

static inline void setDrawLoad(DR_LOAD *packet, const RECT16 *area)
{
    int pixels = (int)area->w * area->h;
    if (pixels < 0 || pixels > 24) {
        setlen(packet, 0);
        return;
    }
    int words = (pixels + 1) / 2;
    setlen(packet, 4 + words);
    packet->code[0] = 0xa0000000u;
    packet->code[1] = xgGpuPair(area->x, area->y);
    packet->code[2] = xgGpuPair(area->w, area->h);
    packet->p[words] = 0x01000000u;
}

#ifdef __cplusplus
extern "C" {
#endif

int ResetGraph(int mode);
int SetGraphDebug(int level);
void SetDispMask(int enabled);
DISPENV *SetDefDispEnv(DISPENV *display, int framebuffer_x, int framebuffer_y,
                       int width, int height);
DISPENV *PutDispEnv(DISPENV *env);
DRAWENV *SetDefDrawEnv(DRAWENV *draw, int clip_x, int clip_y,
                       int width, int height);
DRAWENV *PutDrawEnv(DRAWENV *env);
DRAWENV *GetDrawEnv(DRAWENV *env);

int DrawSync(int mode);
u_int DrawSyncCallback(void (*callback)(void));
int ClearImage(RECT16 *area, u_char red, u_char green, u_char blue);
int MoveImage(RECT16 *source, int x, int y);
int LoadImage(RECT16 *area, u_long *source);
int StoreImage(RECT16 *area, u_long *destination);
int OpenTIM(u_long *image);
TIM_IMAGE *ReadTIM(TIM_IMAGE *image);

u_long *ClearOTagR(u_long *ordering, int count);
void AddPrim(void *ordering, void *packet);
void AddPrims(void *ordering, void *first, void *last);
void TermPrim(void *packet);
void DrawOTag(u_long *ordering);
void DrawOTagEnv(u_long *ordering, DRAWENV *env);
void DrawPrim(void *packet);

u_short GetTPage(int depth, int blend, int x, int y);
u_short GetClut(int x, int y);
void SetDrawMode(DR_MODE *packet, int display, int dither, int page, RECT16 *window);
void SetDrawTPage(DR_TPAGE *packet, int display, int dither, int page);
void SetDrawMove(DR_MOVE *packet, RECT16 *source, int x, int y);
void SetSemiTrans(void *packet, int enabled);
void SetShadeTex(void *packet, int raw);
void SetTile(TILE *packet);
void SetSprt(SPRT *packet);
void SetLineF2(LINE_F2 *packet);
void SetLineF3(LINE_F3 *packet);
void SetLineG2(LINE_G2 *packet);
void SetPolyF3(POLY_F3 *packet);
void SetPolyF4(POLY_F4 *packet);
void SetPolyG3(POLY_G3 *packet);
void SetPolyG4(POLY_G4 *packet);
void SetPolyFT3(POLY_FT3 *packet);
void SetPolyFT4(POLY_FT4 *packet);
void SetPolyGT3(POLY_GT3 *packet);
void SetPolyGT4(POLY_GT4 *packet);

int ClearImage2(RECT16 *area, u_char red, u_char green, u_char blue);
int LoadImage2(RECT16 *area, u_long *source);
int StoreImage2(RECT16 *area, u_long *destination);
DISPENV *GetDispEnv(DISPENV *env);
int GetODE(void);
u_long *ClearOTag(u_long *ordering, int count);
u_long *BreakDraw(void);
int MargePrim(void *first, void *second);
void CatPrim(void *first, void *next);
void SetDrawEnv(DR_ENV *packet, DRAWENV *env);
void SetDrawArea(DR_AREA *packet, RECT16 *area);
void SetDrawLoad(DR_LOAD *packet, RECT16 *area);
u_short LoadClut(u_long *palette, int x, int y);
u_short LoadClut2(u_long *palette, int x, int y);
void SetSprt8(SPRT_8 *packet);
void SetSprt16(SPRT_16 *packet);
void SetPsyXTexture(DR_PSYX_TEX *packet, unsigned int texture, int width, int height);
void SetPsyXDebugMarker(DR_PSYX_DBGMARKER *packet, const char *text);
void SetDumpFnt(int id);
void FntLoad(int x, int y);
int FntOpen(int x, int y, int width, int height, int background, int capacity);
int FntPrint(char *format, ...);
u_long *FntFlush(void);
u_long *KanjiFntFlush(int id);
int KanjiFntOpen(int x, int y, int width, int height, int dx, int dy, int cx,
                 int cy, int background, int capacity);

#ifdef __cplusplus
}
#endif

#endif
