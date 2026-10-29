/* Link the actual temp3.c initializer and system.c decoder. Only archive/heap
 * services are modeled. Original CPU execution is confined to this test oracle.
 * Zero initial decode scratch is the deliberate native determinism baseline;
 * it is not claimed to match arbitrary retail guest-stack history. */
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { STATE_BYTES = 0x2358, NAME_BYTES = 620, NAME_COUNT = 31, RECORD_BYTES = 20 };
#define STATE_ADDRESS 0x8006d634u
#define SOUND_ADDRESS 0x8005a3c0u
#define TABLE_ADDRESS 0x80100000u
#define GLYPH_ADDRESS 0x80101000u
#define HEAP_ADDRESS 0x80110000u
#define STACK_ADDRESS 0x801ff000u
#define HALT_ADDRESS 0xfffffffcu
#define INITIALIZER_ADDRESS 0x8001b970u
#define DECODER_ADDRESS 0x80033b34u

/* Keep the actual production globals below signed 32-bit pointer limits, with
 * visible guards around their packed storage. The volume routine writes forty
 * bytes beginning 32 bytes before its symbol, as the original body does. */
_Alignas(16) uint8_t name_tail_state[STATE_BYTES + 64];
_Alignas(16) uint8_t name_tail_sound[32 + 32 + 0x48 + 32];
__asm__(".globl g_GameState\n.set g_GameState, name_tail_state+32\n"
        ".globl g_SoundVolumeController\n.set g_SoundVolumeController, name_tail_sound+64\n");
extern uint8_t g_GameState[], D_800594CC, D_8005947C;
extern void *g_SystemDataEntries;
extern void func_8001B970(void);
static _Alignas(16) uint8_t native_table[0x100], native_glyphs[0x288];
static uint8_t archive_template[STATE_BYTES], archive_heap[STATE_BYTES];
static uint8_t expected_state[STATE_BYTES], baseline_state[STATE_BYTES];
static uint8_t visible[NAME_COUNT][21], visible_length[NAME_COUNT];
static uint8_t oracle_ram[0x200000];
static uint8_t original_initializer[412], original_decoder[120];
static unsigned service_stage, template_seed, stack_pattern, decoder_calls;
static volatile unsigned stack_witness;

static void fail(const char *what, unsigned detail)
{
    fprintf(stderr, "NAME TAIL FAIL %s detail=%u template=%u stack=%u\n",
            what, detail, template_seed, stack_pattern);
    exit(1);
}

static void require(int condition, const char *what, unsigned detail)
{
    if (!condition) fail(what, detail);
}

static void equal_bytes(const void *actual, const void *expected, size_t length,
                        const char *what)
{
    const uint8_t *a = actual, *e = expected;
    for (size_t i = 0; i < length; ++i)
        if (a[i] != e[i]) fail(what, (unsigned)i);
}

static void service(unsigned stage)
{
    require(service_stage == stage, "archive/heap order", service_stage);
    ++service_stage;
}

int ArchiveSetIndex(int directory, int entry)
{
    service(0);
    require(directory == 0x10 && entry == 0, "archive index", (unsigned)directory);
    return 0;
}

void HeapChangeCurrentUser(unsigned user, char **types)
{
    service(1);
    require(user == 2 && !types, "heap owner", user);
}

int ArchiveDecodeAlignedSize(int file)
{
    service(2);
    require(file == 3, "archive size file", (unsigned)file);
    return STATE_BYTES;
}

void *HeapAlloc(unsigned size, unsigned flags)
{
    service(3);
    require(size == STATE_BYTES && flags == 1, "heap allocation", size);
    return archive_heap;
}

void ArchiveReadFileToBuffer(int file, void *buffer, int offset, int flags)
{
    service(4);
    require(file == 3 && buffer == archive_heap && offset == 0 && flags == 0x80,
            "archive read arguments", (unsigned)file);
    memcpy(buffer, archive_template, STATE_BYTES);
}

int ArchiveCdDataSync(int mode)
{
    service(5);
    require(mode == 0, "archive sync", (unsigned)mode);
    return 0;
}

unsigned HeapFree(void *buffer)
{
    service(6);
    require(buffer == archive_heap, "heap free pointer", 0);
    equal_bytes(g_GameState, archive_template, STATE_BYTES, "template copy before free");
    /* Decoding must operate on the copied state, after the heap is released. */
    memset(archive_heap, 0x93, sizeof(archive_heap));
    return 0;
}

static uint8_t *oracle_address(uint32_t address, unsigned width)
{
    if (address >= 0x80000000u &&
        (uint64_t)address + width <= 0x80200000u)
        return oracle_ram + (address - 0x80000000u);
    return NULL;
}

static int oracle_read(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    (void)opaque;
    const uint8_t *p = oracle_address(address, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i) *value |= (uint32_t)p[i] << (8u * i);
    return 0;
}

static int oracle_write(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    (void)opaque;
    uint8_t *p = oracle_address(address, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; ++i) p[i] = (uint8_t)(value >> (8u * i));
    return 0;
}

static void oracle_word(uint32_t address, uint32_t value)
{
    require(oracle_write(NULL, address, 4, value) == 0, "oracle fixture address", address);
}

static int oracle_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)opaque;
    const uint32_t a = cpu->gpr[4], b = cpu->gpr[5];
    const uint32_t c = cpu->gpr[6], d = cpu->gpr[7];
    if (target == DECODER_ADDRESS) {
        require(a == STACK_ADDRESS - 0x48u && b == STACK_ADDRESS - 0x30u,
                "original decode scratch", a);
        require(decoder_calls < NAME_COUNT, "original decode count", decoder_calls);
        ++decoder_calls;
        return 0; /* Execute all 120 original decoder bytes; no decoder model. */
    }
    if (target - INITIALIZER_ADDRESS < sizeof(original_initializer) ||
        target - DECODER_ADDRESS < sizeof(original_decoder)) return 0;
    switch (target) {
    case 0x80028470u:
        cpu->gpr[2] = (uint32_t)ArchiveSetIndex((int)a, (int)b);
        break;
    case 0x80032498u:
        HeapChangeCurrentUser(a, (char **)(uintptr_t)b);
        break;
    case 0x800288ecu:
        cpu->gpr[2] = (uint32_t)ArchiveDecodeAlignedSize((int)a);
        break;
    case 0x80031bdcu:
        (void)HeapAlloc(a, b);
        cpu->gpr[2] = HEAP_ADDRESS;
        break;
    case 0x800295d8u:
        require(b == HEAP_ADDRESS, "original heap read pointer", b);
        ArchiveReadFileToBuffer((int)a, archive_heap, (int)c, (int)d);
        memcpy(oracle_address(HEAP_ADDRESS, STATE_BYTES), archive_heap, STATE_BYTES);
        break;
    case 0x80028a60u:
        cpu->gpr[2] = (uint32_t)ArchiveCdDataSync((int)a);
        break;
    case 0x8003f99cu:
        require(service_stage == 6 && a == STATE_ADDRESS && b == HEAP_ADDRESS &&
                c == STATE_BYTES, "original template copy", a);
        memcpy(oracle_address(a, c), oracle_address(b, c), c);
        cpu->gpr[2] = a;
        break;
    case 0x800320e8u:
        service(6);
        require(a == HEAP_ADDRESS, "original heap free", a);
        equal_bytes(oracle_address(STATE_ADDRESS, STATE_BYTES), archive_template,
                    STATE_BYTES, "original template before free");
        memset(oracle_address(HEAP_ADDRESS, STATE_BYTES), 0x93, STATE_BYTES);
        cpu->gpr[2] = 0;
        break;
    default:
        fail("unexpected original callee", target);
    }
    return 1;
}

static void prepare_template(void)
{
    static const struct Glyph { uint16_t code; uint8_t first, second; } glyphs[] = {
        {1, 0, 'A'}, {2, 0x81, 0x41}, {3, 0, 'z'}, {4, 0x82, 0x42},
        {14, 0, 'Q'}, {16, 0x83, 0x43}, {0x10f, 0, 'x'}, {0x143, 0x84, 0x44}
    };
    /* Short initial output exposes the fresh tail; long then empty preserves
     * within-call history. Ten encoded pairs exercise the full input record. */
    static const uint8_t counts[NAME_COUNT] = {
        1, 10, 0, 2, 9, 3, 0, 10, 4, 1, 8, 2, 7, 0, 6, 5,
        1, 10, 3, 9, 0, 4, 8, 2, 7, 5, 0, 6, 1, 10, 2
    };
    memset(native_table, 0, sizeof(native_table));
    memset(native_glyphs, 0xe7, sizeof(native_glyphs));
    for (unsigned i = 0; i < sizeof(glyphs) / sizeof(glyphs[0]); ++i) {
        native_glyphs[glyphs[i].code * 2u] = glyphs[i].first;
        native_glyphs[glyphs[i].code * 2u + 1u] = glyphs[i].second;
    }
    uint32_t glyph_pointer = (uint32_t)(uintptr_t)native_glyphs;
    memcpy(native_table + 0x6c, &glyph_pointer, sizeof(glyph_pointer));
    g_SystemDataEntries = native_table;
    for (unsigned i = 0; i < STATE_BYTES; ++i)
        archive_template[i] = (uint8_t)(i * 37u + template_seed * 29u + 0x51u);
    memset(visible, 0, sizeof(visible));
    for (unsigned row = 0; row < NAME_COUNT; ++row) {
        uint8_t *encoded = archive_template + row * RECORD_BYTES;
        unsigned length = 0;
        for (unsigned column = 0; column < counts[row]; ++column) {
            /* Include all-two-byte full records as well as mixed records. */
            unsigned index = (row + column + template_seed) % 8u;
            if (row == 1) index = 1u + 2u * ((column + template_seed) % 4u);
            const struct Glyph *g = &glyphs[index];
            encoded[column * 2u] = (uint8_t)g->code;
            encoded[column * 2u + 1u] = (uint8_t)(g->code >> 8);
            if (g->first) visible[row][length++] = g->first;
            visible[row][length++] = g->second;
        }
        if (counts[row] < 10) {
            encoded[counts[row] * 2u] = 0x0f;
            encoded[counts[row] * 2u + 1u] = 0;
        }
        visible[row][length] = 0;
        visible_length[row] = (uint8_t)length;
    }
    memcpy(expected_state, archive_template, sizeof(expected_state));
    uint8_t history[24] = {0};
    for (unsigned row = 0; row < NAME_COUNT; ++row) {
        memcpy(history, visible[row], visible_length[row] + 1u);
        memcpy(expected_state + row * RECORD_BYTES, history, RECORD_BYTES);
    }
    require(visible_length[0] < RECORD_BYTES - 1, "fixture initial tail", 0);
    require(visible_length[1] == RECORD_BYTES && visible_length[2] == 0,
            "fixture long-empty transition", 0);
    require(expected_state[2 * RECORD_BYTES + 1] != 0,
            "fixture retains preceding decoded tail", 0);
}

__attribute__((noinline)) static void poison_stack(unsigned pattern)
{
    volatile uint8_t poison[4096];
    for (unsigned i = 0; i < sizeof(poison); ++i)
        poison[i] = (uint8_t)(pattern + i * 17u);
    stack_witness = poison[pattern % sizeof(poison)];
}

static void check_native(void)
{
    memset(name_tail_state, 0xa6, sizeof(name_tail_state));
    memset(name_tail_sound, 0x5c, sizeof(name_tail_sound));
    memset(archive_heap, (int)stack_pattern, sizeof(archive_heap));
    D_800594CC = D_8005947C = 0xbb;
    service_stage = 0;
    poison_stack(stack_pattern);
    require(setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1) == 0 &&
            PcPortMipsInterpreterDisabled(), "disable native-leg interpreter", 0);
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    func_8001B970();
    PcPortMipsExecutionStats after = PcPortMipsExecutionStatsGet();
    require(after.interpreted_instructions == before.interpreted_instructions,
            "native initializer used CPU interpreter", 0);
    require(service_stage == 7, "native service count", service_stage);
    equal_bytes(g_GameState, expected_state, STATE_BYTES, "native GameState");
    equal_bytes(g_GameState + NAME_BYTES, archive_template + NAME_BYTES,
                STATE_BYTES - NAME_BYTES, "native non-name bytes");
    for (unsigned i = 0; i < 32; ++i) {
        require(name_tail_state[i] == 0xa6, "state leading guard", i);
        require(name_tail_state[32 + STATE_BYTES + i] == 0xa6, "state trailing guard", i);
    }
    for (unsigned i = 0; i < sizeof(name_tail_sound); ++i)
        require(name_tail_sound[i] == (i >= 32 && i < 72 ? 0 : 0x5c),
                "native volume reset extent", i);
    require(D_800594CC == 6 && D_8005947C == 0, "native mode bytes", 0);
    if (stack_pattern == 0) memcpy(baseline_state, g_GameState, STATE_BYTES);
    else equal_bytes(g_GameState, baseline_state, STATE_BYTES, "repeatability");
}

static void check_original(unsigned initial_tail)
{
    require(setenv("XENO_DISABLE_CPU_INTERPRETER", "0", 1) == 0 &&
            !PcPortMipsInterpreterDisabled(), "enable test-only original oracle", 0);
    memset(oracle_ram, 0xac, sizeof(oracle_ram));
    memcpy(oracle_address(INITIALIZER_ADDRESS, sizeof(original_initializer)),
           original_initializer, sizeof(original_initializer));
    memcpy(oracle_address(DECODER_ADDRESS, sizeof(original_decoder)),
           original_decoder, sizeof(original_decoder));
    oracle_word(0x80059360u, TABLE_ADDRESS);
    oracle_word(TABLE_ADDRESS + 0x6cu, GLYPH_ADDRESS);
    memcpy(oracle_address(GLYPH_ADDRESS, sizeof(native_glyphs)),
           native_glyphs, sizeof(native_glyphs));
    /* The only chosen retail-history baseline is the decoder's 24-byte output
     * scratch. Other stack bytes retain poison; no instruction is patched. */
    memset(oracle_address(STACK_ADDRESS - 0x30u, 24), (int)initial_tail, 24);
    service_stage = decoder_calls = 0;
    PcPortMipsBus bus = {.read = oracle_read, .write = oracle_write, .bridge = oracle_bridge};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[28] = 0x80059170u;
    cpu.gpr[29] = STACK_ADDRESS;
    cpu.gpr[31] = HALT_ADDRESS;
    int result = PcPortMipsRun(&cpu, INITIALIZER_ADDRESS, HALT_ADDRESS, 200000);
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "NAME TAIL original failure: %s\n", cpu.error);
        fail("original did not return", cpu.pc);
    }
    require(service_stage == 7 && decoder_calls == NAME_COUNT,
            "original complete call graph", decoder_calls);
    uint8_t *state = oracle_address(STATE_ADDRESS, STATE_BYTES);
    if (initial_tail == 0) {
        equal_bytes(state, expected_state, STATE_BYTES, "complete original zero-tail oracle");
        equal_bytes(state, baseline_state, STATE_BYTES, "native versus complete original");
    } else {
        /* This witness prevents claiming arbitrary guest history equals zero. */
        require(memcmp(state, expected_state, NAME_BYTES) != 0,
                "nonzero retail tail must remain distinct", 0);
        unsigned changed = 0;
        for (unsigned row = 0; row < NAME_COUNT; ++row) {
            const unsigned length = visible_length[row];
            const unsigned prefix = length < RECORD_BYTES ? length + 1u : RECORD_BYTES;
            equal_bytes(state + row * RECORD_BYTES, visible[row], prefix,
                        "retail visible prefix and terminator");
            for (unsigned i = 0; i < RECORD_BYTES; ++i)
                if (state[row * RECORD_BYTES + i] != expected_state[row * RECORD_BYTES + i]) {
                    require(row == 0 && i > length && state[i] == initial_tail,
                            "deliberate initial-history divergence", row * RECORD_BYTES + i);
                    ++changed;
                }
        }
        require(changed > 0, "nonzero retail tail witness", 0);
    }
    equal_bytes(state + NAME_BYTES, archive_template + NAME_BYTES,
                STATE_BYTES - NAME_BYTES, "original non-name bytes");
    for (unsigned i = 0; i < 32; ++i) {
        require(*oracle_address(STATE_ADDRESS - 32u + i, 1) == 0xac,
                "original state leading guard", i);
        require(*oracle_address(STATE_ADDRESS + STATE_BYTES + i, 1) == 0xac,
                "original state trailing guard", i);
    }
    for (unsigned i = 0; i < sizeof(name_tail_sound); ++i)
        require(*oracle_address(SOUND_ADDRESS - 64u + i, 1) ==
                    (i >= 32 && i < 72 ? 0 : 0xac),
                "original volume reset extent", i);
    require(*oracle_address(0x800594ccu, 1) == 6 &&
            *oracle_address(0x8005947cu, 1) == 0, "original mode bytes", 0);
}

int main(void)
{
    require((uintptr_t)name_tail_state + sizeof(name_tail_state) < INT32_MAX &&
            (uintptr_t)native_glyphs + sizeof(native_glyphs) < INT32_MAX,
            "non-PIE low-address fixture required", 0);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    require(image != NULL, "private original required", 0);
    require(fseek(image, 0xc170, SEEK_SET) == 0 &&
            fread(original_initializer, 1, sizeof(original_initializer), image) ==
                sizeof(original_initializer), "read complete original initializer", 0);
    require(fseek(image, 0x24334, SEEK_SET) == 0 &&
            fread(original_decoder, 1, sizeof(original_decoder), image) ==
                sizeof(original_decoder), "read complete original decoder", 0);
    require(fclose(image) == 0, "close private original", 0);
    PcPortMipsExecutionStatsInit();
    for (template_seed = 0; template_seed < 8; ++template_seed) {
        prepare_template();
        for (stack_pattern = 0; stack_pattern < 8; ++stack_pattern) check_native();
        check_original(0);
        check_original(0x5a);
        check_original(0xa5);
    }
    puts("NAME TAIL PASS 64 native initializations, 24 complete original-body oracles, "
         "31 full records each; deterministic initial zero, retained intra-call tails, "
         "unchanged non-name state and exact service/reset effects");
    return 0;
}
