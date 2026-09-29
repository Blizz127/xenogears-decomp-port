/* remu smoke test: func_80077844 stores a0..a3 + 6 stack args as 9 shorts. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "remu.h"

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, "asm/field/matchings/main/main/func_80077844.s");
    if (entry != 0x80077844u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    uint32_t dst = 0x80180000u;
    uint16_t vals[9] = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555,
                         0x6666, 0x7777, 0x8888, 0x9999 };
    /* stack args d..i at sp+0x10..sp+0x24 (o32 caller layout) */
    uint32_t sp = remu_get_reg(m, 29);
    uint32_t stackargs[6];
    for (int i = 0; i < 6; i++) stackargs[i] = vals[3 + i];
    if (remu_poke(m, sp + 0x10, stackargs, sizeof stackargs)) {
        printf("FAIL poke stack\n"); return 1;
    }
    uint32_t zero[9]; memset(zero, 0, sizeof zero);
    if (remu_poke(m, dst, zero, sizeof zero)) { printf("FAIL poke dst\n"); return 1; }

    int rc = remu_call(m, entry, dst, vals[0], vals[1], vals[2]);
    if (rc != 0) { printf("FAIL call rc=%d stubs=%d%s\n", rc,
        remu_stub_calls(m), remu_stub_log(m)); return 1; }
    uint16_t got[9];
    remu_peek(m, dst, got, sizeof got);
    for (int i = 0; i < 9; i++) {
        if (got[i] != (vals[i] & 0xFFFF)) {
            printf("FAIL dst[%d]=%04X want %04X\n", i, got[i], vals[i]);
            return 1;
        }
    }
    printf("REMU SMOKE OK\n");
    remu_destroy(m);
    return 0;
}
