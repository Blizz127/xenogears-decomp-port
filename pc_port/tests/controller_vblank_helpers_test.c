#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t D_800501F8, D_80059370, D_80059418, D_80059420, D_80059484;
_Alignas(8) uint8_t D_8005A1BC[16];
extern void func_80035E44(void);
extern void func_80036188(uint8_t*);
extern void func_80036220(void);

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "ASSERTION vblank.helpers line=%d %s\n", __LINE__, #x); \
    exit(1); \
} } while (0)

int main(void)
{
    /* Exhaust every 16-bit countdown, all byte states and both enable paths.
     * The expected transition is independent of the production routine. */
    for (unsigned count = 0; count <= UINT16_MAX; ++count) {
        for (unsigned state = 0; state <= UINT8_MAX; ++state) {
            for (unsigned disabled = 0; disabled < 2; ++disabled) {
                /* Guard byte, the 8-byte record, guard byte.  The record
                 * itself is 8-aligned like D_8005A1BC (retail reads the
                 * count with LH, which faults on a misaligned address). */
                union { uint8_t b[24]; uint64_t align; } window;
                const uint8_t init[10] = {0xAC, 7, 8, 9, 10,
                    (uint8_t)count, (uint8_t)(count >> 8),
                    (uint8_t)state, (uint8_t)disabled, 0xCA};
                uint8_t* record = window.b + 7;
                uint8_t expected[10];
                memcpy(record, init, sizeof(init));
                memcpy(expected, record, sizeof(init));
                if (!disabled) {
                    if (count || state == 1) {
                        expected[1] = 1;
                        expected[2] = 0x40;
                        expected[3] = count != 0;
                        expected[4] = 0;
                        expected[7] = count ? 1 : 2;
                        if (count) {
                            expected[5] = (uint8_t)(count - 1);
                            expected[6] = (uint8_t)((count - 1) >> 8);
                        }
                    } else if (state == 2) {
                        expected[1] = expected[7] = 0;
                    }
                }
                func_80036188(record + 1);
                CHECK(memcmp(record, expected, sizeof(expected)) == 0);
            }
        }
    }
    memset(D_8005A1BC, 0, sizeof(D_8005A1BC));
    D_8005A1BC[4] = 1;
    D_8005A1BC[8 + 6] = 1;
    func_80036220();
    CHECK(D_8005A1BC[4] == 0 && D_8005A1BC[6] == 1);
    CHECK(D_8005A1BC[8 + 2] == 0 && D_8005A1BC[8 + 6] == 2);

    /* Each counter comparison is independent, not nested in the prior carry. */
    for (unsigned stage = 0; stage < 4; ++stage) {
        for (unsigned value = 0; value <= UINT8_MAX; ++value) {
            uint8_t counters[4] = {13, 17, 19, 23};
            uint8_t expected[4];
            counters[stage] = (uint8_t)value;
            memcpy(expected, counters, sizeof(expected));
            expected[0]++;
            for (unsigned i = 0; i < 3; ++i) {
                if (expected[i] == 60) {
                    expected[i] = 0;
                    expected[i + 1]++;
                }
            }
            D_800501F8 = 0;
            D_80059370 = counters[0]; D_80059418 = counters[1];
            D_80059420 = counters[2]; D_80059484 = counters[3];
            func_80035E44();
            CHECK(D_80059370 == expected[0] && D_80059418 == expected[1]);
            CHECK(D_80059420 == expected[2] && D_80059484 == expected[3]);
            CHECK(D_800501F8 == (expected[3] == 100));
        }
    }
    D_800501F8 = 0;
    D_80059370 = D_80059418 = D_80059420 = 59;
    D_80059484 = 99;
    func_80035E44();
    CHECK(D_80059370 == 0 && D_80059418 == 0 && D_80059420 == 0);
    CHECK(D_80059484 == 100 && D_800501F8 == 1);
    for (unsigned flag = 1; flag <= UINT8_MAX; ++flag) {
        D_800501F8 = (uint8_t)flag;
        func_80035E44();
        CHECK(D_80059370 == 0 && D_80059418 == 0 && D_80059420 == 0);
        CHECK(D_80059484 == 100 && D_800501F8 == flag);
    }
    puts("CONTROLLER VBLANK HELPERS PASS: countdown domain, record pair, clock carries and freeze");
    return 0;
}
