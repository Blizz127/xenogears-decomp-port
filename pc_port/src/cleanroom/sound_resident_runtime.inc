/* Host ABI bindings for the hybrid resident sound path. Code fetches use the
 * immutable, SHA-verified image supplied by the user through the retail loader. */
#include "battle_mips_adapter.h"
#include "retail_data.h"
#include <stdatomic.h>

typedef struct {
    uint32_t entry;
    uint16_t code_size;
    uint16_t span[3];
    unsigned pointer_mask;
    unsigned write_mask;
    unsigned cursor_return;
    unsigned seed_flags;
} SoundResidentEntry;

enum {
    SOUND_ARGUMENT_BASE = 0x04000000,
    SOUND_ARGUMENT_STRIDE = 0x10000,
    SOUND_STACK_BASE = 0x05000000,
    SOUND_RETURN_PC = 0x50000000,
    SOUND_STACK_SIZE = 256,
    SOUND_PENDING_BYTES = 128,
    SOUND_RESIDENT_SEED = 1,
    SOUND_RESIDENT_RANDOM_CALL = 2
};

#define SOUND_SEED_ADDRESS UINT32_C(0x800594e4)
#define SOUND_RANDOM_ENTRY UINT32_C(0x8003f43c)
#define SOUND_RANDOM_SIZE 44u

typedef struct {
    unsigned char *address;
    unsigned char value;
} SoundResidentChange;

typedef struct {
    const SoundResidentEntry *entry;
    const XenoRetailFileSlot *image;
    const uintptr_t *arguments;
    unsigned char stack[SOUND_STACK_SIZE];
    SoundResidentChange changes[SOUND_PENDING_BYTES];
    unsigned change_count;
} SoundResidentCall;

static atomic_uint_least64_t sound_resident_calls;
static atomic_uint_least64_t sound_resident_instructions;
static atomic_uint_least64_t sound_resident_refusals;
static atomic_flag sound_resident_seed_lock = ATOMIC_FLAG_INIT;

uint64_t PcPort_SoundResidentFallbackCalls(void)
{
    return atomic_load_explicit(&sound_resident_calls, memory_order_relaxed);
}

uint64_t PcPort_SoundResidentFallbackInstructions(void)
{
    return atomic_load_explicit(&sound_resident_instructions, memory_order_relaxed);
}

uint64_t PcPort_SoundResidentFallbackRefusals(void)
{
    return atomic_load_explicit(&sound_resident_refusals, memory_order_relaxed);
}

static int sound_resident_contains(uint32_t base, uint32_t length,
                                   uint32_t address, unsigned width)
{
    return address >= base && width <= length && address - base <= length - width;
}

static int sound_resident_code(const SoundResidentCall *call, uint32_t address)
{
    return (address & 3u) == 0u &&
        (sound_resident_contains(call->entry->entry, call->entry->code_size, address, 4) ||
         ((call->entry->seed_flags & SOUND_RESIDENT_RANDOM_CALL) &&
          sound_resident_contains(SOUND_RANDOM_ENTRY, SOUND_RANDOM_SIZE, address, 4)));
}

static int sound_resident_pc(void *opaque, PcPortMipsCpu *cpu, uint32_t address)
{
    (void)cpu;
    return sound_resident_code(opaque, address) ? 0 : -1;
}

static unsigned char *sound_resident_pointer(SoundResidentCall *call,
                                             uint32_t address, unsigned width, int writing)
{
    if (width != 1 && width != 2 && width != 4)
        return NULL;
    if (sound_resident_contains(SOUND_STACK_BASE, sizeof call->stack, address, width))
        return call->stack + (address - SOUND_STACK_BASE);
    if ((call->entry->seed_flags & SOUND_RESIDENT_SEED) && address == SOUND_SEED_ADDRESS && width == 4)
        return (unsigned char *)&D_800594E4;
    if (address < SOUND_ARGUMENT_BASE)
        return NULL;
    uint32_t delta = address - SOUND_ARGUMENT_BASE;
    unsigned arg = delta / SOUND_ARGUMENT_STRIDE;
    unsigned offset = delta % SOUND_ARGUMENT_STRIDE;
    if (arg >= 3 || !call->arguments[arg] || !(call->entry->pointer_mask & (1u << arg)) ||
        (writing && !(call->entry->write_mask & (1u << arg))) ||
        !sound_resident_contains(0, call->entry->span[arg], offset, width) ||
        offset + width - 1 > UINTPTR_MAX - call->arguments[arg])
        return NULL;
    return (unsigned char *)(call->arguments[arg] + offset);
}

static int sound_resident_read(void *opaque, uint32_t address, unsigned width, uint32_t *out)
{
    SoundResidentCall *call = opaque;
    uint32_t value = 0;
    if (width == 4 && sound_resident_code(call, address)) {
        size_t offset = XENO_RD_SLUS_OFF(address);
        if (!call->image || offset > call->image->size || width > call->image->size - offset)
            return -1;
        for (unsigned i = 0; i < width; ++i)
            value |= (uint32_t)call->image->bytes[offset + i] << (8u * i);
    } else {
        unsigned char *p = sound_resident_pointer(call, address, width, 0);
        if (!p)
            return -1;
        for (unsigned i = 0; i < width; ++i) {
            unsigned index = 0;
            while (index < call->change_count && call->changes[index].address != p + i)
                ++index;
            unsigned byte = index == call->change_count ? p[i] : call->changes[index].value;
            value |= (uint32_t)byte << (8u * i);
        }
    }
    *out = value;
    return 0;
}

static int sound_resident_write(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    SoundResidentCall *call = opaque;
    unsigned char *p = sound_resident_pointer(call, address, width, 1);
    if (!p)
        return -1;
    for (unsigned i = 0; i < width; ++i) {
        unsigned index = 0;
        while (index < call->change_count && call->changes[index].address != p + i)
            ++index;
        if (index == call->change_count) {
            if (index == SOUND_PENDING_BYTES)
                return -1;
            call->changes[index].address = p + i;
            ++call->change_count;
        }
        call->changes[index].value = (unsigned char)(value >> (8u * i));
    }
    return 0;
}

static int sound_resident_opted_in(void)
{
    const char *value = getenv("XENO_SOUND_RESIDENT_HYBRID");
    return value && strcmp(value, "1") == 0;
}

int PcPort_SoundResidentHybridEnabled(void)
{
    return !PcPortMipsInterpreterDisabled() && sound_resident_opted_in();
}

static uintptr_t sound_resident_run(const SoundResidentEntry *entry, const uintptr_t args[3])
{
    SoundResidentCall call = { .entry = entry, .arguments = args };
    PcPortMipsBus bus = { .opaque = &call, .read = sound_resident_read,
                          .write = sound_resident_write, .bridge = sound_resident_pc };
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    for (unsigned n = 0; n < 3; ++n) {
        if (entry->pointer_mask & (1u << n))
            cpu.gpr[4 + n] = args[n] ? SOUND_ARGUMENT_BASE + n * SOUND_ARGUMENT_STRIDE : 0;
        else
            cpu.gpr[4 + n] = (uint32_t)args[n];
    }
    cpu.gpr[29] = SOUND_STACK_BASE + SOUND_STACK_SIZE;
    cpu.gpr[31] = SOUND_RETURN_PC;
    int locked = 0;
    if (!PcPortMipsInterpreterDisabled()) {
        if (!sound_resident_opted_in()) {
            fprintf(stderr, "[xeno-port] unadmitted resident sound entry 0x%08x, hybrid sequence execution disabled\n",
                    (unsigned)entry->entry);
            atomic_fetch_add_explicit(&sound_resident_refusals, 1, memory_order_relaxed);
            abort();
        }
        call.image = XenoRetailData_File(XENO_RD_SLUS);
        if (!call.image || call.image->state != 1 || !call.image->bytes) {
            fprintf(stderr, "[xeno-port] verified resident sound image unavailable for 0x%08x\n",
                    (unsigned)entry->entry);
            atomic_fetch_add_explicit(&sound_resident_refusals, 1, memory_order_relaxed);
            abort();
        }
        if (entry->seed_flags & SOUND_RESIDENT_SEED) {
            while (atomic_flag_test_and_set_explicit(&sound_resident_seed_lock, memory_order_acquire)) {
            }
            locked = 1;
        }
    }
    int rc = PcPortMipsRun(&cpu, entry->entry, SOUND_RETURN_PC, 512);
    if (cpu.steps) {
        if (!atomic_fetch_add_explicit(&sound_resident_calls, 1, memory_order_relaxed))
            fprintf(stderr, "[xeno-port] resident sound is using the CPU interpreter\n");
        atomic_fetch_add_explicit(&sound_resident_instructions, cpu.steps, memory_order_relaxed);
    }
    uintptr_t result = cpu.gpr[2];
    if (rc == PC_PORT_MIPS_HALTED && entry->cursor_return && result) {
        if (!args[0] || result < SOUND_ARGUMENT_BASE ||
            result - SOUND_ARGUMENT_BASE > entry->cursor_return ||
            result - SOUND_ARGUMENT_BASE > UINTPTR_MAX - args[0])
            rc = PC_PORT_MIPS_FAULT;
        else
            result = args[0] + (result - SOUND_ARGUMENT_BASE);
    }
    if (rc == PC_PORT_MIPS_HALTED) {
        for (unsigned i = 0; i < call.change_count; ++i)
            *call.changes[i].address = call.changes[i].value;
    }
    if (locked)
        atomic_flag_clear_explicit(&sound_resident_seed_lock, memory_order_release);
    if (rc != PC_PORT_MIPS_HALTED) {
        atomic_fetch_add_explicit(&sound_resident_refusals, 1, memory_order_relaxed);
        fprintf(stderr, "[xeno-port] resident sound refused 0x%08x (%d, %s), executed %llu instructions\n",
                (unsigned)entry->entry, rc, cpu.error, (unsigned long long)cpu.steps);
        abort();
    }
    return result;
}

#include "sound_resident_entries.inc"

#include "sound_resident_callbacks.inc"

/* Called only for an operation that actually requires a resident callback.
 * Native sound initialization, WDS loading and transfer do not use this query. */
int PcPort_SoundResidentRuntimeReady(void)
{
    if (PcPortMipsInterpreterDisabled()) {
        fprintf(stderr, "[xeno-port] resident sequence callback requires CPU fallback; native-only sequence declined\n");
        atomic_fetch_add_explicit(&sound_resident_refusals, 1, memory_order_relaxed);
        return 0;
    }
    if (!sound_resident_opted_in()) {
        fprintf(stderr, "[xeno-port] resident sequence callback disabled; set XENO_SOUND_RESIDENT_HYBRID=1 to opt in\n");
        atomic_fetch_add_explicit(&sound_resident_refusals, 1, memory_order_relaxed);
        return 0;
    }
    const XenoRetailFileSlot *slot = XenoRetailData_File(XENO_RD_SLUS);
    return slot && slot->state == 1 && slot->bytes;
}
