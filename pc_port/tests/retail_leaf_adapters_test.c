#include "common.h"
#include "system/font.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void func_8003633C(s32 value);
void func_800379B4(s32 value);
void func_800379C8(char* format, ...);
void func_80026F44(s32 count, s32 factor, u16* destination,
                   const u16* source);
MATRIX* RotMatrixZYX(SVECTOR* rotation, MATRIX* matrix);
void SetPolyFT3(POLY_FT3* primitive);
extern void SetLineF2(LINE_F2* primitive) __attribute__((weak));
s32 ReadGeomScreen(void);

u8 D_8005938C;
s32 D_80050618;
Font* g_Font;

static s32 g_GteData[32];
static int g_MtcCalls;
static int g_MfcCalls;
static int g_GpfCalls;
static int g_FormatCalls;
static char g_Formatted[128];
static unsigned int g_Checks;
static u32 g_GeomScreen;
static int g_CfcReg;

unsigned int CFC2(int reg)
{
    g_CfcReg = reg;
    return g_GeomScreen;
}

/* SetMulMatrix is tested against the real GTE and retail instructions in
 * battle_gte_retail_test.c; this small GPF-only double must not fake it. */
void CTC2(unsigned int value, int reg)
{
    (void)value;
    (void)reg;
    assert(!"CTC2 requires the real-GTE comparison suite");
}

void MTC2(unsigned int value, int reg)
{
    if (reg < 0 || reg >= 32) {
        fprintf(stderr, "RETAIL_LEAF_ADAPTERS FAIL invalid MTC2 register\n");
        return;
    }
    g_GteData[reg] = (s32)value;
    g_MtcCalls++;
}

unsigned int MFC2(int reg)
{
    if (reg < 0 || reg >= 32) {
        fprintf(stderr, "RETAIL_LEAF_ADAPTERS FAIL invalid MFC2 register\n");
        return 0;
    }
    g_MfcCalls++;
    return (u32)g_GteData[reg];
}

static s32 clamp_ir(s64 value)
{
    if (value < -0x8000) return -0x8000;
    if (value > 0x7FFF) return 0x7FFF;
    return (s32)value;
}

int doCOP2(int opcode)
{
    if (opcode != 0x0198003D) {
        fprintf(stderr, "RETAIL_LEAF_ADAPTERS FAIL wrong GTE opcode\n");
        return -1;
    }
    for (int reg = 9; reg <= 11; reg++) {
        g_GteData[reg] = clamp_ir(((s64)(s16)g_GteData[8] *
                                  (s64)(s16)g_GteData[reg]) >> 12);
    }
    g_GpfCalls++;
    return 0;
}

void func_80036718(int mode, char* format, va_list forwarded)
{
    if (mode != 0) {
        fprintf(stderr, "RETAIL_LEAF_ADAPTERS FAIL formatter mode\n");
        return;
    }
    vsnprintf(g_Formatted, sizeof(g_Formatted), format, forwarded);
    g_FormatCalls++;
}

static int require(int condition, const char* message)
{
    g_Checks++;
    if (!condition) {
        fprintf(stderr, "RETAIL_LEAF_ADAPTERS FAIL %s\n", message);
        return 0;
    }
    return 1;
}

static int test_store_leaves(void)
{
    D_8005938C = 0;
    D_80050618 = 0;
    func_8003633C(0x1234ABCD);
    func_800379B4((s32)0x89ABCDEF);
    return require(D_8005938C == 0xCD, "byte-store leaf") &&
           require((u32)D_80050618 == 0x89ABCDEF, "word-store leaf");
}

static int test_printf_alias(void)
{
    Font font;
    memset(&font, 0, sizeof(font));
    memset(g_Formatted, 0, sizeof(g_Formatted));
    g_FormatCalls = 0;
    g_Font = &font;
    func_800379C8("actor=%d %s", 7, "bad");
    if (!require(g_FormatCalls == 1, "printf alias forwards once") ||
        !require(strcmp(g_Formatted, "actor=7 bad") == 0,
                 "printf alias preserves variadic arguments")) {
        return 0;
    }
    g_Font = NULL;
    func_800379C8("must not format %d", 9);
    return require(g_FormatCalls == 1, "printf alias retains null-font guard");
}

static int test_gte_pixel_scale(void)
{
    const u16 source[] = {0xFFFF, 0x4210, 0x8001, 0x0001, 0x0000};
    u16 destination[5] = {0};

    memset(g_GteData, 0, sizeof(g_GteData));
    g_MtcCalls = g_MfcCalls = g_GpfCalls = 0;
    func_80026F44(5, 16, destination, source);
    if (!require(destination[0] == 0xBDEF, "half-scale white preserves STP") ||
        !require(destination[1] == 0x2108, "half-scale packed channels") ||
        !require(destination[2] == 0x8001, "nonzero underflow guard with STP") ||
        !require(destination[3] == 0x0001, "nonzero underflow guard") ||
        !require(destination[4] == 0x0000, "transparent zero remains zero") ||
        !require(g_MtcCalls == 16, "IR0 plus three inputs per pixel") ||
        !require(g_MfcCalls == 15, "three GTE outputs per pixel") ||
        !require(g_GpfCalls == 5, "one GPF12 per pixel")) {
        return 0;
    }

    memset(destination, 0, sizeof(destination));
    func_80026F44(1, 99, destination, source);
    if (!require(destination[0] == source[0], "factor clamps to 32")) {
        return 0;
    }
    destination[0] = 0x6BAD;
    func_80026F44(0, 12, destination, source);
    return require(destination[0] == 0x6BAD, "zero count does not touch output");
}

static int test_psyq_leaf_forwarders(void)
{
    SVECTOR rotation;
    MATRIX matrix;
    POLY_FT3 primitive;
    unsigned char before[sizeof(primitive)];
    unsigned char after[sizeof(primitive)];

    memset(&rotation, 0, sizeof(rotation));
    memset(&matrix, 0, sizeof(matrix));
    if (!require(RotMatrixZYX(&rotation, &matrix) == &matrix,
                 "RotMatrixZYX returns the destination")) {
        return 0;
    }
    /* Actual trig owners are linked; exhaustive retail comparison lives in
     * rotation_zyx_retail_test.c, replacing the obsolete delegate-only spy. */
    for (unsigned row=0;row<3;row++) for (unsigned col=0;col<3;col++)
        if (!require(matrix.m[row][col] == (row==col?4096:0),
                     "RotMatrixZYX zero angles produce identity")) return 0;

    memset(&primitive, 0xA5, sizeof(primitive));
    memcpy(before, &primitive, sizeof(primitive));
    SetPolyFT3(&primitive);
    memcpy(after, &primitive, sizeof(primitive));
    if (!require(getlen(&primitive) == 7, "SetPolyFT3 packet length") ||
        !require(getcode(&primitive) == 0x24, "SetPolyFT3 GPU opcode")) {
        return 0;
    }
    before[3] = after[3];
    before[7] = after[7];
    return require(memcmp(before, after, sizeof(primitive)) == 0,
                   "SetPolyFT3 changes only tag length and packet opcode");
}

static int test_line_f2(void)
{
    struct {
        u32 guardBefore;
        LINE_F2 primitive;
        u32 guardAfter;
    } packet;
    u8 expected[sizeof(packet)];
    const u8 patterns[] = {0, 0xa5, 0xff, 0x3c};
    if (!require(SetLineF2 != NULL, "SetLineF2 production owner missing")) return 0;
    for (unsigned i = 0; i < sizeof(patterns); i++) {
        memset(&packet, patterns[i], sizeof(packet));
        memcpy(expected, &packet, sizeof(expected));
        size_t offset = (u8*)&packet.primitive - (u8*)&packet;
        expected[offset + 3] = 3;
        expected[offset + 7] = 0x40;
        SetLineF2(&packet.primitive);
        if (!require(memcmp(expected, &packet, sizeof(packet)) == 0,
                     "SetLineF2 writes only retail length/opcode bytes, preserving coordinates/link/guards"))
            return 0;
    }
    return 1;
}

int main(void)
{
    g_GeomScreen = 0xffff8000u;
    if (!require((u32)ReadGeomScreen() == g_GeomScreen,
                 "ReadGeomScreen returns all CFC2 bits") ||
        !require(g_CfcReg == 26, "ReadGeomScreen reads control register H")) {
        return 1;
    }
    if (!test_store_leaves() || !test_printf_alias() ||
        !test_gte_pixel_scale() || !test_psyq_leaf_forwarders() || !test_line_f2()) {
        return 1;
    }
    printf("RETAIL_LEAF_ADAPTERS PASS checks=%u\n", g_Checks);
    return 0;
}
