#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "test_input.h"

static int s_failures;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

static void reset_with(const char *schedule)
{
    PcPort_TestInputResetForCertificate();
    if (schedule == NULL)
        unsetenv("XENO_TEST_INPUT");
    else
        setenv("XENO_TEST_INPUT", schedule, 1);
}

static void expect_invalid(const char *schedule, const char *name)
{
    reset_with(schedule);
    check(PcPort_TestInputInit() != 0, name);
}

static void reset_world_with(const char *schedule)
{
    PcPort_WorldTestInputResetForCertificate();
    if (schedule == NULL)
        unsetenv("XENO_WORLD_TEST_INPUT");
    else
        setenv("XENO_WORLD_TEST_INPUT", schedule, 1);
}

static void test_world_schedule(void)
{
    u16 held;
    u16 pressed;
    u16 repeated;

    reset_world_with(NULL);
    check(PcPort_WorldTestInputInit() == 0, "world.unset.init.success");
    held = 0x0001u;
    pressed = 0x0002u;
    repeated = 0x0004u;
    PcPort_WorldTestInputMerge(&held, &pressed, &repeated);
    check(held == 0x0001u && pressed == 0x0002u && repeated == 0x0004u,
          "world.unset.merge.inert");

    reset_world_with("1:0x2000");
    check(PcPort_WorldTestInputInit() != 0,
          "world.malformed.first_boundary");

    reset_world_with("0:0x2000,2:0x20,3:0");
    check(PcPort_WorldTestInputInit() == 0, "world.valid.init.success");

    held = 0x0001u;
    pressed = 0x0002u;
    repeated = 0x0004u;
    PcPort_WorldTestInputMerge(&held, &pressed, &repeated);
    check(held == 0x2001u, "world.frame1.held");
    check(pressed == 0x2002u && repeated == 0x2004u,
          "world.frame1.rising");

    held = 0u;
    pressed = 0u;
    repeated = 0u;
    PcPort_WorldTestInputMerge(&held, &pressed, &repeated);
    check(held == 0x2000u && pressed == 0u && repeated == 0u,
          "world.frame2.hold-no-repeat");

    held = 0x1000u;
    pressed = 0u;
    repeated = 0u;
    PcPort_WorldTestInputMerge(&held, &pressed, &repeated);
    check(held == 0x1020u, "world.frame3.merge-preserves-native-held");
    check(pressed == 0x0020u && repeated == 0x0020u,
          "world.frame3.new-button-rising");

    held = 0u;
    pressed = 0u;
    repeated = 0u;
    PcPort_WorldTestInputMerge(&held, &pressed, &repeated);
    check(held == 0u && pressed == 0u && repeated == 0u,
          "world.frame4.release-has-no-press-edge");
}

int main(void)
{
    static const u16 expected[] = {
        0x2000u, 0x2000u, 0x2000u, 0x4000u, 0x4000u, 0u, 0u
    };
    u16 destination;
    size_t i;

    reset_with(NULL);
    destination = 0xA55Au;
    check(PcPort_TestInputInit() == 0, "unset.init.success");
    PcPort_TestInputAdvanceFrame();
    PcPort_TestInputInject(&destination);
    PcPort_TestInputAdvanceFrame();
    PcPort_TestInputInject(&destination);
    check(destination == 0xA55Au, "unset.injection.inert");

    expect_invalid("", "malformed.empty");
    expect_invalid("1:0", "malformed.first_boundary");
    expect_invalid("0", "malformed.missing_colon");
    expect_invalid("0:0x10000", "malformed.value_range");
    expect_invalid("0:1,", "malformed.trailing_comma");
    expect_invalid("0:1,0:2", "malformed.nonincreasing");
    expect_invalid("0:1x", "malformed.trailing_junk");

    reset_with("0:0x2000,3:0x4000,5:0");
    check(PcPort_TestInputInit() == 0, "valid.init.success");
    for (i = 0u; i < sizeof(expected) / sizeof(expected[0]); i++) {
        destination = 0xFFFFu;
        PcPort_TestInputAdvanceFrame();
        PcPort_TestInputInject(&destination);
        check(destination == expected[i],
              i == 0u ? "boundary.frame0" :
              i == 1u ? "hold.frame1" :
              i == 2u ? "hold.frame2" :
              i == 3u ? "boundary.frame3" :
              i == 4u ? "hold.frame4" :
              i == 5u ? "boundary.frame5" : "hold.frame6");
    }

    reset_with("0:0");
    destination = 0xFFFFu;
    check(PcPort_TestInputInit() == 0, "zero_schedule.init.success");
    PcPort_TestInputAdvanceFrame();
    PcPort_TestInputInject(&destination);
    check(destination == 0u, "set.schedule.injects.zero");

    test_world_schedule();

    if (s_failures != 0) {
        fprintf(stderr, "%d assertion(s) failed\n", s_failures);
        return 1;
    }
    puts("W34C1 scripted input certificate PASS");
    return 0;
}
