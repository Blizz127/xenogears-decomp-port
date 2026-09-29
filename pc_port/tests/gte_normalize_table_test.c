/* The computed normalisation table (pc_port/src/gte_normalize_table.h) must
 * equal the table the retail executable carries at 0x80056B94, read from the
 * user's disc/SLUS_006.64 at run time (no retail bytes in this file). */
#include <stdio.h>
#include <stdlib.h>
#include "../src/gte_normalize_table.h"

int main(int argc, char** argv)
{
    const char* path = argc > 1 ? argv[1] : "disc/SLUS_006.64";
    unsigned char raw[XENO_GTE_NORM_TABLE_LEN * 2];
    int16_t table[XENO_GTE_NORM_TABLE_LEN];
    FILE* f = fopen(path, "rb");
    int i, bad = 0;
    if (f == NULL || fseek(f, 0x800 + 0x80056B94 - 0x80010000, SEEK_SET) != 0 ||
        fread(raw, 1, sizeof raw, f) != sizeof raw) {
        fprintf(stderr, "gte_normalize_table: cannot read %s\n", path);
        return 2;
    }
    fclose(f);
    XenoGteNormTableFill(table);
    for (i = 0; i < XENO_GTE_NORM_TABLE_LEN; i++) {
        int16_t want = (int16_t)(raw[2 * i] | raw[2 * i + 1] << 8);
        if (table[i] != want) {
            if (bad++ < 8)
                fprintf(stderr, "entry %d: computed %d retail %d\n", i, table[i], want);
        }
    }
    printf("gte_normalize_table: %s (%d/%d entries differ)\n", bad ? "FAIL" : "PASS",
           bad, XENO_GTE_NORM_TABLE_LEN);
    return bad != 0;
}
