/* Retail world-map matrix-to-Euler helper [0x80097070,0x80097244). */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_97070.h"

#define WM_97070_BASE_MATRIX 0x8009A180u
#define WM_97070_SOURCE      0x1F8000F0u
#define WM_97070_ROTATION    0x1F800110u
#define WM_97070_PRODUCT     0x1F800130u

static s16 h97070_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 __attribute__((unused)) h97070_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void h97070_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void h97070_copy32(u32 destination, u32 source)
{
    memcpy(PSX_ADDR(destination), PSX_ADDR(source), 32u);
}

void wm_80097070(u32 matrix, u32 angles)
{
    int angle;

#if !defined(W34N59_MUTANT_SKIP_ZERO_GUARD)
    if ((h97070_lhu(matrix + 0x0Cu) |
         h97070_lhu(matrix + 0x10u)) == 0u)
        return;
#endif

#if defined(W34N59_MUTANT_SWAP_FIRST_RATAN_ARGS)
    angle = ratan2((int)h97070_lh(matrix + 0x10u),
                   (int)h97070_lh(matrix + 0x0Cu));
#else
    angle = ratan2((int)h97070_lh(matrix + 0x0Cu),
                   (int)h97070_lh(matrix + 0x10u));
#endif
    h97070_sh(angles + 2u, (u16)((u32)angle & 0x0FFFu));

    h97070_copy32(WM_97070_SOURCE, matrix);
#if defined(W34N59_MUTANT_WRONG_BASE_SOURCE)
    h97070_copy32(WM_97070_ROTATION, matrix);
#else
    h97070_copy32(WM_97070_ROTATION, WM_97070_BASE_MATRIX);
#endif
    (void)RotMatrixY((int)h97070_lh(angles + 2u),
                     (MATRIX*)PSX_ADDR(WM_97070_ROTATION));
    (void)MulMatrix0((MATRIX*)PSX_ADDR(WM_97070_SOURCE),
                     (MATRIX*)PSX_ADDR(WM_97070_ROTATION),
                     (MATRIX*)PSX_ADDR(WM_97070_PRODUCT));

    angle = ratan2((int)h97070_lh(WM_97070_PRODUCT + 0x0Au),
                   (int)h97070_lh(WM_97070_PRODUCT + 0x08u));
    h97070_sh(angles + 0u, (u16)angle);

    h97070_copy32(WM_97070_ROTATION, WM_97070_BASE_MATRIX);
    (void)RotMatrixX((int)h97070_lh(angles + 0u),
                     (MATRIX*)PSX_ADDR(WM_97070_ROTATION));
    (void)MulMatrix0((MATRIX*)PSX_ADDR(WM_97070_PRODUCT),
                     (MATRIX*)PSX_ADDR(WM_97070_ROTATION),
                     (MATRIX*)PSX_ADDR(WM_97070_SOURCE));

    angle = ratan2((int)h97070_lh(WM_97070_SOURCE + 0x06u),
                   (int)h97070_lh(WM_97070_SOURCE + 0x08u));
#if defined(W34N59_MUTANT_SKIP_FINAL_NEGATION)
    h97070_sh(angles + 4u, (u16)angle);
#else
    h97070_sh(angles + 4u, (u16)(0u - (u32)angle));
#endif
}

/* Entry for the matched C in src/world_map/main.c (0x80097070 is not
 * matched yet): its callers pass host pointers into emulated RAM. */
void func_80097070(void* matrix, void* angles)
{
    wm_80097070(PsxMemory_GuestAddr(matrix), PsxMemory_GuestAddr(angles));
}
