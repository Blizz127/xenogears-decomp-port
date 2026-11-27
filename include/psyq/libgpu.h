#ifndef XG_CONSOLE_GPU_H
#define XG_CONSOLE_GPU_H

#ifdef XENO_PC_PORT
#include "../../pc_port/include_shim/psyq/libgpu.h"
#else

#include "sys/types.h"
#include "libgte.h"

/* PSX GPU packets follow the DMA link word with little-endian GP0 words.
 * Vertex and texture words are described by psx-spx, Graphics Processing Unit.
 * Field names and environment/image records are the Xenogears call-site ABI. */
typedef struct {
    short x, y, w, h;
} RECT;

typedef struct {
    int mode;
    RECT *crect;
    u_long *caddr;
    RECT *prect;
    u_long *paddr;
} TIM_IMAGE;

typedef struct {
    unsigned int addr : 24;
    unsigned int len : 8;
    unsigned char r0, g0, b0, code;
} P_TAG;

#define XG_GP0_HEADER \
    unsigned int tag; \
    unsigned char r0, g0, b0, code
#define XG_GP0_XY(i) short x##i, y##i
#define XG_GP0_RGB(i) unsigned char r##i, g##i, b##i, p##i
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
    XG_GP0_RGB(1); XG_GP0_XY(1);
    XG_GP0_RGB(2); XG_GP0_XY(2);
} POLY_G3;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0);
    XG_GP0_RGB(1); XG_GP0_XY(1);
    XG_GP0_RGB(2); XG_GP0_XY(2);
    XG_GP0_RGB(3); XG_GP0_XY(3);
} POLY_G4;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0); XG_GP0_UV(0, clut);
    XG_GP0_RGB(1); XG_GP0_XY(1); XG_GP0_UV(1, tpage);
    XG_GP0_RGB(2); XG_GP0_XY(2); XG_GP0_UV(2, pad2);
} POLY_GT3;

typedef struct {
    XG_GP0_HEADER;
    XG_GP0_XY(0); XG_GP0_UV(0, clut);
    XG_GP0_RGB(1); XG_GP0_XY(1); XG_GP0_UV(1, tpage);
    XG_GP0_RGB(2); XG_GP0_XY(2); XG_GP0_UV(2, pad2);
    XG_GP0_RGB(3); XG_GP0_XY(3); XG_GP0_UV(3, pad3);
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
    XG_GP0_RGB(1); XG_GP0_XY(1);
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

#undef XG_GP0_UV
#undef XG_GP0_RGB
#undef XG_GP0_XY
#undef XG_GP0_HEADER

typedef struct { unsigned int tag, code[1]; } DR_TPAGE;
typedef struct { unsigned int tag, code[2]; } DR_MODE;
typedef struct { unsigned int tag, code[5]; } DR_MOVE;
typedef struct { unsigned int tag, code[15]; } DR_ENV;

typedef struct {
    RECT clip;
    short ofs[2];
    RECT tw;
    unsigned short tpage;
    unsigned char dtd, dfe;
    unsigned char isbg, r0, g0, b0;
    DR_ENV dr_env;
} DRAWENV;

typedef struct {
    RECT disp, screen;
    unsigned char isinter, isrgb24, pad0, pad1;
} DISPENV;

/* PSX GPU packets carry the DMA link in the low 24 bits of their first
 * word and the number of following GP0 words in its top byte. */
typedef struct {
    unsigned int addr : 24;
    unsigned int len : 8;
} XgGpuLink;

#define setlen(packet, count) (((XgGpuLink *)(packet))->len = (count))
#define setaddr(packet, target) \
    (((XgGpuLink *)(packet))->addr = (unsigned long)(target))

/* Link the previous head first. The table slot is updated only after the
 * packet has acquired its successor. The upper byte of each word survives. */
#define addPrim(ordering, packet) \
    (setaddr(packet, ((XgGpuLink *)(ordering))->addr), setaddr(ordering, packet))

#define getTPage(depth, blend, x, y) \
    ((((unsigned int)(x) >> 6) & 15u) | (((unsigned int)(y) >> 4) & 16u) | \
     (((unsigned int)(blend) & 3u) << 5) | (((unsigned int)(depth) & 3u) << 7) | \
     (((unsigned int)(y) & 512u) << 2))
#define getClut(x, y) \
    ((((unsigned int)(x) >> 4) & 63u) | (((unsigned int)(y) & 1023u) << 6))
#define setTPage(packet, depth, blend, x, y) \
    ((packet)->tpage = GetTPage(depth, blend, x, y))
#define setClut(packet, x, y) ((packet)->clut = GetClut(x, y))

/* The GP0 command byte is the high byte of the first payload word. */
#define XG_GPU_OPCODE(packet) (((unsigned char *)(packet))[7])
#define setSemiTrans(packet, enabled) do { \
    unsigned char *xg_op = &XG_GPU_OPCODE(packet); \
    *xg_op = (*xg_op & ~2u) | ((enabled) ? 2u : 0u); \
} while (0)
#define setShadeTex(packet, raw) do { \
    unsigned char *xg_op = &XG_GPU_OPCODE(packet); \
    *xg_op = (*xg_op & ~1u) | ((raw) ? 1u : 0u); \
} while (0)
#define XG_GPU_INIT(packet, length, command) do { \
    void *xg_packet = (packet); \
    setlen(xg_packet, length); \
    XG_GPU_OPCODE(xg_packet) = (command); \
} while (0)
#define setTile(packet) XG_GPU_INIT(packet, 3, 0x60)
#define setSprt(packet) XG_GPU_INIT(packet, 4, 0x64)
#define setPolyFT4(packet) XG_GPU_INIT(packet, 9, 0x2c)

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
int ClearImage(RECT *area, u_char red, u_char green, u_char blue);
int MoveImage(RECT *source, int x, int y);
int LoadImage(RECT *area, u_long *source);
int StoreImage(RECT *area, u_long *destination);
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
void SetDrawMode(DR_MODE *packet, int display, int dither, int page, RECT *window);
void SetDrawTPage(DR_TPAGE *packet, int display, int dither, int page);
void SetDrawMove(DR_MOVE *packet, RECT *source, int x, int y);
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

#ifdef __cplusplus
}
#endif

#endif /* XENO_PC_PORT */
#endif /* XG_CONSOLE_GPU_H */
