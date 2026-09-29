#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include "psx_memory.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
extern uint8_t *D_800595D0, *D_800595A8, *D_80059480, *D_800594AC;
extern uint8_t D_8005954C;
uint8_t D_8004F388[768];
/* Keep the legacy generated-data symbol zero: production must use the
 * initialized retail image, not its unrelated native placeholder. */
#define D_8004F388 ((uint8_t *)PSX_ADDR(0x8004f388u))
int16_t D_8006F9BC, D_8006F9C4, D_8006F9CC, D_8006F9D4;
uint8_t *D_8006F9C0, *D_8006F9C8;
uint32_t D_8006F9D0;
int32_t D_8006F9D8;
static jmp_buf stop;
static unsigned calls;
static uint32_t reservation, first_address;
static int full_load, sound_calls;
static uint16_t bank_id = 0x1234;
static uint8_t seds[128], second_buffer[256];
typedef struct { int16_t index; uint16_t pad; void *data; } HostQueueEntry;

void HeapChangeCurrentUser(int user, void *tag) { assert(user == 2 && !tag); }
void ArchiveSetIndex(int index, int disc) { assert(index == 12 && disc == 0); }
void *HeapAlloc(uint32_t size, int flags)
{
    assert(flags == 1);
    if (calls++ == 0) {
        assert(size == 4);
        return PSX_ADDR(first_address);
    }
    if (calls == 2) {
        reservation = size;
        if (!full_load) longjmp(stop, 1);
        return PSX_ADDR(0x801e4000u);
    }
    if (calls == 3) { assert(size == sizeof(seds)); return seds; }
    assert(calls == 4 && size == sizeof(second_buffer));
    return second_buffer;
}
int ArchiveDecodeAlignedSize(int index)
{ assert(index == 2 || index == 3); return index == 2 ? sizeof(seds) : sizeof(second_buffer); }
int func_80029AFC(void *p, int a, int b)
{
    HostQueueEntry *q = p;
    assert(a == 0 && b == 128);
    assert(q[0].index == 2 && q[0].data == seds);
    assert(q[1].index == 3 && q[1].data == second_buffer);
    assert(q[2].index == 4 && q[2].data == PSX_ADDR(0x801e4000u));
    assert(q[3].index == 0 && q[3].data == NULL);
    seds[0x14] = bank_id & 255; seds[0x15] = bank_id >> 8;
    return 0;
}
int ArchiveDataSync(void) { return 0; }
void SoundAddSedsEntry(void *p) { assert(p == seds); }
void func_80039DB8(int value)
{ assert((uint32_t)value == ((uint32_t)bank_id << 16 | (sound_calls == 0 ? 7u : 9u))); ++sound_calls; }
extern void func_8001BBAC(void);

int main(void)
{
    const uint32_t addresses[] = {0x801ffff4u, 0x801fffc0u, 0x801f0000u};
    for (volatile unsigned i = 0; i < sizeof(addresses) / sizeof(addresses[0]); ++i) {
        first_address = addresses[i];
        calls = 0;
        if (setjmp(stop) == 0) func_8001BBAC();
        /* Retail 8001BBE4..8001BBF8: modulo-32-bit addition of 7FE1C000. */
        uint32_t expected = first_address - 0x801e4000u;
        if (reservation != expected) {
            fprintf(stderr, "reservation=%08x expected=%08x guest=%08x\n",
                    reservation, expected, first_address);
            return 1;
        }
    }
    puts("BATTLE HEAP RESERVATION PASS cases=3");
    full_load = 1;
    calls = 0;
    D_8005954C = 1;
    D_8004F388[3] = 7; D_8004F388[4] = 255; D_8004F388[5] = 9;
    func_8001BBAC();
    assert(sound_calls == 2);
    bank_id = 0xffff;
    calls = sound_calls = 0;
    func_8001BBAC();
    assert(sound_calls == 2);
    D_8005954C = 4;
    calls = sound_calls = 0;
    func_8001BBAC();
    assert(sound_calls == 0);
    D_8005954C = 1;
    D_8004F388[3] = D_8004F388[5] = 255;
    calls = 0;
    func_8001BBAC();
    assert(sound_calls == 0);
    puts("BATTLE AUDIO LOAD QUEUE AND BANK ID PASS");
    return 0;
}
