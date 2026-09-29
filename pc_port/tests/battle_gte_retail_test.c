/* Differential boundary test: execute the supplied retail leaf instructions
 * and the production native bridge against the SAME PsyCross GTE backend.
 * This proves wrapper/ABI equivalence, not PS1 hardware accuracy of that GTE. */
#include "../src/battle_mips_runtime.c"
#include "psx/gtereg.h"
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
void ControllerPushState(void) {}
void ControllerPoll(void) {}
/* This isolated math fixture has no elapsed-time/input callbacks. The runtime
 * bridge pumps this peripheral before native calls; it is not SDK math. */
void PcPort_ServiceVblank(void) {}
void PsyX_UpdateInput(void) {}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
unsigned g_PcPortPresentedFrames;
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int single) { (void)p; (void)single; abort(); }

#define VERTICES 0x80180000u
#define OUTPUTS  0x801f0000u
#define STACK    0x801ff000u
static uint32_t fixture_dqa, fixture_dqb;

/* Weak only in this test: a missing production owner must fail explicitly,
 * never silently resolve to the generated zero-return gameplay stubs. */
extern MATRIX *SetMulMatrix(MATRIX *, MATRIX *) __attribute__((weak));
extern VECTOR *Square0(VECTOR *, VECTOR *);
extern long FieldGetVec2Magnitude(long, long);
extern long VectorNormalSS(SVECTOR *, SVECTOR *) __attribute__((weak));
extern long VectorNormal(VECTOR *, VECTOR *);
extern long RotAverage4(SVECTOR *, SVECTOR *, SVECTOR *, SVECTOR *,
                        long *, long *, long *, long *, long *, long *);

static void reset_gte(int translation_z)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    CTC2(4096, 0); CTC2(4096, 2); CTC2(4096, 4);
    CTC2((uint32_t)translation_z, 7);
    CTC2(160u << 16, 24); CTC2(112u << 16, 25);
    CTC2(512, 26); CTC2(0x400, 30);
    CTC2(fixture_dqa, 27); CTC2(fixture_dqb, 28);
}

typedef struct ProjectionLeaf {
    uint32_t entry;
    void *native;
    const char *name;
    unsigned kind;
    unsigned flag_offset;
} ProjectionLeaf;

static const ProjectionLeaf leaves[] = {
    {0x8004a64cu, RotTransPers, "RotTransPers", 1, 16},
    {0x8004a67cu, RotTransPers3, "RotTransPers3", 3, 32},
    {0x8004a73cu, RotTransPers4, "RotTransPers4", 4, 40},
    {0x8004a7bcu, RotAverage4, "RotAverage4", 5, 40},
    {0x8004a83cu, RotAverageNclip4, "RotAverageNclip4", 0, 48},
};

static void setup_cpu(PcPortMipsCpu *cpu, BattleMipsRuntime *runtime, unsigned kind)
{
    initialize_cpu(cpu, runtime);
    cpu->gpr[29] = STACK;
    for (unsigned i = 0; i < 4; i++) cpu->gpr[4 + i] = VERTICES + i * 8;
    for (unsigned i = 0; i < 7; i++) store_le(PSX_ADDR(STACK + 16 + i * 4), 4, OUTPUTS + i * 8);
    if (kind == 1) {
        cpu->gpr[5] = OUTPUTS; cpu->gpr[6] = OUTPUTS + 8; cpu->gpr[7] = OUTPUTS + 16;
    } else if (kind == 3) {
        cpu->gpr[7] = OUTPUTS;
        for (unsigned i = 1; i < 5; i++) store_le(PSX_ADDR(STACK + 12 + i * 4), 4, OUTPUTS + i * 8);
    }
    memset(PSX_ADDR(OUTPUTS - 4), 0xa5, 68);
}

static int compare_leaf(const ProjectionLeaf *leaf, const char *name,
                        const SVECTOR vertices[4], int tz, int alias)
{
    const uint32_t entry = leaf->entry;
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu;
    uint8_t expected[68];
    GTERegisters expected_gte;
    uint32_t expected_result;
    runtime.functions[0] = (ResolvedFunction){entry, leaf->native, leaf->name};
    runtime.function_count = 1;
    memcpy(PSX_ADDR(VERTICES), vertices, 4 * sizeof(*vertices));
    reset_gte(tz);
    setup_cpu(&cpu, &runtime, leaf->kind);
    if (alias) store_le(PSX_ADDR(STACK + 0x24), 4, OUTPUTS + 32);
    cpu.bus.bridge = NULL;
    if (PcPortMipsRun(&cpu, entry, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "%s %s: retail execution failed: %s\n", leaf->name, name, cpu.error);
        return 0;
    }
    memcpy(expected, PSX_ADDR(OUTPUTS - 4), sizeof(expected));
    expected_gte = gteRegs;
    expected_result = cpu.gpr[2];

    if (leaf->kind == 5) {
        /* The field also calls this owner directly, with full host longs for
         * p/FLAG but packed four-byte XY slots inside GPU packets. Particle
         * rendering aliases p and FLAG; verify the final FLAG wins there. */
        long values[6];
        memset(values, 0xa5, sizeof(values));
        reset_gte(tz);
        long result = RotAverage4((SVECTOR *)PSX_ADDR(VERTICES),
            (SVECTOR *)PSX_ADDR(VERTICES + 8), (SVECTOR *)PSX_ADDR(VERTICES + 16),
            (SVECTOR *)PSX_ADDR(VERTICES + 24), &values[0], &values[1],
            &values[2], &values[3], &values[4], &values[alias ? 4 : 5]);
        for (unsigned i = 0; i < 6; i++) {
            uint32_t wanted = load_le(expected + 4 + i * 8, 4);
            if ((uint32_t)values[i] != wanted) {
                fprintf(stderr, "RotAverage4 direct %s alias=%d output[%u] native=%08x retail=%08x\n",
                        name, alias, i, (uint32_t)values[i], wanted);
                return 0;
            }
            if (i < 4 && sizeof(long) == 8 &&
                (uint64_t)values[i] >> 32 != 0xa5a5a5a5u) return 0;
            if ((i == 4 || (i == 5 && !alias)) && values[i] != (long)(int32_t)wanted) return 0;
        }
        if (result != (long)(int32_t)expected_result ||
            memcmp(&gteRegs.CP2D, &expected_gte.CP2D, sizeof(gteRegs.CP2D)) != 0 ||
            memcmp(&gteRegs.CP2C, &expected_gte.CP2C, sizeof(gteRegs.CP2C)) != 0) return 0;
    }

    reset_gte(tz);
    setup_cpu(&cpu, &runtime, leaf->kind);
    if (alias) store_le(PSX_ADDR(STACK + 0x24), 4, OUTPUTS + 32);
    if (runtime_bridge(&runtime, &cpu, entry) != 1 || cpu.gpr[2] != expected_result) {
        fprintf(stderr, "%s %s: result native=%08x retail=%08x\n", leaf->name, name, cpu.gpr[2], expected_result);
        return 0;
    }
    for (unsigned i = 0; i < sizeof(expected); i += 4) {
        uint32_t actual = load_le(PSX_ADDR(OUTPUTS - 4 + i), 4);
        uint32_t wanted = load_le(expected + i, 4);
        if (actual != wanted) {
            fprintf(stderr, "%s %s: output/guard at %+d native=%08x retail=%08x\n",
                    leaf->name, name, (int)i - 4, actual, wanted);
            return 0;
        }
    }
    if (memcmp(&gteRegs.CP2D, &expected_gte.CP2D, sizeof(gteRegs.CP2D)) != 0 ||
        memcmp(&gteRegs.CP2C, &expected_gte.CP2C, sizeof(gteRegs.CP2C)) != 0) {
        fprintf(stderr, "%s %s: final GTE state differs\n", leaf->name, name);
        return 0;
    }
    printf("BATTLE GTE RETAIL PASS %s %s alias=%d result=%d flag=%08x\n", leaf->name, name,
           alias, (int32_t)expected_result, load_le(PSX_ADDR(OUTPUTS + (alias ? 32 : leaf->flag_offset)), 4));
    return 1;
}

static int compare_all(const char *name, const SVECTOR vertices[4], int tz)
{
    for (unsigned i = 0; i < sizeof(leaves) / sizeof(leaves[0]); i++) {
        if (!compare_leaf(&leaves[i], name, vertices, tz, 0)) return 0;
        if (leaves[i].kind == 5 && !compare_leaf(&leaves[i], name, vertices, tz, 1)) return 0;
    }
    return 1;
}

static int compare_set_mul_matrix(unsigned fixture, int alias)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu;
    uint8_t original[128];
    GTERegisters expected_gte;
    uint32_t matrix0 = VERTICES, matrix1 = alias ? VERTICES : VERTICES + 64;
    if (SetMulMatrix == NULL) {
        fprintf(stderr, "BATTLE GTE RETAIL FAIL SetMulMatrix has no production owner\n");
        return 0;
    }
    memset(PSX_ADDR(VERTICES), 0xa5, sizeof(original));
    uint32_t seed = 0x12345678u + fixture;
    for (unsigned matrix = 0; matrix < 2; matrix++) {
        for (unsigned element = 0; element < 9; element++) {
            seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
            uint16_t value = fixture == 0 ? (element % 4 == 0 ? 4096 : 0) :
                             fixture == 1 ? 0 : fixture == 2 ? 0x7fff :
                             fixture == 3 ? 0x8000 : (uint16_t)seed;
            store_le(PSX_ADDR(VERTICES + matrix * 64 + element * 2), 2, value);
        }
    }
    memcpy(original, PSX_ADDR(VERTICES), sizeof(original));
    reset_gte(12345);
    initialize_cpu(&cpu, &runtime);
    cpu.bus.bridge = NULL;
    cpu.gpr[4] = matrix0; cpu.gpr[5] = matrix1;
    if (PcPortMipsRun(&cpu, 0x8004987cu, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SetMulMatrix retail execution failed: %s\n", cpu.error);
        return 0;
    }
    expected_gte = gteRegs;
    uint32_t expected_result = cpu.gpr[2];
    if (memcmp(original, PSX_ADDR(VERTICES), sizeof(original))) return 0;
    reset_gte(12345);
    MATRIX *result = SetMulMatrix(PSX_ADDR(matrix0), PSX_ADDR(matrix1));
    /* VZ0..VZ2 (data regs 1, 3, 5) are 16-bit on the GTE: MTC2 keeps the low
     * halfword and reads return it sign-extended (psx-spx "GTE Registers").
     * Retail loads the VZ column with a full-word LW, so PsyCross's 32-bit
     * register model also keeps the neighbouring halfword in the upper bits,
     * which no hardware read can observe.  Compare them as the hardware does. */
    for (unsigned vz = 1; vz <= 5; vz += 2) {
        uint32_t a = load_le((uint8_t*)&gteRegs.CP2D + vz * 4, 4);
        uint32_t e = load_le((uint8_t*)&expected_gte.CP2D + vz * 4, 4);
        store_le((uint8_t*)&gteRegs.CP2D + vz * 4, 4, (uint32_t)(int32_t)(int16_t)a);
        store_le((uint8_t*)&expected_gte.CP2D + vz * 4, 4, (uint32_t)(int32_t)(int16_t)e);
    }
    if (PsxMemory_GuestAddr(result) != expected_result ||
        memcmp(original, PSX_ADDR(VERTICES), sizeof(original)) ||
        memcmp(&gteRegs.CP2D, &expected_gte.CP2D, sizeof(gteRegs.CP2D)) ||
        memcmp(&gteRegs.CP2C, &expected_gte.CP2C, sizeof(gteRegs.CP2C))) {
        fprintf(stderr, "SetMulMatrix retail mismatch fixture=%u alias=%d\n", fixture, alias);
        for (unsigned reg = 0; reg < 32; reg++) {
            uint32_t actual_d = load_le((uint8_t*)&gteRegs.CP2D + reg * 4, 4);
            uint32_t expected_d = load_le((uint8_t*)&expected_gte.CP2D + reg * 4, 4);
            uint32_t actual_c = load_le((uint8_t*)&gteRegs.CP2C + reg * 4, 4);
            uint32_t expected_c = load_le((uint8_t*)&expected_gte.CP2C + reg * 4, 4);
            if (actual_d != expected_d || actual_c != expected_c)
                fprintf(stderr, "  GTE reg=%u data=%08x/%08x control=%08x/%08x (native/retail)\n",
                        reg, actual_d, expected_d, actual_c, expected_c);
        }
        return 0;
    }
    return 1;
}

static int compare_normal_ss(int16_t x, int16_t y, int16_t z, int alias)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu;
    uint8_t expected[32];
    GTERegisters expected_gte;
    uint32_t expected_result;
    uint32_t out = alias ? VERTICES : VERTICES + 16;
    SVECTOR input = {x, y, z, 0x1234};
    memset(PSX_ADDR(VERTICES), 0xa5, sizeof(expected));
    memcpy(PSX_ADDR(VERTICES), &input, sizeof(input));
    reset_gte(1234);
    initialize_cpu(&cpu, &runtime);
    cpu.gpr[4] = VERTICES;
    cpu.gpr[5] = out;
    cpu.bus.bridge = NULL;
    if (PcPortMipsRun(&cpu, 0x80048da8u, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "normal SS retail failed: %s\n", cpu.error);
        return 0;
    }
    memcpy(expected, PSX_ADDR(VERTICES), sizeof(expected));
    expected_gte = gteRegs;
    expected_result = cpu.gpr[2];
    memset(PSX_ADDR(VERTICES), 0xa5, sizeof(expected));
    memcpy(PSX_ADDR(VERTICES), &input, sizeof(input));
    reset_gte(1234);
    uint32_t result = (uint32_t)VectorNormalSS((SVECTOR *)PSX_ADDR(VERTICES),
                                              (SVECTOR *)PSX_ADDR(out));
    if (result != expected_result || memcmp(expected, PSX_ADDR(VERTICES), sizeof(expected)) ||
        memcmp(&gteRegs, &expected_gte, sizeof(gteRegs))) {
        fprintf(stderr, "normal SS mismatch (%d,%d,%d) alias=%d return=%08x/%08x\n",
                x, y, z, alias, result, expected_result);
        return 0;
    }
    return 1;
}

static int compare_normal_long(int32_t x, int32_t y, int32_t z, int alias)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu;
    uint8_t expected[48];
    GTERegisters expected_gte;
    uint32_t expected_result;
    uint32_t out = alias == 0 ? VERTICES + 24 : alias == 1 ? VERTICES : VERTICES + 4;
    VECTOR input = {x, y, z, 0x1234};
    memset(PSX_ADDR(VERTICES), 0xa5, sizeof(expected));
    memcpy(PSX_ADDR(VERTICES), &input, sizeof(input));
    reset_gte(1234);
    initialize_cpu(&cpu, &runtime);
    cpu.gpr[4] = VERTICES;
    cpu.gpr[5] = out;
    cpu.bus.bridge = NULL;
    if (PcPortMipsRun(&cpu, 0x80048d7cu, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "normal long retail failed: %s\n", cpu.error);
        return 0;
    }
    memcpy(expected, PSX_ADDR(VERTICES), sizeof(expected));
    expected_gte = gteRegs;
    expected_result = cpu.gpr[2];
    memset(PSX_ADDR(VERTICES), 0xa5, sizeof(expected));
    memcpy(PSX_ADDR(VERTICES), &input, sizeof(input));
    reset_gte(1234);
    uint32_t result = (uint32_t)VectorNormal((VECTOR *)PSX_ADDR(VERTICES),
                                              (VECTOR *)PSX_ADDR(out));
    if (result != expected_result || memcmp(expected, PSX_ADDR(VERTICES), sizeof(expected)) ||
        memcmp(&gteRegs, &expected_gte, sizeof(gteRegs))) {
        fprintf(stderr, "normal long mismatch (%d,%d,%d) alias=%d return=%08x/%08x\n",
                x, y, z, alias, result, expected_result);
        return 0;
    }
    return 1;
}

static int normal_overflow_fails_closed(int fullword)
{
    /* These overflow the retail signed ADD, not the GTE SQR. Our CPU test
     * interpreter does not model that exception, so check the explicit host
     * failure boundary separately rather than treating wrapped output as an
     * oracle. No core dumps are needed for these expected child failures. */
    const SVECTOR inputs[] = {{-32768, -32768, 0, 0},
                              {32767, 32767, 363, 0},
                              {-32768, -32768, -32768, 0}};
    for (unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        pid_t child = fork();
        if (child < 0) return 0;
        if (child == 0) {
            struct rlimit no_core = {0, 0};
            SVECTOR input = inputs[i], output;
            if (setrlimit(RLIMIT_CORE, &no_core)) _exit(2);
            if (fullword) {
                VECTOR wide = {input.vx,input.vy,input.vz,0}, wideOutput;
                VectorNormal(&wide, &wideOutput);
            } else VectorNormalSS(&input, &output);
            _exit(3);
        }
        int status;
        if (waitpid(child, &status, 0) != child || !WIFSIGNALED(status) ||
            WTERMSIG(status) != SIGABRT) return 0;
    }
    return 1;
}

static int compare_square0(uint32_t bits, int alias)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu;
    uint32_t destination = alias ? VERTICES : OUTPUTS;
    uint8_t expected[24];
    GTERegisters expected_gte;
    uint32_t values[4] = {bits, bits ^ 0xffff0000u, bits ^ 0x80008000u, 0xa5a5a5a5u};
    memset(PSX_ADDR(destination - 4), 0xa5, sizeof(expected));
    memcpy(PSX_ADDR(VERTICES), values, sizeof(values));
    reset_gte(0);
    initialize_cpu(&cpu, &runtime);
    cpu.bus.bridge = NULL;
    cpu.gpr[4] = VERTICES; cpu.gpr[5] = destination; cpu.gpr[29] = STACK;
    if (PcPortMipsRun(&cpu, 0x8004a414u, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_HALTED)
        return 0;
    memcpy(expected, PSX_ADDR(destination - 4), sizeof(expected));
    expected_gte = gteRegs;
    if (cpu.gpr[2] != destination) return 0;
    memset(PSX_ADDR(destination - 4), 0xa5, sizeof(expected));
    memcpy(PSX_ADDR(VERTICES), values, sizeof(values));
    reset_gte(0);
    VECTOR* result = Square0((VECTOR*)PSX_ADDR(VERTICES), (VECTOR*)PSX_ADDR(destination));
    if (result != (VECTOR*)PSX_ADDR(destination) ||
        memcmp(expected, PSX_ADDR(destination - 4), sizeof(expected)) ||
        memcmp(&gteRegs.CP2D, &expected_gte.CP2D, sizeof(gteRegs.CP2D)) ||
        memcmp(&gteRegs.CP2C, &expected_gte.CP2C, sizeof(gteRegs.CP2C))) {
        fprintf(stderr, "SQUARE0 RETAIL FAIL bits=%08x alias=%d output/GTE/return\n", bits, alias);
        return 0;
    }
    return 1;
}

static unsigned overflow_table_reads;
static int overflow_read(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    if (address == 0x80056880u && width == 2) ++overflow_table_reads;
    return runtime_read(opaque, address, width, value);
}

static int magnitude2_overflow_oracle(void)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu;
    reset_gte(0);initialize_cpu(&cpu, &runtime);cpu.bus.bridge = NULL;
    cpu.bus.read = overflow_read;overflow_table_reads = 0;
    cpu.gpr[4] = cpu.gpr[5] = (uint32_t)-32768;cpu.gpr[29] = STACK;
    if (PcPortMipsRun(&cpu, 0x80099a4cu, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_HALTED ||
        overflow_table_reads != 1 || cpu.gpr[2] != 0x39a80u) {
        fprintf(stderr,"MAGNITUDE2 OVERFLOW ORACLE FAIL reads=%u result=%08x\n",overflow_table_reads,cpu.gpr[2]);
        return 0;
    }
    puts("MAGNITUDE2 OVERFLOW ORACLE OBSERVED address=80056880 halfword=7350 result=00039a80");
    return 1;
}

static int compare_magnitude2(int32_t x, int32_t y)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu;
    reset_gte(0);initialize_cpu(&cpu, &runtime);cpu.bus.bridge = NULL;
    cpu.gpr[4] = x;cpu.gpr[5] = y;cpu.gpr[29] = STACK;
    if (PcPortMipsRun(&cpu, 0x80099a4cu, BATTLE_HALT_PC, 1000) != PC_PORT_MIPS_HALTED) return 0;
    uint32_t expected = cpu.gpr[2];
    GTERegisters expected_gte = gteRegs;
    reset_gte(0);
    long actual = FieldGetVec2Magnitude(x, y);
    if (actual != (long)(int32_t)expected ||
        memcmp(&gteRegs.CP2D, &expected_gte.CP2D, sizeof(gteRegs.CP2D)) ||
        memcmp(&gteRegs.CP2C, &expected_gte.CP2C, sizeof(gteRegs.CP2C))) {
        fprintf(stderr,"MAGNITUDE2 RETAIL FAIL x=%d y=%d actual=%ld expected=%u result/GTE\n",x,y,actual,expected);
        return 0;
    }
    return 1;
}

int main(void)
{
    SVECTOR vertices[4] = {{-1000, -1000, 10000, 0}, {1000, -1000, 10000, 0},
                           {-1000, 1000, 10000, 0}, {1000, 1000, 10000, 0}};
    const uint32_t start = 0x8004a64cu, end = 0x8004a8ecu;
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    if (image == NULL || fseek(image, start - 0x8000f800u, SEEK_SET) != 0 ||
        fread(PSX_ADDR(start), 1, end - start, image) != end - start) return 1;
    if (fseek(image, 0x8004987cu - 0x8000f800u, SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x8004987cu), 1, 0x110, image) != 0x110) return 1;
    if (fseek(image, 0x80048d7cu - 0x8000f800u, SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x80048d7cu), 1, 0x118, image) != 0x118) return 1;
    if (fseek(image, 0x80056b14u - 0x8000f800u, SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x80056b14u), 1, 0x200, image) != 0x200) return 1;
    if (fseek(image, 0x8004a414u - 0x8000f800u, SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x8004a414u), 1, 0x28, image) != 0x28) return 1;
    if (fseek(image, 0x80048c4cu - 0x8000f800u, SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x80048c4cu), 1, 0x84, image) != 0x84) return 1;
    if (fseek(image, 0x80056a00u - 0x8000f800u, SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x80056a00u), 1, 0x180, image) != 0x180) return 1;
    if (fseek(image, 0x80056880u - 0x8000f800u, SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x80056880u), 1, 2, image) != 2) return 1;
    fclose(image);
    image = fopen("disc/field.bin", "rb");
    if (!image || fseek(image, 0x80099a4cu-0x8006faf0u, SEEK_SET) ||
        fread(PSX_ADDR(0x80099a4cu),1,0x40,image)!=0x40) return 1;
    fclose(image);
    if (!magnitude2_overflow_oracle()) return 1;
    if (getenv("XENO_MAGNITUDE_OVERFLOW_PROBE")) {
        return compare_magnitude2(-32768,-32768) ? 0 : 1;
    }
    {
        const int32_t values[] = {0,1,-1,181,32767,-32768,65535,65536,INT32_MAX};
        unsigned count = 0;
        for (unsigned i=0;i<sizeof(values)/sizeof(values[0]);++i)
            for (unsigned j=0;j<sizeof(values)/sizeof(values[0]);++j) {
                if (!compare_magnitude2(values[i],values[j])) return 1;
                ++count;
            }
        printf("MAGNITUDE2 RETAIL PASS cases=%u full SDK chain/result/GTE-state\n",count);
    }
    {
        const uint16_t samples[] = {0,0x8001,0xffff};
        for (unsigned i=0;i<sizeof(samples)/sizeof(samples[0]);++i) {
            store_le(PSX_ADDR(0x80056880u),2,samples[i]);
            if (!compare_magnitude2(-32768,-32768)) return 1;
        }
        store_le(PSX_ADDR(0x80056880u),2,0x7350);
        puts("MAGNITUDE2 RETAIL MEMORY PASS 3 test-only lookup variations including signed halfwords");
    }
    for (uint32_t bits = 0; bits < 65536; ++bits)
        for (int alias = 0; alias < 2; ++alias)
            if (!compare_square0(bits, alias) || !compare_square0(bits | 0x12340000u, alias)) return 1;
    puts("SQUARE0 RETAIL PASS cases=262144, operand-width/in-place/guards/return/GTE-state");
    for (int value = -32768; value <= 32767; ++value) {
        for (int alias = 0; alias < 3; ++alias) {
            if (!compare_normal_long(value, 0, 0, alias) ||
                !compare_normal_long(0, value, 0, alias) ||
                !compare_normal_long(0, 0, value, alias) ||
                !compare_normal_long((int32_t)((uint32_t)value | 0x56780000u),
                    value / 2, -value / 3, alias)) return 1;
        }
    }
    puts("NORMAL LONG RETAIL PASS cases=786432, fullword inputs, separate/in-place/partial-overlap, return/guards/GTE-state");
    for (int alias = 0; alias < 3; ++alias)
        if (!compare_normal_long(32767,32767,362,alias)) return 1;
    if (!normal_overflow_fails_closed(1)) return 1;
    puts("NORMAL LONG OVERFLOW BOUNDARY PASS near-limit=3, fail-closed=3; CPU signed-overflow exception not modeled");
    if (VectorNormalSS == NULL) {
        fputs("NORMAL SS FAIL: missing production owner\n", stderr);
        return 1;
    }
    for (int value = -32768; value <= 32767; value++) {
        for (int alias = 0; alias < 2; alias++) {
            if (!compare_normal_ss(value, 0, 0, alias) ||
                !compare_normal_ss(0, value, 0, alias) ||
                !compare_normal_ss(0, 0, value, alias) ||
                !compare_normal_ss(value, value / 2, -value / 3, alias)) return 1;
        }
    }
    puts("NORMAL SS RETAIL PASS cases=524288, in-place/separate, guards/return/GTE-state");
    for (int alias = 0; alias < 2; alias++)
        if (!compare_normal_ss(32767, 32767, 362, alias)) return 1;
    if (!normal_overflow_fails_closed(0)) return 1;
    puts("NORMAL SS OVERFLOW BOUNDARY PASS near-limit=2, fail-closed=3");
    for (unsigned fixture = 0; fixture < 68; fixture++)
        for (int alias = 0; alias < 2; alias++)
            if (!compare_set_mul_matrix(fixture, alias)) return 1;
    puts("BATTLE GTE MATRIX RETAIL PASS cases=136, input/guards/return/GTE-state");
    if (!compare_all("front-facing", vertices, 0)) return 1;
    SVECTOR swap = vertices[1]; vertices[1] = vertices[2]; vertices[2] = swap;
    if (!compare_all("negative-winding", vertices, 0)) return 1;
    swap = vertices[1]; vertices[1] = vertices[2]; vertices[2] = swap;
    vertices[2] = vertices[1];
    if (!compare_all("zero-area", vertices, 0)) return 1;
    vertices[2] = (SVECTOR){-1000, 1000, 10000, 0};
    for (unsigned i = 0; i < 3; i++) vertices[i].vz = 30000;
    vertices[3].vz = -10000;
    if (!compare_all("first-three-depth-flags-survive-fourth", vertices, 40000)) return 1;
    for (unsigned i = 0; i < 4; i++) vertices[i].vz = 0;
    if (!compare_all("zero-depth", vertices, 0)) return 1;
    for (unsigned i = 0; i < 4; i++) vertices[i].vz = -10000;
    if (!compare_all("negative-depth", vertices, 0)) return 1;
    for (unsigned i = 0; i < 4; i++) vertices[i].vz = 10000;
    vertices[3].vz = 0;
    if (!compare_all("fourth-vertex-flags", vertices, 0)) return 1;
    vertices[3].vz = 10000;
    fixture_dqa = 0x100; fixture_dqb = 0x123400;
    if (!compare_all("nonzero-depth-cue", vertices, 0)) return 1;
    fixture_dqa = (uint32_t)-0x100; fixture_dqb = 0;
    if (!compare_all("negative-depth-cue-clamp", vertices, 0)) return 1;
    fixture_dqa = 0x7fff; fixture_dqb = 0x7fffffff;
    return compare_all("positive-depth-cue-clamp", vertices, 0) ? 0 : 1;
}
