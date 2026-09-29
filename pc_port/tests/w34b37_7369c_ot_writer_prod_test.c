#include <stdio.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"

extern u32 wm_8007369C_test_host_to_psx(void* p);

int main(void)
{
    void* first = PSX_ADDR(0x80012340u);
    void* second = PSX_ADDR(0x8001E7A0u);
    u32 first_psx = wm_8007369C_test_host_to_psx(first);
    u32 second_psx = wm_8007369C_test_host_to_psx(second);

    if (first_psx != 0x80012340u || second_psx != 0x8001E7A0u ||
        first_psx == second_psx) {
        fprintf(stderr,
                "W34B37 FAIL first=0x%08x second=0x%08x\n",
                first_psx, second_psx);
        return 1;
    }
    printf("W34B37 OT WRITER PASS first=0x%08x second=0x%08x\n",
           first_psx, second_psx);
    return 0;
}
