#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main65", func_8009F5B8);
typedef struct {
    u8 pad0[0x5C];
    void* buf; /* +0x5C */
    s32 size;  /* +0x60 */
    u8 pad64[0x18];
} Entry; /* 0x7C */

typedef struct {
    u8 pad0[0xA];
    u16 count;        /* +0xA */
    Entry entries[1]; /* +0xC */
} Set;


/* free every entry buffer of the set (clearing buf/size), then the set. */
void func_8009F708(Set* set) {
    s32 i;
    Entry* e;

    if (set != NULL) {
        i = 0;
        if (i < set->count) {
            e = set->entries;
            do {
                void* buf = e->buf;

                i++;
                if (buf != NULL) {
                    HeapFree(buf);
                    e->buf = NULL;
                    e->size = 0;
                }
                e++;
            } while (i < set->count);
        }
        set->count = 0;
        HeapFree(set);
    }
}

typedef struct {
    u8** items;
    u32 count;
} ItemList;

extern void func_8002CBBC(u8* item);

/* release an ItemList: optionally func_8002CBBC every item, then free the
 * pointer array. */
void func_8009F794(ItemList* list, s32 release) {
    u32 i;

    if (list != NULL) {
        for (i = 0; i < list->count; i++) {
            if (list->items != NULL && list->items[i] != NULL && release != 0) {
                func_8002CBBC(list->items[i]);
            }
        }
        if (list->items != NULL) {
            HeapFree(list->items);
            list->items = NULL;
        }
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main65", func_8009F844);
INCLUDE_ASM("asm/battle/nonmatchings/main65", func_800A0838);
INCLUDE_ASM("asm/battle/nonmatchings/main65", func_800A1B50);
INCLUDE_ASM("asm/battle/nonmatchings/main65", func_800A1CF4);
#endif
extern void func_800A23E8(u8* a0, u32 p);
/* func_800A216C.s: release up to three slot pointers (+0x70/+0x74/+0x78) of
 * entry idx in the 124-byte array at a1, each gated by its own mask bit, when
 * idx is below the +0xA count. */
void func_800A216C(u8* a0, u8* a1, s32 idx, u32 mask) {
    u8* e = a1;

    if (idx < *(u16*)(e + 0xA)) {
        e += idx * 124;

        if (*(u32*)(e + 0x70) != 0 && (mask & 1)) {
            func_800A23E8(a0, *(u32*)(e + 0x70));
            *(u32*)(e + 0x70) = 0;
        }
        if (*(u32*)(e + 0x74) != 0 && (mask & 2)) {
            func_800A23E8(a0, *(u32*)(e + 0x74));
            *(u32*)(e + 0x74) = 0;
        }
        if (*(u32*)(e + 0x78) != 0 && (mask & 4)) {
            func_800A23E8(a0, *(u32*)(e + 0x78));
            *(u32*)(e + 0x78) = 0;
        }
    }
}
/* func_800A2234.s: the halfword store precedes the HeapChangeCurrentUser call
 * in C; sched sinks it below the arg setups into the jal delay slot. */
extern void HeapChangeCurrentUser(u32 a0, u32 a1);
extern void* HeapAlloc(u32 size, u32 flag);
extern void func_800A22E8(u8* p);
u8* func_800A2234(u8* a0, int a1) {
    void* p;
    int size;
    if (a1 <= 0)
        return 0;
    *(u16*)(a0 + 6) = a1;
    HeapChangeCurrentUser(4, 0);
    size = a1 * 20;
    p = HeapAlloc(size, 0);
    *(void**)a0 = p;
    if (p != 0) {
        func_800A22E8(a0);
        return a0;
    }
    return 0;
}


extern void HeapFree(u32 p);


/* func_800A22A8.s */
void func_800A22A8(u8* p) {
    u32 q = *(u32*)p;

    *(u16*)(p + 4) = 0;
    if (q != 0) {
        HeapFree(q);
    }
    *(u32*)p = 0;
}
