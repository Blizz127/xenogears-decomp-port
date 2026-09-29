/* retail_data.h -- load retail game data from the user's disc at runtime.
 *
 * Policy (pc_port/THIRD_PARTY.md): the port ships no game data.  Tables the
 * port reads as native globals (strings, layout tables, compressed images,
 * archive index...) are declared in the pc_port/src/data_*.c TUs as
 * zero-initialised storage, and each TU lists WHERE the bytes live -- a
 * retail file, an offset and a size, never the bytes -- in a
 * XenoRetailRange table.  XENO_RETAIL_DATA_BEGIN/END register a constructor
 * that fills the storage from the user's own files before main() runs:
 *
 *   disc/SLUS_006.64, disc/field.bin, disc/menu.bin, disc/shop_menu.bin,
 *   disc/member_change_menu.bin
 *
 * (the files the matching build extracts; the directory is found like the
 * disc image: $XENO_DATA_DIR, the directory of $XENO_SLUS, disc/, ../disc/,
 * ../../disc/ relative to the working directory, then disc/ or the files
 * next to the executable).  Every file is
 * checked against its retail SHA-256 (config/checksum.sha) before use; a
 * missing or different file stops the program with an explanation.
 * XENO_RETAIL_DATA_OPTIONAL=1 continues with zero-filled data instead
 * (developer escape hatch; the game will not work).
 *
 * The loader is header-only with weak linkage so that every test that links
 * a data TU gets it without extra build inputs; the linker keeps one copy.
 */
#ifndef XENO_RETAIL_DATA_H
#define XENO_RETAIL_DATA_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    XENO_RD_SLUS = 0,        /* SLUS_006.64 (PS-X EXE; offsets include the 0x800 header) */
    XENO_RD_FIELD,           /* field.bin */
    XENO_RD_MENU,            /* menu.bin */
    XENO_RD_SHOP_MENU,       /* shop_menu.bin */
    XENO_RD_MEMBER_CHANGE,   /* member_change_menu.bin */
    XENO_RD_WORLD_MAP,       /* world_map.bin */
    XENO_RD_FILE_COUNT
} XenoRetailFile;

typedef enum {
    XENO_RD_BYTES = 0,  /* copy `size` bytes verbatim */
    XENO_RD_PTR32       /* `size` bytes of little-endian guest pointers -> host
                         * pointers into g_PsxRam (dst is void*[size / 4]) */
} XenoRetailKind;

typedef struct {
    void* dst;
    uint32_t size;      /* bytes in the retail file */
    uint32_t offset;    /* file offset in the retail file */
    uint8_t file;       /* XenoRetailFile */
    uint8_t kind;       /* XenoRetailKind */
    const char* name;
} XenoRetailRange;

/* Retail file offset of a guest address. */
#define XENO_RD_SLUS_OFF(vaddr)    (0x800u + (uint32_t)(vaddr) - 0x80010000u)
#define XENO_RD_FIELD_OFF(vaddr)   ((uint32_t)(vaddr) - 0x8006FAF0u)
#define XENO_RD_MENU_OFF(vaddr)    ((uint32_t)(vaddr) - 0x801C5000u)

#define XENO_RD(sym, file, off, size) \
    { (void*)&(sym), (size), (off), (file), XENO_RD_BYTES, #sym }
#define XENO_RD_PTRS(sym, file, off, count) \
    { (void*)&(sym), (count) * 4u, (off), (file), XENO_RD_PTR32, #sym }

/* One table per data TU, loaded by a constructor before main():
 *   XENO_RETAIL_DATA_BEGIN(tag)
 *       XENO_RD(D_80010004, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80010004), 0x8000),
 *   XENO_RETAIL_DATA_END(tag)
 */
#define XENO_RETAIL_DATA_BEGIN(tag) \
    static const XenoRetailRange s_XenoRetailRanges_##tag[] = {
#define XENO_RETAIL_DATA_END(tag)                                               \
    };                                                                          \
    __attribute__((constructor)) static void XenoRetailDataLoad_##tag(void) {   \
        XenoRetailData_LoadTable(#tag, s_XenoRetailRanges_##tag,                \
            sizeof(s_XenoRetailRanges_##tag) / sizeof(s_XenoRetailRanges_##tag[0])); \
    }

#define XENO_RD_WEAK __attribute__((weak))

/* Guest <-> host pointer mapping for pointer tables, provided by
 * psx_memory.c.  Referenced weakly and never by naming g_PsxRam, so tests
 * that link a data TU without psx_memory.c still link (a weak reference to
 * g_PsxRam is not enough: any other object's strong reference, even in a
 * section --gc-sections later drops, makes the symbol strongly undefined). */
void* PsxMemory_GuestToHost(uint32_t guest) __attribute__((weak));
uint32_t PsxMemory_HostToGuest(const void* host) __attribute__((weak));

typedef struct {
    uint8_t* bytes;
    size_t size;
    int state;          /* 0 unresolved, 1 loaded + verified, -1 unavailable */
    char path[1024];
} XenoRetailFileSlot;

XENO_RD_WEAK XenoRetailFileSlot g_XenoRetailFiles[XENO_RD_FILE_COUNT];
XENO_RD_WEAK unsigned g_XenoRetailRangesLoaded;
XENO_RD_WEAK unsigned g_XenoRetailBytesLoaded;

typedef struct { const char* name; const char* sha256; size_t size; } XenoRetailFileInfo;

XENO_RD_WEAK const XenoRetailFileInfo* XenoRetailData_FileInfo(int file)
{
    /* Retail USA (SLUS-00664) hashes, as in config/checksum.sha. */
    static const XenoRetailFileInfo info[XENO_RD_FILE_COUNT] = {
        { "SLUS_006.64",
          "dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119", 303104 },
        { "field.bin",
          "38a1ce829a6f094c505f67143d6ace2d328418c65425a7383991179467e1fdfc", 260862 },
        { "menu.bin",
          "82f84a24ac1fd1979754e1cc9c861405fb59ce95dd78db00a76f00c8f1bd177d", 153864 },
        { "shop_menu.bin",
          "7890e14bcabddcf85368de10783ecff8daa166dc5ac254e06db5e27959cab4cf", 55296 },
        { "member_change_menu.bin",
          "3b9e2b890c27ae0de97fe343ac75cd35c05fe7f9605ae387d2166da0be78109c", 26624 },
        { "world_map.bin",
          "4c15fd32b3a03d7cd5ea4403dcaabc70abaf99b6aaca65d63d6866803edaac70", 180422 },
    };
    return (file >= 0 && file < XENO_RD_FILE_COUNT) ? &info[file] : NULL;
}

/* --- SHA-256 (FIPS 180-4), for the retail file check ---------------------- */
typedef struct { uint32_t h[8]; uint64_t len; uint8_t buf[64]; size_t n; } XenoSha256;

XENO_RD_WEAK void XenoSha256_Block(XenoSha256* s, const uint8_t* p)
{
    static const uint32_t k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2,
    };
#define XENO_ROR(x, r) (((x) >> (r)) | ((x) << (32 - (r))))
    uint32_t w[64], a, b, c, d, e, f, g, h;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 |
               (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
    for (i = 16; i < 64; i++) {
        uint32_t s0 = XENO_ROR(w[i - 15], 7) ^ XENO_ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = XENO_ROR(w[i - 2], 17) ^ XENO_ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    a = s->h[0]; b = s->h[1]; c = s->h[2]; d = s->h[3];
    e = s->h[4]; f = s->h[5]; g = s->h[6]; h = s->h[7];
    for (i = 0; i < 64; i++) {
        uint32_t t1 = h + (XENO_ROR(e, 6) ^ XENO_ROR(e, 11) ^ XENO_ROR(e, 25)) +
                      ((e & f) ^ (~e & g)) + k[i] + w[i];
        uint32_t t2 = (XENO_ROR(a, 2) ^ XENO_ROR(a, 13) ^ XENO_ROR(a, 22)) +
                      ((a & b) ^ (a & c) ^ (b & c));
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
#undef XENO_ROR
    s->h[0] += a; s->h[1] += b; s->h[2] += c; s->h[3] += d;
    s->h[4] += e; s->h[5] += f; s->h[6] += g; s->h[7] += h;
}

XENO_RD_WEAK void XenoSha256_Init(XenoSha256* s)
{
    static const uint32_t iv[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    memcpy(s->h, iv, sizeof(iv));
    s->len = 0;
    s->n = 0;
}

XENO_RD_WEAK void XenoSha256_Update(XenoSha256* s, const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    s->len += len;
    while (len > 0) {
        size_t take = 64 - s->n;
        if (take > len)
            take = len;
        memcpy(s->buf + s->n, p, take);
        s->n += take;
        p += take;
        len -= take;
        if (s->n == 64) {
            XenoSha256_Block(s, s->buf);
            s->n = 0;
        }
    }
}

XENO_RD_WEAK void XenoSha256_HexFinal(XenoSha256* s, char out[65])
{
    uint64_t bits = s->len * 8;
    uint8_t pad = 0x80, zero = 0, lenbe[8];
    int i;
    XenoSha256_Update(s, &pad, 1);
    while (s->n != 56)
        XenoSha256_Update(s, &zero, 1);
    for (i = 0; i < 8; i++)
        lenbe[i] = (uint8_t)(bits >> (56 - 8 * i));
    XenoSha256_Update(s, lenbe, 8);
    for (i = 0; i < 8; i++)
        sprintf(out + 8 * i, "%08x", s->h[i]);
    out[64] = '\0';
}

/* --- file resolution --------------------------------------------------------- */
XENO_RD_WEAK uint8_t* XenoRetailData_ReadFile(const char* path, size_t* size)
{
    FILE* f = fopen(path, "rb");
    uint8_t* buf = NULL;
    long len;
    if (f == NULL)
        return NULL;
    if (fseek(f, 0, SEEK_END) == 0 && (len = ftell(f)) > 0 && fseek(f, 0, SEEK_SET) == 0) {
        buf = (uint8_t*)malloc((size_t)len);
        if (buf != NULL && fread(buf, 1, (size_t)len, f) != (size_t)len) {
            free(buf);
            buf = NULL;
        }
        *size = (size_t)len;
    }
    fclose(f);
    return buf;
}

XENO_RD_WEAK int XenoRetailData_Optional(void)
{
    const char* v = getenv("XENO_RETAIL_DATA_OPTIONAL");
    return v != NULL && v[0] == '1';
}

XENO_RD_WEAK void XenoRetailData_Fail(const char* what, int file, const char* detail)
{
    const XenoRetailFileInfo* fi = XenoRetailData_FileInfo(file);
    fprintf(stderr,
            "\n[xeno-port] ===== retail data check failed =====\n"
            "[xeno-port] %s: %s\n"
            "[xeno-port] %s\n"
            "[xeno-port] The port contains no game data; it reads it from YOUR copy of\n"
            "[xeno-port] Xenogears (USA, SLUS-00664).  Put the files extracted from your\n"
            "[xeno-port] disc (see README: SLUS_006.64, field.bin, menu.bin, shop_menu.bin,\n"
            "[xeno-port] member_change_menu.bin) in disc/, or point XENO_DATA_DIR (or\n"
            "[xeno-port] XENO_SLUS for the executable) at them.  Expected %s:\n"
            "[xeno-port]   %zu bytes, sha256 %s\n",
            what, fi ? fi->name : "?", detail, fi ? fi->name : "?",
            fi ? fi->size : (size_t)0, fi ? fi->sha256 : "?");
    if (XenoRetailData_Optional()) {
        fprintf(stderr, "[xeno-port] XENO_RETAIL_DATA_OPTIONAL=1: continuing with zero-filled "
                        "retail data (the game will not work correctly)\n\n");
        return;
    }
    fprintf(stderr, "[xeno-port] (XENO_RETAIL_DATA_OPTIONAL=1 skips this check for development)\n\n");
    exit(78); /* EX_CONFIG */
}

/* Directory of the running executable (Linux: /proc/self/exe); 0 if unknown. */
XENO_RD_WEAK int XenoRetailData_ExeDir(char* out, size_t out_size)
{
#if defined(__linux__)
    /* Declared here rather than via <unistd.h>, which clashes with the psyq
     * type definitions some including TUs use; compatible with the libc
     * prototype (ssize_t / size_t are long / unsigned long on LP64). */
    extern long readlink(const char* path, char* buf, unsigned long size);
    long n = readlink("/proc/self/exe", out, (unsigned long)(out_size - 1));
    char* slash;
    if (n <= 0 || (size_t)n >= out_size - 1)
        return 0;
    out[n] = '\0';
    slash = strrchr(out, '/');
    if (slash == NULL)
        return 0;
    *slash = '\0';
    return 1;
#else
    (void)out; (void)out_size;
    return 0;
#endif
}

/* Resolve a user file by name; the single search order for the port's
 * disc/storage layer (xg_plat/disc.h uses it too):
 *   $XENO_BIOS (scph5500.bin only), $XENO_DATA_DIR/<name>,
 *   $XENO_SLUS (SLUS_006.64) or its directory, disc/, ../disc/, ../../disc/
 *   (working directory), then <exe dir>/disc/ and <exe dir>/ (installs).
 * Returns 1 and fills `out`; `tried` (optional) lists the candidates. */
XENO_RD_WEAK int XenoRetailData_Resolve(const char* name, char* out, size_t out_size,
                                        char* tried, size_t tried_size)
{
    static const char* const dirs[] = { "disc", "../disc", "../../disc" };
    const char* env_bios = getenv("XENO_BIOS");
    const char* env_dir = getenv("XENO_DATA_DIR");
    const char* env_slus = getenv("XENO_SLUS");
    char cand[1024];
    char exe_dir[1024];
    unsigned i;
    FILE* f;

    if (tried && tried_size)
        tried[0] = '\0';
    for (i = 0; i < 8; i++) {
        cand[0] = '\0';
        if (i == 0 && env_bios && *env_bios && strcmp(name, "scph5500.bin") == 0)
            snprintf(cand, sizeof cand, "%s", env_bios);
        else if (i == 1 && env_dir && *env_dir)
            snprintf(cand, sizeof cand, "%s/%s", env_dir, name);
        else if (i == 2 && env_slus && *env_slus) {
            const char* slash = strrchr(env_slus, '/');
            if (strcmp(name, "SLUS_006.64") == 0)
                snprintf(cand, sizeof cand, "%s", env_slus);
            else if (slash)
                snprintf(cand, sizeof cand, "%.*s/%s", (int)(slash - env_slus), env_slus, name);
        } else if (i >= 3 && i < 6)
            snprintf(cand, sizeof cand, "%s/%s", dirs[i - 3], name);
        else if (i >= 6 && XenoRetailData_ExeDir(exe_dir, sizeof exe_dir))
            /* An installed port started from any directory: the files next
             * to the executable, or in a disc/ folder beside it. */
            snprintf(cand, sizeof cand, i == 6 ? "%s/disc/%s" : "%s/%s", exe_dir, name);
        if (cand[0] == '\0')
            continue;
        if (tried && strlen(tried) + strlen(cand) + 3 < tried_size) {
            strcat(tried, tried[0] ? ", " : "");
            strcat(tried, cand);
        }
        if ((f = fopen(cand, "rb")) != NULL) {
            fclose(f);
            snprintf(out, out_size, "%s", cand);
            return 1;
        }
    }
    return 0;
}

XENO_RD_WEAK XenoRetailFileSlot* XenoRetailData_File(int file)
{
    XenoRetailFileSlot* slot = &g_XenoRetailFiles[file];
    const XenoRetailFileInfo* fi = XenoRetailData_FileInfo(file);
    char tried[2048] = "";
    char cand[1024];
    char digest[65];
    XenoSha256 sha;

    if (slot->state != 0)
        return slot->state > 0 ? slot : NULL;
    slot->state = -1;
    if (XenoRetailData_Resolve(fi->name, cand, sizeof cand, tried, sizeof tried)) {
        slot->bytes = XenoRetailData_ReadFile(cand, &slot->size);
        if (slot->bytes)
            snprintf(slot->path, sizeof slot->path, "%s", cand);
    }
    if (slot->bytes == NULL) {
        char msg[2200];
        snprintf(msg, sizeof msg, "not found (tried %s)", tried);
        XenoRetailData_Fail("missing retail file", file, msg);
        return NULL;
    }
    XenoSha256_Init(&sha);
    XenoSha256_Update(&sha, slot->bytes, slot->size);
    XenoSha256_HexFinal(&sha, digest);
    if (slot->size != fi->size || strcmp(digest, fi->sha256) != 0) {
        char msg[1400];
        snprintf(msg, sizeof msg, "%s is %zu bytes, sha256 %s -- not the retail USA file "
                 "(wrong region/version, damaged, or a modified build output)",
                 slot->path, slot->size, digest);
        free(slot->bytes);
        slot->bytes = NULL;
        XenoRetailData_Fail("wrong retail file", file, msg);
        return NULL;
    }
    slot->state = 1;
    return slot;
}

/* Developer check: XENO_RETAIL_DATA_DUMP=<dir> writes each loaded object as
 * <dir>/<symbol>.bin (guest u32 values for pointer tables) so a build can be
 * compared with another; the files hold retail bytes, keep them out of git. */
XENO_RD_WEAK void XenoRetailData_Dump(const XenoRetailRange* r, const XenoRetailFileSlot* slot)
{
    const char* dir = getenv("XENO_RETAIL_DATA_DUMP");
    char path[1200];
    FILE* f;
    (void)slot;
    if (dir == NULL || dir[0] == '\0')
        return;
    snprintf(path, sizeof path, "%s/%s.bin", dir, r->name);
    f = fopen(path, "wb");
    if (f == NULL)
        return;
    if (r->kind == XENO_RD_PTR32) {
        void* const* in = (void* const*)r->dst;
        uint32_t n;
        for (n = 0; n < r->size / 4u; n++) {
            uint32_t (*volatile h2g)(const void*) = PsxMemory_HostToGuest; /* weak */
            uint32_t v = (in[n] && h2g) ? h2g(in[n]) : 0;
            fwrite(&v, 4, 1, f);
        }
    } else {
        fwrite(r->dst, 1, r->size, f);
    }
    fclose(f);
}

XENO_RD_WEAK void XenoRetailData_LoadTable(const char* tag, const XenoRetailRange* ranges, size_t count)
{
    size_t i;
    for (i = 0; i < count; i++) {
        const XenoRetailRange* r = &ranges[i];
        XenoRetailFileSlot* slot = XenoRetailData_File(r->file);
        if (slot == NULL)
            continue; /* optional mode: storage stays zero */
        if ((size_t)r->offset + r->size > slot->size) {
            fprintf(stderr, "[xeno-port] retail data %s/%s: range 0x%x+0x%x is past the end of %s\n",
                    tag, r->name, r->offset, r->size, slot->path);
            exit(70);
        }
        if (r->kind == XENO_RD_PTR32) {
            void** out = (void**)r->dst;
            uint32_t n;
            for (n = 0; n < r->size / 4u; n++) {
                const uint8_t* p = slot->bytes + r->offset + 4u * n;
                uint32_t v = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 |
                             (uint32_t)p[3] << 24;
                /* PSX_ADDR() mapping; NULL where psx_memory.c is not linked. */
                void* (*volatile g2h)(uint32_t) = PsxMemory_GuestToHost; /* weak */
                out[n] = (v == 0 || g2h == NULL) ? NULL : g2h(v);
            }
        } else {
            memcpy(r->dst, slot->bytes + r->offset, r->size);
        }
        g_XenoRetailRangesLoaded++;
        g_XenoRetailBytesLoaded += r->size;
        XenoRetailData_Dump(r, slot);
    }
}

#endif /* XENO_RETAIL_DATA_H */
