#include "hd2d_sprite_parts.h"
#include <assert.h>
#include <string.h>
static int enabled;
int PcPort_Hd2dEnabled(void) { return enabled; }
int PcPort_Hd2dOpeningActive(void) { assert(enabled); return 1; }
int PcPort_Hd2dCurrentMap(void) { assert(enabled); return 7; }
int main(void)
{
    PcPortHd2dPartMapping mapping = {123, 7, 0, 2, 0, 9, 8,
        {1, 2, 1, 2}, {3, 3, 4, 4}, 0, 0, 16, 32};
    PcPortHd2dPartProfile profile = {64, 64, &mapping, 1, "pc_port/assets/hd2d/test.rgba"};
    PcPortHd2dSpriteInfo info = {123, 2, 0, NULL};
    PcPortHd2dRetailQuad q = {{-5, 8, -5, 8}, {9, 9, 40, 40},
        {1, 2, 1, 2}, {3, 3, 4, 4}, 9, 8, (void *)1, (void *)2}, original = q;
    assert(!PcPort_Hd2dSpriteBegin((void *)1));
    assert(!PcPort_Hd2dSpriteDraw((void *)1, 0, (void *)1));
    assert(PcPort_Hd2dPartLookup(&profile, 7, &info, 0, &q) == &mapping);
    assert(!memcmp(&original, &q, sizeof q)); /* exact screen/anchor/OT preserved */
    q.u[0]++;
    assert(!PcPort_Hd2dPartLookup(&profile, 7, &info, 0, &q));
    q = original; info.frame++;
    assert(!PcPort_Hd2dPartLookup(&profile, 7, &info, 0, &q));
    info.frame--; info.actor = -1;
    assert(!PcPort_Hd2dPartLookup(&profile, 7, &info, 0, &q));
    enabled = 1;
    assert(!PcPort_Hd2dSpriteDraw(&info, 0, &q)); /* missing transport/profile */
    assert(!memcmp(&original, &q, sizeof q));
    mapping.width = 65;
    assert(!PcPort_Hd2dPartProfileValidate(&profile));
    return 0;
}
