/* TEST TOOLING: per-frame battle state hash (XENO_BATTLE_STATE_HASH_LOG=<path>).
 * Adapted from the diagnostic recorder at ebbeb468; no assists gameplay
 * implementation imported. Off unless the variable is set. Used by
 * battle_results_route_driver.py for battle-frame inputs and diagnostic logs.
 * Decoded W records miss native writes and must not be used as parity proof;
 * the separate battle_results_trace.c snapshots observe canonical memory.
 *
 * Called from the battle runtime bridge (battle_mips_runtime.c) before every
 * guest instruction fetch; it only reads guest memory.  Lines:
 *   B <battle> start
 *   F <battle> <frame> <crc32> hp=<slot hp list> gear=<mask> active=<mask>
 *     ticks=<n> d494=<v> acc=<hex>                        at each battle DrawOTag
 *       ticks: TimerWorkListUpdate calls this battle (one per battle tick:
 *       func_800BE790 runs D_80059494-1 catch-up ticks per frame), d494:
 *       D_80059494 (elapsed-vblank tick count, cap 4), acc: D_800C3E88
 *       (motion step accumulator, func_800A9A50)
 *   P <battle> <frame> <mask>                             XENO_BATTLE_TEST_PAD press
 *   D <battle> <frame> <start-of-0x235C-byte g_GameState + transition tuple, hex>  when it changed
 *   A <battle> <frame> row=<r> amounts=<11 x u16>             damage routine entry
 *   R <battle> <frame> results                                 end-of-battle module call
 *   W <battle> <frame> pc=<pc> gs+<off> w=<bytes> old=<v> new=<v>
 *       a guest store into g_GameState's character/gear array (0x26C..0x1648)
 *       or gold (0x1924), decoded before the store executes
 *
 * XENO_BATTLE_TEST_PAD=<start>:<period>:<mask>[,<mask>...] (needs the log
 * variable too) presses game buttons on BATTLE frames: from battle frame
 * <start>, every <period> frames the next mask of the list is held for two
 * Vsyncs (PcPort_PadConsoleHold).  Inputs keyed to the battle's own frame
 * count make two runs of one battle comparable frame by frame.
 *
 * The frame CRC covers the eleven 0x170-byte battler records (D_800CCCE8),
 * the 32 action rows (D_800C3FE8), the enemy AI headers/variables
 * (D_800D3400), the enemy group counters (D_800D301C) and g_GameState. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "battle_mips_adapter.h"

#define BSH_BATTLE_ENTRY 0x80070F40u
#define BSH_DRAW_OTAG    0x80044BD0u
#define BSH_DAMAGE_FN    0x80085618u
#define BSH_GAMESTATE_SIZE 0x235Cu
#define BSH_RESULTS_FN   0x801E252Cu  /* called from battle.bin 0x80071690 */
#define BSH_GAMESTATE    0x8006D634u
#define BSH_TIMER_WORKLIST_UPDATE 0x8001C964u  /* TimerWorkListUpdate, bridged */
#define BSH_MOTION_ACC   0x800C3E88u

extern short D_80059494;

extern unsigned char g_GameState[];

void PcPort_BattleStateHashBeforeGuest(PcPortMipsCpu* cpu, uint32_t target);
void PcPort_PadConsoleHold(unsigned short gameMask, int frames) __attribute__((weak));

static unsigned pad_start, pad_period, pad_count;
static unsigned short pad_masks[16];

static void pad_init(void)
{
    const char* e = getenv("XENO_BATTLE_TEST_PAD");
    char* end;
    if (!e || !*e) return;
    pad_start = (unsigned)strtoul(e, &end, 0);
    if (*end != ':') return;
    pad_period = (unsigned)strtoul(end + 1, &end, 0);
    if (*end != ':' || pad_period == 0) { pad_period = 0; return; }
    do {
        pad_masks[pad_count++] = (unsigned short)strtoul(end + 1, &end, 0);
    } while (*end == ',' && pad_count < 16);
}

static uint32_t crc_table[256];

static uint32_t crc_bytes(uint32_t crc, const unsigned char* p, size_t n)
{
    size_t i;
    if (!crc_table[1]) {
        uint32_t k, c;
        int j;
        for (k = 0; k < 256; k++) {
            for (c = k, j = 0; j < 8; j++) c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            crc_table[k] = c;
        }
    }
    crc = ~crc;
    for (i = 0; i < n; i++) crc = crc_table[(crc ^ p[i]) & 255u] ^ (crc >> 8);
    return ~crc;
}

static uint32_t crc_guest(PcPortMipsCpu* cpu, uint32_t crc, uint32_t address, uint32_t size)
{
    unsigned char buf[256];
    uint32_t done = 0;
    while (done < size) {
        uint32_t n = size - done < sizeof buf ? size - done : (uint32_t)sizeof buf, i, v;
        for (i = 0; i < n; i++)
            buf[i] = cpu->bus.read(cpu->bus.opaque, address + done + i, 1, &v) == 0 ? (unsigned char)v : 0xEEu;
        crc = crc_bytes(crc, buf, n);
        done += n;
    }
    return crc;
}

void PcPort_BattleStateHashBeforeGuest(PcPortMipsCpu* cpu, uint32_t target)
{
    static int inited;
    static FILE* f;
    static unsigned battle, frame;
    static uint32_t last_gs;
    static unsigned ticks;
    unsigned slot;

    if (!inited) {
        const char* path = getenv("XENO_BATTLE_STATE_HASH_LOG");
        inited = 1;
        if (path && path[0]) f = fopen(path, "w");
        if (f) pad_init();
    }
    if (!f) return;
    if (target == BSH_BATTLE_ENTRY) {
        battle++;
        frame = 0;
        last_gs = 0;
        ticks = 0;
        fprintf(f, "B %u start\n", battle);
        fflush(f);
        return;
    }
    if (target == BSH_RESULTS_FN) {
        /* XENO_BATTLE_STATE_HASH_MODULE=<path>: also save the end-of-battle
         * module's guest bytes (0x801DE000..0x801E5000, read from the disc
         * by battle.bin 0x800715AC) for disassembly.  Retail bytes: keep the
         * file out of the repository. */
        const char* mod = getenv("XENO_BATTLE_STATE_HASH_MODULE");
        fprintf(f, "R %u %u results\n", battle, frame);
        fflush(f);
        if (mod && mod[0]) {
            FILE* m = fopen(mod, "wb");
            uint32_t a, v;
            for (a = 0x801DE000u; m && a < 0x801E5000u; a++) {
                if (cpu->bus.read(cpu->bus.opaque, a, 1, &v)) v = 0;
                fputc((int)(v & 255u), m);
            }
            if (m) fclose(m);
        }
    }
    {
        uint32_t ins, op, addr, off, width, old = 0, val;
        if (cpu->bus.read(cpu->bus.opaque, target, 4, &ins) == 0) {
            op = ins >> 26;
            if (op == 0x28 || op == 0x29 || op == 0x2B) {
                width = op == 0x28 ? 1u : op == 0x29 ? 2u : 4u;
                addr = cpu->gpr[(ins >> 21) & 31u] + (uint32_t)(int32_t)(int16_t)(ins & 0xFFFFu);
                off = addr - BSH_GAMESTATE;
                if ((off >= 0x26Cu && off < 0x1648u) || (off >= 0x1924u && off < 0x1928u)) {
                    val = cpu->gpr[(ins >> 16) & 31u];
                    if (width == 1) val &= 0xFFu; else if (width == 2) val &= 0xFFFFu;
                    cpu->bus.read(cpu->bus.opaque, addr, width, &old);
                    fprintf(f, "W %u %u pc=%08x gs+%04x w=%u old=%u new=%u\n",
                            battle, frame, target, off, width, old, val);
                    fflush(f);
                }
            }
        }
    }
    if (target == BSH_TIMER_WORKLIST_UPDATE) {
        ticks++;
        return;
    }
    if (target == BSH_DAMAGE_FN) {
        uint32_t row = (cpu->gpr[4] & 255u) * 72u, v;
        fprintf(f, "A %u %u row=%u amounts=", battle, frame, cpu->gpr[4] & 255u);
        for (slot = 0; slot < 11; slot++) {
            if (cpu->bus.read(cpu->bus.opaque, 0x800C3FE8u + row + slot * 2u, 2, &v)) v = 0xFFFFFFFFu;
            fprintf(f, "%s%u", slot ? "," : "", v);
        }
        fputc('\n', f);
        fflush(f);
        return;
    }
    if (target != BSH_DRAW_OTAG) return;
    {
        uint32_t crc = 0, gs = crc_bytes(0, g_GameState, BSH_GAMESTATE_SIZE), v;
        crc = crc_guest(cpu, crc, 0x800CCCE8u, 11u * 0x170u);
        crc = crc_guest(cpu, crc, 0x800C3FE8u, 32u * 72u);
        crc = crc_guest(cpu, crc, 0x800D3400u, 8u * 64u);
        crc = crc_guest(cpu, crc, 0x800D301Cu, 0x40u);
        crc = crc_bytes(crc, (const unsigned char*)&gs, sizeof gs);
        {
            unsigned gears = 0, actives = 0;
            fprintf(f, "F %u %u %08x hp=", battle, frame, crc);
            for (slot = 0; slot < 11; slot++) {
                uint32_t gear = 0, active = 0;
                cpu->bus.read(cpu->bus.opaque, 0x800C3EB8u + slot * 28u, 1, &gear);
                cpu->bus.read(cpu->bus.opaque, 0x800D2DCCu + slot, 1, &active);
                if (gear) gears |= 1u << slot;
                if (active) actives |= 1u << slot;
                if (cpu->bus.read(cpu->bus.opaque, (gear ? 0x800CCDECu : 0x800CCD34u) + slot * 0x170u,
                                  gear ? 4 : 2, &v)) v = 0;
                fprintf(f, "%s%u", slot ? "," : "", v);
            }
            if (cpu->bus.read(cpu->bus.opaque, BSH_MOTION_ACC, 4, &v)) v = 0xFFFFFFFFu;
            fprintf(f, " gear=%03x active=%03x ticks=%u d494=%d acc=%08x\n", gears, actives,
                    ticks, (int)D_80059494, v);
        }
        if (gs != last_gs) {
            unsigned i;
            last_gs = gs;
            fprintf(f, "D %u %u ", battle, frame);
            for (i = 0; i < BSH_GAMESTATE_SIZE; i++) fprintf(f, "%02x", g_GameState[i]);
            fputc('\n', f);
        }
        if (pad_period && pad_count && frame >= pad_start && (frame - pad_start) % pad_period == 0 &&
            PcPort_PadConsoleHold) {
            unsigned short m = pad_masks[((frame - pad_start) / pad_period) % pad_count];
            PcPort_PadConsoleHold(m, 2);
            fprintf(f, "P %u %u 0x%04x\n", battle, frame, m);
        }
        fflush(f);
        frame++;
    }
}
