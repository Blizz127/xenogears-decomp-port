/* Dirty-room black-box vector generator. TEST ORACLE ONLY.
 *
 * Reads the owner's executable at runtime; no SDK instructions or table bytes
 * are copied into this source or the JSONL output. An isolated implementer
 * receives only the interface spec and resulting I/O vectors, never this
 * generator, the executable, the adapter or the old compatibility sources.
 * The CFC2 model follows the public psx-spx screen-offset/distance description.
 */
#include "battle_mips_adapter.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXE_SIZE 0x4A000u
#define EXE_BASE 0x8000F800u
#define OUTPUT_BASE 0x80100000u
#define HALT_PC 0xFFFFFFFCu
#define MAX_EVENTS 64u
#define JR_RA 0x03E00008u /* public MIPS I encoding of `jr $ra`; not a retail byte sequence */
#define READ_GEOM_OFFSET_ENTRY 0x8004A0BCu
#define READ_GEOM_SCREEN_ENTRY 0x8004A0DCu
/* Reachable words of each leaf function (entry through the delay slot of its
 * first `jr $ra`), re-derived from the executable at runtime and cross-checked
 * against these sealed expectations. Kit v3 (job ...-v3-331775) expected 4
 * words for ReadGeomScreen, but the fourth word is alignment padding after the
 * delay slot and can never be fetched: every one of its 526 cases reached
 * exactly the first 3 words (coverage mask 0x7), while ReadGeomOffset reached
 * all 8 (0xFF). */
#define READ_GEOM_OFFSET_WORDS 8u
#define READ_GEOM_SCREEN_WORDS 3u

typedef struct {
    const char *operation;
    uint32_t location, width, value;
} Event;

typedef struct {
    uint32_t entry, end;
    uint32_t control[32], initial_control[32];
    unsigned char output[32], initial_output[32];
    Event events[MAX_EVENTS];
    unsigned count, coverage;
    int unexpected;
} Case;

static unsigned char executable[EXE_SIZE];
static uint32_t seed = 0x047A0BCu;

static void fail(const char *why)
{
    fprintf(stderr, "cleanroom read-geom vectors: %s\n", why);
    exit(1);
}

static uint32_t exe_word(uint32_t address)
{
    const unsigned char *p = executable + (address - EXE_BASE);
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* Words from entry through the delay slot of the first `jr $ra`. Anything after
 * that word belongs to padding or the next function and is unreachable from a
 * call to this leaf, so the coverage gate must not expect it. */
static unsigned reachable_words(uint32_t entry, unsigned expected)
{
    for (unsigned i = 0; i < 16u; ++i) {
        if (exe_word(entry + 4u * i) == JR_RA) {
            if (i + 2u != expected)
                fail("retail function extent differs from the sealed expectation");
            return i + 2u;
        }
    }
    fail("no jr $ra within 16 words of the retail entry");
    return 0;
}

static uint32_t random_word(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static void event(Case *test, const char *operation, uint32_t location,
                  unsigned width, uint32_t value)
{
    if (test->count == MAX_EVENTS)
        fail("event capacity exceeded");
    test->events[test->count++] = (Event){operation, location, width, value};
}

static int read_memory(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    Case *test = opaque;
    const unsigned char *source;
    if (width != 1 && width != 2 && width != 4)
        return -1;
    if (address >= test->entry && address <= test->end - width) {
        source = executable + (address - EXE_BASE);
    } else if (address >= OUTPUT_BASE && address - OUTPUT_BASE <= sizeof(test->output) - width) {
        source = test->output + (address - OUTPUT_BASE);
    } else {
        return -1;
    }
    *value = 0;
    for (unsigned i = 0; i < width; ++i)
        *value |= (uint32_t)source[i] << (i * 8);
    if (address >= OUTPUT_BASE)
        event(test, "read", address - OUTPUT_BASE, width, *value);
    return 0;
}

static int write_memory(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    Case *test = opaque;
    if ((width != 1 && width != 2 && width != 4) || address < OUTPUT_BASE ||
        address - OUTPUT_BASE > sizeof(test->output) - width)
        return -1;
    event(test, "write", address - OUTPUT_BASE, width, value);
    for (unsigned i = 0; i < width; ++i)
        test->output[address - OUTPUT_BASE + i] = (unsigned char)(value >> (8 * i));
    return 0;
}

static int bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t address)
{
    Case *test = opaque;
    (void)cpu;
    if (address < test->entry || address >= test->end || (address & 3u))
        return -1;
    test->coverage |= 1u << ((address - test->entry) / 4u);
    return 0;
}

static uint32_t cop2_read(void *opaque, int control, unsigned reg)
{
    Case *test = opaque;
    if (!control || reg < 24 || reg > 26) {
        test->unexpected = 1;
        return 0;
    }
    uint32_t value = test->control[reg];
    if (reg == 26) {
        value &= 0xFFFFu;
        if (value & 0x8000u)
            value |= 0xFFFF0000u;
    }
    event(test, "cfc2", reg, 4, value);
    return value;
}

static void cop2_write(void *opaque, int control, unsigned reg, uint32_t value)
{
    Case *test = opaque;
    event(test, control ? "ctc2" : "mtc2", reg, 4, value);
    test->unexpected = 1;
}

static int cop2_command(void *opaque, uint32_t instruction)
{
    Case *test = opaque;
    (void)instruction;
    test->unexpected = 1;
    return -1;
}

static void print_words(const uint32_t *words, unsigned count)
{
    putchar('[');
    for (unsigned i = 0; i < count; ++i)
        printf("%s%u", i ? "," : "", words[i]);
    putchar(']');
}

static void print_bytes(const unsigned char *bytes, unsigned count)
{
    putchar('"');
    for (unsigned i = 0; i < count; ++i)
        printf("%02x", bytes[i]);
    putchar('"');
}

static unsigned run_case(const char *name, unsigned id, uint32_t entry, uint32_t end,
                         uint32_t ofx, uint32_t ofy, uint32_t h, int alias)
{
    Case test = {.entry = entry, .end = end};
    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {&test, read_memory, write_memory, bridge,
                        cop2_read, cop2_write, cop2_command};
    for (unsigned i = 0; i < 32; ++i) {
        test.control[i] = random_word();
        test.output[i] = (unsigned char)random_word();
    }
    test.control[24] = ofx;
    test.control[25] = ofy;
    test.control[26] = h;
    memcpy(test.initial_control, test.control, sizeof(test.control));
    memcpy(test.initial_output, test.output, sizeof(test.output));
    PcPortMipsCpuInit(&cpu, &bus);
    for (unsigned i = 1; i < 32; ++i)
        cpu.gpr[i] = random_word();
    cpu.gpr[4] = OUTPUT_BASE + 8;
    cpu.gpr[5] = OUTPUT_BASE + (alias ? 8 : 20);
    cpu.gpr[31] = HALT_PC;
    int rc = PcPortMipsRun(&cpu, entry, HALT_PC, 64);
    if (rc != PC_PORT_MIPS_HALTED || test.unexpected) {
        fprintf(stderr, "%s case %u: rc=%d unexpected=%d %s\n", name, id, rc,
                test.unexpected, cpu.error);
        fail("unsupported observation; extend the oracle model before implementing");
    }
    printf("{\"function\":\"%s\",\"case\":%u,\"alias\":%s,\"input_control\":",
           name, id, alias ? "true" : "false");
    print_words(test.initial_control, 32);
    printf(",\"initial_output\":");
    print_bytes(test.initial_output, sizeof(test.initial_output));
    printf(",\"output\":");
    print_bytes(test.output, sizeof(test.output));
    if (entry == READ_GEOM_SCREEN_ENTRY)
        printf(",\"return_u32\":%u", cpu.gpr[2]);
    printf(",\"final_control\":");
    print_words(test.control, 32);
    printf(",\"trace\":[");
    for (unsigned i = 0; i < test.count; ++i) {
        const Event *e = &test.events[i];
        printf("%s{\"op\":\"%s\",\"location\":%u,\"width\":%u,\"value\":%u}",
               i ? "," : "", e->operation, e->location, e->width, e->value);
    }
    printf("],\"instruction_coverage_mask\":%u}\n", test.coverage);
    return test.coverage;
}

int main(int argc, char **argv)
{
    static const uint32_t edge[] = {0, 1, 0x7FFF, 0x8000, 0xFFFF, 0x10000, 0x18000,
        0x7FFFFFFF, 0x80000000, 0xFFFF0000, 0xFFFF7FFF, 0xFFFF8000, 0xFFFFFFFF, 0xFFFE8000};
    unsigned offset_coverage = 0, screen_coverage = 0, id = 0;
    /* Negative controls (runner-side, asserted by run_cleanroom_read_geom_vectors.sh):
     *   drop-reachable : ReadGeomScreen range excludes its executed delay slot ->
     *                    the fetch faults and the generator must exit 1;
     *   extend-padding : the kit v3 expectation (padding word counted) must still
     *                    exit 1 with "coverage incomplete".
     * They prove that narrowing or widening the range cannot silence the gate. */
    enum { CONTROL_NONE, CONTROL_DROP_REACHABLE, CONTROL_EXTEND_PADDING } control = CONTROL_NONE;
    if (argc == 3 && !strcmp(argv[2], "drop-reachable"))
        control = CONTROL_DROP_REACHABLE;
    else if (argc == 3 && !strcmp(argv[2], "extend-padding"))
        control = CONTROL_EXTEND_PADDING;
    else if (argc != 2)
        fail("usage: generator SHA-VERIFIED-SLUS_006.64 [drop-reachable|extend-padding]");
    FILE *file = fopen(argv[1], "rb");
    if (!file)
        fail("cannot open executable");
    size_t size = fread(executable, 1, sizeof(executable), file);
    if (size != EXE_SIZE || fgetc(file) != EOF || ferror(file))
        fail("unexpected executable size (runner must verify SHA first)");
    fclose(file);
    unsigned offset_words = reachable_words(READ_GEOM_OFFSET_ENTRY, READ_GEOM_OFFSET_WORDS);
    unsigned screen_words = reachable_words(READ_GEOM_SCREEN_ENTRY, READ_GEOM_SCREEN_WORDS);
    uint32_t offset_end = READ_GEOM_OFFSET_ENTRY + 4u * offset_words;
    uint32_t screen_end = READ_GEOM_SCREEN_ENTRY + 4u * screen_words;
    unsigned offset_expect = (1u << offset_words) - 1u;
    unsigned screen_expect = (1u << screen_words) - 1u;
    if (control == CONTROL_DROP_REACHABLE) {
        screen_end -= 4u;                 /* excludes the executed delay slot */
        screen_expect = (1u << (screen_words - 1u)) - 1u;
    } else if (control == CONTROL_EXTEND_PADDING) {
        screen_end += 4u;                 /* kit v3 defect: unreachable padding word counted */
        screen_expect = (1u << (screen_words + 1u)) - 1u;
    }
    if (control != CONTROL_NONE) {        /* controls exercise a short ReadGeomScreen series only */
        for (unsigned i = 0; i < 8; ++i)
            screen_coverage |= run_case("ReadGeomScreen", id++, READ_GEOM_SCREEN_ENTRY, screen_end,
                                        random_word(), random_word(), random_word(), 0);
        if (screen_coverage != screen_expect)
            fail("retail instruction coverage incomplete");
        fprintf(stderr, "control %s unexpectedly passed\n", argv[2]);
        return 0;
    }
    for (unsigned x = 0; x < sizeof(edge) / sizeof(edge[0]); ++x) {
        for (unsigned y = 0; y < sizeof(edge) / sizeof(edge[0]); ++y) {
            for (int alias = 0; alias < 2; ++alias)
                offset_coverage |= run_case("ReadGeomOffset", id++, READ_GEOM_OFFSET_ENTRY, offset_end,
                                            edge[x], edge[y], random_word(), alias);
        }
        uint32_t screen_ofx = random_word(), screen_ofy = random_word();
        screen_coverage |= run_case("ReadGeomScreen", id++, READ_GEOM_SCREEN_ENTRY, screen_end,
                                    screen_ofx, screen_ofy, edge[x], 0);
    }
    for (unsigned i = 0; i < 512; ++i) {
        uint32_t ofx = random_word(), ofy = random_word(), h = random_word();
        offset_coverage |= run_case("ReadGeomOffset", id++, READ_GEOM_OFFSET_ENTRY, offset_end,
                                    ofx, ofy, h, (int)(i & 1));
        screen_coverage |= run_case("ReadGeomScreen", id++, READ_GEOM_SCREEN_ENTRY, screen_end,
                                    ofx, ofy, h, 0);
    }
    if (offset_coverage != offset_expect || screen_coverage != screen_expect)
        fail("retail instruction coverage incomplete");
    fprintf(stderr, "read-geom vectors: %u cases; all %u + %u reachable retail instructions reached\n",
            id, offset_words, screen_words);
    return 0;
}
