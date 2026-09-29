#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_86124.h"

#define D7E8 0x8009D7E8u
#define D7EC 0x8009D7ECu
#define D7F8 0x8009D7F8u
#define D7FC 0x8009D7FCu
#define BE1C 0x8009BE1Cu
#define BE20 0x8009BE20u

static void* frees[8];
static unsigned int free_count;
static int failures;

unsigned int HeapFree(void* ptr)
{
    frees[free_count++] = ptr;
    return 0u;
}

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        failures++;
    }
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void reset_trace(void)
{
    memset(frees, 0, sizeof(frees));
    free_count = 0u;
}

static void test_86124(void)
{
    write32(D7EC, 0x80012000u);
    write32(D7E8, 0x80013000u);
    reset_trace();
    wm_80086124();
    check(free_count == 2u && frees[0] == PSX_ADDR(0x80012000u) &&
              frees[1] == PSX_ADDR(0x80013000u),
          "86124.frees.D7EC.then.D7E8");
}

static void test_866c8(void)
{
    write32(D7FC, 0x80014000u);
    write32(D7F8, 0x80015000u);
    reset_trace();
    wm_800866C8();
    check(free_count == 2u && frees[0] == PSX_ADDR(0x80014000u) &&
              frees[1] == PSX_ADDR(0x80015000u),
          "866C8.frees.D7FC.then.D7F8");
}

static void test_89128(void)
{
    write32(BE1C, 0x00123450u);
    write32(BE20, 0x00124560u);
    reset_trace();
    wm_80089128();
    check(free_count == 2u && frees[0] == (void*)(uintptr_t)0x00123450u &&
              frees[1] == (void*)(uintptr_t)0x00124560u,
          "89128.frees.BE1C.then.BE20");
}

static void test_null_is_forwarded(void)
{
    write32(D7EC, 0u);
    write32(D7E8, 0u);
    reset_trace();
    wm_80086124();
    check(free_count == 2u && frees[0] == NULL && frees[1] == NULL,
          "retail.unconditionally.forwards.null.values");
}

int main(void)
{
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    test_86124();
    test_866c8();
    test_89128();
    test_null_is_forwarded();
    if (failures != 0)
        return 1;
    puts("W34N15 PAIRED-FREE PRODUCTION CERTIFICATE PASS");
    return 0;
}
