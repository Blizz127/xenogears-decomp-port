/* remu sweep: load every matched func .s, run with stubbed callees,
 * report rc distribution. rc 0/1/3/-1 = decode OK; -2/-3/-4 need triage.
 * Usage: remu_sweep <file...> (paths printed with rc). Pre-touches all RAM
 * with zeros so uninit tracking does not fire; seeds stack + regs. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "remu.h"

int main(int argc, char **argv) {
    int dist[16]; memset(dist, 0, sizeof dist);
    int n = 0;
    for (int i = 1; i < argc; i++) {
        remu_t *m = remu_create();
        if (!m) { printf("NOMEM\n"); return 1; }
        /* pre-touch all RAM with zeros */
        static uint8_t page[4096]; memset(page, 0, sizeof page);
        for (uint32_t a = 0x80000000u; a < 0x80200000u; a += sizeof page)
            remu_poke(m, a, page, sizeof page);
        uint32_t entry = remu_load_s(m, argv[i]);
        if (!entry) { printf("LOADFAIL %s\n", argv[i]); remu_destroy(m); continue; }
        uint32_t sp = remu_get_reg(m, 29);
        uint32_t seed[16]; for (int k = 0; k < 16; k++) seed[k] = 0x100 + k;
        remu_poke(m, sp, seed, sizeof seed);
        int rc = remu_call(m, entry, 0x80180000u, 3, 5, 7);
        int idx = rc < 0 ? 8 - rc : rc;
        if (idx >= 0 && idx < 16) dist[idx]++;
        if (rc == -2 || rc == -3 || rc == -4)
            printf("rc=%d %s stubs=%d%s\n", rc, argv[i],
                   remu_stub_calls(m), remu_stub_log(m));
        remu_destroy(m);
        n++;
    }
    printf("swept=%d rc0=%d rc1=%d rc2=%d rc3=%d budget=%d memfault=%d unknown=%d cop=%d\n",
           n, dist[0], dist[1], dist[2], dist[3], dist[9], dist[10], dist[11], dist[12]);
    return 0;
}
