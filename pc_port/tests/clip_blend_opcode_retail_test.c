#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "field_clip_control.h"

/* Executes the pinned retail opcode 13 and its E44D4 tail through E5974.
 * E6910, DEF10, DF0B4 and E632C are explicit recording boundaries. Their
 * outputs/mutations are controlled test inputs, NOT helper fidelity proof. */
enum { STACK = 0x1ff000, MAX_CALLS = 4 };
static uint8_t ram[0x200000];
uint32_t D_801E85CC;
uint32_t D_801E8670[10];
static struct Fixture {
    uint32_t object[2][48], root[2][32], pose[2][16], pool[32];
    uint16_t script[8];
} fixture, initial, expected;
struct Working {
    uint32_t object, pool, stream, limit, ticks, running, postprocess;
    uint32_t operand, origin;
};
static struct Call {
    uint32_t target, count, args[7];
    struct Working state;
    struct Fixture memory;
    uint32_t global, slots[10];
} calls[MAX_CALLS], expected_calls[MAX_CALLS];
static struct Configuration {
    uint32_t lookup_flags, pose, lookup_mutations, global_after_lookup;
    uint32_t callback_mutations, blend_result, sentinel_result, stream;
} config;
static unsigned call_count, cases;
static const char *case_name;
static PcPortMipsCpu *active_cpu;
static PcPortFieldClipControl *active_native;

static uint32_t pointer(const void *p) { return (uint32_t)(uintptr_t)p; }
static void put16(void *p, uint16_t v) { memcpy(p, &v, sizeof(v)); }
static void put32(void *p, uint32_t v) { memcpy(p, &v, sizeof(v)); }
static uint8_t *address(uint32_t a, unsigned width)
{
    if (a >= 0x801e85ccu && (uint64_t)a + width <= 0x801e85d0u)
        return (uint8_t *)&D_801E85CC + a - 0x801e85ccu;
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
static uint32_t stack_value(unsigned offset, unsigned width)
{
    uint32_t result;
    assert(!read_bus(NULL, 0x80000000u + STACK + offset, width, &result));
    return result;
}
static struct Working snapshot(void)
{
    struct Working state = {0};
    if (active_cpu) {
        state.object = active_cpu->gpr[20];
        state.pool = stack_value(0xc8, 4);
        state.stream = active_cpu->gpr[19];
        state.limit = stack_value(0xd0, 4);
        state.ticks = stack_value(0xd8, 4);
        state.running = stack_value(0xe8, 4);
        state.postprocess = stack_value(0xf0, 4);
        state.operand = stack_value(0x6c, 2);
        state.origin = stack_value(0xe0, 4);
    } else {
        assert(active_native);
        state.object = pointer(active_native->object);
        state.pool = pointer(active_native->pool);
        state.stream = active_native->stream;
        state.limit = (uint32_t)active_native->limit;
        state.ticks = (uint32_t)active_native->ticks;
        state.running = (uint32_t)active_native->running;
        state.postprocess = (uint32_t)active_native->postprocess;
        state.operand = active_native->operand;
        state.origin = pointer(active_native->origin);
    }
    return state;
}
static void record(uint32_t target, const uint32_t *args, unsigned count)
{
    if (call_count >= MAX_CALLS || count > 7) {
        fprintf(stderr, "CLIP BLEND FAIL %s: extra callback\n", case_name);
        exit(1);
    }
    struct Call *call = &calls[call_count++];
    call->target = target;
    call->count = count;
    memcpy(call->args, args, count * sizeof(*args));
    call->state = snapshot();
    call->memory = fixture;
    call->global = D_801E85CC;
    memcpy(call->slots, D_801E8670, sizeof(call->slots));
}
static void mutate_callback(unsigned stage)
{
    if (!config.callback_mutations) return;
    /* Distinct controlled effects make skipped/reordered callbacks visible
     * in later snapshots and in the complete final fixture comparison. */
    ((uint8_t *)fixture.object[0])[0x20 + stage] ^= (uint8_t)(0x91 + stage);
    fixture.root[1][10 + stage] ^= 0x13579bdfu + stage;
    fixture.pool[4 + stage] += 0x10203u + stage;
    D_801E8670[stage] ^= 0xabcdef01u + stage;
}
uint32_t func_801E6910(uint8_t *object, int32_t selector, uint32_t *flags)
{
    /* The flags-cell machine address differs on native/retail stacks. Its
     * controlled output is recorded instead of that incomparable address. */
    const uint32_t args[] = {pointer(object), (uint32_t)selector,
                            config.lookup_flags, config.pose};
    record(0x801e6910, args, 4);
    *flags = config.lookup_flags;
    if (config.lookup_mutations & 1u)
        put32(object + 4, pointer(fixture.root[1]));
    if (config.lookup_mutations & 2u)
        D_801E85CC = config.global_after_lookup;
    if (config.lookup_mutations & 4u) {
        uint8_t *stream = address(config.stream, 6);
        assert(stream);
        put16(stream + 2, 0x3d72);
        put16(stream + 4, 0x81a9);
    }
    mutate_callback(0);
    return config.pose;
}
void func_801DEF10(uint8_t *root, uint8_t *pose)
{
    const uint32_t args[] = {pointer(root), pointer(pose)};
    record(0x801def10, args, 2);
    mutate_callback(1);
}
uint32_t func_801DF0B4(uint8_t *pool, uint8_t *root, uint8_t *pose,
                       int32_t duration, int32_t absolute, int32_t loop, int32_t tag)
{
    const uint32_t args[] = {pointer(pool), pointer(root), pointer(pose),
        (uint32_t)duration, (uint32_t)absolute, (uint32_t)loop, (uint32_t)tag};
    record(0x801df0b4, args, 7);
    mutate_callback(1);
    return config.blend_result;
}
int32_t func_801E632C(uint8_t *object)
{
    const uint32_t args[] = {pointer(object)};
    record(0x801e632c, args, 1);
    mutate_callback(2);
    return (int32_t)config.sentinel_result;
}
static void unexpected(void)
{
    fprintf(stderr, "CLIP BLEND FAIL %s: unexpected dependency\n", case_name);
    exit(1);
}
uint32_t func_801E6830(uint8_t *o, int32_t s, uint16_t *m)
{ (void)o; (void)s; (void)m; unexpected(); return 0; }
void func_801E6D94(uint8_t *o, uint8_t *n, int32_t f)
{ (void)o; (void)n; (void)f; unexpected(); }
void func_801E6974(uint8_t *o, uint8_t *p, uint8_t *n, int32_t a, int32_t b,
                   int32_t c, int32_t d, int32_t e, int32_t f, int32_t g,
                   int32_t h, int32_t i, int32_t j, int32_t k)
{ (void)o; (void)p; (void)n; (void)a; (void)b; (void)c; (void)d; (void)e;
  (void)f; (void)g; (void)h; (void)i; (void)j; (void)k; unexpected(); }
int32_t func_801DC848(uint8_t *r, int32_t s)
{ (void)r; (void)s; unexpected(); return 0; }
int32_t func_801DC5C0(uint8_t *r, int32_t s)
{ (void)r; (void)s; unexpected(); return 0; }
void func_801DFE8C(uint8_t *p, uint8_t *r)
{ (void)p; (void)r; unexpected(); }
void func_801DF52C(uint8_t *p, uint8_t *r, int32_t i, int32_t m)
{ (void)p; (void)r; (void)i; (void)m; unexpected(); }
uint32_t func_801DF7F4(uint8_t *p, uint8_t *r, uint8_t *v, int32_t l, int32_t t)
{ (void)p; (void)r; (void)v; (void)l; (void)t; unexpected(); return 0; }
void func_801E5C74(uint8_t *o, uint8_t *d, int32_t l)
{ (void)o; (void)d; (void)l; unexpected(); }

static int bridge(void *unused, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)unused;
    uint32_t *r = cpu->gpr;
    switch (target) {
    case 0x801e6910: {
        uint32_t flags;
        r[2] = func_801E6910((uint8_t *)(uintptr_t)r[4], (int32_t)r[5], &flags);
        assert(!write_bus(NULL, r[6], 4, flags));
        break;
    }
    case 0x801def10:
        func_801DEF10((uint8_t *)(uintptr_t)r[4], (uint8_t *)(uintptr_t)r[5]);
        break;
    case 0x801df0b4:
        r[2] = func_801DF0B4((uint8_t *)(uintptr_t)r[4], (uint8_t *)(uintptr_t)r[5],
            (uint8_t *)(uintptr_t)r[6], (int32_t)r[7],
            (int32_t)stack_value(0x10, 4), (int32_t)stack_value(0x14, 4),
            (int32_t)stack_value(0x18, 4));
        break;
    case 0x801e632c:
        r[2] = (uint32_t)func_801E632C((uint8_t *)(uintptr_t)r[4]);
        break;
    default:
        return target >= 0x801e3d44u && target < 0x801e5974u ? 0 : -1;
    }
    return 1;
}
static void reset_calls(void)
{ call_count = 0; memset(calls, 0, sizeof(calls)); }
static void compare(PcPortFieldClipControl state, const char *name)
{
    case_name = name;
    initial = fixture;
    uint32_t initial_global = D_801E85CC, initial_slots[10], expected_slots[10];
    memcpy(initial_slots, D_801E8670, sizeof(initial_slots));
    memset(ram + STACK, 0x36, 0x100);
    PcPortMipsBus bus = {.read = read_bus, .write = write_bus, .bridge = bridge};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[16] = cpu.gpr[19] = state.stream;
    cpu.gpr[20] = pointer(state.object);
    cpu.gpr[29] = 0x80000000u + STACK;
    put32(ram + STACK + 0xc8, pointer(state.pool));
    put32(ram + STACK + 0xd0, (uint32_t)state.limit);
    put32(ram + STACK + 0xd8, (uint32_t)state.ticks);
    put32(ram + STACK + 0xe0, pointer(state.origin));
    put32(ram + STACK + 0xe8, (uint32_t)state.running);
    put32(ram + STACK + 0xf0, (uint32_t)state.postprocess);
    reset_calls();
    active_cpu = &cpu;
    active_native = NULL;
    if (PcPortMipsRun(&cpu, 0x801e3d44, 0x801e5974, 2000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "CLIP BLEND FAIL %s: retail oracle %s\n", name, cpu.error);
        exit(1);
    }
    struct Working expected_state = snapshot();
    expected = fixture;
    uint32_t expected_global = D_801E85CC;
    memcpy(expected_slots, D_801E8670, sizeof(expected_slots));
    memcpy(expected_calls, calls, sizeof(calls));
    unsigned expected_count = call_count;
    fixture = initial;
    D_801E85CC = initial_global;
    memcpy(D_801E8670, initial_slots, sizeof(initial_slots));
    reset_calls();
    active_cpu = NULL;
    active_native = &state;
    int handled = PcPort_FieldClipDataStep(&state);
    struct Working native_state = snapshot();
    int bad = handled != 1 || memcmp(&native_state, &expected_state, sizeof(native_state)) ||
        memcmp(&fixture, &expected, sizeof(fixture)) || D_801E85CC != expected_global ||
        memcmp(D_801E8670, expected_slots, sizeof(expected_slots)) ||
        call_count != expected_count || memcmp(calls, expected_calls, sizeof(calls));
    if (bad) {
        fprintf(stderr, "CLIP BLEND FAIL %s case=%u handled=%d callbacks=%u/%u "
            "stream=%08x/%08x limit=%08x/%08x operand=%04x/%04x\n", name, cases,
            handled, call_count, expected_count, native_state.stream, expected_state.stream,
            native_state.limit, expected_state.limit, native_state.operand, expected_state.operand);
        for (unsigned i = 0; i < MAX_CALLS; ++i) {
            if (!memcmp(calls + i, expected_calls + i, sizeof(calls[i]))) continue;
            fprintf(stderr, "CLIP BLEND FAIL callback[%u] target=%08x/%08x "
                "args=%d working=%d memory=%d globals=%d\n", i,
                calls[i].target, expected_calls[i].target,
                memcmp(calls[i].args, expected_calls[i].args, sizeof(calls[i].args)) != 0,
                memcmp(&calls[i].state, &expected_calls[i].state, sizeof(calls[i].state)) != 0,
                memcmp(&calls[i].memory, &expected_calls[i].memory, sizeof(calls[i].memory)) != 0,
                calls[i].global != expected_calls[i].global ||
                memcmp(calls[i].slots, expected_calls[i].slots, sizeof(calls[i].slots)) != 0);
        }
        exit(1);
    }
    ++cases;
    active_native = NULL;
}
static PcPortFieldClipControl setup(unsigned parameter, unsigned w0, unsigned w1,
                                     unsigned seed)
{
    uint8_t *bytes = (uint8_t *)&fixture;
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        bytes[i] = (uint8_t)(i * 37u + (i >> 4) + seed * 19u);
    for (unsigned i = 0; i < 10; ++i) D_801E8670[i] = seed * 31u + i * 0x10001u;
    fixture.object[0][1] = pointer(fixture.root[0]);
    fixture.object[1][1] = pointer(fixture.root[1]);
    fixture.script[0] = (uint16_t)((parameter << 8) | 0x13);
    fixture.script[1] = (uint16_t)w0;
    fixture.script[2] = (uint16_t)w1;
    config = (struct Configuration){.pose = pointer(fixture.pose[seed & 1]),
        .blend_result = 0xcafebabe,
        .sentinel_result = 0x87654321, .stream = pointer(fixture.script)};
    D_801E85CC = 0;
    PcPortFieldClipControl state = {.object = (uint8_t *)fixture.object[0],
        .pool = (uint8_t *)fixture.pool, .stream = pointer(fixture.script),
        .limit = 0x12345678, .ticks = -17, .running = 1, .postprocess = 0x1234,
        .operand = 0xa55a, .origin = (uint8_t *)fixture.object[1]};
    return state;
}
int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *disc = fopen("disc/disc1.bin", "rb");
    assert(disc);
    for (unsigned i = 0; i < 25; ++i) {
        assert(!fseek(disc, (231361 + i) * 2352L + 24, SEEK_SET));
        assert(fread(ram + 0x1dc000 + i * 2048, 1, 2048, disc) == 2048);
    }
    assert(!fclose(disc));
    compare(setup(0xa5, 0x9ac3, 0xfed2, 0), "opcode13 reproduction");

    const uint32_t values[] = {0, 1, 0x7fffffff, 0x80000000, 0xffffffff};
    for (unsigned parameter = 0; parameter < 256; ++parameter) {
        for (unsigned i = 0; i < 5; ++i) {
            PcPortFieldClipControl state = setup(parameter, 0x80ff, 0xff80, parameter + i);
            D_801E85CC = values[i];
            config.lookup_flags = values[(i + 1) % 5];
            config.blend_result = values[(i + 3) % 5];
            config.sentinel_result = values[(i + 2) % 5];
            state.limit = (int32_t)values[(i + 3) % 5];
            state.ticks = (int32_t)values[(i + 4) % 5];
            state.running = (int32_t)values[i];
            state.postprocess = (int32_t)values[(i + 1) % 5];
            compare(state, "parameter/global/working-state edges");
        }
    }
    for (unsigned mutations = 0; mutations < 8; ++mutations) {
        for (unsigned route = 0; route < 4; ++route) {
            for (unsigned nulls = 0; nulls < 4; ++nulls) {
                for (unsigned alias = 0; alias < 5; ++alias) {
                    PcPortFieldClipControl state = setup(0x80, 0xff7f, 0x01fe,
                                                        mutations + alias + nulls);
                    uint8_t *streams[] = {(uint8_t *)fixture.script,
                        (uint8_t *)fixture.object[0] + 0x70,
                        (uint8_t *)fixture.root[0] + 0x20,
                        (uint8_t *)fixture.pose[0] + 0x10,
                        (uint8_t *)fixture.pool + 0x40};
                    memcpy(streams[alias], fixture.script, 6);
                    state.stream = config.stream = pointer(streams[alias]);
                    D_801E85CC = route & 1u ? 0x80000000u : 0;
                    config.lookup_mutations = mutations;
                    config.global_after_lookup = route & 2u ? 0xffffffffu : 0;
                    config.lookup_flags = 0xffffffff;
                    config.callback_mutations = 1;
                    if (nulls & 1u) state.pool = NULL;
                    if (nulls & 2u) config.pose = 0;
                    compare(state, "lookup ordering, aliases and controlled callback effects");
                }
            }
        }
    }
    /* Each following word is exhaustive independently; both global routes
     * and every instruction parameter are exercised with nonzero flags. */
    for (unsigned word = 0; word < 65536; ++word) {
        for (unsigned route = 0; route < 2; ++route) {
            PcPortFieldClipControl state = setup(word >> 8, word,
                (word * 313u + 0x817f) & 0xffffu, word);
            D_801E85CC = route ? 0x80000000u : 0;
            config.lookup_flags = word * 0x10001u;
            compare(state, "exhaustive selector/tag word");
            state = setup(word & 255, (word * 197u + 0x1f80) & 0xffffu, word, word + 1);
            D_801E85CC = route ? 0xffffffffu : 0;
            config.lookup_flags = word ^ 0x80000000u;
            compare(state, "exhaustive loop/duration word");
        }
    }
    printf("CLIP BLEND PASS %u retail opcode13 cases; full working/memory/global "
           "and callback-order snapshots; controlled helper boundaries only\n", cases);
    return 0;
}
