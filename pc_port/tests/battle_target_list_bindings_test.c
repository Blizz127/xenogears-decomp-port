#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
struct Fixture {
    u8 count, list[14], scratch[14], matching[14], group[256];
    uint16_t rank[11];
    unsigned mask, actor, calls;
};
static u8 *entry(struct Fixture *f, unsigned index) {
    assert(index < 12);
    return &f->list[index+1];
}
static u32 predicate(struct Fixture *f, u32 actor, u32 target) {
    unsigned start = actor < 3 ? 3 : 0;
    assert(actor == f->actor && target == start + f->calls);
    assert(f->count <= 8);
    return 0x80000100u | ((f->mask >> f->calls++) & 1);
}
#define XBT_LIST_CUSTOM_BINDINGS
#define XBT_LIST_SIGNATURE static u32 build(struct Fixture *f, u32 arg0)
#define XBT_LIST_LOCALS u8 *candidates = f->scratch+1; u8 *matching = f->matching+1;
#define XBT_LIST_COUNT f->count
#define XBT_LIST_ENTRY(i) (*entry(f, i))
#define XBT_LIST_GROUP(i) f->group[i]
#define XBT_LIST_RANK(i) f->rank[i]
#define XBT_LIST_ELIGIBLE(a, t) predicate(f, a, t)
#include "../../src/battle/target_list_impl.inc"

int main(void) {
    unsigned cases = 0;
    for (unsigned actor=0; actor<=3; actor+=3)
    for (unsigned mask=0; mask<256; mask++)
    for (unsigned mode=0; mode<4; mode++) {
        struct Fixture f;
        memset(&f, 0xA5, sizeof(f));
        f.actor=actor; f.mask=mask; f.calls=0;
        for (unsigned i=0; i<256; i++) f.group[i] = mode ? (i+mask)%3 : 0;
        for (unsigned i=0; i<11; i++) f.rank[i] = (i*193+mask*71) ^ (mode*0x4000);
        u8 expected[12], selected[12]; unsigned count=0, out=0;
        memset(expected, 255, sizeof(expected));
        unsigned start=actor<3 ? 3 : 0, end=actor<3 ? 11 : 3;
        for (unsigned i=start; i<end; i++) if (mask & (1u<<(i-start))) selected[count++]=i;
        for (unsigned group=0; group<2; group++)
            for (unsigned i=0; i<count; i++)
                if ((f.group[selected[i]] != f.group[actor]) == group) expected[out++]=selected[i];
        for (unsigned i=1; i<count; i++) {
            int matching = f.group[expected[0]] == f.group[actor];
            if ((!matching || f.group[expected[i]] == f.group[actor]) &&
                f.rank[expected[0]] > f.rank[expected[i]]) {
                u8 tmp=expected[0]; expected[0]=expected[i]; expected[i]=tmp;
            }
        }
        assert(build(&f, actor | 0xABCDE000u) == expected[0]);
        assert(f.count == count && f.calls == end-start);
        assert(memcmp(f.list+1, expected, 12) == 0);
        assert(f.list[0] == 0xA5 && f.list[13] == 0xA5);
        assert(f.scratch[0] == 0xA5 && f.scratch[13] == 0xA5);
        assert(f.matching[0] == 0xA5 && f.matching[13] == 0xA5);
        cases++;
    }
    printf("TARGET LIST BINDINGS PASS %u cases\n", cases);
}
