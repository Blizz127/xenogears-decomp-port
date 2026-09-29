/*
 * World-map camera builders 0x80097244 and 0x80097440.
 *
 * Retail boundaries:
 *   wm_80097244 [0x80097244, 0x80097440)
 *   wm_80097440 [0x80097440, 0x8009766C)
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_97244.h"

extern long VectorNormal(VECTOR *input, VECTOR *output);
extern void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *output);

#define WM_CAMERA_MATRIX       0x8009C808u
#define WM_BASE_MATRIX         0x8009A180u
#define WM_ANGLE_X             0x8009BD38u
#define WM_ANGLE_Y             0x8009BD3Au
#define WM_ANGLE_Z             0x8009BD3Cu

#define SC_VECTOR_OUT          0x1F800000u
#define SC_LOOK_N2             0x1F800010u
#define SC_LOOK_N3             0x1F800020u
#define SC_LOOK_N1             0x1F800030u
#define SC_LOOK_NEG_EYE        0x1F800040u
#define SC_LOOK_MATRIX         0x1F800048u

#define SC_EULER_NEG_EYE       0x1F8000A0u
#define SC_EULER_X             0x1F8000F0u
#define SC_EULER_Y             0x1F800110u
#define SC_EULER_Z             0x1F800130u
#define SC_EULER_XY            0x1F800150u

#if defined(__GNUC__)
#define WM_UNUSED __attribute__((unused))
#else
#define WM_UNUSED
#endif

static s16 WM_UNUSED wm_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 WM_UNUSED wm_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 WM_UNUSED wm_lw(u32 address)
{
    s32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void WM_UNUSED wm_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void WM_UNUSED wm_sw(u32 address, s32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void WM_UNUSED wm_copy_matrix(u32 destination, u32 source)
{
    u32 i;

    for (i = 0; i < 8u; i++)
        wm_sw(destination + i * 4u, wm_lw(source + i * 4u));
}

#if defined(WM_CAMERA_MUTANT_CURRENT)

/* Certificate-only reproduction of the materially wrong landed dataflow. */
void wm_80097244(u32 input_data)
{
    VECTOR edge1;
    VECTOR edge2;
    VECTOR edge3;
    MATRIX *matrix;

    edge1.vx = (s32)wm_lh(input_data + 8u) - (s32)wm_lh(input_data);
    edge1.vy = (s32)wm_lh(input_data + 0xAu) - (s32)wm_lh(input_data + 2u);
    edge1.vz = (s32)wm_lh(input_data + 0xCu) - (s32)wm_lh(input_data + 4u);
    VectorNormal(&edge1, (VECTOR *)PSX_ADDR(SC_LOOK_N1));

    edge2.vx = wm_lh(input_data + 0x10u);
    edge2.vy = wm_lh(input_data + 0x12u);
    edge2.vz = wm_lh(input_data + 0x14u);
    VectorNormal(&edge2, (VECTOR *)PSX_ADDR(SC_LOOK_N2));
    OuterProduct12((VECTOR *)PSX_ADDR(SC_LOOK_N1),
                   (VECTOR *)PSX_ADDR(SC_LOOK_N2),
                   (VECTOR *)PSX_ADDR(SC_LOOK_MATRIX));

    edge3.vx = wm_lh(input_data + 0x20u);
    edge3.vy = wm_lh(input_data + 0x22u);
    edge3.vz = wm_lh(input_data + 0x24u);
    VectorNormal(&edge3, (VECTOR *)PSX_ADDR(SC_LOOK_N3));
    OuterProduct12((VECTOR *)PSX_ADDR(SC_LOOK_N3),
                   (VECTOR *)PSX_ADDR(SC_LOOK_N2),
                   (VECTOR *)PSX_ADDR(0x1F800054u));
    OuterProduct12((VECTOR *)PSX_ADDR(SC_LOOK_N1),
                   (VECTOR *)PSX_ADDR(SC_LOOK_N3),
                   (VECTOR *)PSX_ADDR(0x1F800060u));

    matrix = (MATRIX *)PSX_ADDR(SC_LOOK_MATRIX);
    matrix->m[0][0] = (s16)(((VECTOR *)PSX_ADDR(SC_LOOK_N1))->vx >> 4);
    matrix->m[0][1] = (s16)(((VECTOR *)PSX_ADDR(SC_LOOK_N1))->vy >> 4);
    matrix->m[0][2] = (s16)(((VECTOR *)PSX_ADDR(SC_LOOK_N1))->vz >> 4);
    matrix->m[1][0] = (s16)(((VECTOR *)PSX_ADDR(SC_LOOK_N2))->vx >> 4);
    matrix->m[1][1] = (s16)(((VECTOR *)PSX_ADDR(SC_LOOK_N2))->vy >> 4);
    matrix->m[1][2] = (s16)(((VECTOR *)PSX_ADDR(SC_LOOK_N2))->vz >> 4);
    matrix->m[2][0] = (s16)(((VECTOR *)PSX_ADDR(SC_LOOK_N3))->vx >> 4);
    matrix->m[2][1] = (s16)(((VECTOR *)PSX_ADDR(SC_LOOK_N3))->vy >> 4);
    matrix->m[2][2] = (s16)(((VECTOR *)PSX_ADDR(SC_LOOK_N3))->vz >> 4);
    memcpy(PSX_ADDR(input_data + 0x10u), matrix, sizeof(*matrix));
}

void wm_80097440(u32 input_data)
{
    MATRIX temporary;
    MATRIX *matrix = (MATRIX *)PSX_ADDR(input_data);
    VECTOR position;
    VECTOR translation;

    RotMatrixX(wm_lh(input_data + 0x20u), &temporary);
    MulMatrix0(matrix, &temporary, matrix);
    RotMatrixY(wm_lh(input_data + 0x22u), &temporary);
    MulMatrix0(matrix, &temporary, matrix);
    RotMatrixZ(wm_lh(input_data + 0x24u), &temporary);
    MulMatrix0(matrix, &temporary, matrix);

    position.vx = wm_lw(input_data + 0x28u);
    position.vy = wm_lw(input_data + 0x2Cu);
    position.vz = wm_lw(input_data + 0x30u);
    ApplyMatrix(matrix, (SVECTOR *)&position, &position);

    translation.vx = wm_lw(input_data + 0x34u);
    translation.vy = wm_lw(input_data + 0x38u);
    translation.vz = wm_lw(input_data + 0x3Cu);
    TransMatrix(matrix, &translation);
}

#else

void wm_80097244(u32 input_data)
{
#if defined(WM_CAMERA_MUTANT_WRONG_LOOK_ROWS)
    static const u32 selected_rows[9] = {
        SC_LOOK_N1, SC_LOOK_N1 + 4u, SC_LOOK_N1 + 8u,
        SC_LOOK_N2, SC_LOOK_N2 + 4u, SC_LOOK_N2 + 8u,
        SC_LOOK_N3, SC_LOOK_N3 + 4u, SC_LOOK_N3 + 8u,
    };
#else
    static const u32 selected_rows[9] = {
        SC_LOOK_N2, SC_LOOK_N2 + 4u, SC_LOOK_N2 + 8u,
        SC_LOOK_N3, SC_LOOK_N3 + 4u, SC_LOOK_N3 + 8u,
        SC_LOOK_N1, SC_LOOK_N1 + 4u, SC_LOOK_N1 + 8u,
    };
#endif
    u32 i;

    wm_sw(SC_VECTOR_OUT,
          (s32)wm_lh(input_data + 8u) - (s32)wm_lh(input_data));
    wm_sw(SC_VECTOR_OUT + 4u,
          (s32)wm_lh(input_data + 0xAu) -
              (s32)wm_lh(input_data + 2u));
    wm_sw(SC_VECTOR_OUT + 8u,
          (s32)wm_lh(input_data + 0xCu) -
              (s32)wm_lh(input_data + 4u));

    (void)VectorNormal((VECTOR *)PSX_ADDR(SC_VECTOR_OUT),
                       (VECTOR *)PSX_ADDR(SC_LOOK_N1));
    OuterProduct12((VECTOR *)PSX_ADDR(SC_LOOK_N1),
                   (VECTOR *)PSX_ADDR(input_data + 0x10u),
                   (VECTOR *)PSX_ADDR(SC_VECTOR_OUT));
    (void)VectorNormal((VECTOR *)PSX_ADDR(SC_VECTOR_OUT),
                       (VECTOR *)PSX_ADDR(SC_LOOK_N2));
    OuterProduct12((VECTOR *)PSX_ADDR(SC_LOOK_N1),
                   (VECTOR *)PSX_ADDR(SC_LOOK_N2),
                   (VECTOR *)PSX_ADDR(SC_VECTOR_OUT));
    (void)VectorNormal((VECTOR *)PSX_ADDR(SC_VECTOR_OUT),
                       (VECTOR *)PSX_ADDR(SC_LOOK_N3));

    for (i = 0; i < 9u; i++)
        wm_sh(WM_CAMERA_MATRIX + i * 2u, (u16)wm_lw(selected_rows[i]));

    wm_sh(SC_LOOK_NEG_EYE, (u16)(0u - (u32)wm_lhu(input_data)));
    wm_sh(SC_LOOK_NEG_EYE + 2u,
          (u16)(0u - (u32)wm_lhu(input_data + 2u)));
    wm_sh(SC_LOOK_NEG_EYE + 4u,
          (u16)(0u - (u32)wm_lhu(input_data + 4u)));
    wm_copy_matrix(SC_LOOK_MATRIX, WM_CAMERA_MATRIX);

#if !defined(WM_CAMERA_MUTANT_NO_TRANSLATION)
    (void)ApplyMatrix((MATRIX *)PSX_ADDR(SC_LOOK_MATRIX),
                      (SVECTOR *)PSX_ADDR(SC_LOOK_NEG_EYE),
                      (VECTOR *)PSX_ADDR(SC_VECTOR_OUT));
    (void)TransMatrix((MATRIX *)PSX_ADDR(WM_CAMERA_MATRIX),
                      (VECTOR *)PSX_ADDR(SC_VECTOR_OUT));
#endif
}

void wm_80097440(u32 input_data)
{
    s32 angle_x;
    s32 angle_y;
    s32 angle_z;

    wm_copy_matrix(SC_EULER_X, WM_BASE_MATRIX);
    wm_copy_matrix(SC_EULER_Y, SC_EULER_X);
    wm_copy_matrix(SC_EULER_Z, SC_EULER_X);

#if defined(WM_CAMERA_MUTANT_WRONG_ANGLE_SOURCE)
    angle_x = wm_lh(input_data + 0x20u);
    angle_y = wm_lh(input_data + 0x22u);
    angle_z = wm_lh(input_data + 0x24u);
#else
    angle_x = wm_lh(WM_ANGLE_X);
    angle_y = wm_lh(WM_ANGLE_Y);
    angle_z = wm_lh(WM_ANGLE_Z);
#endif

#if defined(WM_CAMERA_MUTANT_NO_ANGLE_NEGATION)
    (void)RotMatrixX(angle_x, (MATRIX *)PSX_ADDR(SC_EULER_X));
    (void)RotMatrixY(angle_y, (MATRIX *)PSX_ADDR(SC_EULER_Y));
    (void)RotMatrixZ(angle_z, (MATRIX *)PSX_ADDR(SC_EULER_Z));
#else
    (void)RotMatrixX(-angle_x, (MATRIX *)PSX_ADDR(SC_EULER_X));
    (void)RotMatrixY(-angle_y, (MATRIX *)PSX_ADDR(SC_EULER_Y));
    (void)RotMatrixZ(-angle_z, (MATRIX *)PSX_ADDR(SC_EULER_Z));
#endif

    (void)MulMatrix0((MATRIX *)PSX_ADDR(SC_EULER_X),
                     (MATRIX *)PSX_ADDR(SC_EULER_Y),
                     (MATRIX *)PSX_ADDR(SC_EULER_XY));
    (void)MulMatrix0((MATRIX *)PSX_ADDR(SC_EULER_Z),
                     (MATRIX *)PSX_ADDR(SC_EULER_XY),
                     (MATRIX *)PSX_ADDR(WM_CAMERA_MATRIX));

    wm_sh(SC_EULER_NEG_EYE, (u16)(0u - (u32)wm_lhu(input_data)));
    wm_sh(SC_EULER_NEG_EYE + 2u,
          (u16)(0u - (u32)wm_lhu(input_data + 2u)));
    wm_sh(SC_EULER_NEG_EYE + 4u,
          (u16)(0u - (u32)wm_lhu(input_data + 4u)));
    wm_copy_matrix(SC_EULER_X, WM_CAMERA_MATRIX);

#if !defined(WM_CAMERA_MUTANT_NO_TRANSLATION)
    (void)ApplyMatrix((MATRIX *)PSX_ADDR(SC_EULER_X),
                      (SVECTOR *)PSX_ADDR(SC_EULER_NEG_EYE),
                      (VECTOR *)PSX_ADDR(SC_VECTOR_OUT));
    (void)TransMatrix((MATRIX *)PSX_ADDR(WM_CAMERA_MATRIX),
                      (VECTOR *)PSX_ADDR(SC_VECTOR_OUT));
#endif
}

#endif
