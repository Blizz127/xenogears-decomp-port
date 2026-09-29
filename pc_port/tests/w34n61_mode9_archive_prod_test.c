/* Focused production certificate for retail mode-9 relocation 0x80076954. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_76954.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define COMPRESSED 0x800A0000u
#define DECODED    0x800A1000u
#define C180       0x8009C180u
#define CD48       0x8009CD48u
#define BD30       0x8009BD30u
#define D308       0x8009D308u
#define BCC0       0x8009BCC0u
#define C7EC       0x8009C7ECu
#define D77C       0x8009D77Cu
#define D7C8       0x8009D7C8u

static int failures;
static int decompress_calls;
static void* decompress_source;
static int decompress_flags;
static int decompress_returns_null;
static int free_calls;
static void* freed_pointer;

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

void* LZSSHeapDecompress(void* source, int flags)
{
    decompress_calls++;
    decompress_source = source;
    decompress_flags = flags;
    return decompress_returns_null != 0 ? NULL : PSX_ADDR(DECODED);
}

unsigned int HeapFree(void* pointer)
{
    free_calls++;
    freed_pointer = pointer;
    return 0u;
}

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    decompress_calls = 0;
    decompress_source = NULL;
    decompress_flags = -1;
    decompress_returns_null = 0;
    free_calls = 0;
    freed_pointer = NULL;
    write32(C180, COMPRESSED);
    write32(DECODED + 0x08u, 0x0100u);
    write32(DECODED + 0x0Cu, 0x0200u);
    write32(DECODED + 0x10u, 0x0300u);
    write32(DECODED + 0x14u, 0x0400u);
    write32(DECODED + 0x18u, 0x0500u);
    write32(DECODED + 0x20u, 0x0600u);
    write32(DECODED + 0x24u, 0x0700u);
}

static void test_relocation(void)
{
    seed();
    check(wm_80076954() == 0, "relocation.return_zero");
    check(decompress_calls == 1 &&
          decompress_source == PSX_ADDR(COMPRESSED) &&
          decompress_flags == 0,
          "relocation.decompress_args");
    check(read32(C180) == DECODED, "relocation.guest_publication");
    check(free_calls == 1 && freed_pointer == PSX_ADDR(COMPRESSED),
          "relocation.compressed_free");
    check(read32(CD48) == DECODED + 0x0100u &&
          read32(BD30) == DECODED + 0x0300u &&
          read32(D308) == DECODED + 0x0200u &&
          read32(BCC0) == DECODED + 0x0500u &&
          read32(C7EC) == DECODED + 0x0400u &&
          read32(D77C) == DECODED + 0x0600u &&
          read32(D7C8) == DECODED + 0x0700u,
          "relocation.header_offsets");
}

static void test_failure_bounds(void)
{
    seed();
    write32(C180, 0u);
    check(wm_80076954() == -1 && decompress_calls == 0 && free_calls == 0,
          "relocation.null_input");

    seed();
    decompress_returns_null = 1;
    check(wm_80076954() == -1 && read32(C180) == 0u &&
          decompress_calls == 1 && free_calls == 1,
          "relocation.null_output");
}

int main(void)
{
    test_relocation();
    test_failure_bounds();
    if (failures != 0) {
        fprintf(stderr, "W34N61 MODE9 ARCHIVE CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N61 MODE9 ARCHIVE CERTIFICATE PASS");
    return 0;
}
