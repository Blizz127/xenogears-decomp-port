/* Retail 8001CE8D8..8001CE91C byte lookup versus the actual native owner. */
#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "battle_mips_adapter.h"

#define ENTRY 0x801CE8D8u
#define END 0x801CE91Cu
#define RETAIL_SIZE (END - ENTRY)
#define RAM_SIZE 0x200000u
#define KEY_ADDR 0x80060000u
#define VALUE_ADDR 0x80064000u
#define STACK_ADDR 0x801FF000u
#define HALT 0xFFFFFFFCu
#define LARGE_COUNT 0x1100

extern u8 func_801CE8D8(u8 *pKeys, u8 *pValues, s32 count, s32 target);

static unsigned char ram[RAM_SIZE];
static unsigned char retail[RETAIL_SIZE];
static unsigned seen[RETAIL_SIZE / 4];
static unsigned data_reads;
static unsigned cases;
static unsigned checks;
static PcPortMipsCpu *active_cpu;

static void require(int ok, const char *why) {
    ++checks;
    if (!ok) {
        fprintf(stderr, "SHOP BYTE LOOKUP FAIL case=%u %s\n", cases, why);
        exit(1);
    }
}

static unsigned char *ram_ptr(uint32_t address) {
    return ram + (address & (RAM_SIZE - 1));
}

static int bus_read(void *opaque, uint32_t address, unsigned width, uint32_t *value) {
    (void)opaque;
    if (address < 0x80000000u || (uint64_t)address + width > 0x80200000u) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i) *value |= (uint32_t)ram_ptr(address)[i] << (8 * i);
    if (active_cpu && width == 4 && address == active_cpu->pc && address >= ENTRY && address < END)
        ++seen[(address - ENTRY) / 4];
    if (width == 1 && ((address >= KEY_ADDR && address < KEY_ADDR + LARGE_COUNT) ||
                       (address >= VALUE_ADDR && address < VALUE_ADDR + LARGE_COUNT)))
        ++data_reads;
    return 0;
}

static int bus_write(void *opaque, uint32_t address, unsigned width, uint32_t value) {
    (void)opaque;
    if (address < 0x80000000u || (uint64_t)address + width > 0x80200000u) return -1;
    for (unsigned i = 0; i < width; ++i) ram_ptr(address)[i] = (unsigned char)(value >> (8 * i));
    return 0;
}

static int bus_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target) {
    (void)opaque; (void)cpu; (void)target; return 0;
}

static u8 run_retail(s32 count, s32 target, unsigned present, unsigned position, unsigned *reads) {
    PcPortMipsBus bus = {0};
    PcPortMipsCpu cpu;
    memset(ram, 0xA5, sizeof(ram));
    memcpy(ram_ptr(ENTRY), retail, RETAIL_SIZE);
    for (unsigned i = 0; i < LARGE_COUNT; ++i) {
        ram_ptr(KEY_ADDR)[i] = 0x11;
        ram_ptr(VALUE_ADDR)[i] = (unsigned char)(0x80u + (i * 13u) % 0x70u);
    }
    if (present) ram_ptr(KEY_ADDR)[position] = (unsigned char)target;
    data_reads = 0;
    bus.read = bus_read; bus.write = bus_write; bus.bridge = bus_bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = KEY_ADDR; cpu.gpr[5] = VALUE_ADDR; cpu.gpr[6] = (uint32_t)count;
    cpu.gpr[7] = (uint32_t)target; cpu.gpr[29] = STACK_ADDR; cpu.gpr[31] = HALT;
    active_cpu = &cpu;
    int rc = PcPortMipsRun(&cpu, ENTRY, HALT, 100000);
    active_cpu = NULL;
    require(rc == PC_PORT_MIPS_HALTED, "retail execution");
    require(cpu.gpr[29] == STACK_ADDR, "retail stack");
    if (reads) *reads = data_reads;
    return (u8)cpu.gpr[2];
}

typedef struct { void *mapping; size_t length; u8 *keys; u8 *values; uintptr_t base; } NativeArrays;

static NativeArrays map_native_arrays(void) {
    static const uintptr_t candidates[] = {
        0x000000017FFFF000ULL, 0x000000027FFFF000ULL,
        0x000000037FFFF000ULL, 0x000000047FFFF000ULL,
        0x000000057FFFF000ULL,
    };
    const size_t length = 0x5000;
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        void *mapping = mmap((void *)candidates[i], length, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
        if (mapping == MAP_FAILED) continue;
        uintptr_t base = (uintptr_t)mapping;
        if ((uint32_t)base != 0x7FFFF000u) { munmap(mapping, length); continue; }
        NativeArrays result = {mapping, length, (u8 *)(base + 0x2100),
                               (u8 *)(base + 0x100), base};
        require((uint32_t)(uintptr_t)result.values < 0x80000000u, "native values start boundary");
        require((uint32_t)((uintptr_t)result.values + LARGE_COUNT) >= 0x80000000u,
                "native values cross boundary");
        return result;
    }
    fprintf(stderr, "SHOP BYTE LOOKUP FAIL cannot reserve safe high mapping: %s\n", strerror(errno));
    exit(1);
}

static void unmap_native_arrays(NativeArrays *arrays) {
    require(munmap(arrays->mapping, arrays->length) == 0, "native unmap");
}

static u8 run_native(NativeArrays *arrays, s32 count, s32 target) {
    return func_801CE8D8(arrays->keys, arrays->values, count, target);
}

static void assert_native_no_read(s32 count) {
    long page_size = sysconf(_SC_PAGESIZE);
    require(page_size > 0, "page size");
    void *bad = mmap(NULL, (size_t)page_size, PROT_NONE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    require(bad != MAP_FAILED, "native no-read mapping");
    pid_t child = fork();
    require(child >= 0, "native no-read fork");
    if (child == 0) { (void)func_801CE8D8((u8 *)bad, (u8 *)bad, count, 0x5A); _exit(0); }
    int status = 0;
    require(waitpid(child, &status, 0) == child, "native no-read wait");
    require(WIFEXITED(status) && WEXITSTATUS(status) == 0, "nonpositive count touched array");
    require(munmap(bad, (size_t)page_size) == 0, "native no-read unmap");
}

static void run_positive_case(NativeArrays *arrays, s32 count, unsigned position,
                              unsigned present, s32 target) {
    memset(arrays->keys, 0x11, (size_t)count);
    for (s32 i = 0; i < count; ++i)
        arrays->values[i] = (unsigned char)(0x80u + ((unsigned)i * 13u) % 0x70u);
    if (present) arrays->keys[position] = (unsigned char)target;
    u8 expected = present ? arrays->values[position] : 0;
    unsigned raw_reads = 0;
    u8 raw = run_retail(count, target, present, position, &raw_reads);
    u8 native = run_native(arrays, count, target);
    require(raw == expected && native == expected, "lookup result");
    if (present) require(raw_reads == position + 2u, "retail match reads");
    else require(raw_reads == (unsigned)count, "retail absent reads");
    ++cases;
}

int main(int argc, char **argv) {
    require(argc == 2, "retail shop module argument");
    FILE *file = fopen(argv[1], "rb");
    require(file != NULL, "open retail shop module");
    require(fseek(file, 0x98D8, SEEK_SET) == 0 &&
                fread(retail, 1, RETAIL_SIZE, file) == RETAIL_SIZE, "read retail function");
    fclose(file);

    NativeArrays arrays = map_native_arrays();
    assert_native_no_read(-1); assert_native_no_read(0);
    unsigned raw_reads = 123;
    require(run_retail(-1, 0x5A, 0, 0, &raw_reads) == 0 && raw_reads == 0, "retail negative reads");
    require(run_retail(0, 0x5A, 0, 0, &raw_reads) == 0 && raw_reads == 0, "retail zero reads");
    ++cases;

    const s32 counts[] = {1, 3, 17, LARGE_COUNT};
    const s32 high_bits[] = {0, 0x100, (s32)0x80000000u, (s32)0xFFFF0000u};
    for (unsigned c = 0; c < sizeof(counts) / sizeof(counts[0]); ++c) {
        s32 count = counts[c];
        unsigned positions[] = {0, (unsigned)count / 2u, (unsigned)count - 1u};
        for (unsigned p = 0; p < 3; ++p)
            for (unsigned h = 0; h < sizeof(high_bits) / sizeof(high_bits[0]); ++h)
                run_positive_case(&arrays, count, positions[p], 1,
                                  (s32)(0x5Au | high_bits[h]));
        for (unsigned h = 0; h < sizeof(high_bits) / sizeof(high_bits[0]); ++h)
            run_positive_case(&arrays, count, 0, 0, (s32)(0xA6u | high_bits[h]));
    }
    unmap_native_arrays(&arrays);
    for (unsigned i = 0; i < RETAIL_SIZE / 4; ++i) require(seen[i] > 0, "retail instruction coverage");
    printf("SHOP BYTE LOOKUP PASS cases=%u checks=%u instructions=%u\n", cases, checks, RETAIL_SIZE / 4);
    return 0;
}
