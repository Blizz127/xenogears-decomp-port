#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "system/controller_vblank.h"

static uint32_t clock_count;
static unsigned first_calls, second_calls;
static int action;

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "ASSERTION vblank.service line=%d %s\n", __LINE__, #x); \
    exit(1); \
} } while (0)

int PsyX_Sys_GetVBlankCount(void) { return (int)clock_count; }
static void second(void) { ++second_calls; }
static void first(void)
{
    ++first_calls;
    if (action == 1) func_8004B7D0(second);
    if (action == 2) func_8004B7D0(NULL);
    if (action == 3) PcPort_ServiceVblank();
    if (action == 4) ++clock_count;
    if (action == 5) PcPort_MaskVblank();
}

int main(void)
{
    clock_count = 100;
    PcPort_ServiceVblank(); /* No registration, no callback. */
    CHECK(first_calls == 0);
    func_8004B7D0(first);
    PcPort_ServiceVblank();
    CHECK(first_calls == 0);
    clock_count = 103;
    PcPort_ServiceVblank();
    CHECK(first_calls == 3);
    for (int query = 0; query < 50; ++query) PcPort_ServiceVblank();
    CHECK(first_calls == 3);
    clock_count = 104;
    PcPort_ServiceVblank();
    CHECK(first_calls == 4);

    /* Replacement during a callback applies to the remaining queued ticks. */
    action = 1;
    clock_count = 107;
    PcPort_ServiceVblank();
    CHECK(first_calls == 5 && second_calls == 2);
    func_8004B7D0(NULL);
    clock_count = 200;
    PcPort_ServiceVblank();
    CHECK(first_calls == 5 && second_calls == 2);
    func_8004B7D0(first);
    action = 2;
    clock_count = 203;
    PcPort_ServiceVblank();
    CHECK(first_calls == 6 && second_calls == 2);
    PcPort_ServiceVblank();
    CHECK(first_calls == 6);

    /* Reentrant service cannot consume an event twice or recursively drain. */
    action = 3;
    func_8004B7D0(first);
    clock_count = 206;
    PcPort_ServiceVblank();
    CHECK(first_calls == 9);

    /* Unsigned host counter wrap is a forward interval of three ticks. */
    clock_count = UINT32_MAX - 1;
    PcPort_ResetVblankService();
    func_8004B7D0(first);
    clock_count = 1;
    PcPort_ServiceVblank();
    CHECK(first_calls == 12);
    func_8004B7D0(NULL);
    clock_count = 5;
    PcPort_ServiceVblank();
    CHECK(first_calls == 12);
    /* Ticks arriving during a callback are consumed on the next service,
     * never by an unbounded moving-target drain. */
    func_8004B7D0(first);
    action = 4;
    clock_count = 7;
    PcPort_ServiceVblank();
    CHECK(first_calls == 14 && clock_count == 9);
    action = 0;
    PcPort_ServiceVblank();
    CHECK(first_calls == 16);
    /* Critical sections are an enable bit, not a nesting-depth counter. */
    PcPort_ResetVblankService();
    func_8004B7D0(second);
    CHECK(PcPort_MaskVblank() == 1);
    CHECK(PcPort_MaskVblank() == 0);
    clock_count += 20;
    PcPort_ServiceVblank();
    CHECK(second_calls == 2 && PcPort_GetServicedVblankCount() == 0);
    PcPort_UnmaskVblank();
    PcPort_ServiceVblank();
    CHECK(second_calls == 3 && PcPort_GetServicedVblankCount() == 1);
    PcPort_ServiceVblank();
    CHECK(second_calls == 3);
    clock_count++;
    PcPort_ServiceVblank();
    CHECK(second_calls == 4 && PcPort_GetServicedVblankCount() == 2);
    func_8004B7D0(NULL);
    clock_count++;
    PcPort_ServiceVblank();
    CHECK(second_calls == 4 && PcPort_GetServicedVblankCount() == 3);
    PcPort_ResetVblankService();
    action = 5;
    func_8004B7D0(first);
    clock_count += 3;
    PcPort_ServiceVblank();
    CHECK(first_calls == 17 && PcPort_GetServicedVblankCount() == 1);
    PcPort_ServiceVblank();
    CHECK(first_calls == 17);
    action = 0;
    PcPort_UnmaskVblank();
    PcPort_ServiceVblank();
    CHECK(first_calls == 18 && PcPort_GetServicedVblankCount() == 2);
    /* Re-registering the same callback is not a clock reset. */
    clock_count++;
    func_8004B7D0(first);
    PcPort_ServiceVblank();
    CHECK(first_calls == 19 && PcPort_GetServicedVblankCount() == 3);
    puts("CONTROLLER VBLANK SERVICE PASS: elapsed ticks, duplicate suppression, replacement, unregister, reentry, wrap");
    return 0;
}
