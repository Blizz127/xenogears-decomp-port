/*
 * World-map terrain-attribute nibble wrapper 0x80093FE4.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80093FE4, 0x80094004).  Thin wrapper: calls 0x80093E8C and
 * masks the 32-bit result register with `andi $v0, $v0, 0xf`.
 *
 *   80093fe4  addiu $sp, $sp, -0x18
 *   80093fe8  sw    $ra, 0x10($sp)
 *   80093fec  jal   0x80093e8c
 *   80093ff0  nop
 *   80093ff4  lw    $ra, 0x10($sp)
 *   80093ff8  andi  $v0, $v0, 0xf
 *   80093ffc  jr    $ra
 *   80094000  addiu $sp, $sp, 0x18
 */
#include <string.h>

#include "common.h"
#include "world_map_helper_93e8c.h"
#include "world_map_helper_93fe4.h"

#if defined(WM_93FE4_TEST_TRACE)
extern s32 wm_93fe4_test_93e8c(u32 vec_addr);
#define WM_93FE4_CALL_93E8C(a) wm_93fe4_test_93e8c(a)
#else
#define WM_93FE4_CALL_93E8C(a) wm_80093E8C(a)
#endif

s32 wm_80093FE4(u32 vec_addr)
{
    u32 raw;
    s32 result;

    /* $v0 holds the full 32-bit sign-extended halfword from 93E8C;
     * andi keeps the low nibble (zero-extended). */
    {
        s32 v = WM_93FE4_CALL_93E8C(vec_addr);
        memcpy(&raw, &v, sizeof(raw));
    }
#if defined(WM_93FE4_MUTANT_WRONG_MASK)
    raw &= 0x7u;
#elif defined(WM_93FE4_MUTANT_MISSING_MASK)
    /* keep raw */
#else
    raw &= 0xFu;
#endif
    memcpy(&result, &raw, sizeof(result));
    return result;
}
