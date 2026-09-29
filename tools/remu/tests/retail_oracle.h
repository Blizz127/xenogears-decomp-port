#ifndef REMU_RETAIL_ORACLE_H
#define REMU_RETAIL_ORACLE_H

#include <stdio.h>
#include <string.h>
#include "remu.h"

/* Splat moves function oracles when a body is promoted/demoted. Only retry
 * a missing file: a present but invalid oracle must still fail validation. */
static uint32_t remu_load_oracle(remu_t *m, const char *path) {
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return remu_load_s(m, path);
    }
    const char *part = strstr(path, "/nonmatchings/");
    const char *replacement = "/matchings/";
    size_t skip = strlen("/nonmatchings/");
    if (!part) {
        part = strstr(path, "/matchings/");
        replacement = "/nonmatchings/";
        skip = strlen("/matchings/");
    }
    if (!part) return 0;
    char alternate[1024];
    int n = snprintf(alternate, sizeof alternate, "%.*s%s%s",
                     (int)(part - path), path, replacement, part + skip);
    if (n < 0 || (size_t)n >= sizeof alternate) return 0;
    return remu_load_s(m, alternate);
}
#endif
