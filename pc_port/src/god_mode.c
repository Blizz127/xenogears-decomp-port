/* Optional host debugging aids.  Both default to retail behaviour: GOD mode off
 * and random encounters on.  Turning either off preserves the retail instruction
 * stream - every hook below is host-side state or a guarded call-site skip, and
 * the random-battle switch writes no guest word. */
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include "god_mode.h"

/* Retail 85618 consumes an action row: HP damage kinds 0/5/7/8, amounts at
 * C3FE8 + action*72 + slot*2, kinds at C4000+action*72.  Slots 0..2 are party,
 * 3..10 enemies.  C3EB8+slot*28 selects Gear. */
#define XENO_ACTION_FN      0x80085618u
#define XENO_ACTIVE_TABLE   0x800D2DCCu
#define XENO_DAMAGE_TABLE   0x800C3FE8u
#define XENO_KIND_TABLE     0x800C4000u
#define XENO_GEAR_TABLE     0x800C3EB8u
#define XENO_PARTY_SLOTS    3u
#define XENO_ENEMY_SLOTS    11u
/* Enough to one-shot every ordinary forest enemy (they sit in the low hundreds
 * of HP) while staying a sane 16-bit amount, so a boss with more HP still takes
 * the hit and the row-consume path stays honest. */
#define XENO_MASS_DAMAGE    9999u
#define XENO_DAMAGE_ABSORB  0x8000u

static atomic_int enabled;
static atomic_int random_battles = 1;

/* XENO_GOD_MODE=1 starts the run with party protection already on.  The
 * toolbar click and the hotkey both need the window to own input, which a
 * headless or scripted session cannot rely on -- and a driver that has to
 * reach a boss it is 40 levels short of needs protection from frame one, not
 * from whenever a keypress happens to land. */
static void god_mode_init(void)
{
    static int inited;
    const char* env;

    if (inited) return;
    inited = 1;
    env = getenv("XENO_GOD_MODE");
    if (env != NULL && env[0] != '\0' && env[0] != '0') {
        atomic_store(&enabled, 1);
        fprintf(stderr, "[xeno-port][god-mode] ON from XENO_GOD_MODE\n");
    }
}

int PcPort_GodModeEnabled(void) { god_mode_init(); return atomic_load(&enabled); }

void PcPort_GodModeToggle(void)
{
    int value = !PcPort_GodModeEnabled();
    atomic_store(&enabled, value);
    fprintf(stderr, "[xeno-port][god-mode] %s (party HP damage blocked, "
            "enemy damage forced to %u, foot and Gear)\n",
            value ? "ON" : "OFF", (unsigned)XENO_MASS_DAMAGE);
}

int PcPort_RandomBattlesEnabled(void) { return atomic_load(&random_battles); }

void PcPort_RandomBattlesToggle(void)
{
    int value = !PcPort_RandomBattlesEnabled();
    atomic_store(&random_battles, value);
    fprintf(stderr, "[xeno-port][random-battles] %s (route testing; field "
            "encounter rolls suppressed while off)\n",
            value ? "ON" : "OFF");
}

void PcPort_GodModeBeforeGuest(PcPortMipsCpu* cpu, uint32_t target)
{
    /* Change the queued damage before HP subtraction/death flags, never HP
     * initialization, healing, fuel, EP, enemy state or story flags. */
    if (target != XENO_ACTION_FN || !PcPort_GodModeEnabled()) return;

    uint32_t row = (cpu->gpr[4] & 255u) * 72u;
    for (uint32_t slot = 0; slot < XENO_ENEMY_SLOTS; ++slot) {
        uint32_t active, kind, gear, amount;
        int party = slot < XENO_PARTY_SLOTS;
        uint32_t address = XENO_DAMAGE_TABLE + row + slot * 2u;

        if (cpu->bus.read(cpu->bus.opaque, XENO_ACTIVE_TABLE + slot, 1, &active) ||
            !active ||
            cpu->bus.read(cpu->bus.opaque, XENO_KIND_TABLE + row + slot, 1, &kind) ||
            (kind != 0 && kind != 5 && kind != 7 && kind != 8) ||
            cpu->bus.read(cpu->bus.opaque, XENO_GEAR_TABLE + slot * 28u, 1, &gear) ||
            cpu->bus.read(cpu->bus.opaque, address, 2, &amount)) {
            continue;
        }
        if (amount == 0 || (!gear && (amount & XENO_DAMAGE_ABSORB))) continue;

        if (party) {
            /* Immortal party: drop the incoming damage entirely. */
            if (cpu->bus.write(cpu->bus.opaque, address, 2, 0) == 0) {
                fprintf(stderr, "[xeno-port][god-mode] blocked %u HP damage "
                        "slot=%u gear=%u\n",
                        amount, slot, gear != 0);
            }
        } else if (amount < XENO_MASS_DAMAGE) {
            /* Mass damage: make every enemy hit lethal.  The Gear flag rides in
             * the amount for Gear actions and must be preserved. */
            uint32_t boosted = XENO_MASS_DAMAGE;
            if (cpu->bus.write(cpu->bus.opaque, address, 2, boosted) == 0) {
                fprintf(stderr, "[xeno-port][god-mode] mass damage %u -> %u "
                        "slot=%u gear=%u\n",
                        amount, boosted, slot, gear != 0);
            }
        }
    }
}
