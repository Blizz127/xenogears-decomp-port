#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int32_t s32;
typedef struct { char member; } HeapBlock;
static s32 first, second, busy, after_wait;
static HeapBlock replacement;
static void *buffer, *unpin_arg, *free_arg;
static int loader_arg;
static char observed[64];
static size_t count;

static void event(char c)
{
    assert(count + 1 < sizeof observed);
    observed[count++] = c;
    observed[count] = 0;
}

/* a/b/p report evaluation of the first key, second key and pointer lvalues. */
static s32 *first_slot(void) { event('a'); return &first; }
static s32 *second_slot(void) { event('b'); return &second; }
static void **buffer_slot(void) { event('p'); return &buffer; }
#define D_8004F330 (*first_slot())
#define D_8004F334 (*second_slot())
#define D_8005A4E0 (*buffer_slot())

static int ArchiveDataSync(void) { event('s'); return busy; }
static int ArchiveCdDataSync(int mode)
{
    assert(mode == 0);
    event('w');
    second = after_wait;
    return -17;
}
static void HeapUnpinBlock(HeapBlock *ptr)
{
    event('u');
    unpin_arg = ptr;
    buffer = &replacement;
}
static unsigned int HeapFree(void *ptr) { event('f'); free_arg = ptr; return 13; }
static void func_8001B53C(int index) { event('l'); loader_arg = index; }

#include "field_archive_request.inc"

static void expect(int key0, s32 key1, int status, const char *events)
{
    count = 0;
    observed[0] = 0;
    assert(func_8001B484(key0, key1) == status);
    if (strcmp(observed, events) != 0) {
        fprintf(stderr, "expected %s, observed %s\n", events, observed);
        assert(0);
    }
}

int main(void)
{
    first = 0x12340051;
    second = -1;
    expect(first, -1, 0, "ba");
    second = 0x76540017;
    expect(first, second, 0, "ba");

    busy = -81;
    expect(first, 0x43210017, -1, "bs");
    busy = 3;
    expect(0x43210051, second, -1, "bas");
    assert(first == 0x12340051 && second == 0x76540017);

    busy = 0;
    second = -1;
    after_wait = 37;
    buffer = NULL;
    unpin_arg = &replacement;
    expect(-654321, 37, -1, "bswbpupflba");
    assert(unpin_arg == NULL && free_arg == &replacement);
    assert(loader_arg == -654321 && first == -654321 && second == 37);
    expect(-654321, 37, 0, "ba");

    after_wait = -1;
    expect(-654321, -1, -1, "bswblba");
    assert(loader_arg == -654321 && first == -654321 && second == -1);
    expect(-654321, -1, 0, "ba");
    puts("8 synthetic archive request cases passed");
}
