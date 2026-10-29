/* HD-2D enhancement lane: hook contract (C sources in pc_port/src/hd2d).
 *
 * Every hook is a weak reference at its call site: with no definition linked
 * the port renders exactly as retail. A definition must do nothing (return 0)
 * while its enhancement is off; pc_port/tests/run_hd2d_off_identity_test.sh
 * proves the presented frames are then bit-identical.
 *
 * HOOK 1  post-process (PsyCross PsyX_main.cpp, psycross_hd2d_hooks.patch)
 *   void PcPort_Hd2dPostProcess(int path, int drawable_w, int drawable_h);
 *   Called at both present sites (PsyX_EndScene path 0, PsyX_PresentDisplay-
 *   FromVRAM path 1) just before PsyX_RecordPresentedFrame(). On entry the
 *   default framebuffer holds the finished frame; restore every GL state the
 *   hook touches (program, bindings, blend, viewport, scissor, ...).
 *
 * HOOK 2  sprites (pc_port/src/fei_hd2d.c, offered from func_8001E3D8)
 *   The three functions below, with PcPortHd2dSpriteInfo / RetailQuad.
 *
 * HOOK 3  area textures (PsyCross PsyX_GPU.cpp, psycross_hd2d_hooks.patch)
 *   int PcPort_Hd2dTextureOverride(int tpage, int clut, int u0, int v0,
 *           int u1, int v1, unsigned int *out_gl_texture, float out_uv[4]);
 *   Offered by every textured FT3/FT4/GT3/GT4/SPRT/SPRT_8/SPRT_16 before its
 *   vertices are emitted (not while a DR_PSYX_TEX override is bound). u0..v1
 *   are the primitive's retail UV bounds (inclusive texels). Return 1 with an
 *   RGBA GL texture and out_uv = {s0, t0, s1, t1}: the GL texcoords at the
 *   (u0, v0) and (u1, v1) corners, linear in between. The primitive keeps its
 *   screen position, OT order, colour modulation (128 = neutral) and blend
 *   mode (ABR 0 = 50/50); texels with alpha < 0.5 are cut out as retail
 *   texel 0 is. Cost: one draw split (batch break + bind) per overridden run.
 *
 * Menu: PcPort_DevMenuRegisterEnhancement (pc_port/src/port_dev_menu.h). */
#ifndef PC_PORT_HD2D_SPRITE_H
#define PC_PORT_HD2D_SPRITE_H
#ifdef __cplusplus
extern "C" {
#endif

/* identity: FNV-1a 32 (offset 0x811C9DC5, prime 0x01000193) over, in order,
 *   1. the sprite's frame table: n = (header & 0x1FF) + 1 little-endian u16
 *      entries at SpriteAnimPackage.pFrames (sprite+0x24 -> package +0x0;
 *      header is its first u16; n capped at 512) -- the frame offsets, so it
 *      is a content hash, independent of where the package was loaded;
 *   2. the package's TIM VRAM origin (package +0x4: s16 x, s16 y) and CLUT
 *      VRAM origin (package +0x8: s16 x, s16 y), each as 2 little-endian
 *      bytes.
 * With no package or frame table the frame-table part is omitted.
 * frame:  the sprite's current frame cursor (sprite+0x34, s16).
 * actor:  field actor index whose pSpriteData is this sprite, else -1.
 * sprite: the retail SpriteData (guest RAM). */
typedef struct { unsigned int identity; int frame; int actor; void* sprite; } PcPortHd2dSpriteInfo;

/* One retail part: the POLY_FT4 exactly as func_8001E3D8 built it (screen
 * x/y and u/v in POLY_FT4 vertex order 0..3), and the OT slot it would be
 * linked at. `poly` is the retail packet in the GPU work buffer. */
typedef struct { short x[4], y[4]; unsigned char u[4], v[4]; unsigned short tpage, clut; void* ot; void* poly; } PcPortHd2dRetailQuad;

/* Begin: once per sprite draw call. Nonzero = offer this sprite's parts to
 * SpriteDraw (absent Begin counts as nonzero).
 * Draw: once per drawn part (part = 0.. in retail link order). Return 1 when
 * the hook has drawn the part itself (linked its own packets at quad->ot,
 * e.g. with PcPort_AddPrimDomainAware from guest_prim_link.h, or relinked
 * quad->poly): the retail packet is then not linked. Return 0 to leave the
 * part to the built-in Fei path and then retail.
 * End: once per sprite whose Begin was offered, after its parts are linked. */
int PcPort_Hd2dSpriteBegin(const PcPortHd2dSpriteInfo* info);
int PcPort_Hd2dSpriteDraw(const PcPortHd2dSpriteInfo* info, int part, const PcPortHd2dRetailQuad* quad);
void PcPort_Hd2dSpriteEnd(const PcPortHd2dSpriteInfo* info);

void PcPort_Hd2dPostProcess(int path, int drawable_w, int drawable_h);
int PcPort_Hd2dTextureOverride(int tpage, int clut, int u0, int v0, int u1, int v1,
                               unsigned int* out_gl_texture, float out_uv[4]);

/* Exposed for tests: the identity of a retail SpriteData. */
unsigned int PcPort_Hd2dSpriteIdentity(const void* sprite);

#ifdef __cplusplus
}
#endif
#endif
