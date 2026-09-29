#include <stdio.h>

#include "common.h"
#include "system/archive.h"

extern s32 D_8005A470;
extern u16 D_80062514;
extern void func_8002A498(int channel);

u32 g_ArchiveDebugTable;
int D_8004FE34;
int D_8004FE38;
int g_ArchiveCurFileSize;
s32 D_8004FDFC;
s32 D_8004FE4C;

static int close_calls;
static int close_results[4];
static int close_result_count;

int PCclose(int fd) {
    if (fd != 41) return -99;
    close_calls++;
    if (close_calls <= close_result_count) return close_results[close_calls - 1];
    return 0;
}

static int check(const char* label, int condition) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int main(void) {
    int failures = 0;

    D_8005A470 = 7;
    D_80062514 = 0x1234;
    failures += check("movie globals are writable", D_8005A470 == 7 && D_80062514 == 0x1234);

    g_ArchiveDebugTable = 0;
    D_8004FE34 = 0;
    D_8004FE38 = 99;
    g_ArchiveCurFileSize = 77;
    D_8004FDFC = 1;
    D_8004FE4C = 41;
    close_calls = 0;
    close_result_count = 0;
    func_8002A498(-7);
    failures += check("normal path marks stream stop", D_8004FE34 == 1 && D_8004FE38 == -7);
    failures += check("normal path leaves non-debug state alone",
                      g_ArchiveCurFileSize == 77 && D_8004FDFC == 1 && D_8004FE4C == 41);
    failures += check("normal path does not close debug handle", close_calls == 0);

    g_ArchiveDebugTable = 1;
    D_8004FE34 = 0;
    D_8004FE38 = 0;
    g_ArchiveCurFileSize = 77;
    D_8004FDFC = 1;
    D_8004FE4C = 41;
    close_calls = 0;
    close_results[0] = 1;
    close_results[1] = 2;
    close_results[2] = 0;
    close_result_count = 3;
    func_8002A498(3);
    failures += check("debug path records channel and stop", D_8004FE34 == 1 && D_8004FE38 == 3);
    failures += check("debug path clears transfer state", g_ArchiveCurFileSize == 0 && D_8004FDFC == 0);
    failures += check("debug path retries close and invalidates handle", close_calls == 3 && D_8004FE4C == -1);

    g_ArchiveDebugTable = 1;
    D_8004FE4C = -1;
    close_calls = 0;
    func_8002A498(4);
    failures += check("debug path accepts an already closed handle", close_calls == 0 && D_8004FE4C == -1);

    if (failures) return 1;
    puts("boot archive globals: PASS");
    return 0;
}
