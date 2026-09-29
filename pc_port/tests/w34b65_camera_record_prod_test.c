/* Focused production-linked certificate for retail wm_80096F18. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_96f18.h"

#define OUT 0x8009BD40u
#define POS 0x8009BE28u
#define ROT 0x8009BD38u
#define SC  0x1F800000u

static int failures;
static int rot_count;
static int apply_lv_count;
static int apply_count;
static u32 rot_in[2];
static u32 rot_out[2];
static s16 rot_value[2][3];
static u32 lv_args[3];
static s32 lv_value[3];
static u32 apply_args[3];
static s16 apply_value[3];

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        failures++;
    }
}

static u32 guest_of(const void *pointer)
{
    uintptr_t offset = (uintptr_t)pointer - (uintptr_t)g_PsxRam;
    if (offset < 0x400u)
        return UINT32_C(0x1F800000) + (u32)offset;
    return UINT32_C(0x80000000) | (u32)offset;
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, s32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 read16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

MATRIX *RotMatrixYXZ(SVECTOR *rotation, MATRIX *matrix)
{
    int index = rot_count;
    if (index < 2) {
        rot_in[index] = guest_of(rotation);
        rot_out[index] = guest_of(matrix);
        rot_value[index][0] = rotation->vx;
        rot_value[index][1] = rotation->vy;
        rot_value[index][2] = rotation->vz;
    }
    memset(matrix, (index == 0) ? 0x31 : 0x72, sizeof(*matrix));
    rot_count++;
    return matrix;
}

VECTOR *ApplyMatrixLV(MATRIX *matrix, VECTOR *input, VECTOR *output)
{
    lv_args[0] = guest_of(matrix);
    lv_args[1] = guest_of(input);
    lv_args[2] = guest_of(output);
    lv_value[0] = (s32)input->vx;
    lv_value[1] = (s32)input->vy;
    lv_value[2] = (s32)input->vz;
    output->vx = 0x12345;
    output->vy = -0x23456;
    output->vz = 0x34567;
    apply_lv_count++;
    return output;
}

VECTOR *ApplyMatrix(MATRIX *matrix, SVECTOR *input, VECTOR *output)
{
    apply_args[0] = guest_of(matrix);
    apply_args[1] = guest_of(input);
    apply_args[2] = guest_of(output);
    apply_value[0] = input->vx;
    apply_value[1] = input->vy;
    apply_value[2] = input->vz;
    output->vx = -0x45678;
    output->vy = 0x56789;
    output->vz = -0x6789A;
    apply_count++;
    return output;
}

static int allowed_write(size_t offset)
{
    u32 address = UINT32_C(0x80000000) | (u32)offset;
    if (offset < 0x1B0u)
        return 1;
    return address >= OUT && address < OUT + 0x20u;
}

int main(void)
{
    unsigned char pos_before[16];
    unsigned char rot_before[8];
    unsigned char *before;
    size_t i;

    PsxMemory_Init();
    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    write32(POS + 0u, 0x11111111);
    write32(POS + 4u, 0x01234000);
    write32(POS + 8u, -0x2222222);
    write16(ROT + 0u, (u16)(s16)-0x123);
    write16(ROT + 2u, 0x456u);
    write16(ROT + 4u, (u16)(s16)-0x789);
    write16(ROT + 6u, 0x1357u);
    memcpy(pos_before, PSX_ADDR(POS), sizeof(pos_before));
    memcpy(rot_before, PSX_ADDR(ROT), sizeof(rot_before));
    before = malloc(PSX_RAM_SIZE);
    check(before != NULL, "snapshot.alloc");
    if (before == NULL)
        return 1;
    memcpy(before, g_PsxRam, PSX_RAM_SIZE);

    wm_80096F18(OUT, POS, 0x05678000, ROT);

    check(memcmp(pos_before, PSX_ADDR(POS), sizeof(pos_before)) == 0,
          "position.read_only");
    check(memcmp(rot_before, PSX_ADDR(ROT), sizeof(rot_before)) == 0,
          "rotation.read_only");
    check(rot_count == 2, "rotation.count");
    check(rot_in[0] == SC + 0xA0u && rot_out[0] == SC + 0xF0u,
          "first.rotation.A0_to_F0");
    check(rot_value[0][0] == (s16)-0x123 &&
              rot_value[0][1] == (s16)0x456 && rot_value[0][2] == 0,
          "first.rotation.YXZ.source");
    check(apply_lv_count == 1 && lv_args[0] == SC + 0xF0u &&
              lv_args[1] == SC && lv_args[2] == SC + 0x10u,
          "height.apply.route");
    check(lv_value[0] == 0 && lv_value[1] == 0 && lv_value[2] == -0x5678,
          "height.negated.shift12");
    check(read16(OUT + 0u) == 0x2345u &&
              read16(OUT + 2u) == (u16)(-0x23456 + 0x1234) &&
              read16(OUT + 4u) == 0x4567u,
          "eye.result.plus.position_y");
    check(read16(OUT + 8u) == 0 && read16(OUT + 0xAu) == 0x1234u &&
              read16(OUT + 0xCu) == 0,
          "target.position_column");
    check(rot_in[1] == SC + 0xA0u && rot_out[1] == SC + 0xF0u,
          "second.rotation.A0_to_F0");
    check(rot_value[1][0] == 0 && rot_value[1][1] == (s16)0x456 &&
              rot_value[1][2] == (s16)-0x789,
          "second.rotation.YXZ.source");
    check(apply_count == 1 && apply_args[0] == SC + 0xF0u &&
              apply_args[1] == SC + 0xA0u && apply_args[2] == OUT + 0x10u,
          "reference.apply.route");
    check(apply_value[0] == 0 && apply_value[1] == -0x1000 &&
              apply_value[2] == 0,
          "reference.vector");
    for (i = 0; i < PSX_RAM_SIZE; i++)
        if (!allowed_write(i))
            check(g_PsxRam[i] == before[i], "authorized.write.set");
    free(before);

    if (failures != 0) {
        fprintf(stderr, "%d assertion(s) failed\n", failures);
        return 1;
    }
    puts("W34B65 camera-record producer certificate PASS");
    return 0;
}
