/* input.pad_confirm handling (see port_pad_confirm.h).  Only the host
 * controller mapping (SDL A/B -> PS Cross/Circle) is exchanged; the game's
 * remap table and the keyboard bindings are untouched. */
#include <stdio.h>
#include <string.h>
#include "port_pad_confirm.h"
#include "../include/xg_plat/config.h"
#include "../include/xg_plat/input.h"

static int s_layoutCircle;
static int s_wantCircle;

static void apply(int circle, const char* why)
{
    s_wantCircle = circle ? 1 : 0;
    xg_plat_input_set_pad_face_swap(s_wantCircle != s_layoutCircle);
    printf("[xeno-port] controller confirm: physical %s (%s)\n",
           s_wantCircle ? "Circle (east)" : "Cross (south)", why);
}

void PcPort_PadConfirmInit(int layoutCircle)
{
    const char* confirm = xg_plat_config_get("input.pad_confirm");
    int want = layoutCircle;
    s_layoutCircle = layoutCircle ? 1 : 0;
    if (confirm != NULL && strcmp(confirm, "circle") == 0) {
        want = 1;
    } else if (confirm != NULL && strcmp(confirm, "cross") == 0) {
        want = 0;
    } else if (confirm != NULL) {
        fprintf(stderr, "[xg_plat] config: input.pad_confirm = '%s': expected cross or "
                        "circle; keeping the retail layout\n", confirm);
    }
    apply(want, want != s_layoutCircle ? "input.pad_confirm, Cross/Circle exchanged"
                                       : "retail layout");
}

int PcPort_PadConfirmCircle(void)
{
    return s_wantCircle;
}

void PcPort_PadConfirmSet(int circle, int persist)
{
    apply(circle, "changed at run time");
    if (persist) {
        const char* now;
        if (!xg_plat_config_persist("input.pad_confirm", circle ? "circle" : "cross"))
            fprintf(stderr, "[xeno-port] controller confirm: could not save input.pad_confirm; "
                            "it applies until the game exits\n");
        now = xg_plat_config_get("input.pad_confirm");
        if (now != NULL && strcmp(now, circle ? "circle" : "cross") != 0)
            fprintf(stderr, "[xeno-port] controller confirm: --set or XENO_INPUT_PAD_CONFIRM "
                            "= %s overrides the saved value at the next start\n", now);
    }
}

void PcPort_PadConfirmToggle(void)
{
    PcPort_PadConfirmSet(!s_wantCircle, 1);
}

int PcPort_PadFastForwardEnabled(void)
{
    static int cached = -1;
    if (cached < 0) {
        cached = xg_plat_config_int("input.pad_fast_forward", 0) != 0;
        printf("[xeno-port] pad fast-forward (L3 toggle, R3 hold): %s\n", cached ? "on" : "off");
    }
    return cached;
}
