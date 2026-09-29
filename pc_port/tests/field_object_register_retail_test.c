/* Raw 99-instruction retail oracle for func_800A1364 (field object-register
 * opcode) versus the production misc6.c body; the script-argument, sprite-bind
 * and pose-reset boundaries are explicit spies on both sides. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "field/actor.h"
#include "battle_mips_adapter.h"

#define ENTRY 0x800A1364u
#define END 0x800A14F0u
#define RETAIL_SIZE (END - ENTRY)
#define INSTRS (RETAIL_SIZE / 4)
#define RAW_OFFSET 0x31874u
#define STACK 0x801FF000u
#define HALT 0xFFFFFFFCu
/* Retail globals the function touches, at their retail addresses. */
#define A_INDEX 0x800AFD1Cu
#define A_ACTORS_PTR 0x800AFB10u
#define A_SPRITE_PTR 0x800AFB1Cu
#define A_CUR_PTR 0x800B0078u
#define A_IDS 0x800B21DCu
#define A_FLAGS 0x800B225Fu
#define A_COUNT 0x800B2264u
/* Retail call targets the body reaches (spied through the bridge). */
#define T_GET_ARGUMENT 0x800ACDECu
#define T_BIND_SPRITE 0x80076AC0u
#define T_RESET_POSE 0x800A0C94u
/* Scratch placement for the pointed-to objects in the raw run. */
#define ACTORS_ADDR 0x80130000u
#define SPRITE_ADDR 0x80140000u
#define CUR_ADDR 0x80150000u
#define ACTOR_STRIDE 0x5Cu
#define ACTOR_SLOTS 54u
#define CUR_SIZE 0x138u
#define SPRITE_SIZE 0x40u
#define RED 16u
/* Retail only has room for five flag bytes before the counter itself. */
#define MAX_SLOT 4

_Static_assert(sizeof(FieldActor) == ACTOR_STRIDE, "FieldActor stride");
_Static_assert(sizeof(ActorData) == CUR_SIZE, "ActorData size");
_Static_assert(__builtin_offsetof(FieldActor, status) == 0x58, "status offset");
_Static_assert(__builtin_offsetof(ActorData, scriptInstructionPointer) == 0xCC, "ip offset");
_Static_assert(__builtin_offsetof(ActorData, flags) == 0x4, "flags offset");

extern void func_800A1364(void);
/* Native definitions of the externs the production body reads/writes. */
s32 D_800AFD1C;
#ifdef XENO_PC_PORT
FieldActor* volatile g_FieldActors;
#else
FieldActor* g_FieldActors;
#endif
void* g_FieldSpriteData;
ActorData* g_FieldScriptVMCurActor;
u16 D_800B21DC[8];
u8 D_800B225F[8];
s32 D_800B2264;

typedef struct { uint32_t id, a[7]; } Event;
typedef struct {
    Event trace[3];
    unsigned count;
    uint8_t actor[RED + ACTOR_STRIDE + RED];
    uint8_t cur[RED + CUR_SIZE + RED];
    uint16_t ids[8];
    uint8_t flags[MAX_SLOT + 1];
    int32_t counter, index;
    uint8_t sprite[SPRITE_SIZE];
} Result;

static uint8_t ram[0x200000], retail[RETAIL_SIZE];
static uint8_t actorsBuf[RED + ACTOR_SLOTS * ACTOR_STRIDE + RED], curBuf[RED + CUR_SIZE + RED], spriteBuf[RED + SPRITE_SIZE + RED];
static unsigned native_mode, cases, checks, seen[INSTRS];
static int32_t spriteId;
static PcPortMipsCpu* active_cpu;
static Result result;

static void require(int ok, const char* why) {
    checks++;
    if (!ok) { fprintf(stderr, "FIELD OBJECT REGISTER FAIL case=%u native=%u %s\n", cases, native_mode, why); exit(1); }
}
static uint8_t* ptr(uint32_t a) { return ram + (a & 0x1FFFFFu); }
static uint32_t r32(uint32_t a) { uint32_t v; memcpy(&v, ptr(a), 4); return v; }
static void w32(uint32_t a, uint32_t v) { memcpy(ptr(a), &v, 4); }
static void w16(uint32_t a, uint16_t v) { memcpy(ptr(a), &v, 2); }
static void record(uint32_t id, const uint32_t* a, unsigned n) {
    unsigned k = result.count;
    require(k < 3, "spy count");
    result.trace[k].id = id;
    for (unsigned i = 0; i < 7; i++) result.trace[k].a[i] = i < n ? a[i] : 0;
    result.count++;
}
/* Native spies. func_800A0C94 is defined in misc6.c; the runner weakens it. */
s32 FieldScriptVMGetArgument(s32 n) { uint32_t a[1] = { (uint32_t)n }; record(1, a, 1); return spriteId; }
void func_80076AC0(s32 idx, s32 a1, void* sprite, s32 a3, s32 a4, s32 a5, s32 a6) {
    uint32_t a[7] = { (uint32_t)idx, (uint32_t)a1, (uint32_t)(uintptr_t)sprite - (uint32_t)(uintptr_t)(spriteBuf + RED), (uint32_t)a3, (uint32_t)a4, (uint32_t)a5, (uint32_t)a6 };
    record(2, a, 7);
}
void func_800A0C94(void) { record(3, NULL, 0); }

static int read_bus(void* o, uint32_t a, unsigned width, uint32_t* v) {
    (void)o; if (a < 0x80000000u || (uint64_t)a + width > 0x80200000u) return -1;
    *v = 0; for (unsigned i = 0; i < width; i++) *v |= (uint32_t)ptr(a)[i] << (8 * i);
    if (active_cpu && width == 4 && a == active_cpu->pc && a >= ENTRY && a < END) seen[(a - ENTRY) / 4]++;
    return 0;
}
static int write_bus(void* o, uint32_t a, unsigned width, uint32_t v) {
    (void)o; if (a < 0x80000000u || (uint64_t)a + width > 0x80200000u) return -1;
    for (unsigned i = 0; i < width; i++) ptr(a)[i] = (uint8_t)(v >> (8 * i)); return 0;
}
static int bridge(void* o, PcPortMipsCpu* c, uint32_t target) {
    (void)o;
    if (target == T_GET_ARGUMENT) { uint32_t a[1] = { c->gpr[4] }; record(1, a, 1); c->gpr[2] = (uint32_t)spriteId; return 1; }
    if (target == T_BIND_SPRITE) {
        uint32_t sp = c->gpr[29];
        uint32_t a[7] = { c->gpr[4], c->gpr[5], c->gpr[6] - SPRITE_ADDR, c->gpr[7], r32(sp + 0x10), r32(sp + 0x14), r32(sp + 0x18) };
        require(sp == STACK - 0x30, "retail frame size 0x30 at the sprite-bind call");
        record(2, a, 7); return 1;
    }
    if (target == T_RESET_POSE) { record(3, NULL, 0); return 1; }
    return 0;
}

typedef struct { int32_t index, count, id; uint16_t status, ip; uint32_t flags0, flags4, word12C, spriteOff; uint8_t pattern; } Case;

static void snapshot(unsigned native, const Case* c) {
    if (native) {
        memcpy(result.actor, actorsBuf + c->index * ACTOR_STRIDE, RED + ACTOR_STRIDE + RED);
        memcpy(result.cur, curBuf, sizeof(curBuf));
        memcpy(result.ids, D_800B21DC, sizeof(result.ids));
        memcpy(result.flags, D_800B225F, sizeof(result.flags));
        result.counter = D_800B2264; result.index = D_800AFD1C;
        memcpy(result.sprite, spriteBuf + RED, SPRITE_SIZE);
    } else {
        memcpy(result.actor, ptr(ACTORS_ADDR - RED + c->index * ACTOR_STRIDE), RED + ACTOR_STRIDE + RED);
        memcpy(result.cur, ptr(CUR_ADDR - RED), sizeof(curBuf));
        memcpy(result.ids, ptr(A_IDS), sizeof(result.ids));
        memcpy(result.flags, ptr(A_FLAGS), sizeof(result.flags));
        result.counter = (int32_t)r32(A_COUNT); result.index = (int32_t)r32(A_INDEX);
        memcpy(result.sprite, ptr(SPRITE_ADDR), SPRITE_SIZE);
    }
}

static Result run(unsigned native, const Case* c) {
    native_mode = native; spriteId = c->id;
    memset(&result, 0, sizeof(result));
    if (native) {
        memset(actorsBuf, c->pattern, sizeof(actorsBuf)); memset(curBuf, c->pattern, sizeof(curBuf)); memset(spriteBuf, c->pattern, sizeof(spriteBuf));
        g_FieldActors = (FieldActor*)(actorsBuf + RED); g_FieldScriptVMCurActor = (ActorData*)(curBuf + RED); g_FieldSpriteData = spriteBuf + RED;
        D_800AFD1C = c->index; D_800B2264 = c->count;
        for (unsigned i = 0; i < 8; i++) D_800B21DC[i] = (uint16_t)(0x1100u + i * 0x11u);
        for (unsigned i = 0; i <= MAX_SLOT; i++) D_800B225F[i] = (uint8_t)(0x30u + i);
        g_FieldActors[c->index].status = (short)c->status;
        g_FieldScriptVMCurActor->scriptFlags.flags = c->flags0;
        g_FieldScriptVMCurActor->flags = c->flags4;
        g_FieldScriptVMCurActor->scriptInstructionPointer = c->ip;
        memcpy(curBuf + RED + 0x12C, &c->word12C, 4);
        memcpy(spriteBuf + RED + 4, &c->spriteOff, 4);
        func_800A1364();
    } else {
        memset(ram, c->pattern, sizeof(ram));
        memcpy(ptr(ENTRY), retail, RETAIL_SIZE);
        w32(A_INDEX, (uint32_t)c->index); w32(A_ACTORS_PTR, ACTORS_ADDR); w32(A_SPRITE_PTR, SPRITE_ADDR); w32(A_CUR_PTR, CUR_ADDR);
        w32(A_COUNT, (uint32_t)c->count);
        for (unsigned i = 0; i < 8; i++) w16(A_IDS + 2 * i, (uint16_t)(0x1100u + i * 0x11u));
        for (unsigned i = 0; i <= MAX_SLOT; i++) ptr(A_FLAGS)[i] = (uint8_t)(0x30u + i);
        w16(ACTORS_ADDR + c->index * ACTOR_STRIDE + 0x58, c->status);
        w32(CUR_ADDR + 0x0, c->flags0); w32(CUR_ADDR + 0x4, c->flags4); w16(CUR_ADDR + 0xCC, c->ip); w32(CUR_ADDR + 0x12C, c->word12C);
        w32(SPRITE_ADDR + 4, c->spriteOff);
        PcPortMipsBus bus = { 0 }; PcPortMipsCpu cpu;
        bus.read = read_bus; bus.write = write_bus; bus.bridge = bridge;
        PcPortMipsCpuInit(&cpu, &bus); cpu.gpr[29] = STACK; cpu.gpr[31] = HALT; active_cpu = &cpu;
        int rc = PcPortMipsRun(&cpu, ENTRY, HALT, 1000); active_cpu = NULL;
        require(rc == PC_PORT_MIPS_HALTED, "retail run halted"); require(cpu.gpr[29] == STACK, "retail stack restored");
    }
    require(result.count == 3, "three boundary calls");
    require(result.trace[0].id == 1 && result.trace[1].id == 2 && result.trace[2].id == 3, "boundary order argument/bind/pose");
    snapshot(native, c);
    return result;
}

static void check_semantics(const Result* r, const Case* c) {
    /* Independent expectations from the retail asm, checked on both sides. */
    const uint8_t* actor = r->actor + RED; const uint8_t* cur = r->cur + RED;
    uint16_t status; uint32_t f0, f4, w; uint16_t ip;
    memcpy(&status, actor + 0x58, 2); memcpy(&f0, cur, 4); memcpy(&f4, cur + 4, 4); memcpy(&ip, cur + 0xCC, 2); memcpy(&w, cur + 0x12C, 4);
    require(status == (uint16_t)(((c->status & 0xF07Fu) | 0x200u) & 0xFFDFu), "actor status mask/set/clear");
    require(ip == (uint16_t)(c->ip + 3), "IP advanced by 3");
    require(f0 == (c->flags0 | 0x100u), "scriptFlags |= 0x100");
    require(f4 == ((c->flags4 | 0x2000u) & ~0x800u), "flags |= 0x2000 & ~0x800");
    require(w == ((c->word12C & 0xFFFF1FFFu) | (((uint32_t)c->count & 7u) << 13)), "slot stamp at +0x12C");
    require(r->ids[c->count] == (uint16_t)((uint32_t)c->id << 1), "object id stored << 1");
    require(r->flags[c->count] == 0, "object flag cleared");
    require(r->counter == c->count + 1, "object count incremented");
    require(r->trace[0].a[0] == 1, "argument index 1");
    require(r->trace[1].a[0] == (uint32_t)c->index && r->trace[1].a[1] == 0 && r->trace[1].a[2] == c->spriteOff && r->trace[1].a[3] == 0 && r->trace[1].a[4] == 0 && r->trace[1].a[5] == 0x80 && r->trace[1].a[6] == 1, "sprite-bind arguments");
    for (unsigned i = 0; i < RED; i++) require(r->actor[i] == c->pattern && r->actor[RED + ACTOR_STRIDE + i] == c->pattern && r->cur[i] == c->pattern && r->cur[RED + CUR_SIZE + i] == c->pattern, "redzones preserved");
}

int main(int argc, char** argv) {
    require(argc == 2, "retail field module argument");
    FILE* f = fopen(argv[1], "rb"); require(f != NULL, "retail open");
    require(fseek(f, RAW_OFFSET, SEEK_SET) == 0 && fread(retail, 1, RETAIL_SIZE, f) == RETAIL_SIZE, "retail function read"); fclose(f);
    static const int32_t ids[] = { 0, 1, 0x3F, 0x7F, 0x80, 0xFF, 0x100, 0x3FFF, 0x7FFF, -1, 0x12345, (int32_t)0x80000001 };
    static const uint16_t statuses[] = { 0x0000, 0xFFFF, 0x0F80, 0xF07F, 0x1234, 0x0020 };
    static const uint32_t f0s[] = { 0, 0xFFFFFFFFu, 0xFEFFu, 0x12345678u, 0x100u };
    static const uint32_t f4s[] = { 0, 0xFFFFFFFFu, 0x800u, 0x2800u, 0xDEADBEEFu };
    static const uint16_t ips[] = { 0, 0x7FFF, 0xFFFD, 0xFFFF, 0x1234 };
    static const uint32_t w12s[] = { 0, 0xFFFFFFFFu, 0xE000u, 0xFFFF1FFFu, 0x5A5AA5A5u };
    /* The production sprite-package idiom is (s32)base + (s32)offset (shared
     * with the matched func_800A0D3C); keep the large positive offset below
     * the host base's signed-overflow bound, which real packages never reach. */
    static const uint32_t offs[] = { 0, 0x10, 0x1234, 0xFFFFFF00u, 0x40000000u };
    static const int32_t indices[] = { 0, 1, 7, 51 };
    unsigned k = 0;
    for (unsigned ii = 0; ii < 4; ii++)
        for (int32_t count = 0; count <= MAX_SLOT; count++)
            for (unsigned si = 0; si < sizeof(ids) / sizeof(ids[0]); si++)
                for (unsigned st = 0; st < sizeof(statuses) / sizeof(statuses[0]); st++, k++) {
                    Case c = { indices[ii], count, ids[si], statuses[st], ips[(k / 25) % 5], f0s[k % 5], f4s[(k / 5) % 5], w12s[(k / 125) % 5], offs[k % 5], (uint8_t)((k & 1) ? 0xA5 : 0x3C) };
                    Result raw = run(0, &c), native = run(1, &c);
                    require(memcmp(&raw, &native, sizeof(raw)) == 0, "native/raw complete state and boundary trace");
                    check_semantics(&raw, &c);
                    cases++;
                }
    for (unsigned i = 0; i < INSTRS; i++) require(seen[i] > 0, "raw instruction coverage");
    printf("FIELD OBJECT REGISTER PASS cases=%u checks=%u instructions=%u\n", cases, checks, (unsigned)INSTRS);
    return 0;
}
