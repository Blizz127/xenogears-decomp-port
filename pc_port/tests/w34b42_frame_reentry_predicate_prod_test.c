/* W34B42 production-linked certificate for the reviewed one-reentry gate. */
#include <stdio.h>

#include "common.h"
#include "world_map_frame_driver.h"

static int s_pass;
static int s_fail;
static int s_total;

static void check_case(const char *name, int expected, int actual)
{
    s_total++;
    if (expected == actual) {
        s_pass++;
    } else {
        s_fail++;
        printf("FAIL [%s]: expected=%d actual=%d\n", name, expected, actual);
    }
}

int main(void)
{
    printf("=== W34B42 0x800719C8 re-entry predicate ===\n");
    check_case("gate-off-d554-zero", 0,
               wm_800719C8_should_reenter_once(0, 0u));
    check_case("gate-off-d554-one", 0,
               wm_800719C8_should_reenter_once(0, 1u));
    check_case("gate-on-d554-zero", 0,
               wm_800719C8_should_reenter_once(1, 0u));
    check_case("gate-on-d554-one", 1,
               wm_800719C8_should_reenter_once(1, 1u));
    check_case("gate-on-d554-high", 1,
               wm_800719C8_should_reenter_once(1, 0xFFFFFFFFu));
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
