/* Isolated fixture memory for the channel-view and heap/list checks. The SDL
 * event reservation functions are supplied by the prepared renderer. */
#include <stdlib.h>
#include <string.h>

#define BATTLE_BUS_KUSEG_RAM_END 0x00200000u
static _Alignas(16) uint8_t g_PsxRam[BATTLE_BUS_KUSEG_RAM_END];

typedef struct {
    uint32_t address;
    size_t size;
    const char *name;
    uint8_t *host;
} ResolvedData;

typedef struct {
    ResolvedData channels;
} BattleMipsRuntime;

static ResolvedData *find_data(BattleMipsRuntime *runtime, uint32_t address,
                               size_t width)
{
    ResolvedData *channels = &runtime->channels;
    if (address != channels->address || width > channels->size)
        return NULL;
    return channels;
}

static int host_address_valid(BattleMipsRuntime *runtime, uintptr_t address,
                              size_t width, int write)
{
    uintptr_t base = (uintptr_t)g_PsxRam;
    (void)runtime;
    (void)write;
    return address >= base && address - base < sizeof(g_PsxRam) &&
           width <= sizeof(g_PsxRam) - (address - base);
}

static uint32_t load_le(const uint8_t *data, unsigned width)
{
    uint32_t value = 0;
    for (unsigned i = 0; i < width; i++)
        value |= (uint32_t)data[i] << (8u * i);
    return value;
}

static void store_le(uint8_t *data, unsigned width, uint32_t value)
{
    for (unsigned i = 0; i < width; i++)
        data[i] = (uint8_t)(value >> (8u * i));
}

#include "slot-projection.inc"
#include "sound-heap-membership.inc"

static void test_channel_projection(BattleMipsRuntime *runtime, void **channels)
{
    const uint32_t first = runtime->channels.address;
    uint32_t word = 0;
    const uint32_t offset = 0x3456u;

    channels[0] = g_PsxRam + offset;
    assert(unsetenv("XENO_SCRATCH_SOUND_SLOT_GUEST_VIEW") == 0);
    assert(scratch_sound_project_read(runtime, first, 4, &word) == 1);
    assert(word == (uintptr_t)channels[0]);

    assert(setenv("XENO_SCRATCH_SOUND_SLOT_GUEST_VIEW", "1", 1) == 0);
    assert(scratch_sound_project_read(runtime, first, 4, &word) == 1);
    assert(word == 0x80000000u + offset);
    assert(scratch_sound_project_read(runtime, first & 0x1fffffu, 4, &word) == 1);
    assert(word == 0x80000000u + offset);
    assert(scratch_sound_project_read(runtime, first + 0x20000000u, 4, &word) == 1);
    assert(word == 0x80000000u + offset);
    assert(scratch_sound_project_read(runtime, first + 1, 1, &word) == 1);
    assert(word == ((offset >> 8u) & 0xffu));
    assert(scratch_sound_project_read(runtime, first + 3, 2, &word) == -1);
    assert(scratch_sound_project_read(runtime, first + 24u * 4u, 4, &word) == 0);

    assert(setenv("XENO_SCRATCH_SOUND_SLOT_GUEST_VIEW", "2", 1) == 0);
    assert(scratch_sound_project_read(runtime, first, 4, &word) == 1);
    assert(word == 0xa0000000u + offset);
    assert(scratch_sound_project_write(runtime, first + 4u, 4, word) == 1);
    assert(channels[1] == channels[0]);
    assert(scratch_sound_project_write(runtime, first + 4u, 4, 0x40123456u) == -1);
    assert(channels[1] == channels[0]);
    assert(scratch_sound_project_write(runtime, first + 4u, 4, 0) == 1);
    assert(channels[1] == NULL);
    channels[1] = (void *)(uintptr_t)(UINT64_C(1) << 32);
    assert(scratch_sound_project_read(runtime, first + 4u, 4, &word) == -1);
    channels[1] = NULL;
    assert(unsetenv("XENO_SCRATCH_SOUND_SLOT_GUEST_VIEW") == 0);
}

static void test_heap_membership(BattleMipsRuntime *runtime, unsigned event)
{
    uint8_t *heap = g_PsxRam;
    const uint32_t base = (uint32_t)(uintptr_t)heap;
    const uint32_t manager = base + 0x110u;
    const uint32_t second = base + 0x510u;
    const uint32_t extent = 0x94u + 2u * 0x158u;
    SyntheticSoundHeap view;

    store_le(heap, 2, 0x8000u);
    store_le(heap + 8, 4, base + 16u);
    store_le(heap + 12, 4, base + 0x100u);
    store_le(heap + 0x100u, 2, 2);
    store_le(heap + 0x108u, 4, manager + extent);
    heap[0x124u] = 2;

    assert(prepare_synthetic_sound_heap(runtime, base, 0x6300u, manager, 2, event, &view));
    assert(!capture_synthetic_sound_heap(&view));
    assert(PsyX_Sys_ReserveSoundEvent(event));
    assert(capture_synthetic_sound_heap(&view));
    assert(capture_synthetic_manager_list(&view, manager));

    store_le(heap + 0x10cu, 4, base + 0x500u);
    store_le(heap + 0x500u, 2, 2);
    store_le(heap + 0x508u, 4, second + extent);
    heap[0x524u] = 2;
    store_le(heap + 0x110u, 4, second);
    assert(capture_synthetic_manager_list(&view, manager));
    store_le(heap + 0x510u, 4, manager);
    assert(!capture_synthetic_manager_list(&view, manager));
    store_le(heap + 0x510u, 4, 0);
    store_le(heap + 0x110u, 4, 0);
    store_le(heap + 0x10cu, 4, 0);
    assert(!capture_synthetic_manager_list(&view, 0));
    heap[0x124u] = 3;
    assert(!capture_synthetic_sound_heap(&view));
    heap[0x124u] = 2;
    store_le(heap + 0x108u, 4, manager + 0x94u + 0x158u);
    assert(!capture_synthetic_sound_heap(&view));
    store_le(heap + 0x108u, 4, manager + extent);
    store_le(heap + 0x10cu, 4, base + 0x100u);
    assert(!capture_synthetic_sound_heap(&view));
    assert(PsyX_Sys_ReleaseSoundEventReservation());
}

static void test_fixture_domains(unsigned event)
{
    void *channels[24] = {0};
    BattleMipsRuntime runtime = {
        .channels = {0x8006252cu, sizeof(channels), "g_SoundChannels", (uint8_t *)channels}
    };
    assert(sizeof(void *) == 8);
    assert((uintptr_t)(g_PsxRam + sizeof(g_PsxRam) - 1) <= UINT32_MAX);
    test_channel_projection(&runtime, channels);
    test_heap_membership(&runtime, event);
    puts("PASS synthetic channel projection and reserved heap/list membership");
}
