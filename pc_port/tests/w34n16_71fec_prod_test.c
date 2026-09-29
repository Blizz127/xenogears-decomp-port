#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_71fec.h"

#define REQUEST 0x8009D3F8u
#define MIRROR 0x8009D528u

void* D_8005945C;

static u32 decoded[4];
static u32 decode_count;
static u32 alloc_sizes[4];
static u32 alloc_flags[4];
static u32 alloc_count;
static void* queue_arg;
static int queue_arg1;
static int queue_arg2;
static u8 queue_snapshot[24];
static int failures;

static void* const first_host = (void*)(g_PsxRam + 0x12000u);
static void* const second_host = (void*)(g_PsxRam + 0x18000u);

int ArchiveDecodeAlignedSize(unsigned int entry_index)
{
    decoded[decode_count++] = entry_index;
    return entry_index == 0x26u ? 0x2600 : 0x2500;
}

void* HeapAlloc(u_int size, u_int flags)
{
    void* result = alloc_count == 0u ? first_host : second_host;
    alloc_sizes[alloc_count] = size;
    alloc_flags[alloc_count] = flags;
    alloc_count++;
    return result;
}

int func_80029AFC(void* entries, int arg1, int arg2)
{
    queue_arg = entries;
    queue_arg1 = arg1;
    queue_arg2 = arg2;
    memcpy(queue_snapshot, PSX_ADDR(REQUEST), sizeof(queue_snapshot));
    return 7;
}

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        failures++;
    }
}

static u16 read16(const u8* bytes, u32 offset)
{
    u16 value;
    memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

static u32 read32(const u8* bytes, u32 offset)
{
    u32 value;
    memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

int main(void)
{
    const u32 first_guest = 0x80012000u;
    const u32 second_guest = 0x80018000u;

    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(PSX_ADDR(REQUEST), 0xA5, 24u);
    D_8005945C = NULL;
    wm_80071FEC();

    check(decode_count == 2u && decoded[0] == 0x26u && decoded[1] == 0x25u,
          "decode.order.26.then.25");
    check(alloc_count == 2u && alloc_sizes[0] == 0x2600u &&
              alloc_sizes[1] == 0x2500u && alloc_flags[0] == 1u &&
              alloc_flags[1] == 1u,
          "alloc.sizes.and.flags.match.retail");
    check(D_8005945C == first_host,
          "menu.pointer.remains.native.authority");
    check(read32(PSX_ADDR(MIRROR), 0u) == second_guest &&
              read32(queue_snapshot, 4u) == second_guest,
          "second.pointer.published.as.guest.address");
    check(read16(queue_snapshot, 0u) == 0x25u &&
              read16(queue_snapshot, 8u) == 0x26u &&
              read32(queue_snapshot, 12u) == first_guest,
          "queue.payloads.match.retail.order");
    check(read16(queue_snapshot, 16u) == 0u &&
              read32(queue_snapshot, 20u) == 0u,
          "queue.has.zero.terminator");
    check(read16(queue_snapshot, 2u) == 0xA5A5u &&
              read16(queue_snapshot, 10u) == 0xA5A5u &&
              read16(queue_snapshot, 18u) == 0xA5A5u,
          "queue.padding.is.preserved");
    check(queue_arg == PSX_ADDR(REQUEST) && queue_arg1 == 0 && queue_arg2 == 0,
          "queue.submitted.from.D3F8.with.zero.args");

    if (failures != 0)
        return 1;
    puts("W34N16 71FEC PRODUCTION CERTIFICATE PASS");
    return 0;
}
