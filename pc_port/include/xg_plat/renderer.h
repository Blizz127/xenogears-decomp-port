/* xg_plat/renderer.h -- renderer interface: window/output setup and
 * presentation options.  The game still submits PSX primitives through the
 * PsyCross libgpu layer; this interface owns what a replacement renderer
 * would decide (output size, filtering, vsync, aspect).  Backend: PsyCross
 * (src/plat/xg_plat_psycross.c). */
#ifndef XG_PLAT_RENDERER_H
#define XG_PLAT_RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int window_width;    /* output window size in pixels */
    int window_height;
    int fullscreen;      /* 0/1 */
    int vsync;           /* 0/1: swap interval */
    int bilinear;        /* 0/1: bilinear texture filtering */
    int widescreen;      /* 0/1: 16:9 view (needs a backend that supports it) */
} XgPlatVideoConfig;

/* Defaults: 640x480 window, vsync on, no filtering, 4:3. */
void xg_plat_renderer_default_config(XgPlatVideoConfig* cfg);
/* Open the output.  Options the backend cannot honour are reported once
 * and ignored; returns the number of such options. */
int  xg_plat_renderer_init(const char* title, const XgPlatVideoConfig* cfg);
/* 1 when the backend implements the option (e.g. "widescreen"). */
int  xg_plat_renderer_supports(const char* option);

/* VRAM upload (the game's LoadImage): `pixels` holds w*h 16-bit words for
 * the rect at (x, y) in the 1024x512 VRAM.  Routed here so a backend can
 * cache or replace textures; mods replace an upload keyed by the SHA-256 of
 * its original pixels (same size only: VRAM layout is fixed).  Returns the
 * backend's LoadImage result. */
int  xg_plat_renderer_upload_image(int x, int y, int w, int h, const void* pixels);

/* Frame submission.  The game builds PSX-format data (an ordering table of
 * linked GPU primitives, DRAWENV / DISPENV structs); these entry points take
 * it as-is, so a backend other than PsyCross interprets that format.  The
 * game's DrawOTag / PutDrawEnv / PutDispEnv / ClearOTag(R) calls arrive here
 * (link-time routing, build_port.sh XG_PLAT_WRAP_SYMS). */
void  xg_plat_renderer_draw_ot(void* ot);                 /* draw a linked primitive list */
void* xg_plat_renderer_set_draw_env(void* drawenv);       /* drawing area, offset, clip */
void* xg_plat_renderer_set_display_env(void* dispenv);    /* displayed framebuffer area */
void* xg_plat_renderer_clear_ot(void* ot, int n, int reverse); /* link n empty entries */

#ifdef __cplusplus
}
#endif

#endif
