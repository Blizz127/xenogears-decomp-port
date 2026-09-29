#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

extern void func_800864F0(void);

u16 D_800AFE88[9];
u16 D_800AFE8A[9];
s16 D_800B233C;

static s32 s_stoppedSlots[4];
static s32 s_stopCount;

void func_8003A20C(s32 slot)
{
    if (s_stopCount >= 4) {
        fputs("field sound teardown: too many stop calls\n", stderr);
        exit(1);
    }
    s_stoppedSlots[s_stopCount++] = slot;
}

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "field sound teardown: FAIL: %s\n", message);
        exit(1);
    }
}

static void seed(u16 mask)
{
    int i;

    for (i = 0; i < 9; i++) {
        D_800AFE88[i] = (u16)(0x1100 + i);
        D_800AFE8A[i] = (u16)(0x2200 + i);
    }
    for (i = 0; i < 4; i++) {
        s_stoppedSlots[i] = -1;
    }
    s_stopCount = 0;
    D_800B233C = (s16)mask;
}

static void require_slot_table_reset(void)
{
    int i;

    for (i = 0; i < 3; i++) {
        require(D_800AFE88[i * 3] == UINT16_C(0xffff),
                "actor owner slot was not reset");
        require(D_800AFE8A[i * 3] == UINT16_C(0xffff),
                "effect id slot was not reset");
    }
}

static void test_unreserved_field_channels_stop(void)
{
    seed(0);
    func_800864F0();

    require_slot_table_reset();
    require(s_stopCount == 4, "mask zero must release all four field pairs");
    require(s_stoppedSlots[0] == 0 && s_stoppedSlots[1] == 2 &&
            s_stoppedSlots[2] == 4 && s_stoppedSlots[3] == 6,
            "field pairs must be released in retail order 0,2,4,6");
    require(D_800B233C == 0, "zero channel mask must remain zero");
}

static void test_reserved_field_channels_survive(void)
{
    seed(UINT16_C(0x000a));
    func_800864F0();

    require_slot_table_reset();
    require(s_stopCount == 2, "reserved field pairs must not be released");
    require(s_stoppedSlots[0] == 0 && s_stoppedSlots[1] == 4,
            "only clear mask bits may release their field pairs");
    require(D_800B233C == 0, "four consumed low mask bits must be shifted out");
}

static void test_upper_mask_bits_shift_down(void)
{
    seed(UINT16_C(0xabcd));
    func_800864F0();

    require_slot_table_reset();
    require(s_stopCount == 1 && s_stoppedSlots[0] == 2,
            "0xabcd low nibble must release only field pair 2");
    require((u16)D_800B233C == UINT16_C(0x0abc),
            "retail teardown must retain the mask after four logical shifts");
}

int main(void)
{
    test_unreserved_field_channels_stop();
    test_reserved_field_channels_survive();
    test_upper_mask_bits_shift_down();
    puts("FIELD SOUND TEARDOWN PASS checks=19");
    return 0;
}
