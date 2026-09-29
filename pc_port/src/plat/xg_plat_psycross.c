/* PsyCross backend for the xg_plat renderer / audio / input / timing
 * interfaces (pc_port/include/xg_plat/).  Thin: each call maps to the
 * PsyCross (MIT, pinned in pc_port/psycross.lock) facility that already
 * implements it.  A different backend provides the same functions. */
#include <stdio.h>
#include <string.h>

#include "../../include/xg_plat/renderer.h"
#include "../../include/xg_plat/audio.h"
#include "../../include/xg_plat/input.h"
#include "../../include/xg_plat/timing.h"
#include "../../include/xg_plat/disc.h"

#include <stdlib.h>

#include "../../include/xg_plat/mods.h"
#include "PsyX/PsyX_public.h"
#include "../host_speed.h"

extern int PsyX_Sys_GetVBlankCount(void);

/* --- renderer ------------------------------------------------------------ */
void xg_plat_renderer_default_config(XgPlatVideoConfig* cfg)
{
    cfg->window_width = 640;
    cfg->window_height = 480;
    cfg->fullscreen = 0;
    cfg->vsync = 1;
    cfg->bilinear = 0;
    cfg->widescreen = 0;
}

int xg_plat_renderer_supports(const char* option)
{
    /* PsyCross renders the PSX framebuffer 4:3; a widescreen view needs
     * game-side projection changes plus a backend that draws past 320px. */
    return strcmp(option, "widescreen") != 0;
}

int xg_plat_renderer_init(const char* title, const XgPlatVideoConfig* cfg)
{
    int ignored = 0;
    int w = cfg->window_width > 0 ? cfg->window_width : 640;
    int h = cfg->window_height > 0 ? cfg->window_height : 480;
    g_cfg_bilinearFiltering = cfg->bilinear ? 1 : 0;
    PsyX_Initialise((char*)title, w, h, cfg->fullscreen ? 1 : 0);
    if (!cfg->vsync)
        PsyX_EnableSwapInterval(0);
    if (cfg->widescreen && !xg_plat_renderer_supports("widescreen")) {
        fprintf(stderr, "[xg_plat] widescreen is not supported by the PsyCross "
                        "renderer backend yet; using 4:3\n");
        ignored++;
    }
    return ignored;
}

/* --- audio --------------------------------------------------------------- */
int xg_plat_audio_set_user_volume(int percent)
{
    /* PsyX_SPUAL_SetMasterVolume is the game's SpuSetCommonAttr mvol and is
     * overwritten by the game; PsyCross has no separate user gain yet. */
    (void)percent;
    return 0;
}

/* --- input --------------------------------------------------------------- */
static const char* const s_button_names[XG_PLAT_BTN_COUNT] = {
    "square", "circle", "triangle", "cross", "l1", "l2", "r1", "r2",
    "start", "select", "left", "right", "up", "down",
};

const char* xg_plat_input_button_name(XgPlatButton button)
{
    return (button >= 0 && button < XG_PLAT_BTN_COUNT) ? s_button_names[button] : NULL;
}

static int* key_slot(XgPlatButton button)
{
    PsyXKeyboardMapping* m = &g_cfg_keyboardMapping;
    switch (button) {
    case XG_PLAT_BTN_SQUARE: return &m->kc_square;
    case XG_PLAT_BTN_CIRCLE: return &m->kc_circle;
    case XG_PLAT_BTN_TRIANGLE: return &m->kc_triangle;
    case XG_PLAT_BTN_CROSS: return &m->kc_cross;
    case XG_PLAT_BTN_L1: return &m->kc_l1;
    case XG_PLAT_BTN_L2: return &m->kc_l2;
    case XG_PLAT_BTN_R1: return &m->kc_r1;
    case XG_PLAT_BTN_R2: return &m->kc_r2;
    case XG_PLAT_BTN_START: return &m->kc_start;
    case XG_PLAT_BTN_SELECT: return &m->kc_select;
    case XG_PLAT_BTN_LEFT: return &m->kc_dpad_left;
    case XG_PLAT_BTN_RIGHT: return &m->kc_dpad_right;
    case XG_PLAT_BTN_UP: return &m->kc_dpad_up;
    case XG_PLAT_BTN_DOWN: return &m->kc_dpad_down;
    default: return NULL;
    }
}

int xg_plat_input_bind_key(XgPlatButton button, const char* key_name)
{
    int* slot = key_slot(button);
    int code;
    if (slot == NULL || key_name == NULL)
        return 0;
    code = PsyX_LookupKeyboardMapping(key_name, -1);
    if (code < 0)
        return 0;
    *slot = code;
    return 1;
}

/* --- timing -------------------------------------------------------------- */
int xg_plat_timing_speed(void)
{
    return PsyX_GetSpeedMultiplier();
}

void xg_plat_timing_set_speed(int multiplier)
{
    if (multiplier < XG_PLAT_SPEED_MIN)
        multiplier = XG_PLAT_SPEED_MIN;
    if (multiplier > XG_PLAT_SPEED_MAX)
        multiplier = XG_PLAT_SPEED_MAX;
    PsyX_SetSpeedMultiplier(multiplier);
}

void xg_plat_timing_fast_forward(int held)
{
    PsyX_SetFastForwardHeld(held);
}

void xg_plat_timing_set_fast_forward_speed(int multiplier)
{
    PsyX_SetFastForwardSpeed(multiplier);
}

int xg_plat_timing_vblank_count(void)
{
    return PsyX_Sys_GetVBlankCount();
}

/* --- routing of the game's SDK calls (link-time --wrap, build_port.sh) --- */
typedef struct { short x, y, w, h; } XgPlatPsxRect;

extern int __real_LoadImage(XgPlatPsxRect* rect, unsigned long* pixels);
extern unsigned int __real_SpuWrite(unsigned char* addr, unsigned int size);

int __wrap_LoadImage(XgPlatPsxRect* rect, unsigned long* pixels)
{
    return xg_plat_renderer_upload_image(rect->x, rect->y, rect->w, rect->h, pixels);
}

unsigned int __wrap_SpuWrite(unsigned char* addr, unsigned int size)
{
    return (unsigned int)xg_plat_audio_upload_samples(addr, size);
}

extern void __real_DrawOTag(unsigned long* ot);
extern void* __real_PutDrawEnv(void* env);
extern void* __real_PutDispEnv(void* env);
extern unsigned long* __real_ClearOTag(unsigned long* ot, int n);
extern unsigned long* __real_ClearOTagR(unsigned long* ot, int n);

void __wrap_DrawOTag(unsigned long* ot) { xg_plat_renderer_draw_ot(ot); }
void* __wrap_PutDrawEnv(void* env) { return xg_plat_renderer_set_draw_env(env); }
void* __wrap_PutDispEnv(void* env) { return xg_plat_renderer_set_display_env(env); }
unsigned long* __wrap_ClearOTag(unsigned long* ot, int n)
{
    return (unsigned long*)xg_plat_renderer_clear_ot(ot, n, 0);
}
unsigned long* __wrap_ClearOTagR(unsigned long* ot, int n)
{
    return (unsigned long*)xg_plat_renderer_clear_ot(ot, n, 1);
}

void xg_plat_renderer_draw_ot(void* ot) { __real_DrawOTag((unsigned long*)ot); }
void* xg_plat_renderer_set_draw_env(void* drawenv) { return __real_PutDrawEnv(drawenv); }
void* xg_plat_renderer_set_display_env(void* dispenv) { return __real_PutDispEnv(dispenv); }
void* xg_plat_renderer_clear_ot(void* ot, int n, int reverse)
{
    return reverse ? __real_ClearOTagR((unsigned long*)ot, n)
                   : __real_ClearOTag((unsigned long*)ot, n);
}

int xg_plat_renderer_upload_image(int x, int y, int w, int h, const void* pixels)
{
    XgPlatPsxRect rect = { (short)x, (short)y, (short)w, (short)h };
    size_t bytes = (w > 0 && h > 0) ? (size_t)w * (size_t)h * 2u : 0;
    size_t repl_size = 0;
    void* repl = (bytes && pixels) ? xg_plat_mods_lookup_asset(pixels, bytes, &repl_size) : NULL;
    int result;

    if (repl != NULL && repl_size != bytes) {
        fprintf(stderr, "[xg_plat] mods: texture replacement for %dx%d at (%d,%d) is %zu bytes, "
                        "expected %zu; not applied\n", w, h, x, y, repl_size, bytes);
        free(repl);
        repl = NULL;
    }
    result = __real_LoadImage(&rect, (unsigned long*)(repl ? repl : pixels));
    free(repl); /* PsyCross copies the pixels into its VRAM during LoadImage */
    return result;
}

unsigned long xg_plat_audio_upload_samples(const void* data, unsigned long size)
{
    size_t repl_size = 0;
    void* repl = (data && size) ? xg_plat_mods_lookup_asset(data, size, &repl_size) : NULL;
    unsigned long sent;

    if (repl != NULL && repl_size > size) {
        fprintf(stderr, "[xg_plat] mods: sound replacement is %zu bytes, larger than the "
                        "original %lu (SPU RAM layout is fixed); not applied\n", repl_size, size);
        free(repl);
        repl = NULL;
    }
    if (repl != NULL && repl_size < size) {
        void* padded = calloc(1, size);
        if (padded)
            memcpy(padded, repl, repl_size);
        free(repl);
        repl = padded;
    }
    sent = __real_SpuWrite((unsigned char*)(repl ? repl : data), (unsigned int)size);
    free(repl);
    return sent;
}

/* --- disc: the game's libcd drive commands and reads -------------------- */
extern int __real_CdControl(unsigned char com, unsigned char* param, unsigned char* result);
extern int __real_CdControlB(unsigned char com, unsigned char* param, unsigned char* result);
extern int __real_CdControlF(unsigned char com, unsigned char* param);
extern int __real_CdRead(int sectors, unsigned long* buf, int mode);

int __wrap_CdControl(unsigned char com, unsigned char* param, unsigned char* result)
{
    return xg_plat_disc_cd_control(com, param, result, XG_PLAT_CD_ASYNC);
}
int __wrap_CdControlB(unsigned char com, unsigned char* param, unsigned char* result)
{
    return xg_plat_disc_cd_control(com, param, result, XG_PLAT_CD_BLOCKING);
}
int __wrap_CdControlF(unsigned char com, unsigned char* param)
{
    return xg_plat_disc_cd_control(com, param, NULL, XG_PLAT_CD_NO_RESULT);
}
int __wrap_CdRead(int sectors, unsigned long* buf, int mode)
{
    return xg_plat_disc_cd_read(sectors, buf, mode);
}

int xg_plat_disc_cd_control(unsigned char com, unsigned char* param, unsigned char* result,
                            XgPlatCdWait wait)
{
    switch (wait) {
    case XG_PLAT_CD_BLOCKING: return __real_CdControlB(com, param, result);
    case XG_PLAT_CD_NO_RESULT: return __real_CdControlF(com, param);
    default: return __real_CdControl(com, param, result);
    }
}

int xg_plat_disc_cd_read(int sectors, void* buf, int mode)
{
    return __real_CdRead(sectors, (unsigned long*)buf, mode);
}

/* --- input: the game's pad buffers ------------------------------------- */
extern void __real_PsyX_Pad_InitPad(int slot, unsigned char* padData);
extern void __real_PsyX_UpdateInput(void);
extern const char* SDL_GetScancodeName(int scancode);

static unsigned char* s_pad_buffer[2];

void __wrap_PsyX_Pad_InitPad(int slot, unsigned char* padData) { xg_plat_input_attach_pad(slot, padData); }
void __wrap_PsyX_UpdateInput(void) { xg_plat_input_poll(); }

void xg_plat_input_attach_pad(int slot, unsigned char* buffer)
{
    if (slot == 0 || slot == 1)
        s_pad_buffer[slot] = buffer;
    __real_PsyX_Pad_InitPad(slot, buffer);
}

void xg_plat_input_poll(void) { __real_PsyX_UpdateInput(); }

unsigned int xg_plat_input_read_pads(void)
{
    unsigned int state = 0;
    int slot;
    for (slot = 0; slot < 2; slot++) {
        const unsigned char* b = s_pad_buffer[slot];
        if (b != NULL && b[0] == 0 && (b[1] == 0x41 || b[1] == 0x73))
            state |= (unsigned int)(~(b[2] | (b[3] << 8)) & 0xFFFF) << (16 * slot);
    }
    return state;
}

const char* xg_plat_input_key_name(XgPlatButton button)
{
    int* slot = key_slot(button);
    return slot ? SDL_GetScancodeName(*slot) : NULL;
}

/* --- timing: the game's VSync and root counters ------------------------- */
extern int __real_VSync(int mode);
extern int __real_SetRCnt(int spec, unsigned short target, int mode);
extern int __real_GetRCnt(int spec);
extern int __real_StartRCnt(int spec);

int __wrap_VSync(int mode) { return xg_plat_timing_vsync(mode); }
int __wrap_SetRCnt(int spec, unsigned short target, int mode)
{
    return xg_plat_timing_counter_set(spec, target, mode);
}
int __wrap_GetRCnt(int spec) { return xg_plat_timing_counter_get(spec); }
int __wrap_StartRCnt(int spec) { return xg_plat_timing_counter_start(spec); }

int xg_plat_timing_vsync(int mode) { return __real_VSync(mode); }
int xg_plat_timing_counter_set(int spec, unsigned short target, int mode)
{
    return __real_SetRCnt(spec, target, mode);
}
int xg_plat_timing_counter_get(int spec) { return __real_GetRCnt(spec); }
int xg_plat_timing_counter_start(int spec) { return __real_StartRCnt(spec); }
