#include "common.h"


#ifndef XENO_PC_PORT
/* Pool20 (base, firstFree, count) is typedef'd just after this site, so the
 * fields are reached by offset: +0 base, +4 firstFree, +6 count. */
void func_800A22E8(u8* pool) {
    u8 pad[1];
    u8* slot;
    s32 i;

    if (*(u8**)pool != NULL) {
        slot = *(u8**)pool;
        *(u16*)(pool + 4) = 0;
        for (i = 0; i < *(u16*)(pool + 6); i++) {
            *slot = 0;
            slot += 20;
        }
    }
}

#endif
#ifndef XENO_PC_PORT
/* A pool of 20-byte slots: base, first free index, slot count. */
typedef struct {
    u8* base;
    u16 firstFree;
    u16 count;
} Pool20;

/* Take the first free slot (NULL when full) and advance the first-free index. */
u8* func_800A2330(Pool20* pool) {
    u8* slot;

    if (pool->firstFree < pool->count) {
        slot = pool->base + pool->firstFree * 20;
        if (*slot != 0) {
            return NULL;
        }
        for (pool->firstFree++; pool->firstFree < pool->count; pool->firstFree++) {
            if (pool->base[pool->firstFree * 20] == 0) {
                break;
            }
        }
        return slot;
    }
    return NULL;
}

/* Release a 20-byte pool slot: clear it and lower the first-free index. */
s32 func_800A23E8(Pool20* pool, u8* slot) {
    s32 index;

    if (slot == NULL) {
        return -1;
    }
    index = (u32)(slot - pool->base) / 20;
    if (index < pool->firstFree) {
        pool->firstFree = index;
    }
    *slot = 0;
    return index;
}
#endif
#ifndef XENO_PC_PORT
/* The 0x7C-byte animated node record (main65 views it as M65Node). */
typedef struct BattleNode7C {
    struct BattleNode7C* parent;
    u8 unk4[3];
    u8 pending;
    u8 unk8[2];
    u16 index;
    u8 unkC[0x48];
    s16 rot[3];
    u16 unk5A;
    s32 pos[3];
    u8 unk68[8];
    u8* handle70;
    u8* handle74;
    u8* handle78;
} BattleNode7C;

u16 func_800A1B50(BattleNode7C* nodes, s16* anim);
void func_800A2ACC(s32 ctx, u8* nodes);

/* func_800A2434.s: start the anim record `anim` on a node set.  With +6 set
 * it releases the nodes' handles (func_800A2ACC) and applies the record at
 * once (func_800A1B50), returning 1.  Otherwise each of the first
 * min(+0xC count + 1, node count) nodes gets (or reuses) a rot handle
 * (+0x70) and a pos handle (+0x74), new ones from `list`, keyed into the
 * record past its triples; a handle whose byte 3 is 0xFF is left alone.  A
 * 0xFFFF key instead releases that handle, except on node 0; returns 0. */
s32 func_800A2434(Pool20* list, BattleNode7C* nodes, u8* anim, s32 loop, s32 arg4) {
    u16 n;
    u16 nRot;
    u16 nPos;
    u16 frames;
    u16 flags;
    u8* keys;
    u8* args;
    u8* ch;
    s32 i;

    if (*(u16*)(anim + 6) != 0) {
        func_800A2ACC((s32)list, (u8*)nodes);
        func_800A1B50(nodes, (s16*)anim);
        return 1;
    }
    nRot = *(u16*)(anim + 0xC);
    n = nodes->index;
    nPos = *(u16*)(anim + 0xE);
    loop &= 1;
    if (nRot + 1 < n) {
        n = nRot + 1;
    }
    frames = *(u16*)(anim + 2);
    if (!loop) {
        frames--;
    }
    flags = *(u16*)(anim + 4);
    anim += 0x18;
    keys = anim + (nRot + 1) * 6;
    if (!(flags & 1)) {
        keys += nRot * 6;
    }
    if (!(flags & 2)) {
        keys += nPos * 6;
    }
    for (i = 0; i < n; i++) {
        args = anim + 4;
        if (*(u16*)anim != 0xFFFF) {
            if (nodes->handle70 != NULL) {
                ch = nodes->handle70;
                if (ch[3] == 0xFF) {
                    goto pos;
                }
            } else {
                ch = func_800A2330(list);
            }
            if (ch != NULL) {
                ch[0] = 1;
                ch[1] = loop;
                ch[2] = args[0];
                ch[3] = arg4;
                *(u8**)(ch + 8) = *(u8**)(ch + 4) = keys + *(u16*)anim;
                *(u16*)(ch + 0x10) = 0;
                *(u16*)(ch + 0x12) = frames;
                nodes->handle70 = ch;
            }
        } else if (nodes->handle70 != NULL && i != 0 && (nodes->handle70)[3] != 0xFF) {
            func_800A23E8(list, nodes->handle70);
            nodes->handle70 = NULL;
        }
    pos:
        anim += 2;
        if (*(u16*)anim != 0xFFFF) {
            if (nodes->handle74 != NULL) {
                ch = nodes->handle74;
                if (ch[3] == 0xFF) {
                    goto next;
                }
            } else {
                ch = func_800A2330(list);
            }
            if (ch != NULL) {
                ch[0] = 1;
                ch[1] = loop;
                ch[2] = args[1];
                ch[3] = arg4;
                *(u8**)(ch + 8) = *(u8**)(ch + 4) = keys + *(u16*)anim;
                *(u16*)(ch + 0x10) = 0;
                *(u16*)(ch + 0x12) = frames;
                nodes->handle74 = ch;
            }
        } else if (nodes->handle74 != NULL && i != 0 && (nodes->handle74)[3] != 0xFF) {
            func_800A23E8(list, nodes->handle74);
            nodes->handle74 = NULL;
        }
    next:
        anim += 4;
        nodes++;
    }
    return 0;
}
/* func_800A2704.s: func_800A2434 plus the record's current triples.  When
 * the record carries rot (pos) triples (flags bit 0 (1) clear), each node
 * after node 0 takes the next one into `rot` (`pos`) while they last and is
 * marked at +4 (and +5 for rot); a 0xFFFF key or a handle whose byte 3 is
 * 0xFF only skips that triple.  Unlike func_800A2434 it never releases
 * handles.  The declaration order gives retail's spill slots (args, nRot,
 * nPos, n, frames). */
s32 func_800A2704(Pool20* list, BattleNode7C* nodes, u8* anim, s32 loop, s32 arg4) {
    u8* args;
    u16 nRot;
    u16 nPos;
    u16 n;
    u16 frames;
    u16 flags;
    u16 crot;
    u16 cpos;
    s16* tri;
    u8* keys;
    u8* ch;
    s32 i;

    if (*(u16*)(anim + 6) != 0) {
        func_800A2ACC((s32)list, (u8*)nodes);
        func_800A1B50(nodes, (s16*)anim);
        return 1;
    }
    loop &= 1;
    crot = 0;
    nRot = *(u16*)(anim + 0xC);
    n = nodes->index;
    cpos = 0;
    nPos = *(u16*)(anim + 0xE);
    if (nRot + 1 < n) {
        n = nRot + 1;
    }
    frames = *(u16*)(anim + 2);
    if (!loop) {
        frames--;
    }
    flags = *(u16*)(anim + 4);
    anim += 0x18;
    tri = (s16*)(anim + (nRot + 1) * 6);
    keys = (u8*)tri;
    if (!(flags & 1)) {
        keys += nRot * 6;
    }
    if (!(flags & 2)) {
        keys += nPos * 6;
    }
    for (i = 0; i < n; i++) {
        args = anim + 4;
        if (*(u16*)anim != 0xFFFF) {
            if (nodes->handle70 != NULL) {
                ch = nodes->handle70;
                if (ch[3] == 0xFF) {
                    goto rot_skip;
                }
            } else {
                ch = func_800A2330(list);
            }
            if (!(flags & 1) && i != 0 && crot < nRot) {
                nodes->rot[0] = *tri++;
                crot++;
                nodes->rot[1] = *tri++;
                nodes->rot[2] = *tri++;
                nodes->unk4[0] = 1;
                nodes->unk4[1] = 1;
            }
            if (ch != NULL) {
                ch[0] = 1;
                ch[1] = loop;
                ch[2] = args[0];
                ch[3] = arg4;
                *(u8**)(ch + 8) = *(u8**)(ch + 4) = keys + *(u16*)anim;
                *(u16*)(ch + 0x10) = 0;
                *(u16*)(ch + 0x12) = frames;
                nodes->handle70 = ch;
            }
        } else {
        rot_skip:
            if (!(flags & 1) && i != 0 && crot < nRot) {
                tri += 3;
                crot++;
            }
        }
        anim += 2;
        if (*(u16*)anim != 0xFFFF) {
            if (nodes->handle74 != NULL) {
                ch = nodes->handle74;
                if (ch[3] == 0xFF) {
                    goto pos_skip;
                }
            } else {
                ch = func_800A2330(list);
            }
            if (!(flags & 2) && i != 0 && cpos < nPos) {
                nodes->pos[0] = *tri++;
                cpos++;
                nodes->pos[1] = *tri++;
                nodes->pos[2] = *tri++;
                nodes->unk4[0] = 1;
            }
            if (ch != NULL) {
                ch[0] = 1;
                ch[1] = loop;
                ch[2] = args[1];
                ch[3] = arg4;
                *(u8**)(ch + 8) = *(u8**)(ch + 4) = keys + *(u16*)anim;
                *(u16*)(ch + 0x10) = 0;
                *(u16*)(ch + 0x12) = frames;
                nodes->handle74 = ch;
            }
        } else {
        pos_skip:
            if (!(flags & 2) && i != 0 && cpos < nPos) {
                tri += 3;
                cpos++;
            }
        }
        anim += 4;
        nodes++;
    }
    return 0;
}
/* Walks the BattleNode7C nodes by raw offset: +0xA count, +0x70/+0x74/+0x78
 * handles. */
void func_800A2ACC(s32 ctx, u8* nodes) {
    u8 pad[1];
    s32 i;
    s32 count = *(u16*)(nodes + 0xA);
    u8** h;

    for (i = 0; i < count; i++, nodes += 0x7C) {
        h = (u8**)(nodes + 0x70);
        if (h[0] != NULL && h[0][3] != 0xFF) {
            func_800A23E8(ctx, h[0]);
            h[0] = NULL;
        }
        if (h[1] != NULL && h[1][3] != 0xFF) {
            func_800A23E8(ctx, h[1]);
            h[1] = NULL;
        }
        if (h[2] != NULL && h[2][3] != 0xFF) {
            func_800A23E8(ctx, h[2]);
            h[2] = NULL;
        }
    }
}

void func_800A2BB8(s32 ctx, BattleNode7C* nodes, u8 id) {
    u8 pad[1];
    s32 i;
    s32 count = nodes->index;

    for (i = 0; i < count; i++, nodes++) {
        if (nodes->handle70 != NULL && nodes->handle70[3] == id) {
            func_800A23E8(ctx, nodes->handle70);
            nodes->handle70 = NULL;
        }
        if (nodes->handle74 != NULL && nodes->handle74[3] == id) {
            func_800A23E8(ctx, nodes->handle74);
            nodes->handle74 = NULL;
        }
        if (nodes->handle78 != NULL && nodes->handle78[3] == id) {
            func_800A23E8(ctx, nodes->handle78);
            nodes->handle78 = NULL;
        }
    }
}

#endif
extern void HeapChangeCurrentUser(u32 user, u32 arg);
extern void* HeapAlloc(u32 size, u32 flags);
extern void func_800A2D5C(u8* p);
/* func_800A2CA4.s: allocate (n + 1) 124-byte entries for the list at p
 * (count at +4, cursor +6 zeroed, buffer at +0) and initialise it with
 * func_800A2D5C; NULL when the allocation fails. */
u8* func_800A2CA4(u8* p, s32 n) {
    u8* r;

    HeapChangeCurrentUser(4, 0);
    *(s16*)(p + 4) = n;
    *(s16*)(p + 6) = 0;
    *(u8**)(p + 0) = HeapAlloc((n + 1) * 124, 0);
    if (*(u8**)(p + 0) == NULL) {
        r = NULL;
    } else {
        func_800A2D5C(p);
        r = p;
    }
    return r;
}


extern void HeapFree(u32 p);


/* func_800A2D1C.s */
void func_800A2D1C(u8* p) {
    u32 q = *(u32*)p;

    *(u16*)(p + 4) = 0;
    *(u16*)(p + 6) = 0;
    if (q != 0) {
        HeapFree(q);
    }
    *(u32*)p = 0;
}
