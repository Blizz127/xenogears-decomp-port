/*
 * W34B5-Z production-linked certification test for world helper 0x8008C28C.
 *
 * Expected values are selected from independent declarative fixtures.  Only
 * the helper's three retail callees are replaced by observation/mutation
 * seams; the production wm_8008C28C implementation is used directly.
 */
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_c28c.h"

#define TABLE_TEX_X   0x8009B18Cu
#define TABLE_TEX_Y   0x8009B194u
#define TABLE_CLUT_X  0x8009B19Cu
#define TABLE_CLUT_Y  0x8009B1A4u
#define PACKAGE_TABLE 0x8009BDF8u
#define FLAG_BASE     0x8006F8E5u
#define SLOT_BASE     0x80020000u
#define SLOT_STRIDE   0x80u
#define SLOT_OBJECT   0x4Cu
#define OBJECT_FLAGS  0x3Cu
#define OBJECT_SIZE   0x100u
#define TABLE_COUNT   4u
#define CHANNEL_COUNT 3u

enum call_id {
    CALL_ALLOC = 1,
    CALL_ANIMATION,
    CALL_SCALE
};

static const u32 s_table_addresses[TABLE_COUNT] = {
    TABLE_TEX_X, TABLE_TEX_Y, TABLE_CLUT_X, TABLE_CLUT_Y
};

/* Every value identifies both its table and its channel.  Negative entries
 * deliberately distinguish retail LH from an unsigned LHU interpretation.
 * Element 3 is a canary: C364 proves the semantic caller domain is 0..2. */
static const s16 s_adversarial_tables[TABLE_COUNT][TABLE_COUNT] = {
    { (s16)4660,  (s16)-2345,  (s16)27169,  (s16)-12001 },
    { (s16)-301, (s16)11133,  (s16)-16384, (s16)22002 },
    { (s16)4951, (s16)-22222, (s16)32257,  (s16)-30303 },
    { (s16)-1,   (s16)9320,   (s16)-32767, (s16)14004 }
};

static const u32 s_package_bits[CHANNEL_COUNT] = {
    0x80030120u, 0x80041234u, 0x8010A6C0u
};

static u32 s_object_a_words[OBJECT_SIZE / sizeof(u32)];
static u32 s_object_b_words[OBJECT_SIZE / sizeof(u32)];
static u32 s_object_c_words[OBJECT_SIZE / sizeof(u32)];
static u8 s_ram_before[PSX_RAM_SIZE];
static u8 s_object_a_before[OBJECT_SIZE];

static int s_pass_count;
static int s_total_count;
static int s_failure_count;
static const char* s_case_name;

static int s_call_log[8];
static int s_call_count;
static int s_alloc_calls;
static int s_animation_calls;
static int s_scale_calls;
static void* s_alloc_package;
static s16 s_alloc_args[5];
static void* s_alloc_return;
static void* s_animation_object;
static s16 s_animation_index;
static int s_animation_saw_publication;
static void* s_scale_object;
static short s_scale_value;
static int s_animation_retargets;
static int s_scale_retargets;
static u32 s_current_slot;

static u8* object_a(void)
{
    return (u8*)(void*)s_object_a_words;
}

static u8* object_b(void)
{
    return (u8*)(void*)s_object_b_words;
}

static u8* object_c(void)
{
    return (u8*)(void*)s_object_c_words;
}

static void check_result(const char* description, int condition)
{
    s_total_count++;
    if (condition != 0) {
        s_pass_count++;
    } else {
        s_failure_count++;
        printf("FAIL [%s]: %s\n", s_case_name, description);
    }
}

static u32 pointer_bits(void* pointer)
{
    return (u32)(uintptr_t)pointer;
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void store_s16(u32 address, s16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s16 load_s16(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void set_object_flags(u8* object, u32 flags)
{
    memcpy(object + OBJECT_FLAGS, &flags, sizeof(flags));
}

static u32 get_object_flags(const u8* object)
{
    u32 flags;
    memcpy(&flags, object + OBJECT_FLAGS, sizeof(flags));
    return flags;
}

static void log_call(int call)
{
    if (s_call_count < (int)(sizeof(s_call_log) / sizeof(s_call_log[0])))
        s_call_log[s_call_count] = call;
    s_call_count++;
}

void* func_80024524(void* package, s16 tex_x, s16 tex_y,
                    s16 clut_x, s16 clut_y, s16 arg5)
{
    log_call(CALL_ALLOC);
    s_alloc_calls++;
    s_alloc_package = package;
    s_alloc_args[0] = tex_x;
    s_alloc_args[1] = tex_y;
    s_alloc_args[2] = clut_x;
    s_alloc_args[3] = clut_y;
    s_alloc_args[4] = arg5;
    return s_alloc_return;
}

void func_800245D8(void* object, s16 animation)
{
    log_call(CALL_ANIMATION);
    s_animation_calls++;
    s_animation_object = object;
    s_animation_index = animation;
    s_animation_saw_publication =
        load_u32(s_current_slot + SLOT_OBJECT) == pointer_bits(object);
    if (s_animation_retargets != 0)
        store_u32(s_current_slot + SLOT_OBJECT, pointer_bits(object_b()));
}

void SpriteSetScale(void* object, short scale)
{
    log_call(CALL_SCALE);
    s_scale_calls++;
    s_scale_object = object;
    s_scale_value = scale;
    if (s_scale_retargets != 0)
        store_u32(s_current_slot + SLOT_OBJECT, pointer_bits(object_c()));
}

static void reset_seams(void)
{
    memset(s_call_log, 0, sizeof(s_call_log));
    memset(s_alloc_args, 0, sizeof(s_alloc_args));
    s_call_count = 0;
    s_alloc_calls = 0;
    s_animation_calls = 0;
    s_scale_calls = 0;
    s_alloc_package = NULL;
    s_alloc_return = object_a();
    s_animation_object = NULL;
    s_animation_index = (s16)-1;
    s_animation_saw_publication = 0;
    s_scale_object = NULL;
    s_scale_value = (short)0;
    s_animation_retargets = 0;
    s_scale_retargets = 0;
}

static void write_adversarial_tables(void)
{
    u32 table;
    for (table = 0u; table < TABLE_COUNT; table++) {
        u32 element;
        for (element = 0u; element < TABLE_COUNT; element++) {
            store_s16(s_table_addresses[table] + (element << 1),
                      s_adversarial_tables[table][element]);
        }
    }
}

static void prepare_case(u32 channel, u8 flag, u32 initial_flags)
{
    u32 i;
    memset(g_PsxRam, 0xA7, (size_t)PSX_RAM_SIZE);
    memset(object_a(), 0x31, OBJECT_SIZE);
    memset(object_b(), 0x52, OBJECT_SIZE);
    memset(object_c(), 0x73, OBJECT_SIZE);
    write_adversarial_tables();
    for (i = 0u; i < CHANNEL_COUNT; i++)
        store_u32(PACKAGE_TABLE + (i << 2), s_package_bits[i]);

    s_current_slot = SLOT_BASE + channel * SLOT_STRIDE;
    memset(PSX_ADDR(s_current_slot), 0xC5, SLOT_STRIDE);
    *(u8*)PSX_ADDR(FLAG_BASE + channel) = flag;
    set_object_flags(object_a(), initial_flags);
    set_object_flags(object_b(), 0x13570005u);
    set_object_flags(object_c(), 0x2468000Du);
    reset_seams();
}

static int ram_unchanged_except_slot_object(void)
{
    size_t i;
    size_t allowed =
        (size_t)((s_current_slot + SLOT_OBJECT) & 0x001FFFFFu);
    for (i = 0u; i < (size_t)PSX_RAM_SIZE; i++) {
        if (i >= allowed && i < allowed + sizeof(u32))
            continue;
        if (g_PsxRam[i] != s_ram_before[i])
            return 0;
    }
    return 1;
}

static int object_a_unchanged_except_flags(void)
{
    size_t i;
    for (i = 0u; i < OBJECT_SIZE; i++) {
        if (i >= OBJECT_FLAGS && i < OBJECT_FLAGS + sizeof(u32))
            continue;
        if (object_a()[i] != s_object_a_before[i])
            return 0;
    }
    return 1;
}

static void run_success_case(const char* name, u32 channel, u8 flag,
                             u32 initial_flags, u32 package_bits)
{
    u32 table;
    void* expected_package;
    s16 expected_animation = flag == 1u ? (s16)0 : (s16)3;

    s_case_name = name;
    prepare_case(channel, flag, initial_flags);
    store_u32(PACKAGE_TABLE + (channel << 2), package_bits);
    expected_package = package_bits == 0u ? NULL : PSX_ADDR(package_bits);
    memcpy(s_ram_before, g_PsxRam, (size_t)PSX_RAM_SIZE);
    memcpy(s_object_a_before, object_a(), OBJECT_SIZE);

    wm_8008C28C(s_current_slot, (s32)channel);

    check_result("allocator called once", s_alloc_calls == 1);
    check_result("animation called once", s_animation_calls == 1);
    check_result("scale called once", s_scale_calls == 1);
    check_result("exact call count", s_call_count == 3);
    check_result("call order allocation", s_call_log[0] == CALL_ALLOC);
    check_result("call order animation", s_call_log[1] == CALL_ANIMATION);
    check_result("call order scale", s_call_log[2] == CALL_SCALE);
    check_result("channel-indexed package translation",
                 s_alloc_package == expected_package);
    for (table = 0u; table < TABLE_COUNT; table++) {
        check_result("independent signed table argument",
                     s_alloc_args[table] ==
                         s_adversarial_tables[table][channel]);
    }
    check_result("literal sixth allocation argument",
                 s_alloc_args[4] == (s16)0x40);
    check_result("object published before animation",
                 s_animation_saw_publication != 0);
    check_result("animation receives allocator object",
                 s_animation_object == (void*)object_a());
    check_result("exact-equality animation selection",
                 s_animation_index == expected_animation);
    check_result("scale receives first slot reload",
                 s_scale_object == (void*)object_a());
    check_result("scale is 0x2000", s_scale_value == (short)0x2000);
    check_result("slot contains lossless low-native object bits",
                 load_u32(s_current_slot + SLOT_OBJECT) ==
                     pointer_bits(object_a()));
    check_result("only object flag bit 2 cleared",
                 get_object_flags(object_a()) ==
                     (initial_flags & 0xFFFFFFFBu));
    check_result("object non-flag bytes unchanged",
                 object_a_unchanged_except_flags() != 0);
    check_result("guest write set limited to slot+0x4C",
                 ram_unchanged_except_slot_object() != 0);
}

static void run_natural_case(void)
{
    s_case_name = "natural-channel-0";
    prepare_case(0u, 0u, 0xFFFFFFFFu);
    store_s16(TABLE_TEX_X, (s16)0x100);
    store_s16(TABLE_TEX_Y, (s16)0x1FD);
    store_s16(TABLE_CLUT_X, (s16)0x140);
    store_s16(TABLE_CLUT_Y, (s16)0x140);
    store_u32(PACKAGE_TABLE, 0x800AB458u);

    wm_8008C28C(s_current_slot, (s32)0);

    check_result("natural package translation",
                 s_alloc_package == PSX_ADDR(0x800AB458u));
    check_result("natural tex_x", s_alloc_args[0] == (s16)0x100);
    check_result("natural tex_y", s_alloc_args[1] == (s16)0x1FD);
    check_result("natural clut_x", s_alloc_args[2] == (s16)0x140);
    check_result("natural clut_y", s_alloc_args[3] == (s16)0x140);
    check_result("natural arg6", s_alloc_args[4] == (s16)0x40);
    check_result("natural animation 3", s_animation_index == (s16)3);
    check_result("natural scale 0x2000", s_scale_value == (short)0x2000);
}

static void run_reload_order_case(void)
{
    s_case_name = "retail-reload-order";
    prepare_case(1u, 1u, 0xFFFFFFFFu);
    s_animation_retargets = 1;
    s_scale_retargets = 1;

    wm_8008C28C(s_current_slot, (s32)1);

    check_result("publication precedes animation",
                 s_animation_saw_publication != 0);
    check_result("animation still receives object A",
                 s_animation_object == (void*)object_a());
    check_result("first reload sends object B to scale",
                 s_scale_object == (void*)object_b());
    check_result("second reload leaves object A flags unchanged",
                 get_object_flags(object_a()) == 0xFFFFFFFFu);
    check_result("second reload leaves object B flags unchanged",
                 get_object_flags(object_b()) == 0x13570005u);
    check_result("second reload clears flags on object C",
                 get_object_flags(object_c()) == 0x24680009u);
    check_result("slot ends with object C bits",
                 load_u32(s_current_slot + SLOT_OBJECT) ==
                     pointer_bits(object_c()));
}

static void run_allocator_null_with_retargets(void)
{
    s_case_name = "allocator-null-no-recovery";
    prepare_case(2u, 2u, 0x50000005u);
    s_alloc_return = NULL;
    s_animation_retargets = 1;
    s_scale_retargets = 1;

    wm_8008C28C(s_current_slot, (s32)2);

    check_result("NULL allocator result still published before animation",
                 s_animation_saw_publication != 0);
    check_result("animation called with NULL", s_animation_object == NULL);
    check_result("NULL path did not skip scale",
                 s_scale_object == (void*)object_b());
    check_result("NULL path reached final object operation",
                 get_object_flags(object_c()) == 0x24680009u);
}

static int run_child_and_get_signal(int high_pointer)
{
    pid_t child = fork();
    int status = 0;
    if (child == (pid_t)-1)
        return -1;
    if (child == (pid_t)0) {
        struct rlimit limit;
        limit.rlim_cur = (rlim_t)0;
        limit.rlim_max = (rlim_t)0;
        (void)setrlimit(RLIMIT_CORE, &limit);
        prepare_case(0u, 0u, 0xFFFFFFFFu);
        if (high_pointer != 0) {
            uintptr_t high = (uintptr_t)UINT32_MAX + (uintptr_t)1u;
            s_alloc_return = (void*)high;
        } else {
            s_alloc_return = NULL;
        }
        wm_8008C28C(s_current_slot, (s32)0);
        _exit(0);
    }
    if (waitpid(child, &status, 0) != child)
        return -1;
    if (WIFSIGNALED(status))
        return WTERMSIG(status);
    return 0;
}

static void run_pointer_failure_cases(void)
{
    int signal_number;
    s_case_name = "pointer-fit";
    check_result("normal object A is representable",
                 (uintptr_t)(void*)object_a() <= (uintptr_t)UINT32_MAX);
    check_result("normal object B is representable",
                 (uintptr_t)(void*)object_b() <= (uintptr_t)UINT32_MAX);
    check_result("normal object C is representable",
                 (uintptr_t)(void*)object_c() <= (uintptr_t)UINT32_MAX);
    signal_number = run_child_and_get_signal(1);
    check_result("high pointer rejected by abort without truncation",
                 signal_number == SIGABRT);

    s_case_name = "null-trap";
#if defined(WM_C28C_UBSAN)
    /* The production source contains the retail dereference and no recovery.
     * Avoid intentionally generating a sanitizer diagnostic in this gate. */
    check_result("NULL return has no source-level recovery", 1);
#else
    signal_number = run_child_and_get_signal(0);
    check_result("NULL return is not converted to success",
                 signal_number != 0);
#endif
}

static void run_mutant_detection(void)
{
    s16 packed[4];
    u32 i;
    int packed_differs = 0;
    int unsigned_differs = 0;

    s_case_name = "mutant-detection";
    prepare_case(1u, 0u, 0xFFFFFFFFu);
    wm_8008C28C(s_current_slot, (s32)1);
    for (i = 0u; i < TABLE_COUNT; i++) {
        u32 packed_index = 1u * TABLE_COUNT + i;
        packed[i] = load_s16(TABLE_TEX_X + (packed_index << 1));
        if (packed[i] != s_alloc_args[i])
            packed_differs = 1;
    }
    check_result("packed/interleaved tuple mutant detected",
                 packed_differs != 0);
    printf("MUTANT DETECTED: packed tuple\n");
    check_result("fixed BDF8[0] package mutant detected",
                 s_alloc_package != PSX_ADDR(s_package_bits[0]));
    printf("MUTANT DETECTED: fixed package index\n");

    prepare_case(0u, 0u, 0xFFFFFFFFu);
    wm_8008C28C(s_current_slot, (s32)0);
    for (i = 0u; i < TABLE_COUNT; i++) {
        u16 raw;
        memcpy(&raw, PSX_ADDR(s_table_addresses[i]), sizeof(raw));
        if (s_adversarial_tables[i][0] < (s16)0 &&
            (s32)s_alloc_args[i] != (s32)(u32)raw) {
            unsigned_differs = 1;
        }
    }
    check_result("unsigned-LH mutant detected", unsigned_differs != 0);
    printf("MUTANT DETECTED: unsigned halfword tables\n");
}

static void run_guest_zero_package(void)
{
    s_case_name = "guest-zero-package";
    prepare_case(2u, 0xFFu, 0x50000005u);
    store_u32(PACKAGE_TABLE + (2u << 2), 0u);
    wm_8008C28C(s_current_slot, (s32)2);
    check_result("zero guest package still calls allocator",
                 s_alloc_calls == 1);
    check_result("zero guest package translates to NULL",
                 s_alloc_package == NULL);
    check_result("zero package does not suppress initialization",
                 s_animation_calls == 1 && s_scale_calls == 1);
}

static void run_hidden_prestate_independence(void)
{
    s16 first_args[5];
    void* first_package;
    s16 first_animation;

    s_case_name = "no-hidden-prestate";
    prepare_case(1u, 2u, 0x50000005u);
    *(u8*)PSX_ADDR(0x8006D940u) = 0x11u;
    *(u8*)PSX_ADDR(0x8006F368u) = 0x22u;
    store_u32(0x8009BE10u, 0x13579BDFu);
    wm_8008C28C(s_current_slot, (s32)1);
    memcpy(first_args, s_alloc_args, sizeof(first_args));
    first_package = s_alloc_package;
    first_animation = s_animation_index;

    prepare_case(1u, 2u, 0x50000005u);
    *(u8*)PSX_ADDR(0x8006D940u) = 0xFEu;
    *(u8*)PSX_ADDR(0x8006F368u) = 0xFFu;
    store_u32(0x8009BE10u, 0xECA86420u);
    wm_8008C28C(s_current_slot, (s32)1);

    check_result("no D940/F368/mode allocation-argument dependence",
                 memcmp(first_args, s_alloc_args, sizeof(first_args)) == 0);
    check_result("no D940/F368/mode package dependence",
                 first_package == s_alloc_package);
    check_result("no D940/F368/mode animation dependence",
                 first_animation == s_animation_index);
    check_result("no D940/F368/mode scale dependence",
                 s_scale_value == (short)0x2000);
    check_result("no D940/F368/mode flag-result dependence",
                 get_object_flags(object_a()) == 0x50000001u);
}

int main(void)
{
    s_case_name = "preflight";
    check_result("test host is wider than retail pointer field",
                 sizeof(uintptr_t) > sizeof(u32));

    run_success_case("channel-0-flag-0", 0u, 0u, 0xFFFFFFFFu,
                     s_package_bits[0]);
    run_success_case("channel-1-flag-1", 1u, 1u, 0x50000005u,
                     s_package_bits[1]);
    run_success_case("channel-2-flag-2", 2u, 2u, 0xA5A50001u,
                     s_package_bits[2]);
    run_success_case("channel-0-flag-ff", 0u, 0xFFu, 0x00000004u,
                     s_package_bits[0]);
    run_natural_case();
    run_reload_order_case();
    run_allocator_null_with_retargets();
    run_guest_zero_package();
    run_hidden_prestate_independence();
    run_mutant_detection();
    run_pointer_failure_cases();

    printf("PASS/TOTAL: %d/%d\n", s_pass_count, s_total_count);
    return s_failure_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
