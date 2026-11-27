/* Execute the HUD value routine from the user's battle overlay. These expected
 * states were observed on the owned code in the pre-retirement differential. */
#include <assert.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "battle_mips_adapter.h"
#include "psx_memory.h"

enum { ENTRY = 0x80072f38u, CODE_OFFSET = 0x72f38, CODE_BYTES = 0x448 };
static uint8_t memory[PSX_RAM_SIZE];
static uint32_t fault_address;

static int read_guest(void *context, uint32_t address, unsigned width, uint32_t *value)
{
    (void)context;
    const unsigned offset = address & 0x1fffffu;
    if (address == fault_address || width > 4 || offset + width > sizeof(memory))
        return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i)
        *value |= (uint32_t)memory[offset + i] << (8 * i);
    return 0;
}

static int write_guest(void *context, uint32_t address, unsigned width, uint32_t value)
{
    (void)context;
    const unsigned offset = address & 0x1fffffu;
    if (address == fault_address || width > 4 || offset + width > sizeof(memory))
        return -1;
    for (unsigned i = 0; i < width; ++i)
        memory[offset + i] = (uint8_t)(value >> (8 * i));
    return 0;
}

static void state_digest(const PcPortMipsCpu *cpu, char result[65])
{
    uint8_t registers[34 * 4], digest[32];
    for (unsigned i = 0; i < 34; ++i) {
        uint32_t word = i < 32 ? cpu->gpr[i] : i == 32 ? cpu->hi : cpu->lo;
        for (unsigned byte = 0; byte < 4; ++byte)
            registers[4 * i + byte] = (uint8_t)(word >> (8 * byte));
    }
    /* Omit the unmodified program input from the data-memory fingerprint. */
    EVP_MD_CTX *hash = EVP_MD_CTX_new();
    assert(hash && EVP_DigestInit_ex(hash, EVP_sha256(), NULL));
    assert(EVP_DigestUpdate(hash, memory, CODE_OFFSET));
    const size_t after_code = CODE_OFFSET + CODE_BYTES;
    assert(EVP_DigestUpdate(hash, memory + after_code, sizeof(memory) - after_code));
    assert(EVP_DigestUpdate(hash, registers, sizeof(registers)));
    unsigned bytes = 0;
    assert(EVP_DigestFinal_ex(hash, digest, &bytes) && bytes == sizeof(digest));
    EVP_MD_CTX_free(hash);
    for (unsigned i = 0; i < bytes; ++i)
        snprintf(result + i * 2, 3, "%02x", digest[i]);
}

typedef struct {
    unsigned input;
    uint32_t fault, pc, result;
    const char *digest;
} HudCase;

static void check(const HudCase *test, const uint8_t code[CODE_BYTES])
{
    static const struct {
        uint16_t current, maximum;
        uint32_t other, other_max;
    } values[] = {{10, 400, 0, 400}, {300, 400, 75, 400},
                  {0xff80, 0x7fff, 0xffffff80, 0x7fffffff}};
    const unsigned i = test->input;
    const uint32_t record = i == 2 ? 0x170 : 0;
    memset(memory, 0xa5, sizeof(memory));
    memcpy(memory + CODE_OFFSET, code, CODE_BYTES);
    fault_address = 0;
    assert(!write_guest(NULL, 0x800ccd34u + record, 2, values[i].current));
    assert(!write_guest(NULL, 0x800ccd36u + record, 2, values[i].maximum));
    assert(!write_guest(NULL, 0x800ccdecu + record, 4, values[i].other));
    assert(!write_guest(NULL, 0x800ccdf0u + record, 4, values[i].other_max));
    const PcPortMipsBus bus = {.read = read_guest, .write = write_guest};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    for (unsigned reg = 1; reg < 32; ++reg)
        cpu.gpr[reg] = 0x31400000u + reg;
    cpu.gpr[4] = i == 2;
    cpu.gpr[5] = i != 0;
    cpu.gpr[29] = 0x801ff000u;
    cpu.gpr[31] = 0x8000deacu;
    cpu.hi = 0x24681357u;
    cpu.lo = 0xabcdef01u;
    fault_address = test->fault;
    int status = PcPortMipsRun(&cpu, ENTRY, cpu.gpr[31], 5000);
    assert(status == (test->fault ? PC_PORT_MIPS_FAULT : PC_PORT_MIPS_HALTED));
    assert(cpu.pc == test->pc && cpu.gpr[2] == test->result && cpu.steps > 0);
    char actual[65];
    state_digest(&cpu, actual);
    if (strcmp(actual, test->digest))
        fprintf(stderr, "HUD input %u, fault %08x, state digest %s\n", i, test->fault, actual);
    assert(strcmp(actual, test->digest) == 0);
}

int main(int argc, char **argv)
{
    static const HudCase cases[] = {
        {0, 0, 0x8000deac, 2, "1b47bcfb514e67c59a90c8b1aba0def2a7b7a00cea45ce0c912c8bd27ff2193f"},
        {1, 0, 0x8000deac, 1, "d268ea8d5f82ecce9c47e70b810c17ca47b4a64d39a4312c195c10082ac8f5c1"},
        {2, 0, 0x8000deac, 2, "95076e9629b3d061b4986229cbe6c7713f5a2db24ab85304d655121e48afb5ed"},
        {0, 0x800d3330, 0x80072f98, 0, "abf53a2b6d586cb2081390c2b26358318192c0e986cd541259041c727e6f631f"},
        {0, 0x800d2d38, 0x80072fb0, 0, "24742379bd46bd65d0648320a29cc35cd2036991e7380a7dc60c232216fd65d5"},
        {0, 0x801feff8, 0x80072f54, 0, "9afdfac188a24f08e3861752e9cd99342866e62297fd589e30d4e728fc2f6a77"}
    };
    uint8_t code[CODE_BYTES];
    assert(argc == 2);
    FILE *file = fopen(argv[1], "rb");
    assert(file && fseek(file, 0x3448, SEEK_SET) == 0);
    assert(fread(code, 1, sizeof(code), file) == sizeof(code));
    assert(fclose(file) == 0);
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        check(&cases[i], code);
    puts("HUD guest value checks passed: 3 completed inputs and 3 injected faults");
    return 0;
}
