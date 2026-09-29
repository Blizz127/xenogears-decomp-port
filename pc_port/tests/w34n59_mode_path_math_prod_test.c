/* Focused certificate for retail helpers 0x80076858 and 0x80097070. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_76858.h"
#include "world_map_helper_94154.h"
#include "world_map_helper_97070.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define VECTOR_A    0x800A0000u
#define VECTOR_B    0x800A0010u
#define VECTOR_C    0x800A0020u
#define VECTOR_OUT  0x800A0030u
#define MATRIX_IN   0x800A0100u
#define ANGLES_OUT  0x800A0140u
#define DIST_LEFT   0x800A0180u
#define DIST_RIGHT  0x800A0190u
#define BASE_MATRIX 0x8009A180u
#define SC_SOURCE   0x1F8000F0u
#define SC_ROTATION 0x1F800110u
#define SC_PRODUCT  0x1F800130u

static int failures;
static int ratan_calls;
static int ratan_y[3];
static int ratan_x[3];
static int rot_y_calls;
static int rot_x_calls;
static int rot_y_angle;
static int rot_x_angle;
static uint8_t rot_y_input[32];
static uint8_t rot_x_input[32];
static uint8_t first_mul_left[32];
static int mul_calls;
static int square_root_calls;
static int square_root_input;

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 read16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

int ratan2(int y, int x)
{
    static const int returns[3] = { 0x1234, 0x0345, 0x0456 };
    int index = ratan_calls;

    if (index < 3) {
        ratan_y[index] = y;
        ratan_x[index] = x;
    }
    ratan_calls++;
    return index < 3 ? returns[index] : 0;
}

MATRIX* RotMatrixY(int angle, MATRIX* matrix)
{
    rot_y_calls++;
    rot_y_angle = angle;
    memcpy(rot_y_input, matrix, sizeof(rot_y_input));
    memset(matrix, 0xA5, sizeof(*matrix));
    return matrix;
}

MATRIX* RotMatrixX(int angle, MATRIX* matrix)
{
    rot_x_calls++;
    rot_x_angle = angle;
    memcpy(rot_x_input, matrix, sizeof(rot_x_input));
    memset(matrix, 0x5A, sizeof(*matrix));
    return matrix;
}

MATRIX* MulMatrix0(MATRIX* left, MATRIX* right, MATRIX* output)
{
    (void)right;
    if (mul_calls == 0) {
        memcpy(first_mul_left, left, sizeof(first_mul_left));
        memset(output, 0, sizeof(*output));
        write16(SC_PRODUCT + 0x08u, 111u);
        write16(SC_PRODUCT + 0x0Au, (u16)-222);
    } else {
        memset(output, 0, sizeof(*output));
        write16(SC_SOURCE + 0x06u, (u16)-333);
        write16(SC_SOURCE + 0x08u, 444u);
    }
    mul_calls++;
    return output;
}

int SquareRoot0(int value)
{
    square_root_calls++;
    square_root_input = value;
    return 10;
}

static void seed_vectors(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write16(VECTOR_A + 0u, 1u);
    write16(VECTOR_A + 2u, (u16)-2);
    write16(VECTOR_A + 4u, 3u);
    write16(VECTOR_B + 0u, 4u);
    write16(VECTOR_B + 2u, 5u);
    write16(VECTOR_B + 4u, (u16)-6);
    write16(VECTOR_C + 0u, (u16)-7);
    write16(VECTOR_C + 2u, 8u);
    write16(VECTOR_C + 4u, 9u);
}

static void test_blend(void)
{
    seed_vectors();
    wm_80076858(2048, VECTOR_A, VECTOR_B, VECTOR_C, VECTOR_OUT);
    check(read32(VECTOR_OUT + 0u) == 147456u &&
          read32(VECTOR_OUT + 4u) == 294912u &&
          read32(VECTOR_OUT + 8u) == (u32)(int32_t)-196608,
          "blend.middle_bias");

    wm_80076858(0, VECTOR_A, VECTOR_B, VECTOR_C, VECTOR_OUT);
    check(read32(VECTOR_OUT + 0u) == 163840u &&
          read32(VECTOR_OUT + 4u) == 98304u &&
          read32(VECTOR_OUT + 8u) == (u32)(int32_t)-98304,
          "blend.endpoint_weights");
}

static void test_planar_distance(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    square_root_calls = 0;
    square_root_input = 0;
    write32(DIST_LEFT + 0u, 10u << 12);
    write32(DIST_RIGHT + 0u, 4u << 12);
    write32(DIST_LEFT + 4u, 100u << 12);
    write32(DIST_RIGHT + 4u, 1u << 12);
    write32(DIST_LEFT + 8u, 3u << 12);
    write32(DIST_RIGHT + 8u, (u32)(int32_t)(-5 * 4096));
    check(wm_80094154(DIST_LEFT, DIST_RIGHT) == 10 &&
          square_root_calls == 1 && square_root_input == 100,
          "distance.planar_xz");
}

static void reset_math_trace(void)
{
    ratan_calls = 0;
    rot_y_calls = 0;
    rot_x_calls = 0;
    mul_calls = 0;
    memset(ratan_y, 0, sizeof(ratan_y));
    memset(ratan_x, 0, sizeof(ratan_x));
    memset(rot_y_input, 0, sizeof(rot_y_input));
    memset(rot_x_input, 0, sizeof(rot_x_input));
    memset(first_mul_left, 0, sizeof(first_mul_left));
}

static void seed_decompose(void)
{
    u32 i;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    for (i = 0u; i < 32u; i++) {
        ((uint8_t*)PSX_ADDR(MATRIX_IN))[i] = (uint8_t)(0x20u + i);
        ((uint8_t*)PSX_ADDR(BASE_MATRIX))[i] = (uint8_t)(0x80u + i);
    }
    write16(MATRIX_IN + 0x0Cu, (u16)-90);
    write16(MATRIX_IN + 0x10u, 123u);
    write16(ANGLES_OUT + 0u, 0x7777u);
    write16(ANGLES_OUT + 2u, 0x7777u);
    write16(ANGLES_OUT + 4u, 0x7777u);
    reset_math_trace();
}

static void test_zero_guard(void)
{
    seed_decompose();
    write16(MATRIX_IN + 0x0Cu, 0u);
    write16(MATRIX_IN + 0x10u, 0u);
    wm_80097070(MATRIX_IN, ANGLES_OUT);
    check(ratan_calls == 0 && rot_y_calls == 0 && rot_x_calls == 0 &&
          mul_calls == 0 && read16(ANGLES_OUT) == 0x7777u &&
          read16(ANGLES_OUT + 2u) == 0x7777u &&
          read16(ANGLES_OUT + 4u) == 0x7777u,
          "decompose.zero_guard");
}

static void test_decompose_flow(void)
{
    uint8_t original_matrix[32];
    uint8_t original_base[32];

    seed_decompose();
    memcpy(original_matrix, PSX_ADDR(MATRIX_IN), sizeof(original_matrix));
    memcpy(original_base, PSX_ADDR(BASE_MATRIX), sizeof(original_base));
    wm_80097070(MATRIX_IN, ANGLES_OUT);

    check(ratan_calls == 3 && ratan_y[0] == -90 && ratan_x[0] == 123,
          "decompose.first_ratan_args");
    check(ratan_y[1] == -222 && ratan_x[1] == 111 &&
          ratan_y[2] == -333 && ratan_x[2] == 444,
          "decompose.derived_ratan_args");
    check(rot_y_calls == 1 && rot_y_angle == 0x0234 &&
          rot_x_calls == 1 && rot_x_angle == 0x0345,
          "decompose.rotation_angles");
    check(memcmp(rot_y_input, original_base, sizeof(original_base)) == 0 &&
          memcmp(rot_x_input, original_base, sizeof(original_base)) == 0,
          "decompose.base_matrix");
    check(memcmp(first_mul_left, original_matrix,
                 sizeof(original_matrix)) == 0,
          "decompose.source_matrix");
    check(read16(ANGLES_OUT + 0u) == 0x0345u &&
          read16(ANGLES_OUT + 2u) == 0x0234u,
          "decompose.angle_outputs");
    check(read16(ANGLES_OUT + 4u) == 0xFBAAu,
          "decompose.final_negation");
    check(memcmp(PSX_ADDR(MATRIX_IN), original_matrix,
                 sizeof(original_matrix)) == 0 &&
          memcmp(PSX_ADDR(BASE_MATRIX), original_base,
                 sizeof(original_base)) == 0,
          "decompose.inputs_read_only");
}

int main(void)
{
    test_blend();
    test_planar_distance();
    test_zero_guard();
    test_decompose_flow();
    if (failures != 0) {
        fprintf(stderr, "W34N59 MODE PATH MATH CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N59 MODE PATH MATH CERTIFICATE PASS");
    return 0;
}
