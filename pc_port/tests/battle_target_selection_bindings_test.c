#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
struct Owner { u8 before, selected, after, target[256]; };
struct Fixture {
    struct Owner owners[2];
    u8 list[16], count;
    unsigned owner, mode, actor, wanted, calls;
};
static void build(struct Fixture *f, u32 actor) {
    assert(actor == f->actor);
    assert(f->owners[0].selected == f->wanted);
    f->owner = f->mode & 1;
    f->owners[f->owner].target[actor] = f->wanted ^ ((f->mode >> 1) & 1);
    f->calls++;
}
#define XBT_SELECTION_CUSTOM_BINDINGS
#define XBT_SELECTION_SIGNATURE static void select_target(struct Fixture *f, u32 arg0)
#define XBT_SELECTION_VALUE f->owners[f->owner].selected
#define XBT_SELECTION_TARGET(a) f->owners[f->owner].target[a]
#define XBT_SELECTION_COUNT f->count
#define XBT_SELECTION_ENTRY(i) f->list[i]
#define XBT_SELECTION_BUILD(a) build(f, a)
#include "../../src/battle/target_selection_impl.inc"

int main(void) {
    unsigned cases=0;
    for (unsigned actor=0; actor<=255; actor+=255)
    for (unsigned count=0; count<=16; count++)
    for (unsigned wanted=0; wanted<256; wanted++)
    for (unsigned mode=0; mode<4; mode++) {
        struct Fixture f;
        memset(&f, 0xA5, sizeof(f));
        f.owner=0; f.mode=mode; f.actor=actor; f.wanted=wanted; f.calls=0;
        f.count=count; f.owners[0].target[actor]=wanted;
        for (unsigned i=0; i<16; i++) f.list[i]=i*17+mode;
        struct Fixture expected;
        memcpy(&expected, &f, sizeof(f));
        expected.owners[0].selected=wanted;
        expected.owner=mode&1; expected.calls=1;
        unsigned target=wanted^((mode>>1)&1), found=0;
        expected.owners[expected.owner].target[actor]=target;
        for (unsigned i=0; i<count; i++) if (f.list[i]==target) found=1;
        if (!found) expected.owners[expected.owner].selected=f.list[0];
        select_target(&f, actor | 0xABCDE000u);
        assert(memcmp(&f, &expected, sizeof(f)) == 0);
        cases++;
    }
    printf("TARGET SELECTION BINDINGS PASS %u cases\n", cases);
}
