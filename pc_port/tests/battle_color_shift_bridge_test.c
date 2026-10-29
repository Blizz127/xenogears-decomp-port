/* The battle bridge binds retail 0x80025D4C to the port body func_80025D4C
 * (pc_port/src/retail_leaf_adapters.c) through the generated map row, at its
 * one retail call site 800A6CFC (ra 0x800A6D04).  This runs the production
 * runtime source (battle_mips_runtime.c, included below).
 *
 * The site's caller-host clause returns 0 there; caller hosts are on by
 * default only in the native-only build.  Either way the target is a
 * main-executable one, so runtime_bridge_call binds its native body.
 *
 * Checked, with caller hosts off (the hybrid default) or on (argv[1] "hosts"):
 *  - the target resolves to &func_80025D4C;
 *  - a bridged call with the site's argument shapes returns 1, i.e. handled
 *    natively with no interpreter.  The shapes are: the count in a0; one guest
 *    array as source, target and destination (800A6CE8/CF4) or three separate
 *    ones; and the stack words at sp+0x10..0x20 (three signed-byte offsets,
 *    the lbu kind and factor);
 *  - guest RAM and the GTE register file end up exactly as a direct call of
 *    the port body leaves its host copy.
 * The port body's equivalence to the retail bytes is
 * run_color_shift_25d4c_retail_test.sh's job. */
#include <stdlib.h>
#include <string.h>

#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE
#include "psx/gtereg.h"

extern void func_80025D4C(s32 count, u16* source, u16* target, u16* destination, s32 red,
                          s32 green, s32 blue, s32 kind, s32 factor);
extern void MTC2(unsigned int, int);
extern void CTC2(unsigned int, int);

#define SITE_RA 0x800a6d04u
#define ARRAYS 0x80150000u      /* three 0x100-byte arrays in guest RAM */
#define STACK 0x801fe000u

static u32 seed = 0x9e3779b9u;
static u32 next(void) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; }

static void gte_seed(u32 rgbc, u32 ir, u32 flag)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    MTC2(rgbc, 6);
    MTC2(ir, 9);
    MTC2(ir >> 8, 10);
    MTC2(ir >> 16, 11);
    CTC2(flag, 31);
}

int main(int argc, char** argv)
{
    static const s32 offsets[] = {-128, -31, -1, 0, 1, 5, 31, 127};
    static const s32 factors[] = {0, 1, 16, 31, 32, 255};
    static const u32 counts[] = {1, 7, 16};
    static u8 reference[0x300];
    BattleMipsRuntime* rt = calloc(1, sizeof(*rt));
    const ResolvedFunction* fn;
    unsigned cases = 0, failures = 0;
    int hosts = argc > 1 && strcmp(argv[1], "hosts") == 0;

    if (rt == NULL) return 2;
    if (hosts) setenv("XENO_BATTLE_NATIVE_HOSTS", "1", 1);
    g_ActiveBattleRuntime = rt;
    initialize_runtime(rt);
    fn = find_function(rt, 0x80025d4cu);
    if (fn == NULL || fn->host != (void*)func_80025D4C) {
        printf("FAIL 0x80025d4c resolves to %s, not &func_80025D4C\n",
               fn && fn->name ? fn->name : "nothing");
        return 1;
    }
    printf("ok   0x80025d4c -> &func_80025D4C (map row \"%s\")\n", fn->name);

    for (s32 kind = 0; kind < 4; ++kind)
        for (unsigned fi = 0; fi < sizeof(factors) / sizeof(factors[0]); ++fi)
            for (unsigned ci = 0; ci < sizeof(counts) / sizeof(counts[0]); ++ci)
                for (unsigned alias = 0; alias < 2; ++alias) {
                    u8* guest = PSX_ADDR(ARRAYS);
                    u32 source = ARRAYS, target = alias ? ARRAYS : ARRAYS + 0x100,
                        destination = alias ? ARRAYS : ARRAYS + 0x200;
                    s32 red = offsets[next() % 8], green = offsets[next() % 8],
                        blue = offsets[next() % 8];
                    u32 rgbc = next(), ir = next(), flag = next() & 0x7ffff000u;
                    GTERegisters bridged;
                    PcPortMipsCpu cpu;
                    int rc;

                    for (unsigned i = 0; i < 0x300; ++i) guest[i] = (u8)next();
                    guest[2 * (cases % 5)] = guest[2 * (cases % 5) + 1] = 0;  /* a zero pixel */
                    memcpy(reference, guest, sizeof(reference));
                    memset(&cpu, 0, sizeof(cpu));
                    cpu.gpr[4] = counts[ci];
                    cpu.gpr[5] = source;
                    cpu.gpr[6] = target;
                    cpu.gpr[7] = destination;
                    cpu.gpr[29] = STACK;
                    cpu.gpr[31] = SITE_RA;
                    runtime_write(rt, STACK + 0x10, 4, (u32)red);
                    runtime_write(rt, STACK + 0x14, 4, (u32)green);
                    runtime_write(rt, STACK + 0x18, 4, (u32)blue);
                    runtime_write(rt, STACK + 0x1c, 4, (u32)kind);
                    runtime_write(rt, STACK + 0x20, 4, (u32)factors[fi]);

                    gte_seed(rgbc, ir, flag);
                    rc = runtime_bridge_call(rt, &cpu, 0x80025d4cu);
                    bridged = gteRegs;
                    gte_seed(rgbc, ir, flag);
                    func_80025D4C((s32)counts[ci], (u16*)(reference + (source - ARRAYS)),
                                  (u16*)(reference + (target - ARRAYS)),
                                  (u16*)(reference + (destination - ARRAYS)), red, green, blue,
                                  kind, factors[fi]);
                    if (rc != 1 || memcmp(guest, reference, sizeof(reference)) != 0 ||
                        memcmp(&bridged, &gteRegs, sizeof(gteRegs)) != 0) {
                        if (failures++ < 8)
                            printf("FAIL case %u: bridge rc=%d kind=%d factor=%d count=%u "
                                   "alias=%u guest %s, GTE %s\n", cases, rc, kind, factors[fi],
                                   counts[ci], alias,
                                   memcmp(guest, reference, sizeof(reference)) ? "differs" : "equal",
                                   memcmp(&bridged, &gteRegs, sizeof(gteRegs)) ? "differs" : "equal");
                    }
                    ++cases;
                }
    printf("COLOR SHIFT BRIDGE %s %u/%u cases at ra 0x%08x, caller hosts %s: "
           "handled natively (rc 1), guest RAM and GTE as the direct call\n",
           failures ? "FAIL" : "PASS", cases - failures, cases, SITE_RA, hosts ? "on" : "off");
    return failures ? 1 : 0;
}
