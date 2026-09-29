/* Differential test for the two camera-sector scan helpers that were ASM_ONLY
 * until 2026-09-06 (func_8007234C forward, func_80072398 backward) plus their
 * retail byte pins.
 *
 * Both are compiled from the full production src/field/main/misc2.c TU. The
 * oracle below is an independent formulation: retail mutates `index` and walks
 * a run, this indexes the sector table directly by iteration number, so a
 * transcription slip in the production body cannot be mirrored here.
 *
 * The retail-byte pins read disc/field.bin (the shipped field overlay, sha256
 * 38a1ce82...) so the test also fails if the pinned function moves or the
 * matching build stops corresponding to the shipped bytes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Sector-blocking bit table. Retail lives at 0x800ADC1C; the driver owns the
 * definition natively and reloads it from the shipped overlay at startup. */
unsigned char D_800ADC1C[8];

/* Production definitions under test (src/field/main/misc2.c). */
extern int func_8007234C(unsigned int mask, unsigned int index);
extern int func_80072398(unsigned int mask, unsigned int index);

#define FIELD_BIN_BASE 0x8006FAF0u
#define DC1C_OFFSET (0x800ADC1Cu - FIELD_BIN_BASE)

struct pin {
    const char *name;
    unsigned offset;
    unsigned length;
    unsigned first;
    unsigned last;
    unsigned checksum;
};

/* Pinned from disc/field.bin; see run_field_asmonly_retail_test.sh. */
static const struct pin kPins[] = {
    { "func_8007234C", 0x285C, 76, 0x00001821u, 0x00000000u, 0x00000E04u },
    { "func_80072398", 0x28A8, 76, 0x00001821u, 0x00000000u, 0x00001014u },
};

static int failures;

static void fail(const char *what, unsigned mask, unsigned index, int got, int want)
{
    printf("FIELD ASMONLY FAIL %s mask=0x%02X index=%u got=%d want=%d\n",
           what, mask, index, got, want);
    failures++;
}

/* Independent oracle: how many consecutive sectors, walking from `index` in
 * `step`, are blocked by `mask`. A full lap of 8 reports 0, matching retail's
 * "everything blocked -> no preferred direction" result. */
static int oracle(unsigned mask, unsigned index, int step)
{
    int n;
    for (n = 0; n < 8; n++) {
        unsigned at = (index + (unsigned)(step * n)) & 7u;
        if ((mask & D_800ADC1C[at]) == 0) {
            return n;
        }
    }
    return 0;
}

static unsigned rd32(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) |
           ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "disc/field.bin";
    unsigned char *raw;
    long size;
    FILE *f;
    unsigned mask, index;
    size_t i;
    long cases = 0;

    f = fopen(path, "rb");
    if (!f) {
        printf("FIELD ASMONLY FAIL cannot open %s\n", path);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    raw = malloc((size_t)size);
    if (!raw || fread(raw, 1, (size_t)size, f) != (size_t)size) {
        printf("FIELD ASMONLY FAIL cannot read %s\n", path);
        return 1;
    }
    fclose(f);

    /* Retail byte pins: the shipped overlay still holds the functions we
     * matched, unmoved and unmodified. */
    for (i = 0; i < sizeof(kPins) / sizeof(kPins[0]); i++) {
        const struct pin *p = &kPins[i];
        unsigned sum = 0, j;
        if (p->offset + p->length > (unsigned)size) {
            printf("FIELD ASMONLY FAIL pin %s out of range\n", p->name);
            failures++;
            continue;
        }
        for (j = 0; j < p->length; j++) {
            sum += raw[p->offset + j];
        }
        if (rd32(raw + p->offset) != p->first ||
            rd32(raw + p->offset + p->length - 4) != p->last ||
            sum != p->checksum) {
            printf("FIELD ASMONLY FAIL pin %s first=%08X last=%08X sum=%08X\n",
                   p->name, rd32(raw + p->offset),
                   rd32(raw + p->offset + p->length - 4), sum);
            failures++;
        } else {
            printf("FIELD ASMONLY PIN OK %s @0x%04X %u bytes\n",
                   p->name, p->offset, p->length);
        }
    }

    /* Reload the sector table from the shipped overlay rather than trusting a
     * hand-typed copy. */
    memcpy(D_800ADC1C, raw + DC1C_OFFSET, 8);
    {
        static const unsigned char expect[8] = {
            0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08
        };
        if (memcmp(D_800ADC1C, expect, 8) != 0) {
            printf("FIELD ASMONLY FAIL D_800ADC1C table drifted\n");
            failures++;
        } else {
            printf("FIELD ASMONLY PIN OK D_800ADC1C @0x%05X\n", DC1C_OFFSET);
        }
    }

    /* Exhaustive differential over every mask and every starting sector, in
     * both scan directions. */
    for (mask = 0; mask < 256; mask++) {
        for (index = 0; index < 8; index++) {
            int got = func_8007234C(mask, index);
            int want = oracle(mask, index, +1);
            if (got != want) {
                fail("forward", mask, index, got, want);
            }
            cases++;

            got = func_80072398(mask, index);
            want = oracle(mask, index, -1);
            if (got != want) {
                fail("backward", mask, index, got, want);
            }
            cases++;
        }
    }

    /* The &7 wrap must make high starting indices behave as their low
     * equivalents; retail relies on this for the 0xFFF/0x200 heading sectors. */
    for (mask = 0; mask < 256; mask++) {
        for (index = 0; index < 8; index++) {
            unsigned high = index + 0x7FF8u;
            if (func_8007234C(mask, high) != func_8007234C(mask, index)) {
                fail("forward-wrap", mask, high, 0, 0);
            }
            if (func_80072398(mask, high) != func_80072398(mask, index)) {
                fail("backward-wrap", mask, high, 0, 0);
            }
            cases += 2;
        }
    }

    /* Spot anchors taken straight off the retail algorithm, so a defect that
     * happens to agree with the oracle everywhere still trips here. */
    {
        /* No bits set: first sector already clear -> 0 consecutive. */
        if (func_8007234C(0x00, 0) != 0) { fail("anchor-empty", 0, 0, func_8007234C(0x00, 0), 0); }
        /* All bits set: a full lap of 8 reports 0, not 8. */
        if (func_8007234C(0xFF, 0) != 0) { fail("anchor-full", 0xFF, 0, func_8007234C(0xFF, 0), 0); }
        if (func_80072398(0xFF, 3) != 0) { fail("anchor-full-back", 0xFF, 3, func_80072398(0xFF, 3), 0); }
        /* Sector 0 is bit 0x10, sector 1 is 0x20: two blocked then stop. */
        if (func_8007234C(0x30, 0) != 2) { fail("anchor-run2", 0x30, 0, func_8007234C(0x30, 0), 2); }
        /* Backward from sector 1 covers sectors 1 then 0 -> also 2. */
        if (func_80072398(0x30, 1) != 2) { fail("anchor-run2-back", 0x30, 1, func_80072398(0x30, 1), 2); }
        cases += 5;
    }

    if (failures) {
        printf("FIELD ASMONLY FAILED %d of %ld cases\n", failures, cases);
        return 1;
    }
    printf("FIELD ASMONLY OK %ld cases, 0 failures\n", cases);
    return 0;
}
