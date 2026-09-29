#include <stdio.h>
#include <stdlib.h>

#include "field_direct_boot_route.h"

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FIELD DIRECT BOOT FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    require(PcPort_SelectBootMovie(1) == 16,
            "retail disc 1 boot must select movie 16, not the disc number");
    require(PcPort_SelectBootMovie(2) == 7,
            "retail other-disc boot must select movie 7");
    require(PcPort_SelectBootMovie(0) == 7 && PcPort_SelectBootMovie(-1) == 7,
            "retail branch tests equality with 1, not a guessed disc range");
    require(PcPort_SelectBootState(NULL, NULL) == 6,
            "retail boot must enter MovieMain");
    require(PcPort_SelectBootState("0", "0") == 6,
            "disabled field-test must enter MovieMain");
    require(PcPort_SelectBootState("1", NULL) == 0,
            "interactive field-test must retain KernelMenu");
    require(PcPort_SelectBootState("1", "") == 0,
            "empty selector must retain KernelMenu");
    require(PcPort_SelectBootState("1", "0") == 1,
            "forced Field selection must enter FieldMain without drawing KernelMenu");
    require(PcPort_SelectBootState("1", "4") == 0,
            "other forced selections must retain KernelMenu dispatch");
    puts("FIELD DIRECT BOOT ROUTE PASS");
    return EXIT_SUCCESS;
}
