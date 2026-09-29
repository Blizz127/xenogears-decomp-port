#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psyq_normal_light_col.h"

typedef struct GteCall {
    unsigned int value;
    int reg;
} GteCall;

static GteCall s_mtc2[4];
static int s_mtc2_count;
static int s_cop2_count;
static int s_cop2_op;
static int s_mfc2_count;
static int s_mfc2_reg;
static unsigned int s_mfc2_value;
static int s_passes;
static int s_total;

void MTC2(unsigned int value, int reg)
{
    if (s_mtc2_count < (int)(sizeof(s_mtc2) / sizeof(s_mtc2[0]))) {
        s_mtc2[s_mtc2_count].value = value;
        s_mtc2[s_mtc2_count].reg = reg;
    }
    s_mtc2_count++;
}

unsigned int MFC2(int reg)
{
    s_mfc2_count++;
    s_mfc2_reg = reg;
    return s_mfc2_value;
}

int doCOP2(int op)
{
    s_cop2_count++;
    s_cop2_op = op;
    return 0;
}

static void check(const char* name, int condition)
{
    s_total++;
    if (condition) {
        s_passes++;
        printf("PASS [normal-light-col]: %s\n", name);
    } else {
        printf("FAIL [normal-light-col]: %s\n", name);
    }
}

static u32 load_u32(const u8* source)
{
    u32 value;
    memcpy(&value, source, sizeof(value));
    return value;
}

static void run_unaligned_case(void)
{
    u8 normal_storage[12] = {
        0xA5, 0x34, 0x12, 0x78, 0x56, 0xBC, 0x9A, 0xF0, 0xDE, 0x5A, 0xA5, 0xA5
    };
    u8 color_storage[8] = {
        0xC3, 0x11, 0x22, 0x33, 0x44, 0xC3, 0xC3, 0xC3
    };
    u8 output_storage[10];
    u8 normal_before[sizeof(normal_storage)];
    u8 color_before[sizeof(color_storage)];
    const u8 expected_prefix[3] = {0xEE, 0xEE, 0xEE};
    const u8 expected_suffix[3] = {0xEE, 0xEE, 0xEE};

    memset(output_storage, 0xEE, sizeof(output_storage));
    memcpy(normal_before, normal_storage, sizeof(normal_storage));
    memcpy(color_before, color_storage, sizeof(color_storage));
    memset(s_mtc2, 0, sizeof(s_mtc2));
    s_mtc2_count = 0;
    s_cop2_count = 0;
    s_cop2_op = 0;
    s_mfc2_count = 0;
    s_mfc2_reg = -1;
    s_mfc2_value = UINT32_C(0xA1B2C3D4);

    NormalLightCol(normal_storage + 1, color_storage + 1,
                   output_storage + 3);

    check("three GTE input loads", s_mtc2_count == 3);
    check("normal word zero loads VXY register",
          s_mtc2_count >= 1 && s_mtc2[0].reg == 0 &&
          s_mtc2[0].value == UINT32_C(0x56781234));
    check("normal word one loads VZ register",
          s_mtc2_count >= 2 && s_mtc2[1].reg == 1 &&
          s_mtc2[1].value == UINT32_C(0xDEF09ABC));
    check("input color loads RGBC register",
          s_mtc2_count >= 3 && s_mtc2[2].reg == 6 &&
          s_mtc2[2].value == UINT32_C(0x44332211));
    check("retail NCCS opcode",
          s_cop2_count == 1 && s_cop2_op == 0x0108041B);
    check("reads RGB2 register", s_mfc2_count == 1 && s_mfc2_reg == 22);
    check("stores RGB2 output",
          load_u32(output_storage + 3) == UINT32_C(0xA1B2C3D4));
    check("output prefix preserved",
          memcmp(output_storage, expected_prefix, sizeof(expected_prefix)) == 0);
    check("output suffix preserved",
          memcmp(output_storage + 7, expected_suffix, sizeof(expected_suffix)) == 0);
    check("normal input read-only",
          memcmp(normal_before, normal_storage, sizeof(normal_storage)) == 0);
    check("color input read-only",
          memcmp(color_before, color_storage, sizeof(color_storage)) == 0);
}

int main(void)
{
    run_unaligned_case();
    printf("W34N120 NORMAL LIGHT COL CERTIFICATE %s (%d/%d)\n",
           s_passes == s_total ? "PASS" : "FAIL", s_passes, s_total);
    return s_passes == s_total ? 0 : 1;
}
