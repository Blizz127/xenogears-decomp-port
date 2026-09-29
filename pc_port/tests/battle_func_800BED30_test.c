#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

void func_800BED30(void);

int main(void) {
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    *(u32*)PSX_ADDR(0x800C3E20) = 0x11223344;
    *(u32*)PSX_ADDR(0x800C3610) = 0x80012340;
    *(u16*)PSX_ADDR(0x800D2E54) = 0x5566;

    func_800BED30();

    assert(*(u32*)PSX_ADDR(0x800C3E20) == 0);
    assert(*(u32*)PSX_ADDR(0x800C3610) == 0);
    assert(*(u16*)PSX_ADDR(0x800D2E54) == 0);
    puts("func_800BED30 guest-RAM pointer reset PASS");
    return 0;
}
