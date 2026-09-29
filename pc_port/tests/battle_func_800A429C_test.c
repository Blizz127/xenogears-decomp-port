#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psyq/libgpu.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static void* s_freed[3];
static unsigned s_free_count;
static RECT* s_loaded_rect;
static u_long* s_loaded_data;
static unsigned s_load_count;

u_int HeapFree(void* p) {
    assert(s_free_count < 3);
    s_freed[s_free_count++] = p;
    return 0;
}

int LoadImage(RECT* rect, u_long* data) {
    s_loaded_rect = rect;
    s_loaded_data = data;
    ++s_load_count;
    return 0;
}

void func_800A429C(void* arg0);

int main(void) {
    u8 record[0x30] = {0};

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    *(u16*)(record + 0x1A) = 1;
    record[0x10] = 3;
    *(u32*)(record + 4) = 0x80010000;
    *(u32*)(record + 8) = 0x80011000;
    *(u32*)(record + 0xC) = 0x80012000;
    func_800A429C(record);

    assert(s_load_count == 1);
    assert(s_loaded_rect == (RECT*)(record + 0x28));
    assert(s_loaded_data == (u_long*)PSX_ADDR(0x80010000));
    assert(s_free_count == 3);
    assert(s_freed[0] == PSX_ADDR(0x80010000));
    assert(s_freed[1] == PSX_ADDR(0x80011000));
    assert(s_freed[2] == PSX_ADDR(0x80012000));
    assert(*(u32*)(record + 4) == 0);
    assert(*(u32*)(record + 8) == 0);
    assert(*(u32*)(record + 0xC) == 0);
    assert(*(u16*)(record + 0x1A) == 0);

    puts("func_800A429C host pointer/free and active-flag behavior PASS");
    return 0;
}
