#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battle_mips_adapter.h"

/* Hide game.h's undersized typed declaration in this fixture translation
 * unit.  The production object still refers to the real g_GameState symbol;
 * this definition gives it the native port's actual 0x4600-byte storage. */
#define g_GameState g_GameState_header_declaration_only
#include "common.h"
#include "system/menu.h"
#undef g_GameState

#define RAM_BASE 0x80000000u
#define RAM_SIZE 0x00200000u
#define SWAP_PC 0x801cab48u
#define HELPER_PC 0x801c5018u
#define HELPER_RETURN_CURRENT 0x801cabccu
#define HELPER_RETURN_BENCH 0x801cac0cu
#define HALT_PC 0x80001000u
#define MENU_PTR_ADDR 0x800625a0u
#define GAME_STATE_ADDR 0x8006d634u
#define GAME_MASK_OFFSET 0x2318u
#define GUEST_MENU_ADDR 0x800d0000u
#define GUEST_MANAGER_ADDR 0x800d3000u
#define GUEST_MANAGER_PTR_OFFSET 0x033cu
#define GUEST_PARTY_OFFSET 0x0030u
#define GUEST_BENCH_OFFSET 0x1e14u
#define GUEST_MENU_SIZE 0x1e98u
#define GUEST_MANAGER_SIZE 0x006cu
#define MASK_TABLE_ADDR 0x801cb57cu
#define STACK_ADDR 0x801ff000u
#define BENCH_COUNT 11u
#define TRACE_MAX 2u

extern unsigned char MemberChangeMenuSwapCharacters(unsigned char benched,
    int current, int offset, unsigned char selectedBenched,
    int selected, int selectedOffset);
extern unsigned short MemberChangeMenuIsCharacterFlagSetActual(
    unsigned short value, unsigned char maskIndex);

typedef union NativeGameStorage {
    uint64_t alignment;
    unsigned char bytes[0x4600];
} NativeGameStorage;

NativeGameStorage g_GameState;
SystemMenu *g_Menu;
unsigned short D_801CB57C[16];

typedef struct FlagCall {
    uint16_t value;
    uint8_t index;
    uint16_t result;
} FlagCall;

typedef struct CaseInput {
    uint8_t benched;
    int32_t current;
    int32_t offset;
    uint8_t selected_benched;
    int32_t selected;
    int32_t selected_offset;
    uint16_t mask;
    uint8_t party[3];
    uint8_t bench[BENCH_COUNT];
    uint8_t adversarial_rebind;
} CaseInput;

typedef struct SideResult {
    uint8_t result;
    uint8_t party[3];
    uint8_t bench[BENCH_COUNT];
    FlagCall calls[TRACE_MAX];
    unsigned call_count;
} SideResult;

typedef struct RawContext {
    unsigned char ram[RAM_SIZE];
    FlagCall calls[TRACE_MAX];
    unsigned call_count;
    int adversarial_rebind;
    int failed;
} RawContext;

static SystemMenu native_menu;
static MenuManager native_manager;
static FlagCall native_calls[TRACE_MAX];
static unsigned native_call_count;
static int native_adversarial_rebind;
static unsigned total_cases;
static unsigned failed_cases;

static void fail_case(const char *what, unsigned case_id)
{
    if (failed_cases < 20) {
        fprintf(stderr, "MEMBER_CHANGE_SWAP_RETAIL_FAIL case=%u %s\n",
                case_id, what);
    } else if (failed_cases == 20) {
        fprintf(stderr,
                "MEMBER_CHANGE_SWAP_RETAIL_FAIL further case details suppressed\n");
    }
    failed_cases++;
}

static void write_le(unsigned char *p, unsigned width, uint32_t value)
{
    unsigned i;
    for (i = 0; i < width; i++)
        p[i] = (unsigned char)(value >> (i * 8u));
}

static uint32_t read_le(const unsigned char *p, unsigned width)
{
    uint32_t value = 0;
    unsigned i;
    for (i = 0; i < width; i++)
        value |= (uint32_t)p[i] << (i * 8u);
    return value;
}

static unsigned char *raw_pointer(RawContext *ctx, uint32_t address,
                                  unsigned width)
{
    uint32_t offset;
    if (address < RAM_BASE)
        return NULL;
    offset = address - RAM_BASE;
    if (offset > RAM_SIZE || width > RAM_SIZE - offset)
        return NULL;
    return ctx->ram + offset;
}

static int raw_read(void *opaque, uint32_t address, unsigned width,
                    uint32_t *value)
{
    RawContext *ctx = opaque;
    unsigned char *p = raw_pointer(ctx, address, width);
    if (p == NULL)
        return -1;
    *value = read_le(p, width);
    return 0;
}

static int raw_write(void *opaque, uint32_t address, unsigned width,
                     uint32_t value)
{
    RawContext *ctx = opaque;
    unsigned char *p = raw_pointer(ctx, address, width);
    if (p == NULL)
        return -1;
    write_le(p, width, value);
    return 0;
}

static void raw_apply_adversarial_rebind(RawContext *ctx)
{
    unsigned char *manager = raw_pointer(ctx, GUEST_MANAGER_ADDR,
                                         GUEST_MANAGER_SIZE);
    unsigned char *menu = raw_pointer(ctx, GUEST_MENU_ADDR, GUEST_MENU_SIZE);
    uint8_t party_index;
    uint8_t bench_index;
    uint32_t manager_pointer;

    if (manager == NULL || menu == NULL) {
        ctx->failed = 1;
        return;
    }
    manager_pointer = read_le(menu + GUEST_MANAGER_PTR_OFFSET, 4);
    if (manager_pointer != GUEST_MANAGER_ADDR) {
        ctx->failed = 1;
        return;
    }
    /* The selected indices were saved in otherwise unused bytes by the case
     * setup.  Rebinding after the second helper returns makes the normally
     * unreachable no-party rollback path observable without altering the
     * function under test. */
    party_index = menu[0x1e1f];
    bench_index = manager[0x5c];
    manager[GUEST_PARTY_OFFSET + 0] = 0xff;
    manager[GUEST_PARTY_OFFSET + 1] = 0xff;
    manager[GUEST_PARTY_OFFSET + 2] = 0xff;
    manager[GUEST_PARTY_OFFSET + party_index] = 3;
    menu[GUEST_BENCH_OFFSET + bench_index] = 0xff;
}

static int raw_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    RawContext *ctx = opaque;
    if (target == HELPER_PC) {
        if (ctx->call_count >= TRACE_MAX) {
            ctx->failed = 1;
            return -1;
        }
        ctx->calls[ctx->call_count].value = (uint16_t)cpu->gpr[4];
        ctx->calls[ctx->call_count].index = (uint8_t)cpu->gpr[5];
        ctx->call_count++;
    } else if (target == HELPER_RETURN_CURRENT ||
               target == HELPER_RETURN_BENCH) {
        if (ctx->call_count == 0) {
            ctx->failed = 1;
            return -1;
        }
        ctx->calls[ctx->call_count - 1].result = (uint16_t)cpu->gpr[2];
        if (ctx->adversarial_rebind && ctx->call_count == 2)
            raw_apply_adversarial_rebind(ctx);
    }
    return 0;
}

unsigned short MemberChangeMenuIsCharacterFlagSet(
    unsigned short value, unsigned char mask_index)
{
    unsigned short result;
    if (native_call_count >= TRACE_MAX) {
        fprintf(stderr, "MEMBER_CHANGE_SWAP_RETAIL_FAIL native helper overflow\n");
        abort();
    }
    native_calls[native_call_count].value = value;
    native_calls[native_call_count].index = mask_index;
    result = MemberChangeMenuIsCharacterFlagSetActual(value, mask_index);
    native_calls[native_call_count].result = result;
    native_call_count++;
    if (native_adversarial_rebind && native_call_count == 2) {
        uint8_t party_index = native_menu.unk1E1F;
        uint8_t bench_index = native_manager.unk5C[0];
        memset(native_manager.currentCharacterIDs, 0xff,
               sizeof(native_manager.currentCharacterIDs));
        native_manager.currentCharacterIDs[party_index] = 3;
        native_menu.unk1E14[bench_index] = 0xff;
    }
    return result;
}

static int load_slice(const char *path, unsigned char *destination,
                      size_t expected_size)
{
    FILE *stream = fopen(path, "rb");
    size_t size;
    if (stream == NULL)
        return -1;
    size = fread(destination, 1, expected_size, stream);
    if (size != expected_size || fgetc(stream) != EOF || fclose(stream) != 0)
        return -1;
    return 0;
}

static int run_raw_helper(RawContext *ctx, uint16_t value, uint8_t index,
                          uint16_t *result)
{
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;
    int rc;
    memset(&bus, 0, sizeof(bus));
    bus.opaque = ctx;
    bus.read = raw_read;
    bus.write = raw_write;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = value;
    cpu.gpr[5] = index;
    cpu.gpr[29] = STACK_ADDR;
    cpu.gpr[31] = HALT_PC;
    rc = PcPortMipsRun(&cpu, HELPER_PC, HALT_PC, 100);
    if (rc != PC_PORT_MIPS_HALTED)
        return -1;
    *result = (uint16_t)cpu.gpr[2];
    return 0;
}

static int verify_helper_oracle(const char *helper_path, const char *masks_path)
{
    RawContext ctx;
    static const uint16_t values[] = {
        0x0000, 0xffff, 0xa5a5, 0x5a5a, 0x8001
    };
    unsigned index;
    unsigned v;
    memset(&ctx, 0, sizeof(ctx));
    if (load_slice(helper_path, raw_pointer(&ctx, HELPER_PC, 0x1c), 0x1c) ||
        load_slice(masks_path, raw_pointer(&ctx, MASK_TABLE_ADDR, 0x20), 0x20))
        return -1;
    memcpy(D_801CB57C, raw_pointer(&ctx, MASK_TABLE_ADDR, 0x20), 0x20);
    for (index = 0; index < 16; index++) {
        for (v = 0; v < sizeof(values) / sizeof(values[0]); v++) {
            uint16_t raw_result;
            uint16_t native_result;
            if (run_raw_helper(&ctx, values[v], (uint8_t)index, &raw_result))
                return -1;
            native_result = MemberChangeMenuIsCharacterFlagSetActual(
                values[v], (uint8_t)index);
            if (raw_result != native_result)
                return -1;
        }
    }
    return 0;
}

static void derive_indices(const CaseInput *input, uint8_t *party_index,
                           uint8_t *bench_index)
{
    if (!input->benched) {
        *party_index = (uint8_t)input->current;
        *bench_index = (uint8_t)(input->selected + input->selected_offset);
    } else {
        *party_index = (uint8_t)input->selected;
        *bench_index = (uint8_t)(input->current + input->offset);
    }
}

static int run_raw_swap(const char *swap_path, const char *helper_path,
                        const char *masks_path, const CaseInput *input,
                        SideResult *result, unsigned case_id)
{
    RawContext ctx;
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;
    unsigned char menu_before[GUEST_MENU_SIZE];
    unsigned char manager_before[GUEST_MANAGER_SIZE];
    unsigned char *menu;
    unsigned char *manager;
    unsigned char *stack;
    uint8_t party_index;
    uint8_t bench_index;
    unsigned i;
    int rc;

    memset(&ctx, 0, sizeof(ctx));
    if (load_slice(swap_path, raw_pointer(&ctx, SWAP_PC, 0x1cc), 0x1cc) ||
        load_slice(helper_path, raw_pointer(&ctx, HELPER_PC, 0x1c), 0x1c) ||
        load_slice(masks_path, raw_pointer(&ctx, MASK_TABLE_ADDR, 0x20), 0x20))
        return -1;
    menu = raw_pointer(&ctx, GUEST_MENU_ADDR, GUEST_MENU_SIZE);
    manager = raw_pointer(&ctx, GUEST_MANAGER_ADDR, GUEST_MANAGER_SIZE);
    stack = raw_pointer(&ctx, STACK_ADDR, 0x20);
    if (menu == NULL || manager == NULL || stack == NULL)
        return -1;
    memset(menu, 0xa5, GUEST_MENU_SIZE);
    memset(manager, 0x5a, GUEST_MANAGER_SIZE);
    write_le(raw_pointer(&ctx, MENU_PTR_ADDR, 4), 4, GUEST_MENU_ADDR);
    write_le(menu + GUEST_MANAGER_PTR_OFFSET, 4, GUEST_MANAGER_ADDR);
    memcpy(manager + GUEST_PARTY_OFFSET, input->party, 3);
    memcpy(menu + GUEST_BENCH_OFFSET, input->bench, BENCH_COUNT);
    write_le(raw_pointer(&ctx, GAME_STATE_ADDR + GAME_MASK_OFFSET, 2), 2,
             input->mask);
    derive_indices(input, &party_index, &bench_index);
    menu[0x1e1f] = party_index;
    manager[0x5c] = bench_index;
    memcpy(menu_before, menu, sizeof(menu_before));
    memcpy(manager_before, manager, sizeof(manager_before));

    memset(&bus, 0, sizeof(bus));
    bus.opaque = &ctx;
    bus.read = raw_read;
    bus.write = raw_write;
    bus.bridge = raw_bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = input->benched;
    cpu.gpr[5] = (uint32_t)input->current;
    cpu.gpr[6] = (uint32_t)input->offset;
    cpu.gpr[7] = input->selected_benched;
    cpu.gpr[29] = STACK_ADDR;
    cpu.gpr[31] = HALT_PC;
    write_le(stack + 16, 4, (uint32_t)input->selected);
    write_le(stack + 20, 4, (uint32_t)input->selected_offset);
    ctx.adversarial_rebind = input->adversarial_rebind;
    rc = PcPortMipsRun(&cpu, SWAP_PC, HALT_PC, 1000);
    if (rc != PC_PORT_MIPS_HALTED || ctx.failed) {
        fprintf(stderr,
            "MEMBER_CHANGE_SWAP_RETAIL_FAIL case=%u raw rc=%d pc=%08x err=%s\n",
            case_id, rc, cpu.pc, cpu.error);
        return -1;
    }
    result->result = (uint8_t)cpu.gpr[2];
    memcpy(result->party, manager + GUEST_PARTY_OFFSET, 3);
    memcpy(result->bench, menu + GUEST_BENCH_OFFSET, BENCH_COUNT);
    result->call_count = ctx.call_count;
    memcpy(result->calls, ctx.calls, sizeof(result->calls));

    /* Retail may change only the two character arrays.  The fixture's two
     * adversarial bookkeeping bytes are restored before this whole-region
     * write-set check. */
    menu[0x1e1f] = menu_before[0x1e1f];
    manager[0x5c] = manager_before[0x5c];
    for (i = 0; i < GUEST_MENU_SIZE; i++) {
        if (i >= GUEST_BENCH_OFFSET && i < GUEST_BENCH_OFFSET + BENCH_COUNT)
            continue;
        if (menu[i] != menu_before[i])
            return -1;
    }
    for (i = 0; i < GUEST_MANAGER_SIZE; i++) {
        if (i >= GUEST_PARTY_OFFSET && i < GUEST_PARTY_OFFSET + 3)
            continue;
        if (manager[i] != manager_before[i])
            return -1;
    }
    return 0;
}

static int run_native_swap(const CaseInput *input, SideResult *result)
{
    SystemMenu expected_menu;
    MenuManager expected_manager;
    NativeGameStorage game_before;
    uint8_t party_index;
    uint8_t bench_index;

    memset(&native_menu, 0xa5, sizeof(native_menu));
    memset(&native_manager, 0x5a, sizeof(native_manager));
    memset(&g_GameState, 0xc3, sizeof(g_GameState));
    g_Menu = &native_menu;
    native_menu.pManager = &native_manager;
    memcpy(native_manager.currentCharacterIDs, input->party, 3);
    memcpy(native_menu.unk1E14, input->bench, BENCH_COUNT);
    memcpy(g_GameState.bytes + GAME_MASK_OFFSET, &input->mask,
           sizeof(input->mask));
    derive_indices(input, &party_index, &bench_index);
    native_menu.unk1E1F = party_index;
    native_manager.unk5C[0] = bench_index;
    native_call_count = 0;
    memset(native_calls, 0, sizeof(native_calls));
    native_adversarial_rebind = input->adversarial_rebind;
    memcpy(&expected_menu, &native_menu, sizeof(expected_menu));
    memcpy(&expected_manager, &native_manager, sizeof(expected_manager));
    memcpy(&game_before, &g_GameState, sizeof(game_before));

    result->result = MemberChangeMenuSwapCharacters(input->benched,
        input->current, input->offset, input->selected_benched,
        input->selected, input->selected_offset);
    memcpy(result->party, native_manager.currentCharacterIDs, 3);
    memcpy(result->bench, native_menu.unk1E14, BENCH_COUNT);
    result->call_count = native_call_count;
    memcpy(result->calls, native_calls, sizeof(result->calls));

    memcpy(expected_manager.currentCharacterIDs, result->party, 3);
    memcpy(expected_menu.unk1E14, result->bench, BENCH_COUNT);
    if (input->adversarial_rebind) {
        expected_menu.unk1E1F = native_menu.unk1E1F;
        expected_manager.unk5C[0] = native_manager.unk5C[0];
    }
    if (memcmp(&native_menu, &expected_menu, sizeof(native_menu)) != 0 ||
        memcmp(&native_manager, &expected_manager, sizeof(native_manager)) != 0 ||
        memcmp(&g_GameState, &game_before, sizeof(g_GameState)) != 0 ||
        g_Menu != &native_menu)
        return -1;
    return 0;
}

static void run_case(const char *swap_path, const char *helper_path,
                     const char *masks_path, const CaseInput *input)
{
    SideResult raw;
    SideResult native;
    unsigned id = total_cases++;
    memset(&raw, 0, sizeof(raw));
    memset(&native, 0, sizeof(native));
    if (run_raw_swap(swap_path, helper_path, masks_path, input, &raw, id)) {
        fail_case("raw execution or write-set", id);
        return;
    }
    if (run_native_swap(input, &native)) {
        fail_case("native whole-object write-set", id);
        return;
    }
    if (raw.result != native.result ||
        memcmp(raw.party, native.party, sizeof(raw.party)) != 0 ||
        memcmp(raw.bench, native.bench, sizeof(raw.bench)) != 0) {
        fail_case("result/party/bench differential", id);
        return;
    }
    if (raw.call_count != native.call_count ||
        memcmp(raw.calls, native.calls,
               raw.call_count * sizeof(raw.calls[0])) != 0) {
        fail_case("flag-helper order differential", id);
        return;
    }
}

static CaseInput make_case(unsigned benched, unsigned party_index,
                           unsigned bench_index, unsigned selected_noise,
                           uint16_t mask)
{
    CaseInput input;
    unsigned i;
    memset(&input, 0, sizeof(input));
    input.benched = (uint8_t)benched;
    input.selected_benched = (uint8_t)selected_noise;
    input.mask = mask;
    for (i = 0; i < 3; i++)
        input.party[i] = (uint8_t)(i + 1);
    for (i = 0; i < BENCH_COUNT; i++)
        input.bench[i] = (uint8_t)(10 - i);
    input.party[party_index] = (uint8_t)(party_index + 1);
    input.bench[bench_index] = (uint8_t)(10 - bench_index);
    if (!benched) {
        input.current = 0x100 + (int32_t)party_index;
        input.selected = 0x100 + (int32_t)bench_index - 3;
        input.selected_offset = 3;
        input.offset = -0x234567;
    } else {
        input.selected = 0x100 + (int32_t)party_index;
        input.current = 0x100 + (int32_t)bench_index - 4;
        input.offset = 4;
        input.selected_offset = 0x345678;
    }
    return input;
}

static void run_matrix(const char *swap_path, const char *helper_path,
                       const char *masks_path)
{
    static const uint8_t noise[] = {0, 1, 0xff};
    unsigned benched;
    unsigned party;
    unsigned bench;
    unsigned n;
    for (benched = 0; benched < 2; benched++) {
        for (party = 0; party < 3; party++) {
            for (bench = 0; bench < BENCH_COUNT; bench++) {
                for (n = 0; n < sizeof(noise); n++) {
                    CaseInput base = make_case(benched, party, bench,
                                               noise[n], 0);
                    uint16_t current_bit = (uint16_t)(1u << base.party[party]);
                    uint16_t bench_bit = (uint16_t)(1u << base.bench[bench]);
                    uint16_t masks[] = {0, current_bit, bench_bit,
                                        (uint16_t)(current_bit | bench_bit),
                                        0xa5a5};
                    unsigned m;
                    for (m = 0; m < sizeof(masks) / sizeof(masks[0]); m++) {
                        CaseInput input = base;
                        input.mask = masks[m];
                        run_case(swap_path, helper_path, masks_path, &input);
                    }
                    base.party[party] = 0xff;
                    run_case(swap_path, helper_path, masks_path, &base);
                    base = make_case(benched, party, bench, noise[n], 0);
                    base.bench[bench] = 0xff;
                    run_case(swap_path, helper_path, masks_path, &base);
                }
            }
        }
    }
}

static void run_rollback_boundary(const char *swap_path,
                                  const char *helper_path,
                                  const char *masks_path)
{
    unsigned benched;
    for (benched = 0; benched < 2; benched++) {
        CaseInput input = make_case(benched, 0, 0, 0x7d, 0);
        input.party[1] = 0xff;
        input.party[2] = 0xff;
        input.adversarial_rebind = 1;
        run_case(swap_path, helper_path, masks_path, &input);
    }
}

static void run_third_party_slot_boundary(const char *swap_path,
                                          const char *helper_path,
                                          const char *masks_path)
{
    CaseInput input = make_case(0, 2, 0, 0x39, 0);
    input.party[0] = 0xff;
    input.party[1] = 0xff;
    run_case(swap_path, helper_path, masks_path, &input);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr,
            "MEMBER_CHANGE_SWAP_RETAIL_FAIL usage: swap helper masks\n");
        return 2;
    }
    if (verify_helper_oracle(argv[2], argv[3]) != 0) {
        fprintf(stderr,
            "MEMBER_CHANGE_SWAP_RETAIL_FAIL actual/raw flag helper\n");
        return 1;
    }
    run_matrix(argv[1], argv[2], argv[3]);
    run_rollback_boundary(argv[1], argv[2], argv[3]);
    run_third_party_slot_boundary(argv[1], argv[2], argv[3]);
    if (failed_cases != 0) {
        fprintf(stderr,
            "MEMBER_CHANGE_SWAP_RETAIL_FAIL cases=%u failures=%u\n",
            total_cases, failed_cases);
        return 1;
    }
    printf("MEMBER_CHANGE_SWAP_RETAIL_PASS cases=%u helper_cases=80 "
           "rollback_adversarial=2\n", total_cases);
    return 0;
}
