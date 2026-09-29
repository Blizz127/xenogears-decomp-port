/* Differential test: host-compiled src/field/main/main.c func_80077844
 * vs the retail bytes (asm/field/matchings/main/main/func_80077844.s)
 * executed by remu. The function stores a0..a3 + 6 o32 stack args as 9
 * shorts (last store in the jr delay slot).
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "remu.h"

void func_80077844(short *dst, int a, int b, int c, int d, int e, int f,
                   int g, int h, int i);

#define RETAIL_S "asm/field/matchings/main/main/func_80077844.s"
#define DST 0x80180000u

static int check_one(remu_t *m, uint32_t entry, const int v[9]) {
    /* host side */
    short hdst[9];
    func_80077844(hdst, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]);
    /* retail side: stack args d..i at sp+0x10 */
    uint32_t sp = remu_get_reg(m, 29);
    uint32_t stackargs[6];
    for (int k = 0; k < 6; k++) stackargs[k] = (uint32_t)v[3 + k];
    if (remu_poke(m, sp + 0x10, stackargs, sizeof stackargs)) {
        printf("FAIL poke stack\n");
        return 0;
    }
    uint32_t zero[9]; memset(zero, 0, sizeof zero);
    if (remu_poke(m, DST, zero, sizeof zero)) { printf("FAIL poke dst\n"); return 0; }
    int rc = remu_call(m, entry, DST, (uint32_t)v[0], (uint32_t)v[1], (uint32_t)v[2]);
    if (rc != 0) {
        printf("FAIL retail rc=%d stubs=%d%s\n", rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL unexpected stubs%s\n", remu_stub_log(m));
        return 0;
    }
    uint16_t got[9];
    remu_peek(m, DST, got, sizeof got);
    for (int k = 0; k < 9; k++) {
        if ((short)got[k] != hdst[k]) {
            printf("FAIL argset dst[%d] retail=%d host=%d\n", k,
                   (short)got[k], hdst[k]);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x80077844u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    int n = 0;
    int edges[] = { 0, 1, -1, 0x7FFF, -0x8000, 0x7FFFFFFF, (int)0x80000000,
                    (int)0xFFFFFFFF, 0x1234, 0xFFFF8000 };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        int v[9];
        for (int k = 0; k < 9; k++) v[k] = edges[(i + k) % 10];
        if (!check_one(m, entry, v)) return 1;
        n++;
    }
    uint32_t s = 0x9E3779B9u;
    for (int i = 0; i < 250; i++) {
        int v[9];
        for (int k = 0; k < 9; k++) { s = s * 1103515245u + 12345u; v[k] = (int)(s >> 16) - 0x8000; }
        if (!check_one(m, entry, v)) return 1;
        n++;
    }
    printf("DIFF 80077844 OK (%d argsets, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
