/* Exercise the guest call delay slot from the supplied world-map overlay. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "battle_mips_adapter.h"
#include "psx_memory.h"

#define SLICE_START UINT32_C(0x80090B50)
#define SLICE_END UINT32_C(0x80090B88)
#define OVERLAY_BASE UINT32_C(0x8006FAF0)
#define SLOT_ADDR UINT32_C(0x800D7000)
#define FIRST_RESULT (SLOT_ADDR + 0x38u)
#define SECOND_RESULT (SLOT_ADDR + 0x40u)

uint8_t g_PsxRam[PSX_RAM_SIZE];

typedef struct {
    uint32_t sine;
    uint32_t cosine;
    unsigned calls;
    int drop_store;
    int observed_store;
} TrigProbe;

static uint32_t read_word(uint32_t address)
{
    uint32_t value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static int in_ram(uint32_t address, unsigned width)
{
    return width > 0u && width <= sizeof(uint32_t) &&
           address >= 0x80000000u && address <= 0x80200000u - width;
}

static int guest_read(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    (void)opaque;
    if (!in_ram(address, width)) return -1;
    *value = 0;
    memcpy(value, PSX_ADDR(address), width);
    return 0;
}

static int guest_write(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    TrigProbe *probe = opaque;
    if (!in_ram(address, width)) return -1;
    if (!(probe->drop_store && address == FIRST_RESULT))
        memcpy(PSX_ADDR(address), &value, width);
    return 0;
}

/* Distinct synthetic return values make the ordering visible, independently
 * of the math library. The trigonometric function bodies aren't under test. */
static int observe_trig_call(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    TrigProbe *probe = opaque;
    if (target == 0x8003F8B0u && probe->calls == 0u) {
        cpu->gpr[2] = probe->sine;
    } else if (target == 0x8003F8CCu && probe->calls == 1u) {
        if (read_word(FIRST_RESULT) != probe->sine) {
            fputs("W34CB1 FAIL: delay-slot store absent before second call\n", stderr);
            return -1;
        }
        probe->observed_store = 1;
        cpu->gpr[2] = probe->cosine;
    } else {
        return 0;
    }
    probe->calls++;
    return 1;
}

static int test_slice(uint32_t sine, uint32_t cosine, int drop_store)
{
    TrigProbe probe = {.sine = sine, .cosine = cosine, .drop_store = drop_store};
    PcPortMipsBus bus = {.opaque = &probe, .read = guest_read,
                         .write = guest_write, .bridge = observe_trig_call};
    PcPortMipsCpu cpu;
    uint16_t heading = 0x1000u;
    memcpy(PSX_ADDR(0x8009CD4Cu), &heading, sizeof(heading));
    memset(PSX_ADDR(SLOT_ADDR), 0xA5, 0x80);
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[16] = SLOT_ADDR;
    int status = PcPortMipsRun(&cpu, SLICE_START, SLICE_END, 100u);
    if (status != PC_PORT_MIPS_HALTED || cpu.pc != SLICE_END ||
        cpu.steps == 0u || probe.calls != 2u || !probe.observed_store ||
        read_word(FIRST_RESULT) != sine || read_word(SECOND_RESULT) != 0u - cosine) {
        fprintf(stderr, "W34CB1 FAIL: guest status %d, PC %08x, calls %u\n",
                status, cpu.pc, probe.calls);
        return 1;
    }
    printf("W34CB1 PASS: halted at %08x after %llu guest instructions and two trig calls\n",
           cpu.pc, (unsigned long long)cpu.steps);
    return 0;
}

int main(int argc, char **argv)
{
    int drop_store = argc == 2 && strcmp(argv[1], "--drop-store") == 0;
    if (argc != 1 && !drop_store) return 2;
    FILE *file = fopen("disc/world_map.bin", "rb");
    if (file == NULL) {
        perror("disc/world_map.bin");
        return 2;
    }
    size_t length = SLICE_END - SLICE_START;
    int loaded = fseek(file, (long)(SLICE_START - OVERLAY_BASE), SEEK_SET) == 0 &&
                 fread(PSX_ADDR(SLICE_START), 1, length, file) == length;
    fclose(file);
    if (!loaded) {
        fputs("W34CB1: could not read guest delay-slot region\n", stderr);
        return 2;
    }
    if (test_slice(0xF2345678u, 0x11223344u, drop_store) != 0) return 1;
    return test_slice(0x00001234u, 0xFFFFFFFEu, drop_store);
}
