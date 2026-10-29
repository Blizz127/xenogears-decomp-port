/* Test-only pc_port/src/hd2d-style source for run_hd2d_off_identity_test.sh:
 * defines every HD-2D lane hook, all "off". It still takes every offer the
 * port makes (SpriteBegin says yes, so every sprite part is handed to
 * SpriteDraw with its RetailQuad filled), then declines to draw, so the
 * presented frames must be bit-identical to a build without the lane.
 * Call counts go to stderr so the test can prove the hooks really ran.
 * -DHD2D_OFF_TEST_MUTANT (XENO_HD2D_OFF_TEST_MUTANT=1) makes the post-process
 * hook paint one 4x4 corner block, a control that the gate must FAIL. */
#include <stdio.h>
#ifdef HD2D_OFF_TEST_MUTANT
#include <GL/gl.h>
#endif
#include "hd2d_sprite.h"
#include "port_dev_menu.h"

static unsigned long s_post, s_tex, s_begin, s_draw, s_end;
static unsigned int s_idMix;
static int s_on; /* the enhancement's switch: always off in this test */

static void Report(void)
{
    fprintf(stderr, "[hd2d-off-hooks] post=%lu tex=%lu sprite_begin=%lu sprite_draw=%lu sprite_end=%lu idmix=%08x\n",
            s_post, s_tex, s_begin, s_draw, s_end, s_idMix);
}

void PcPort_Hd2dPostProcess(int path, int drawable_w, int drawable_h)
{
    (void)path; (void)drawable_w; (void)drawable_h;
    if (++s_post % 250 == 0) Report();
#ifdef HD2D_OFF_TEST_MUTANT
    {
        GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
        GLint box[4];
        GLfloat clear[4];
        glGetIntegerv(GL_SCISSOR_BOX, box);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, 4, 4);
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(clear[0], clear[1], clear[2], clear[3]);
        glScissor(box[0], box[1], box[2], box[3]);
        if (!scissor) glDisable(GL_SCISSOR_TEST);
    }
#endif
    if (!s_on) return;
}

int PcPort_Hd2dTextureOverride(int tpage, int clut, int u0, int v0, int u1, int v1,
                               unsigned int *out_gl_texture, float out_uv[4])
{
    (void)tpage; (void)clut; (void)u0; (void)v0; (void)u1; (void)v1;
    (void)out_gl_texture; (void)out_uv;
    s_tex++;
    return 0;
}

int PcPort_Hd2dSpriteBegin(const PcPortHd2dSpriteInfo *info)
{
    s_begin++;
    s_idMix ^= info->identity;
    return 1;
}

int PcPort_Hd2dSpriteDraw(const PcPortHd2dSpriteInfo *info, int part, const PcPortHd2dRetailQuad *quad)
{
    (void)info; (void)part;
    if (quad && quad->poly && quad->ot) s_draw++;
    return 0;
}

void PcPort_Hd2dSpriteEnd(const PcPortHd2dSpriteInfo *info)
{
    (void)info;
    s_end++;
}

static int Get(void) { return s_on; }
static void Toggle(void) { }

__attribute__((constructor)) static void Register(void)
{
    PcPort_DevMenuRegisterEnhancement("HD-2D OFF TEST", Get, Toggle);
}
