#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

void func_8009E3C8(void);

int main(void) {
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    *(u32*)PSX_ADDR(0x800C3D60) = 0x80012345;
    *(u8*)PSX_ADDR(0x800C3E50) = 2;

    func_8009E3C8();

    assert(*(u8*)PSX_ADDR(0x80012345) == 4);
    assert(*(u8*)PSX_ADDR(0x800CCE4A + 2 * 0x170) == 3);
    puts("func_8009E3C8 guest-pointer adaptation PASS");
    return 0;
}
