/* Native binding for the exact archive-20/file-1 controller. This file is
 * included by battle_mips_runtime.c, not compiled as a second translation unit. */
#include <setjmp.h>
#include <stddef.h>

#define FILE1_BASE 0x801e5000u
#define FILE1_BYTES 0x4c3cu
#define FILE1_IMMUTABLE_BYTES 0x4b5cu

static uint32_t file1_ror(uint32_t x, unsigned n)
{ return (x >> n) | (x << (32u - n)); }

/* SHA-256 over the bounded local payload. Constants contain no retail data. */
static void file1_digest(const uint8_t *data, size_t length, uint8_t out[32])
{
    static const uint32_t k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };
    uint32_t h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                      0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    size_t blocks = (length + 9u + 63u) / 64u;
    uint64_t bits = (uint64_t)length * 8u;
    for (size_t block = 0; block < blocks; block++) {
        uint32_t w[64] = {0};
        for (unsigned i = 0; i < 64; i++) {
            size_t pos = block * 64u + i;
            uint8_t byte = pos < length ? data[pos] : pos == length ? 0x80 : 0;
            if (pos >= blocks * 64u - 8u)
                byte = (uint8_t)(bits >> ((blocks * 64u - 1u - pos) * 8u));
            w[i / 4u] |= (uint32_t)byte << (24u - (i % 4u) * 8u);
        }
        for (unsigned i = 16; i < 64; i++) {
            uint32_t a = w[i-15], b = w[i-2];
            w[i] = w[i-16] + (file1_ror(a,7)^file1_ror(a,18)^(a>>3)) +
                   w[i-7] + (file1_ror(b,17)^file1_ror(b,19)^(b>>10));
        }
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];
        for (unsigned i = 0; i < 64; i++) {
            uint32_t t1=z+(file1_ror(e,6)^file1_ror(e,11)^file1_ror(e,25))+
                        ((e&f)^(~e&g))+k[i]+w[i];
            uint32_t t2=(file1_ror(a,2)^file1_ror(a,13)^file1_ror(a,22))+
                        ((a&b)^(a&c)^(b&c));
            z=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;
    }
    for (unsigned i=0;i<32;i++) out[i]=(uint8_t)(h[i/4]>>(24u-(i%4)*8u));
}

static int file1_hash_matches(size_t length, const char *hex)
{
    static const char digits[] = "0123456789abcdef";
    uint8_t digest[32];
    file1_digest(g_PsxRam + (FILE1_BASE - 0x80000000u), length, digest);
    for (unsigned i=0;i<32;i++)
        if (digits[digest[i]>>4] != hex[i*2] || digits[digest[i]&15] != hex[i*2+1]) return 0;
    return 1;
}

static void file1_invalidate(BattleMipsRuntime *runtime)
{
    runtime->file1.verified = 0;
    runtime->file1.generation++;
}

static int file1_identity_current(BattleMipsRuntime *runtime)
{
    if (!runtime->file1.verified) return 0;
    if (!file1_hash_matches(FILE1_IMMUTABLE_BYTES,
        "8c071b7c6edffcb922b798ceddfb1a09167d22b402ae5d0d268d9252bf4a15c4")) {
        file1_invalidate(runtime);
        return 0;
    }
    return 1;
}

/* Only physical RAM and native pointers into that same RAM are accepted.
 * Validate before subtraction/canonicalization; never mask arbitrary values. */
static uint8_t *file1_ram(uintptr_t address, size_t bytes, unsigned alignment)
{
    uintptr_t ram = (uintptr_t)g_PsxRam;
    uintptr_t offset;
    if (address >= 0x80000000u && address < 0x80200000u)
        offset = address - 0x80000000u;
    else if (address >= 0xa0000000u && address < 0xa0200000u)
        offset = address - 0xa0000000u;
    else if (address >= ram && address < ram + 0x200000u)
        offset = address - ram;
    else return NULL;
    if ((offset & (alignment - 1u)) != 0 || bytes > 0x200000u - offset) return NULL;
    return g_PsxRam + offset;
}

static int file1_overlap(uint32_t address, uint32_t bytes)
{
    uint8_t *p = file1_ram(address, 0, 1);
    if (p == NULL) return 1; /* Unknown destination: discard stale authority. */
    uint32_t offset = (uint32_t)(p - g_PsxRam);
    return (uint64_t)offset + bytes > 0x1e5000u && offset < 0x1e9c3cu;
}

static void file1_before_archive(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                 uint32_t target, int *candidate)
{
    uint32_t size = 0, archive = 0, debug = 1;
    *candidate = 0;
    if (target == 0x800295d8u) {
        const ResolvedFunction *decode = find_function(runtime, 0x800288ecu);
        /* This read-only size decoder is also called by the immediately
         * following native read. Debug-file fallback is not a retail identity. */
        if (runtime_read(runtime, 0x8004fe48u, 4, &debug) != 0 || debug != 0 ||
            decode == NULL || xeno_port_is_generated_stub(decode->name)) {
            file1_invalidate(runtime);
            return;
        }
        int32_t n = ((int32_t (*)(uint32_t))decode->host)(cpu->gpr[4]);
        if (n <= 0) { file1_invalidate(runtime); return; }
        size = (uint32_t)n;
        if (file1_overlap(cpu->gpr[5], size)) file1_invalidate(runtime);
        if (runtime_read(runtime, 0x8004fe14u, 4, &archive) == 0 &&
            runtime->file1.archive20_selected && archive == 3087 &&
            cpu->gpr[4] == 1 && cpu->gpr[5] == FILE1_BASE &&
            cpu->gpr[6] == 0 && cpu->gpr[7] == 0x80 && size == FILE1_BYTES)
            *candidate = 1;
    } else if (target == 0x8002954cu || target == 0x80029690u) {
        if (target == 0x8002954cu) size = cpu->gpr[6];
        else if (runtime_read(runtime, 0x8004fdf8u, 4, &size) != 0) size = UINT32_MAX;
        if (file1_overlap(cpu->gpr[5], size)) file1_invalidate(runtime);
    }
}

static void file1_after_archive(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                uint32_t target, uintptr_t result, int candidate)
{
    if (target == 0x80028470u) {
        runtime->file1.archive20_selected = cpu->gpr[4] == 0x20 &&
            cpu->gpr[5] == 0 && (int32_t)result == 3087;
    } else if (target == 0x800295d8u && candidate && (int32_t)result == 0) {
        runtime->file1.verified = file1_hash_matches(FILE1_BYTES,
            "64668d85bf48dea46cf6d5f04b38e38ca887877cfef53961f0c26cd2d363b670");
    }
}

/* setjmp catches only adapter validation/helper errors. Guest writes remain
 * observable on failure; no raw fallback is permitted after selection. */
typedef struct File1Call {
    struct File1Call *previous;
    BattleMipsRuntime *runtime;
    uint64_t generation;
    jmp_buf failure;
} File1Call;
static File1Call *file1_call;
static const char *file1_failure_reason;

static void file1_fail(const char *reason)
{
    file1_failure_reason = reason;
    longjmp(file1_call->failure, 1);
}
static void file1_check_generation(void)
{
    if (file1_call->generation != file1_call->runtime->file1.generation ||
        !file1_identity_current(file1_call->runtime))
        file1_fail("module changed during adopted controller");
}
static void *file1_checked(uintptr_t address, size_t bytes, unsigned alignment)
{
    uint8_t *p = file1_ram(address, bytes, alignment);
    if (p == NULL) file1_fail("invalid packed RAM pointer/range/alignment");
    return p;
}
static uint8_t *file1_global_pointer(uint32_t address, size_t bytes, unsigned alignment)
{
    uint32_t raw = load_le(file1_checked(address, 4, 4), 4);
    return file1_checked(raw, bytes, alignment);
}
static uint32_t file1_raw(const void *p)
{
    uint8_t *checked = file1_checked((uintptr_t)p, 1, 1);
    return 0x80000000u + (uint32_t)(checked - g_PsxRam);
}

static uint32_t file1_guest(uint32_t target, const uint32_t *args, unsigned count)
{
    uint32_t result;
    file1_check_generation();
    if (PcPort_BattleMipsCallGuest(target, args, count, &result) != 0)
        file1_fail("guest helper failed");
    file1_check_generation();
    return result;
}

/* Resident calls use the same bridge and constructor conversion as ordinary
 * retail execution. Keep the original controller's outgoing frame placement. */
static uintptr_t file1_resident(uint32_t target, const uint32_t *args, unsigned count)
{
    BattleMipsRuntime *runtime = file1_call->runtime;
    PcPortMipsCpu *caller = runtime->bridge_cpu, cpu;
    uint32_t words[7], sp;
    int status;
    file1_check_generation();
    if (caller == NULL || count > 7 || (count && args == NULL))
        file1_fail("invalid resident call context/argc");
    sp = caller->gpr[29];
    if ((sp & 7u) != 0 || sp < 0x80000050u || sp > BATTLE_STACK_TOP)
        file1_fail("invalid resident outgoing stack");
    if (count) memcpy(words, args, count * sizeof(*words));
    initialize_cpu(&cpu, runtime);
    memcpy(cpu.gpr, caller->gpr, sizeof(cpu.gpr));
    memcpy(cpu.cp0, caller->cp0, sizeof(cpu.cp0));
    cpu.hi = caller->hi; cpu.lo = caller->lo;
    cpu.gpr[0] = 0; cpu.gpr[29] = sp - 0x50u; cpu.gpr[31] = BATTLE_HALT_PC;
    for (unsigned i=0;i<4;i++) cpu.gpr[4+i] = i<count ? words[i] : 0;
    for (unsigned i=4;i<count;i++)
        if (runtime_write(runtime, cpu.gpr[29]+0x10u+(i-4u)*4u,4,words[i]) != 0)
            file1_fail("resident stack write failed");
    status = runtime_bridge(runtime, &cpu, target);
    if (runtime->bridge_cpu != caller) {
        runtime->bridge_cpu = caller;
        file1_fail("resident bridge context was not restored");
    }
    if (status != 1) file1_fail("resident bridge failed");
    file1_check_generation();
    return (uintptr_t)cpu.gpr[2] | ((uintptr_t)cpu.gpr[3] << 32);
}

static void file1_setup(int32_t a0,uint16_t a1,uint16_t a2,uint16_t a3,
                         uint16_t a4,uint32_t a5,uint32_t a6)
{ uint32_t a[]={(uint32_t)a0,a1,a2,a3,a4,a5,a6}; (void)file1_guest(0x8008f8f4u,a,7); }
static void file1_yield(void) { (void)file1_guest(0x800716d8u,NULL,0); }
static void file1_cleanup(int32_t a0)
{ uint32_t a[]={(uint32_t)a0}; (void)file1_guest(0x8008fa60u,a,1); }
static void file1_local6750(uint8_t a0,uint16_t a1,uint16_t a2,uint16_t a3,uint16_t a4)
{ uint32_t a[]={a0,a1,a2,a3,a4}; (void)file1_guest(0x801e6750u,a,5); }
static void file1_local5b00(int32_t a0,int32_t a1)
{ uint32_t a[]={(uint32_t)a0,(uint32_t)a1}; (void)file1_guest(0x801e5b00u,a,2); }
static void file1_ctor(void *p,int32_t a1,int32_t a2,int32_t a3,
                       int32_t a4,int32_t a5,int32_t a6)
{ uint32_t a[]={file1_raw(p),(uint32_t)a1,(uint32_t)a2,(uint32_t)a3,
                (uint32_t)a4,(uint32_t)a5,(uint32_t)a6};
  (void)file1_resident(0x80032f54u,a,7); }
static void file1_single(uint32_t target,void *p)
{ uint32_t a[]={file1_raw(p)}; (void)file1_resident(target,a,1); }
static void file1_reset(void *p) { file1_single(0x80034614u,p); }
static void file1_clear_wait(void *p) { file1_single(0x800345e0u,p); }
static void file1_destroy(void *p) { file1_single(0x800346d4u,p); }
static void *file1_get_string(void *p,int32_t index)
{
    if (index < 0 || index > 0xffff) file1_fail("invalid string index");
    file1_checked((uintptr_t)p,(size_t)index*2u+6u,2);
    uint32_t a[]={file1_raw(p),(uint32_t)index};
    return file1_checked(file1_resident(0x80033728u,a,2),1,1);
}
static void file1_queue(void *p,void *string)
{ uint32_t a[]={file1_raw(p),file1_raw(string)}; (void)file1_resident(0x80034714u,a,2); }

#define XBCF1_NATIVE_BINDINGS 1
#define xeno_battle_command_file1_controller file1_bound_controller
#define D_800D3278 file1_global_pointer(0x800d3278u,0x828,2)
#define D_800D2D28 file1_global_pointer(0x800d2d28u,0xd0,1)
#define D_800D2DAC file1_global_pointer(0x800d2dacu,0x98,4)
#define D_800D3340 file1_global_pointer(0x800d3340u,2,2)
#define D_800D3014 (*(uint8_t *)file1_checked(0x800d3014u,1,1))
#define xeno_battle_command_file1_defaults_9C10 ((uint16_t *)file1_checked(0x801e9c10u,10,2))
#define xeno_battle_command_file1_cycle_9C1C (*(int32_t *)file1_checked(0x801e9c1cu,4,4))
#define xeno_battle_command_file1_x_9C30 (*(int32_t *)file1_checked(0x801e9c30u,4,4))
#define xeno_battle_command_file1_y_9C34 (*(int32_t *)file1_checked(0x801e9c34u,4,4))
#define func_8008F8F4 file1_setup
#define func_800716D8 file1_yield
#define func_8008FA60 file1_cleanup
#define xeno_battle_command_file1_helper_6750 file1_local6750
#define xeno_battle_command_file1_helper_5B00 file1_local5b00
#define func_80032F54 file1_ctor
#define func_80034614 file1_reset
#define GetStringEntry file1_get_string
#define func_80034714 file1_queue
#define func_800345E0 file1_clear_wait
#define func_800346D4 file1_destroy
#include "../../src/battle_command_file1/message_controller_impl.inc"
#undef XBCF1_NATIVE_BINDINGS
#undef xeno_battle_command_file1_controller
#undef D_800D3278
#undef D_800D2D28
#undef D_800D2DAC
#undef D_800D3340
#undef D_800D3014
#undef xeno_battle_command_file1_defaults_9C10
#undef xeno_battle_command_file1_cycle_9C1C
#undef xeno_battle_command_file1_x_9C30
#undef xeno_battle_command_file1_y_9C34
#undef func_8008F8F4
#undef func_800716D8
#undef func_8008FA60
#undef xeno_battle_command_file1_helper_6750
#undef xeno_battle_command_file1_helper_5B00
#undef func_80032F54
#undef func_80034614
#undef GetStringEntry
#undef func_80034714
#undef func_800345E0
#undef func_800346D4

_Static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "file1 packed native binding requires little endian");
_Static_assert(sizeof(xbcf1_u16)==2 && sizeof(xbcf1_s32)==4, "file1 scalar widths");
_Static_assert(offsetof(struct Xbcf1ControlFields,x)==0x7f6 &&
               offsetof(struct Xbcf1ControlFields,phase)==0x802, "retail control offsets");

static int file1_try_controller(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu)
{
    File1Call call;
    int32_t result;
    if (!file1_identity_current(runtime)) return 0;
    if (g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu)
        return -1;
    call.previous = file1_call;
    call.runtime = runtime;
    call.generation = runtime->file1.generation;
    file1_failure_reason = "adapter failure";
    file1_call = &call;
    if (setjmp(call.failure) != 0) {
        fprintf(stderr,"[xeno-port][battle-mips] file1 adoption stopped: %s\n",file1_failure_reason);
        file1_call = call.previous;
        return -1;
    }
    result = file1_bound_controller((uint16_t)cpu->gpr[4],(uint8_t)cpu->gpr[5],(uint16_t)cpu->gpr[6]);
    file1_check_generation();
    file1_call = call.previous;
    cpu->gpr[2] = (uint32_t)result;
    return 1;
}
