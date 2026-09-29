/* Focused production-linked certificate for the two retail world cameras. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_97244.h"

#define LOOK 0x8009BD40u
#define BASE 0x8009A180u
#define ANGLES 0x8009BD38u
#define CAMERA 0x8009C808u

#define SC_OUT 0x1F800000u
#define SC_N2 0x1F800010u
#define SC_N3 0x1F800020u
#define SC_N1 0x1F800030u
#define SC_LOOK_NEG 0x1F800040u
#define SC_LOOK_M 0x1F800048u
#define SC_EULER_NEG 0x1F8000A0u
#define SC_X 0x1F8000F0u
#define SC_Y 0x1F800110u
#define SC_Z 0x1F800130u
#define SC_XY 0x1F800150u

static int failures;
static int normal_count;
static int outer_count;
static int rotx_count;
static int roty_count;
static int rotz_count;
static int mul_count;
static int apply_count;
static int trans_count;
static u32 normal_in[3];
static u32 normal_out[3];
static u32 outer_a[2];
static u32 outer_b[2];
static u32 outer_out[2];
static s32 rot_angles[3];
static u32 rot_destinations[3];
static u32 mul_args[2][3];
static u32 apply_args[3];
static u32 trans_args[2];
static unsigned char base_at_rotation[3][32];

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        failures++;
    }
}

static u16 read16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, s32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 guest_of(const void *pointer)
{
    uintptr_t base_address = (uintptr_t)g_PsxRam;
    uintptr_t pointer_address = (uintptr_t)pointer;
    uintptr_t offset;

    if (pointer_address < base_address ||
        pointer_address >= base_address + PSX_RAM_SIZE)
        return UINT32_C(0xFFFFFFFF);
    offset = pointer_address - base_address;
    if (offset < 0x400u)
        return UINT32_C(0x1F800000) + (u32)offset;
    return UINT32_C(0x80000000) | (u32)offset;
}

static void reset_trace(void)
{
    normal_count = 0;
    outer_count = 0;
    rotx_count = 0;
    roty_count = 0;
    rotz_count = 0;
    mul_count = 0;
    apply_count = 0;
    trans_count = 0;
    memset(normal_in, 0, sizeof(normal_in));
    memset(normal_out, 0, sizeof(normal_out));
    memset(outer_a, 0, sizeof(outer_a));
    memset(outer_b, 0, sizeof(outer_b));
    memset(outer_out, 0, sizeof(outer_out));
    memset(rot_angles, 0, sizeof(rot_angles));
    memset(rot_destinations, 0, sizeof(rot_destinations));
    memset(mul_args, 0, sizeof(mul_args));
    memset(apply_args, 0, sizeof(apply_args));
    memset(trans_args, 0, sizeof(trans_args));
    memset(base_at_rotation, 0, sizeof(base_at_rotation));
}

long VectorNormal(VECTOR *input, VECTOR *output)
{
    static const s32 values[3][3] = {
        {0x1101, -0x1202, 0x1303},
        {-0x2104, 0x2205, 0x2306},
        {0x3107, 0x3208, -0x3309},
    };
    int index = normal_count;

    if (index < 3) {
        normal_in[index] = guest_of(input);
        normal_out[index] = guest_of(output);
        output->vx = values[index][0];
        output->vy = values[index][1];
        output->vz = values[index][2];
    }
    normal_count++;
    return 1;
}

void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *output)
{
    int index = outer_count;

    if (index < 2) {
        outer_a[index] = guest_of(left);
        outer_b[index] = guest_of(right);
        outer_out[index] = guest_of(output);
    }
    output->vx = 0x4010 + index;
    output->vy = -0x5020 - index;
    output->vz = 0x6030 + index;
    outer_count++;
}

static void paint_matrix(MATRIX *matrix, s16 tag)
{
    int row;
    int column;

    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++)
            matrix->m[row][column] =
                (s16)(tag + (s16)(row * 0x10 + column * 3));
    }
    matrix->t[0] = (s32)tag + 101;
    matrix->t[1] = (s32)tag - 202;
    matrix->t[2] = (s32)tag + 303;
}

MATRIX *RotMatrixX(int angle, MATRIX *matrix)
{
    rot_angles[0] = angle;
    rot_destinations[0] = guest_of(matrix);
    memcpy(base_at_rotation[0], matrix, 32);
    paint_matrix(matrix, 0x1010);
    rotx_count++;
    return matrix;
}

MATRIX *RotMatrixY(int angle, MATRIX *matrix)
{
    rot_angles[1] = angle;
    rot_destinations[1] = guest_of(matrix);
    memcpy(base_at_rotation[1], matrix, 32);
    paint_matrix(matrix, 0x2020);
    roty_count++;
    return matrix;
}

MATRIX *RotMatrixZ(int angle, MATRIX *matrix)
{
    rot_angles[2] = angle;
    rot_destinations[2] = guest_of(matrix);
    memcpy(base_at_rotation[2], matrix, 32);
    paint_matrix(matrix, 0x3030);
    rotz_count++;
    return matrix;
}

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output)
{
    int index = mul_count;

    if (index < 2) {
        mul_args[index][0] = guest_of(left);
        mul_args[index][1] = guest_of(right);
        mul_args[index][2] = guest_of(output);
    }
    paint_matrix(output, (s16)(index == 0 ? 0x4140 : 0x5250));
    mul_count++;
    return output;
}

static void matrix_times_vector(const MATRIX *matrix, const SVECTOR *input,
                                VECTOR *output)
{
    int64_t x = input->vx;
    int64_t y = input->vy;
    int64_t z = input->vz;

    output->vx = (s32)(((int64_t)matrix->m[0][0] * x +
                        (int64_t)matrix->m[0][1] * y +
                        (int64_t)matrix->m[0][2] * z) >> 12);
    output->vy = (s32)(((int64_t)matrix->m[1][0] * x +
                        (int64_t)matrix->m[1][1] * y +
                        (int64_t)matrix->m[1][2] * z) >> 12);
    output->vz = (s32)(((int64_t)matrix->m[2][0] * x +
                        (int64_t)matrix->m[2][1] * y +
                        (int64_t)matrix->m[2][2] * z) >> 12);
}

VECTOR *ApplyMatrix(MATRIX *matrix, SVECTOR *input, VECTOR *output)
{
    apply_args[0] = guest_of(matrix);
    apply_args[1] = guest_of(input);
    apply_args[2] = guest_of(output);
    matrix_times_vector(matrix, input, output);
    apply_count++;
    return output;
}

MATRIX *TransMatrix(MATRIX *matrix, VECTOR *translation)
{
    trans_args[0] = guest_of(matrix);
    trans_args[1] = guest_of(translation);
    matrix->t[0] = translation->vx;
    matrix->t[1] = translation->vy;
    matrix->t[2] = translation->vz;
    trans_count++;
    return matrix;
}

static int allowed_look_write(size_t offset)
{
    return offset < 0x68u ||
           (offset >= 0x9C808u && offset < 0x9C828u);
}

static int allowed_euler_write(size_t offset)
{
    return offset < 0x10u ||
           (offset >= 0xA0u && offset < 0xA8u) ||
           (offset >= 0xF0u && offset < 0x170u) ||
           (offset >= 0x9C808u && offset < 0x9C828u);
}

static void check_outside_unchanged(const unsigned char *before,
                                    int (*allowed)(size_t), const char *name)
{
    size_t i;

    for (i = 0; i < PSX_RAM_SIZE; i++) {
        if (!allowed(i) && before[i] != g_PsxRam[i]) {
            fprintf(stderr, "ASSERTION %s offset=0x%zx\n", name, i);
            failures++;
            return;
        }
    }
}

static void seed_identity(void)
{
    static const u16 identity[16] = {
        0x1000, 0, 0, 0,
        0x1000, 0, 0, 0,
        0x1000, 0, 0, 0,
        0, 0, 0, 0,
    };

    memcpy(PSX_ADDR(BASE), identity, sizeof(identity));
}

static void seed_input(void)
{
    write16(LOOK, (u16)(s16)-123);
    write16(LOOK + 2u, (u16)(s16)234);
    write16(LOOK + 4u, (u16)(s16)-345);
    write16(LOOK + 6u, 0x6A6Au);
    write16(LOOK + 8u, (u16)(s16)456);
    write16(LOOK + 0xAu, (u16)(s16)-567);
    write16(LOOK + 0xCu, (u16)(s16)678);
    write16(LOOK + 0xEu, 0x7B7Bu);
    write32(LOOK + 0x10u, 0x10203040);
    write32(LOOK + 0x14u, -0x1122334);
    write32(LOOK + 0x18u, 0x50607080);
    write32(LOOK + 0x1Cu, (s32)0x91A2B3C4u);
    write16(LOOK + 0x20u, 0x0777u);
    write16(LOOK + 0x22u, 0x0888u);
    write16(LOOK + 0x24u, 0x0999u);
    write32(LOOK + 0x28u, 0x13572468);
    write32(LOOK + 0x2Cu, (s32)0x89ABCDEFu);
    write32(LOOK + 0x30u, 0x24681357);
    write32(LOOK + 0x34u, 0x11121314);
    write32(LOOK + 0x38u, 0x21222324);
    write32(LOOK + 0x3Cu, 0x31323334);
}

static void check_translation(u32 neg_address, const char *prefix)
{
    VECTOR expected;
    MATRIX *camera = (MATRIX *)PSX_ADDR(CAMERA);

    matrix_times_vector(camera, (SVECTOR *)PSX_ADDR(neg_address), &expected);
    check(camera->t[0] == expected.vx, prefix);
    check(camera->t[1] == expected.vy, prefix);
    check(camera->t[2] == expected.vz, prefix);
}

static void test_97244(void)
{
    static const u16 expected_rows[9] = {
        (u16)(s16)-0x2104, 0x2205, 0x2306,
        0x3107, 0x3208, (u16)(s16)-0x3309,
        0x1101, (u16)(s16)-0x1202, 0x1303,
    };
    unsigned char input_before[0x40];
    unsigned char *ram_before;
    int i;

    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    seed_input();
    memcpy(input_before, PSX_ADDR(LOOK), sizeof(input_before));
    ram_before = malloc(PSX_RAM_SIZE);
    check(ram_before != NULL, "97244.snapshot.alloc");
    if (ram_before == NULL)
        return;
    memcpy(ram_before, g_PsxRam, PSX_RAM_SIZE);
    reset_trace();

    wm_80097244(LOOK);

    check(memcmp(input_before, PSX_ADDR(LOOK), sizeof(input_before)) == 0,
          "97244.input.read_only");
    check(normal_count == 3, "97244.normal.count");
    check(outer_count == 2, "97244.outer.count");
    check(normal_in[0] == SC_OUT && normal_out[0] == SC_N1,
          "97244.normal1.route");
    check(outer_a[0] == SC_N1 && outer_b[0] == LOOK + 0x10u &&
              outer_out[0] == SC_OUT,
          "97244.outer1.route");
    check(normal_in[1] == SC_OUT && normal_out[1] == SC_N2,
          "97244.normal2.route");
    check(outer_a[1] == SC_N1 && outer_b[1] == SC_N2 &&
              outer_out[1] == SC_OUT,
          "97244.outer2.route");
    check(normal_in[2] == SC_OUT && normal_out[2] == SC_N3,
          "97244.normal3.route");
    for (i = 0; i < 9; i++)
        check(read16(CAMERA + (u32)i * 2u) == expected_rows[i],
              "97244.rows.N2_N3_N1");
    check(apply_count == 1 && trans_count == 1, "97244.translation.calls");
    check(apply_args[0] == SC_LOOK_M && apply_args[1] == SC_LOOK_NEG &&
              apply_args[2] == SC_OUT,
          "97244.apply.args");
    check(trans_args[0] == CAMERA && trans_args[1] == SC_OUT,
          "97244.trans.args");
    check_translation(SC_LOOK_NEG, "97244.translation.R_minus_eye");
    check_outside_unchanged(ram_before, allowed_look_write,
                            "97244.outside_write_set");
    free(ram_before);
}

static void test_97440(void)
{
    unsigned char input_before[0x40];
    unsigned char base_before[32];
    unsigned char *ram_before;
    int i;

    memset(g_PsxRam, 0x5A, PSX_RAM_SIZE);
    seed_identity();
    seed_input();
    write16(ANGLES, 0x0123u);
    write16(ANGLES + 2u, (u16)(s16)-0x0234);
    write16(ANGLES + 4u, 0x0345u);
    memcpy(input_before, PSX_ADDR(LOOK), sizeof(input_before));
    memcpy(base_before, PSX_ADDR(BASE), sizeof(base_before));
    ram_before = malloc(PSX_RAM_SIZE);
    check(ram_before != NULL, "97440.snapshot.alloc");
    if (ram_before == NULL)
        return;
    memcpy(ram_before, g_PsxRam, PSX_RAM_SIZE);
    reset_trace();

    wm_80097440(LOOK);

    check(memcmp(input_before, PSX_ADDR(LOOK), sizeof(input_before)) == 0,
          "97440.input.read_only");
    check(memcmp(base_before, PSX_ADDR(BASE), sizeof(base_before)) == 0,
          "97440.base.read_only");
    check(rotx_count == 1 && roty_count == 1 && rotz_count == 1,
          "97440.rotation.counts");
    check(rot_angles[0] == -0x0123 && rot_angles[1] == 0x0234 &&
              rot_angles[2] == -0x0345,
          "97440.negated.global.angles");
    check(rot_destinations[0] == SC_X && rot_destinations[1] == SC_Y &&
              rot_destinations[2] == SC_Z,
          "97440.rotation.destinations");
    for (i = 0; i < 3; i++)
        check(memcmp(base_at_rotation[i], base_before, sizeof(base_before)) == 0,
              "97440.base.matrix.origin");
    check(mul_count == 2, "97440.mul.count");
    check(mul_args[0][0] == SC_X && mul_args[0][1] == SC_Y &&
              mul_args[0][2] == SC_XY,
          "97440.mul.X_Y_XY");
    check(mul_args[1][0] == SC_Z && mul_args[1][1] == SC_XY &&
              mul_args[1][2] == CAMERA,
          "97440.mul.Z_XY_CAMERA");
    check(apply_count == 1 && trans_count == 1, "97440.translation.calls");
    check(apply_args[0] == SC_X && apply_args[1] == SC_EULER_NEG &&
              apply_args[2] == SC_OUT,
          "97440.apply.args");
    check(trans_args[0] == CAMERA && trans_args[1] == SC_OUT,
          "97440.trans.args");
    check_translation(SC_EULER_NEG, "97440.translation.R_minus_eye");
    check_outside_unchanged(ram_before, allowed_euler_write,
                            "97440.outside_write_set");
    free(ram_before);
}

int main(void)
{
    PsxMemory_Init();
    test_97244();
    test_97440();
    if (failures != 0) {
        fprintf(stderr, "%d assertion(s) failed\n", failures);
        return 1;
    }
    puts("W34B60 world camera focused certificate PASS");
    return 0;
}
