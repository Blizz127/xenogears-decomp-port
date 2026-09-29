#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psyq_spu_noise_clock.h"

static u8 s_registers[0x200];
void* g_pSoundSpuRegisters = s_registers;

static int s_passes;
static int s_total;

static void check(const char* name, int condition)
{
    s_total++;
    if (condition) {
        s_passes++;
        printf("PASS [spu-noise-clock]: %s\n", name);
    } else {
        printf("FAIL [spu-noise-clock]: %s\n", name);
    }
}

static void store_u16(u8* destination, u16 value)
{
    memcpy(destination, &value, sizeof(value));
}

static u16 load_u16(const u8* source)
{
    u16 value;
    memcpy(&value, source, sizeof(value));
    return value;
}

static long run_case(long input, u16 before, u16* after)
{
    long result;
    memset(s_registers, 0xA5, sizeof(s_registers));
    store_u16(s_registers + 0x1AA, before);
    result = SpuSetNoiseClock(input);
    *after = load_u16(s_registers + 0x1AA);
    return result;
}

int main(void)
{
    u16 after;
    long result;
    u8 expected_prefix[0x1AA];
    u8 expected_suffix[0x200 - 0x1AC];

    result = run_case(-7, UINT16_C(0xFFFF), &after);
    check("negative input clamps to zero",
          result == 0 && after == UINT16_C(0xC0FF));

    result = run_case(64, UINT16_C(0xC055), &after);
    check("above-range input clamps to 63",
          result == 63 && after == UINT16_C(0xFF55));

    result = run_case(17, UINT16_C(0xA5C3), &after);
    check("in-range value occupies SPUCNT bits 13 through 8",
          result == 17 && after == UINT16_C(0x91C3));
    check("non-clock SPUCNT bits preserved",
          (after & UINT16_C(0xC0FF)) == UINT16_C(0x80C3));

    memset(expected_prefix, 0xA5, sizeof(expected_prefix));
    memset(expected_suffix, 0xA5, sizeof(expected_suffix));
    check("writes retail SPUCNT offset",
          load_u16(s_registers + 0x1AA) == UINT16_C(0x91C3));
    check("register prefix untouched",
          memcmp(s_registers, expected_prefix, sizeof(expected_prefix)) == 0);
    check("register suffix untouched",
          memcmp(s_registers + 0x1AC, expected_suffix,
                 sizeof(expected_suffix)) == 0);

    result = run_case(63, UINT16_C(0x0000), &after);
    check("upper boundary accepted",
          result == 63 && after == UINT16_C(0x3F00));
    result = run_case(0, UINT16_C(0xFFFF), &after);
    check("lower boundary accepted",
          result == 0 && after == UINT16_C(0xC0FF));

    printf("W34N121 SPU NOISE CLOCK CERTIFICATE %s (%d/%d)\n",
           s_passes == s_total ? "PASS" : "FAIL", s_passes, s_total);
    return s_passes == s_total ? 0 : 1;
}
