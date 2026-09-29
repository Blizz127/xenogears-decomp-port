#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "field_clip_control.h"
#include "battle_mips_adapter.h"

/* Execute pinned retail dispatch/handlers through E5974. Dependencies are
 * recording boundaries with controlled outputs, NOT callee-fidelity proof.
 * Opcode 25's GTE/vector branch is outside this test's scope. */
enum { STACK = 0x1ff000, OBJECT_BYTES = 0x140, MAX_CALLS = 8 };
static uint8_t ram[0x200000];
uint32_t D_801E8670[10];
uint32_t D_801E85CC;
static struct Fixture {
    uint8_t objects[12][0x200];
    uint32_t nodes[6][31], pool[64], decoy[64];
    uint16_t script[32];
} fixture, initial, expected;
static struct Call {
    uint32_t target, count, args[14];
    uint8_t object[OBJECT_BYTES];
} calls[MAX_CALLS], expected_calls[MAX_CALLS];
static unsigned call_count, cases, failures;
static uint16_t resolver_mask;
static uint32_t resolver_slot, resolver_patch;
static const char *case_name;

static uint32_t pointer(const void *p) { return (uint32_t)(uintptr_t)p; }
static uint8_t *object(unsigned i) { return fixture.objects[i] + 0x80; }
static void put16(void *p, uint16_t v) { memcpy(p, &v, 2); }
static void put32(void *p, uint32_t v) { memcpy(p, &v, 4); }
static uint8_t *address(uint32_t a, unsigned width)
{
    if (a >= 0x801e8670u && (uint64_t)a + width <= 0x801e8698u)
        return (uint8_t *)D_801E8670 + a - 0x801e8670u;
    if (a >= 0x80000000u && (uint64_t)a + width <= 0x80200000u)
        return ram + (a & 0x1fffffu);
    uintptr_t first = (uintptr_t)&fixture;
    if (a >= first && (uint64_t)a + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)a;
    return NULL;
}
static int read_bus(void *unused, uint32_t a, unsigned width, uint32_t *value)
{
    (void)unused;
    uint8_t *p = address(a, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i) *value |= (uint32_t)p[i] << (8 * i);
    return 0;
}
static int write_bus(void *unused, uint32_t a, unsigned width, uint32_t value)
{
    (void)unused;
    uint8_t *p = address(a, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; ++i) p[i] = (uint8_t)(value >> (8 * i));
    return 0;
}
static uint32_t read_stack(unsigned offset, unsigned width)
{
    uint32_t value;
    assert(!read_bus(NULL, 0x80000000u + STACK + offset, width, &value));
    return value;
}
static void record(uint32_t target, const uint32_t *args, unsigned count,
                   const uint8_t *obj)
{
    assert(call_count < MAX_CALLS && count <= 14);
    struct Call *call = &calls[call_count++];
    call->target = target;
    call->count = count;
    memcpy(call->args, args, count * sizeof(*args));
    if (obj) memcpy(call->object, obj, sizeof(call->object));
}
uint32_t func_801E6830(uint8_t *obj, int32_t selector, uint16_t *mask)
{
    /* The output-cell address is local on the host and SP+6C on retail.
     * Record its returned value, not incomparable machine addresses. */
    const uint32_t args[] = {pointer(obj), (uint32_t)selector, resolver_mask};
    record(0x801e6830, args, 3, obj);
    *mask = resolver_mask;
    if (resolver_patch) {
        uint8_t *p = address(resolver_patch, 6);
        assert(p);
        put16(p, 0x8001); put16(p + 2, 0x7ffe); put16(p + 4, 0xffff);
    }
    return resolver_slot;
}
void func_801E6974(uint8_t *obj, uint8_t *pool, uint8_t *node,
                   int32_t flags, int32_t mode, int32_t tag, int32_t loop,
                   int32_t sx, int32_t sy, int32_t sz, int32_t ex, int32_t ey,
                   int32_t ez, int32_t duration)
{
    const uint32_t args[] = {pointer(obj), pointer(pool), pointer(node),
        flags, mode, tag, loop, sx, sy, sz, ex, ey, ez, duration};
    record(0x801e6974, args, 14, obj);
}
void func_801E6D94(uint8_t *obj, uint8_t *node, int32_t flags)
{
    const uint32_t args[] = {pointer(obj), pointer(node), (uint32_t)flags};
    record(0x801e6d94, args, 3, obj);
}
int32_t func_801DC848(uint8_t *root, int32_t scale)
{
    const uint32_t args[] = {pointer(root), (uint32_t)scale};
    record(0x801dc848, args, 2, NULL);
    return -123;
}
int32_t func_801DC5C0(uint8_t *root, int32_t scale)
{
    const uint32_t args[] = {pointer(root), (uint32_t)scale};
    record(0x801dc5c0, args, 2, NULL);
    return 456;
}
static void unexpected(void)
{
    fprintf(stderr, "CLIP TRACK FAIL %s: unexpected dependency\n", case_name);
    exit(1);
}
uint32_t func_801E6910(uint8_t *o, int32_t i, uint32_t *f)
{ (void)o; (void)i; (void)f; unexpected(); return 0; }
void func_801DEF10(uint8_t *r, uint8_t *p)
{ (void)r; (void)p; unexpected(); }
void func_801DFE8C(uint8_t *p, uint8_t *r)
{ (void)p; (void)r; unexpected(); }
void func_801DF52C(uint8_t *p, uint8_t *r, int32_t i, int32_t m)
{ (void)p; (void)r; (void)i; (void)m; unexpected(); }
int32_t func_801E632C(uint8_t *o)
{ (void)o; unexpected(); return 0; }
uint32_t func_801DF7F4(uint8_t *p, uint8_t *r, uint8_t *v, int32_t l, int32_t t)
{ (void)p; (void)r; (void)v; (void)l; (void)t; unexpected(); return 0; }
uint32_t func_801DF0B4(uint8_t *p, uint8_t *r, uint8_t *v, int32_t d,
                      int32_t m, int32_t l, int32_t t)
{ (void)p; (void)r; (void)v; (void)d; (void)m; (void)l; (void)t; unexpected(); return 0; }
void func_801E5C74(uint8_t *o, uint8_t *d, int32_t l)
{ (void)o; (void)d; (void)l; unexpected(); }
static int bridge(void *unused, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)unused;
    uint32_t *r = cpu->gpr;
    uint8_t *obj = (uint8_t *)(uintptr_t)r[4];
    switch (target) {
    case 0x801e6830: {
        uint16_t mask;
        r[2] = func_801E6830(obj, (int32_t)r[5], &mask);
        assert(!write_bus(NULL, r[6], 2, mask));
        break;
    }
    case 0x801e6974:
        func_801E6974(obj, (uint8_t *)(uintptr_t)r[5],
            (uint8_t *)(uintptr_t)r[6], (int32_t)r[7],
            (int32_t)read_stack(0x10, 4), (int32_t)read_stack(0x14, 4),
            (int32_t)read_stack(0x18, 4), (int32_t)read_stack(0x1c, 4),
            (int32_t)read_stack(0x20, 4), (int32_t)read_stack(0x24, 4),
            (int32_t)read_stack(0x28, 4), (int32_t)read_stack(0x2c, 4),
            (int32_t)read_stack(0x30, 4), (int32_t)read_stack(0x34, 4));
        break;
    case 0x801e6d94:
        func_801E6D94(obj, (uint8_t *)(uintptr_t)r[5], (int32_t)r[6]);
        break;
    case 0x801dc848: r[2] = (uint32_t)func_801DC848(obj, (int32_t)r[5]); break;
    case 0x801dc5c0: r[2] = (uint32_t)func_801DC5C0(obj, (int32_t)r[5]); break;
    default: return target >= 0x801e3d44 && target < 0x801e5974 ? 0 : -1;
    }
    return 1;
}
static void setup(unsigned opcode, unsigned parameter, unsigned seed)
{
    uint8_t *bytes = (uint8_t *)&fixture;
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        bytes[i] = (uint8_t)(i * 37u + (i >> 5) + seed * 19u);
    for (unsigned i = 0; i < 12; ++i) {
        put32(object(i), pointer(fixture.decoy));
        put32(object(i) + 4, pointer(fixture.nodes[2]));
        object(i)[0x20] = (uint8_t)(0xc0 + i);
    }
    for (unsigned slot = 0; slot < 10; ++slot)
        D_801E8670[slot] = pointer(object(slot));
    fixture.script[0] = (uint16_t)((parameter << 8) | opcode);
    resolver_mask = (uint16_t)(0x8100u | seed);
    resolver_slot = 0x12340003;
    resolver_patch = 0;
}
static PcPortFieldClipControl state_for(uint8_t *stream)
{
    PcPortFieldClipControl state = {.object = object(10),
        .pool = (uint8_t *)fixture.pool, .stream = pointer(stream),
        .limit = 0x12345678, .ticks = -17, .running = 1,
        .postprocess = 0x1234, .operand = 0xa55a, .origin = object(10)};
    return state;
}
static void reset_calls(void)
{ call_count = 0; memset(calls, 0, sizeof(calls)); }
static int compare(PcPortFieldClipControl *state, const char *name)
{
    case_name = name;
    initial = fixture;
    uint32_t initial_slots[10], expected_slots[10];
    memcpy(initial_slots, D_801E8670, sizeof(initial_slots));
    PcPortFieldClipControl before = *state;
    memset(ram + STACK, 0x36, sizeof(ram) - STACK);
    PcPortMipsBus bus = {.read = read_bus, .write = write_bus, .bridge = bridge};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[16] = cpu.gpr[19] = state->stream;
    cpu.gpr[20] = pointer(state->object);
    cpu.gpr[29] = 0x80000000u + STACK;
    put32(ram + STACK + 0xc8, pointer(state->pool));
    put32(ram + STACK + 0xd0, (uint32_t)state->limit);
    put32(ram + STACK + 0xd8, (uint32_t)state->ticks);
    put32(ram + STACK + 0xe0, pointer(state->origin));
    put32(ram + STACK + 0xe8, (uint32_t)state->running);
    put32(ram + STACK + 0xf0, (uint32_t)state->postprocess);
    reset_calls();
    if (PcPortMipsRun(&cpu, 0x801e3d44, 0x801e5974, 2000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "CLIP TRACK FAIL %s: retail oracle %s\n", name, cpu.error);
        exit(1);
    }
    expected = fixture;
    memcpy(expected_slots, D_801E8670, sizeof(expected_slots));
    memcpy(expected_calls, calls, sizeof(calls));
    unsigned expected_count = call_count;
    fixture = initial;
    memcpy(D_801E8670, initial_slots, sizeof(initial_slots));
    reset_calls();
    int result = PcPort_FieldClipDataStep(state);
    int bad = 0;
#define CHECK(field, actual, wanted) do { \
    uint32_t a_ = (uint32_t)(actual), w_ = (uint32_t)(wanted); \
    if (a_ != w_) { \
        if (failures < 12) fprintf(stderr, \
            "CLIP TRACK FAIL %s case=%u %s native=%08x retail=%08x\n", \
            name, cases, field, a_, w_); \
        bad = 1; \
    } \
} while (0)
    CHECK("handled", result, 1);
    CHECK("object", pointer(state->object), cpu.gpr[20]);
    CHECK("pool", pointer(state->pool), pointer(before.pool));
    CHECK("origin", pointer(state->origin), pointer(before.origin));
    CHECK("stream", state->stream, cpu.gpr[19]);
    CHECK("limit", state->limit, read_stack(0xd0, 4));
    CHECK("ticks", state->ticks, read_stack(0xd8, 4));
    CHECK("running", state->running, read_stack(0xe8, 4));
    CHECK("postprocess", state->postprocess, read_stack(0xf0, 4));
    CHECK("operand", state->operand, read_stack(0x6c, 2));
    CHECK("call count", call_count, expected_count);
    if (memcmp(&fixture, &expected, sizeof(fixture))) {
        const uint8_t *actual = (const uint8_t *)&fixture;
        const uint8_t *wanted = (const uint8_t *)&expected;
        unsigned shown = 0;
        for (unsigned i = 0; i < sizeof(fixture); ++i)
            if (actual[i] != wanted[i] && shown++ < 8)
                CHECK("fixture byte", (i << 8) | actual[i], (i << 8) | wanted[i]);
    }
    if (memcmp(D_801E8670, expected_slots, sizeof(expected_slots)))
        CHECK("slot table bytes", 1, 0);
    if (memcmp(calls, expected_calls, sizeof(calls))) {
        CHECK("ordered call records", 1, 0);
        for (unsigned i = 0; i < call_count && i < expected_count; ++i) {
            CHECK("call target", calls[i].target, expected_calls[i].target);
            for (unsigned j = 0; j < calls[i].count && j < expected_calls[i].count; ++j)
                CHECK("call argument", calls[i].args[j], expected_calls[i].args[j]);
        }
    }
#undef CHECK
    ++cases;
    failures += bad;
    return !bad;
}

static void argument_cases(void)
{
    const uint16_t words[] = {0, 1, 0xff, 0x100, 0x7fff, 0x8000, 0xffff, 0x5aa5};
    const int16_t indices[] = {-32768, -257, -1, 0, 1, 255, 256, 32767};
    const uint8_t scale_modes[] = {0, 1, 2, 0x7f, 0x80, 0xfe, 0xff, 0x40};
    for (unsigned parameter = 0; parameter < 256; ++parameter) {
        for (unsigned seed = 0; seed < 8; ++seed) {
            setup(0x1d, parameter, seed);
            for (unsigned i = 1; i < 10; ++i)
                fixture.script[i] = words[(seed + i) % 8];
            PcPortFieldClipControl state = state_for((uint8_t *)fixture.script);
            if (seed & 1) state.pool = NULL;
            state.limit = seed & 2 ? -1 : (int32_t)words[seed];
            compare(&state, "1D unsigned mode/tag and signed seven-word payload");

            setup(0x23, parameter, seed);
            fixture.script[1] = (uint16_t)indices[seed];
            state = state_for((uint8_t *)fixture.script);
            /* These dependencies record the numeric node address; they do not
             * dereference synthetic extreme-index nodes or prove their body. */
            compare(&state, "23 signed node index and 124-byte stride");

            setup(0x27, parameter, seed);
            object(10)[0x37] = scale_modes[seed];
            put16(object(10) + 0x1c, words[(seed + 4) % 8]);
            state = state_for((uint8_t *)fixture.script);
            compare(&state, "27 object byte selects dependency and signed scale");
        }
        for (unsigned slot = 0; slot < 10; ++slot) {
            for (unsigned missing = 0; missing < 2; ++missing) {
                setup(0x1f, parameter, slot);
                resolver_slot = 0xabcd1200u | slot;
                resolver_mask = (uint16_t)(0xa581u ^ (parameter << 8) ^ slot);
                if (missing) D_801E8670[slot] = 0;
                PcPortFieldClipControl state = state_for((uint8_t *)fixture.script);
                state.object = object(11); /* Entry object remains object 10. */
                compare(&state, "1F origin differs, low-eight return slot and null slot");
            }
        }
    }
}
static void slot_cases(void)
{
    const uint16_t high_masks[] = {0, 0x0100, 0x8000, 0xff00};
    for (unsigned opcode = 0x25; opcode <= 0x26; ++opcode) {
        for (unsigned mask = 0; mask < 256; ++mask) {
            for (unsigned seed = 0; seed < 4; ++seed) {
                /* All 256 high instruction bytes for 26; 25 only even bytes,
                 * selecting its supported direct branch. */
                unsigned parameter = opcode == 0x25 ? mask & 0xfe : mask;
                setup(opcode, parameter, seed);
                resolver_mask = (uint16_t)(mask | high_masks[seed]);
                fixture.script[1] = (uint16_t)((mask << 8) | (255 - mask));
                fixture.script[2] = seed & 1 ? 0xffff : 0x8000;
                fixture.script[3] = seed & 1 ? 1 : 0x7fff;
                fixture.script[4] = seed & 2 ? 0 : 0xa55a;
                for (unsigned slot = 0; slot < 10; ++slot) {
                    if ((slot + seed) % 3 == 0) D_801E8670[slot] = 0;
                    else if (seed == 2) D_801E8670[slot] = pointer(object(10));
                    else if (seed == 3) D_801E8670[slot] = pointer(object(slot % 2));
                }
                PcPortFieldClipControl state = state_for((uint8_t *)fixture.script);
                compare(&state, "25/26 all low masks, ignored high bits, null/shared/self slots");
            }
        }
    }
    for (unsigned selector = 0; selector < 256; ++selector) {
        setup(0x25, 2, selector);
        fixture.script[1] = (uint16_t)((selector << 8) | (255 - selector));
        resolver_mask = 0x81;
        resolver_patch = pointer(fixture.script + 2);
        PcPortFieldClipControl state = state_for((uint8_t *)fixture.script);
        compare(&state, "25 resolver writes orientation payload before handler reads it");

        setup(0x25, 2, selector);
        fixture.script[1] = (uint16_t)((selector << 8) | 0x5a);
        resolver_mask = 3;
        /* Slot zero +5E overlaps source +20. Retail writes the selector high
         * halfword before reading source +20 for destination +5C; slot one
         * must then read the changed source byte as well. */
        uint8_t *alias = object(10) - 0x3e;
        put32(alias, pointer(fixture.decoy));
        D_801E8670[0] = pointer(alias);
        state = state_for((uint8_t *)fixture.script);
        compare(&state, "25 ordered +5E write aliases source +20");
    }
    for (unsigned parameter = 0; parameter < 256; ++parameter) {
        for (unsigned mode = 0; mode < 3; ++mode) {
            setup(0x25, parameter, mode);
            resolver_mask = mode == 0 ? 0 : mode == 1 ? 0xff00 : 0xffff;
            if (mode == 2) memset(D_801E8670, 0, sizeof(D_801E8670));
            PcPortFieldClipControl state = state_for((uint8_t *)fixture.script);
            /* Odd parameters are valid here because no selected nonnull slot
             * enters the GTE branch. This exercises only the common prefix. */
            compare(&state, "25 common prefix with no selected nonnull slot");
        }
    }
}
static void stream_alias_cases(void)
{
    const unsigned opcodes[] = {0x1d, 0x1f, 0x23, 0x25, 0x26, 0x27};
    const unsigned offsets[] = {0x10, 0x20, 0x34, 0x36, 0x5c, 0x5e, 0x6a, 0x6c, 0x6e};
    for (unsigned op = 0; op < sizeof(opcodes) / sizeof(*opcodes); ++op) {
        for (unsigned i = 0; i < sizeof(offsets) / sizeof(*offsets); ++i) {
            setup(opcodes[op], 2, i);
            resolver_mask = 0x8003;
            D_801E8670[0] = pointer(object(10));
            D_801E8670[1] = pointer(object(10));
            uint8_t *stream = object(10) + offsets[i];
            memcpy(stream, fixture.script, 20);
            PcPortFieldClipControl state = state_for(stream);
            compare(&state, "instruction/payload alias current object fields");
        }
    }
    setup(0x1f, 0xfe, 0);
    fixture.script[1] = 0xfd1f;
    fixture.script[2] = 0x0026;
    fixture.script[3] = 0x0027;
    PcPortFieldClipControl state = state_for((uint8_t *)fixture.script);
    resolver_slot = 0x12340003;
    resolver_mask = 0x5555;
    compare(&state, "successive selection first object");
    resolver_slot = 0xabcd0008;
    resolver_mask = 0xaaaa;
    compare(&state, "successive selection retains original entry object");
    resolver_mask = 0x8001;
    compare(&state, "following opcode 26 uses selected object");
    compare(&state, "following opcode 27 uses selected object");
}
int main(int argc, char **argv)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    if (argc == 3 && !strcmp(argv[1], "--reject-slot") &&
        (!strcmp(argv[2], "10") || !strcmp(argv[2], "255"))) {
        /* Native owner guard only: never execute retail's unowned table read.
         * The parent checks SIGABRT and the complete diagnostic, including IP. */
        setup(0x1f, 0x5a, 0);
        unsigned slot = !strcmp(argv[2], "10") ? 10 : 255;
        resolver_slot = 0xa5a50000u | slot;
        case_name = "native unowned-slot guard";
        PcPortFieldClipControl state = state_for((uint8_t *)fixture.script);
        printf("expected-ip=%08x\n", state.stream);
        assert(!fflush(stdout));
        (void)PcPort_FieldClipDataStep(&state);
        fprintf(stderr, "CLIP TRACK FAIL unowned slot %u returned without rejecting\n", slot);
        return 1;
    }
    if (argc != 1) {
        fputs("CLIP TRACK FAIL unexpected test arguments\n", stderr);
        return 2;
    }
    FILE *disc = fopen("disc/disc1.bin", "rb");
    assert(disc);
    for (unsigned i = 0; i < 25; ++i) {
        assert(!fseek(disc, (231361 + i) * 2352L + 24, SEEK_SET));
        assert(fread(ram + 0x1dc000 + i * 2048, 1, 2048, disc) == 2048);
    }
    assert(!fclose(disc));
    const unsigned opcodes[] = {0x1d, 0x1f, 0x23, 0x25, 0x26, 0x27};
    for (unsigned i = 0; i < sizeof(opcodes) / sizeof(*opcodes); ++i) {
        setup(opcodes[i], 2, 5);
        PcPortFieldClipControl state = state_for((uint8_t *)fixture.script);
        compare(&state, "representative");
    }
    if (failures) {
        fprintf(stderr, "CLIP TRACK FAIL %u/%u representative cases\n", failures, cases);
        return 1;
    }
    argument_cases();
    slot_cases();
    stream_alias_cases();
    if (failures) {
        fprintf(stderr, "CLIP TRACK FAIL %u/%u retail cases\n", failures, cases);
        return 1;
    }
    printf("CLIP TRACK PASS %u retail handler/working-state/fixture/call cases; dependencies recorded, opcode 25 GTE excluded\n", cases);
    return 0;
}
