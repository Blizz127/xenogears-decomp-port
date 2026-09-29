#include "krom_rom.h"
#include <errno.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct PcPortKromRom { uint8_t bytes[0x80000]; };

/* Jiskan 16 is a public-domain JIS X 0213 bitmap font. Its 94x94 JIS plane
 * is kept in the same 16-row, two-byte-per-row shape as PS1 KROM glyphs.
 * See pc_port/font_data/README.md for provenance and regeneration. */
static const uint8_t jiskan16[94 * 94][32] = {
#include "krom_jiskan16.inc"
};

static const uint8_t *jiskan16_glyph(uint32_t code) {
    unsigned lead = (code >> 8) & 0xff, trail = code & 0xff;
    unsigned row, cell;
    if (lead >= 0x81 && lead <= 0x9f)
        row = (lead - 0x81) * 2 + 0x21;
    else if (lead >= 0xe0 && lead <= 0xef)
        row = (lead - 0xc1) * 2 + 0x21;
    else
        return (const uint8_t *)(intptr_t)-1;
    if (trail >= 0x40 && trail <= 0x7e)
        cell = trail - 0x1f;
    else if (trail >= 0x80 && trail <= 0xfc)
        cell = trail - 0x20;
    else
        return (const uint8_t *)(intptr_t)-1;
    if (cell > 0x7e) {
        ++row;
        cell -= 0x5e;
    }
    if (row < 0x21 || row > 0x7e || cell < 0x21 || cell > 0x7e)
        return (const uint8_t *)(intptr_t)-1;
    return jiskan16[(row - 0x21) * 94 + cell - 0x21];
}

PcPortKromRomResult PcPortKromRomLoad(const char *path, PcPortKromRom **out) {
    static const uint8_t expected[32] = {
        0x11,0x05,0x2b,0x64,0x99,0xe4,0x66,0xbb,
        0xf0,0xa7,0x09,0xb1,0xf9,0xcb,0x68,0x34,
        0xa9,0x41,0x8e,0x66,0x68,0x03,0x87,0x91,
        0x24,0x51,0xe9,0x71,0xcf,0x8a,0x1f,0xef
    };
    *out = NULL;
    if (!path) return PC_PORT_KROM_ROM_IO;
    /* NONBLOCK prevents a FIFO path from hanging before the regular-file check. */
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) return PC_PORT_KROM_ROM_IO;
    struct stat st;
    PcPortKromRomResult result = PC_PORT_KROM_ROM_IO;
    PcPortKromRom *rom = NULL;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) goto done;
    if (st.st_size != 0x80000) {result = PC_PORT_KROM_ROM_SIZE;goto done;}
    rom = malloc(sizeof *rom);
    if (!rom) {result = PC_PORT_KROM_ROM_MEMORY;goto done;}
    size_t used = 0;
    while (used < sizeof rom->bytes) {
        ssize_t n = read(fd, rom->bytes + used, sizeof rom->bytes - used);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) goto done;
        used += (size_t)n;
    }
    uint8_t extra, digest[SHA256_DIGEST_LENGTH];
    ssize_t n;
    do {n = read(fd, &extra, 1);} while (n < 0 && errno == EINTR);
    if (n < 0) goto done;
    if (n != 0) {result = PC_PORT_KROM_ROM_SIZE;goto done;}
    /* One-shot SHA256 is supported in OpenSSL 3, unlike SHA256_Init/Update.
     * Hash exactly the owned bytes, not a separate pathname read.
     * https://docs.openssl.org/3.0/man3/SHA256_Init/ */
    if (!SHA256(rom->bytes, sizeof rom->bytes, digest) ||
        memcmp(digest, expected, sizeof expected) != 0) {
        result = PC_PORT_KROM_ROM_HASH;
        goto done;
    }
    result = PC_PORT_KROM_ROM_OK;
done:
    if (close(fd) != 0) result = PC_PORT_KROM_ROM_IO;
    if (result == PC_PORT_KROM_ROM_OK) *out = rom;
    else free(rom);
    return result;
}

void PcPortKromRomFree(PcPortKromRom *rom) { free(rom); }

PcPortKromResult PcPortKromRomResolve(const PcPortKromRom *rom, uint32_t code,
                                    const uint8_t *selector, size_t length,
                                    const uint8_t **span) {
    return PcPortKromResolveRomSpan(rom->bytes, code, selector, length, span);
}

static pthread_once_t font_once = PTHREAD_ONCE_INIT;
/* Process-lifetime ownership: published spans must never be invalidated by
 * another lookup or by shutdown ordering between game/render threads. */
static PcPortKromRom *font_rom;
static int use_bios_font;
static void load_font_rom(void) {
    /* The game-data-only default embeds the public-domain Jiskan font. A BIOS
     * is used only when the owner explicitly opts in with XENO_BIOS. */
    const char *path = getenv("XENO_BIOS");
    if (!path || !*path) return;
    use_bios_font = 1;
    PcPortKromRomResult result = PcPortKromRomLoad(path, &font_rom);
    if (result != PC_PORT_KROM_ROM_OK) {
        fprintf(stderr, "KROM_UNRESOLVED load=%d path=%s\n", result, path);
        abort();
    }
}

const uint8_t *PcPortKromFont(uint32_t code, size_t length) {
    if (pthread_once(&font_once, load_font_rom) != 0) {
        fputs("KROM_UNRESOLVED initialization\n", stderr);
        abort();
    }
    if (!use_bios_font) {
        const uint8_t *glyph = jiskan16_glyph(code);
        if (glyph == (const uint8_t *)(intptr_t)-1) return glyph;
        if (length > 32) {
            fprintf(stderr, "KROM_UNRESOLVED font length=%zu\n", length);
            abort();
        }
        return glyph;
    }
    const uint8_t *span;
    PcPortKromResult result = PcPortKromRomResolve(font_rom, code, NULL, length, &span);
    if (result == PC_PORT_KROM_OK) return span;
    if (result == PC_PORT_KROM_REJECTED) return (const uint8_t*)(intptr_t)-1;
    fprintf(stderr, "KROM_UNRESOLVED mapping=%d code=%08x length=%zu\n", result, code, length);
    abort();
}
