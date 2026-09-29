/* Host integration, not a retail movie fixture: real file descriptors feed
 * production func_80028F30. Two sectors become contiguous headers/payloads. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "system/archive.h"
#include "psyq/pc.h"

extern int func_80028F30(u32*, u32*);

static _Alignas(4) u8 stream[0x8000];
void* g_ArchiveCurStreamFile;
u32 g_ArchiveDebugTable;
int g_ArchiveCurFileSize;
int D_8004FE40;
ArchiveStreamFileSectionHeader* D_8004FE2C;
s32 D_8004FE4C, D_8004FE10;
u16 D_8004FE24, D_8004FE26;
s8* D_8004FE08;
u_char D_80059F18[4];
u8 *D_80059F54, *D_80059F58;
s16 D_80059F5C, D_80059F60;
u16 D_8005A4B8;
u8 D_800596F8[8];

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "ASSERTION archive.file line=%d %s\n", __LINE__, #expr); \
    exit(1); \
} } while (0)

static void run_case(const char* directory, int cd_layout)
{
    u8 input[0x1240] = {0};
    u8 headers[2][0x20], payloads[2][0x7E0];
    char path[4096];
    const int stride = cd_layout ? 0x920 : 0x800;
    const int prefix = cd_layout ? 8 : 0;
    struct { u32 before, payload, header, after; } out = {
        0xABCD1234, 0xCCCCCCCC, 0xDDDDDDDD, 0x1234ABCD
    };
    int writer;
    u8* buffers = stream + 4 * 8 + 0x24;

    for (int sector = 0; sector < 2; ++sector) {
        for (int i = 0; i < 0x20; ++i)
            headers[sector][i] = (u8)(i + 17 * sector);
        headers[sector][6] = 2;
        headers[sector][7] = 0;
        headers[sector][8] = 0x34;
        headers[sector][9] = 0x12;
        for (int i = 0; i < 0x7E0; ++i)
            payloads[sector][i] = (u8)(i * 37 + sector * 83);
        memcpy(input + sector * stride + prefix, headers[sector], 0x20);
        memcpy(input + sector * stride + prefix + 0x20,
               payloads[sector], 0x7E0);
    }
    CHECK(snprintf(path, sizeof(path), "%s/archive-file-XXXXXX", directory)
          < (int)sizeof(path));
    writer = mkstemp(path);
    CHECK(writer >= 0);
    /* Independent POSIX writer: the bridge under test only reads the fixture. */
    CHECK(write(writer, input, (size_t)(2 * stride)) == 2 * stride);
    CHECK(close(writer) == 0);
    D_8004FE4C = PCopen(path, 0, 0);
    CHECK(unlink(path) == 0);
    CHECK(D_8004FE4C >= 0);

    memset(stream, 0, sizeof(stream));
    *(s32*)stream = 4;
    CHECK((uintptr_t)(stream + sizeof(stream)) <= UINT32_MAX);
    g_ArchiveCurStreamFile = stream;
    g_ArchiveDebugTable = 1;
    g_ArchiveCurFileSize = 0x1000;
    D_8004FE40 = 4;
    D_8004FE2C = (ArchiveStreamFileSectionHeader*)(stream + 4);
    D_8004FE08 = (s8*)buffers;
    D_8004FE10 = D_8004FE24 = D_8004FE26 = 0;
    D_80059F54 = D_80059F58 = NULL;
    D_80059F5C = D_80059F60 = D_8005A4B8 = 0;
    memset(D_80059F18, 0, sizeof(D_80059F18));
    D_80059F18[0] = cd_layout ? 8 : 0;
    memset(D_800596F8, 0, sizeof(D_800596F8));

    /* A too-small free slot must rewind the header (including the CD probe). */
    D_8004FE2C[0].size = 1;
    CHECK(func_80028F30(&out.payload, &out.header) == 1);
    CHECK(lseek(D_8004FE4C, 0, SEEK_CUR) == 0);
    CHECK(out.header == 0 && out.payload == 0xCCCCCCCC);
    CHECK(g_ArchiveCurFileSize == 0x1000 && D_80059F60 == 0);

    D_8004FE2C[0].size = 4;
    CHECK(func_80028F30(&out.payload, &out.header) == 1);
    CHECK(lseek(D_8004FE4C, 0, SEEK_CUR) == stride);
    CHECK(g_ArchiveCurFileSize == 0x800 && D_8004FE24 == 0);
    CHECK(D_80059F60 == 1);
    CHECK(func_80028F30(&out.payload, &out.header) == 0);
    CHECK(lseek(D_8004FE4C, 0, SEEK_CUR) == 2 * stride);
    CHECK(out.header == (u32)(uintptr_t)buffers);
    CHECK(out.payload == (u32)(uintptr_t)(buffers + 0x40));
    CHECK(memcmp((void*)(uintptr_t)out.header, headers, sizeof(headers)) == 0);
    CHECK(memcmp((void*)(uintptr_t)out.payload, payloads, sizeof(payloads)) == 0);
    CHECK(g_ArchiveCurFileSize == 0 && D_8004FE24 == 2);
    CHECK(D_80059F60 == 0 && D_8005A4B8 == 0x1234);
    CHECK(out.before == 0xABCD1234 && out.after == 0x1234ABCD);
    CHECK(func_80028F30(&out.payload, &out.header) == 1);
    CHECK(out.header == 0 && D_8004FE24 == 2);
    CHECK(PCclose(D_8004FE4C) == 0);
}

int main(int argc, char** argv)
{
    CHECK(argc == 2);
    run_case(argv[1], 0);
    run_case(argv[1], 1);
    puts("ARCHIVE FILE INTEGRATION PASS: rewind, two-sector assembly, CD-layout seeks, output ABI");
    return 0;
}
