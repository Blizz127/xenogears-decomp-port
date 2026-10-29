#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
extern u8 g_GameState[];

/* 1 when entry idx's +0x5B47 byte (20-byte stride off D_800C34B0) is one of
 * the three Gear slot ids (+0x98D/+0x995/+0x99D) of the current Gear party
 * record (g_GameState[0x59C] selects the 0xA4-stride record). */
s32 func_8009E53C(u8 idx) {
    u8 id = D_800C34B0[idx * 20 + 0x5B47];
    s32 o = g_GameState[0x59C] * 0xA4;

    if (g_GameState[0x98D + o] == id || g_GameState[0x995 + o] == id) {
        return 1;
    }
    return g_GameState[0x99D + o] == id;
}

#ifndef XENO_PC_PORT
/* func_8009E5C8.s, byte-exact: the Gear twin of main55's func_8009A854.
 * Copy entry a0's bytes +0xE/+0x11/+0x10 (20-byte stride at
 * D_800C34B0+0x5750) into the slot of the current Gear record
 * (g_GameState[0x59C] selects the 0xA4-stride record) whose id
 * (+0x98D/+0x995/+0x99D) matches the entry's +0xF byte.  The three ids
 * select slots 0, 1 and 3, the last match wins, and no match keeps a1.
 * Then record a0 at g_GameState[rec + 0x97C + slot], mirror +0xC into
 * g_GameState+0x2286[a0] and patch every type-4 actor among the first
 * three.  Retail stores +0x11 twice.  The 0xA4 product is spelled
 * (x * 41) << 2: under gcc 2.6.3 a plain `* 0xA4` leaves the product in a
 * temporary plus a copy into the variable, CSE splits the uses between
 * the two and every register shifts.  `s = a1` is retail's int copy of
 * the slot (made after k), and the loop recomputes s * 8. */
void func_8009E5C8(u8 a0, u8 a1) {
    u8* e = D_800C34B0 + (a0 * 20 + 0x5750);
    s32 rec = (g_GameState[0x59C] * 41) << 2;
    u8 id = e[0xF];
    s32 k;
    s32 s;
    u8 i;

    if (g_GameState[0x98D + rec] == id) {
        a1 = 0;
    }
    if (g_GameState[0x995 + rec] == id) {
        a1 = 1;
    }
    if (g_GameState[0x99D + rec] == id) {
        a1 = 3;
    }
    k = a1 * 8 + rec;
    g_GameState[k + 0x98A] = e[0xE];
    g_GameState[k + 0x98C] = e[0x11];
    g_GameState[k + 0x98B] = e[0x10];
    g_GameState[k + 0x98C] = e[0x11];
    s = a1;
    g_GameState[0x97C + s + rec] = a0;
    g_GameState[0x2286 + a0] = e[0xC];
    for (i = 0; i < 3; i++) {
        s32 a = i * 0x170;

        if (D_800C34B0[a + 0x56] == 4) {
            s32 x = s * 8 + a;

            *(D_800C34B0 + x + 0xB6) = e[0xE];
            *(D_800C34B0 + x + 0xB8) = e[0x11];
            *(D_800C34B0 + x + 0xB7) = e[0x10];
            *(D_800C34B0 + x + 0xB8) = e[0x11];
            *(D_800C34B0 + a + s + 0xA8) = a0;
        }
    }
}

extern u8* D_800D2D6C;

/* func_8009E788.s, byte-exact: by the D_800C34B0+0x5FC2 mode, tick down
 * the g_GameState+0x22B6 counter of D_800D2D6C's +4 id (modes 0, 15), of
 * its +7 id (modes 2, 17), or of both (modes 3-14).  The switch table is
 * this object's .rodata (placed at jtbl_800704CC by config/battle.yaml).
 * Each case has its own decrement: the final jump pass cross-jumps the
 * copies into retail's shared tail, and modes 0 and 15 keep separate
 * blocks.  One decrement after the switch lets it merge those two blocks
 * (six instructions short).  -= 1 gives retail's `addiu 0xFF` under gcc
 * 2.6.x (main64's BattleGcc263Div preset is 2.6.3), as in main57's
 * func_8009AFD8. */
void func_8009E788(void) {
    u32 id;

    switch (D_800C34B0[0x5FC2]) {
    case 0:
        id = D_800D2D6C[4];
        if (g_GameState[0x22B6 + id] != 0) {
            g_GameState[0x22B6 + id] -= 1;
        }
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        id = D_800D2D6C[4];
        if (g_GameState[0x22B6 + id] != 0) {
            g_GameState[0x22B6 + id] -= 1;
        }
        id = D_800D2D6C[7];
        if (g_GameState[0x22B6 + id] != 0) {
            g_GameState[0x22B6 + id] -= 1;
        }
        break;
    case 15:
        id = D_800D2D6C[4];
        if (g_GameState[0x22B6 + id] != 0) {
            g_GameState[0x22B6 + id] -= 1;
        }
        break;
    case 2:
    case 17:
        id = D_800D2D6C[7];
        if (g_GameState[0x22B6 + id] != 0) {
            g_GameState[0x22B6 + id] -= 1;
        }
        break;
    }
}

/* func_8009E868.s, byte-exact: map (menu kind, bit mask) to the
 * D_800C34B0+0x5FC7 command code for kinds 0, 1 and 3; unknown pairs store
 * nothing.  Each case stores its code itself (retail loads D_800C34B0 in
 * every case; the first case of each kind stores in its own delay slot,
 * the rest share one `sb`), in retail's block order. */
void func_8009E868(u8 kind, u16 mask) {
    switch (kind) {
    case 0:
        switch (mask) {
        case 0x400:
            D_800C34B0[0x5FC7] = 0x24;
            break;
        case 0x200:
            D_800C34B0[0x5FC7] = 0x25;
            break;
        case 0x100:
            D_800C34B0[0x5FC7] = 0x26;
            break;
        case 0x80:
            D_800C34B0[0x5FC7] = 0x27;
            break;
        case 0x40:
            D_800C34B0[0x5FC7] = 0x28;
            break;
        case 0x20:
            D_800C34B0[0x5FC7] = 0x29;
            break;
        case 0x10:
            D_800C34B0[0x5FC7] = 0x2A;
            break;
        case 0x4:
            D_800C34B0[0x5FC7] = 0x2B;
            break;
        }
        break;
    case 1:
        switch (mask) {
        case 0x1000:
            D_800C34B0[0x5FC7] = 0x2D;
            break;
        case 0x800:
            D_800C34B0[0x5FC7] = 0x2E;
            break;
        case 0x400:
            D_800C34B0[0x5FC7] = 0x2F;
            break;
        case 0x40:
            D_800C34B0[0x5FC7] = 0x15;
            break;
        case 0x20:
            D_800C34B0[0x5FC7] = 0x16;
            break;
        case 0x8:
        case 0x2:
            D_800C34B0[0x5FC7] = 0x19;
            break;
        case 0x4:
        case 0x1:
            D_800C34B0[0x5FC7] = 0x1A;
            break;
        }
        break;
    case 3:
        switch (mask) {
        case 0x8000:
            D_800C34B0[0x5FC7] = 0x1B;
            break;
        case 0x4000:
            D_800C34B0[0x5FC7] = 0x1C;
            break;
        case 0x2000:
            D_800C34B0[0x5FC7] = 0x1D;
            break;
        case 0x1000:
            D_800C34B0[0x5FC7] = 0x1E;
            break;
        case 0x400:
            D_800C34B0[0x5FC7] = 0x1F;
            break;
        case 0x800:
            D_800C34B0[0x5FC7] = 0x20;
            break;
        case 0x100:
            D_800C34B0[0x5FC7] = 0x21;
            break;
        case 0x200:
            D_800C34B0[0x5FC7] = 0x22;
            break;
        }
        break;
    }
}

typedef struct {
    u8** items;
    u32 count;
} ItemList;

extern void HeapChangeCurrentUser(u32 user, u32 a1);
extern void* HeapAlloc(u32 size, u32 flags);
extern u32 func_8002C3E8(u8* data);

/* build a pointer list over the 0x38-byte records at data + 0x10. */
ItemList* func_8009EBA8(u8* data, ItemList* list) {
    u32 n;
    u32 i;

    HeapChangeCurrentUser(4, 0);
    n = func_8002C3E8(data);
    list->items = HeapAlloc(n * 4, 0);
    list->count = n;
    if (list->items != NULL) {
        for (i = 0; i < n; i++) {
            list->items[i] = data + 0x10 + i * 0x38;
        }
    }
    return list;
}

#include "psyq/libgte.h"

/* 0x7C-byte node of the set built by func_8009EC4C; node 0 is the header. */
typedef struct BattlePartNode {
    struct BattlePartNode* parent; /* +0x00 */
    u8 unk4;
    u8 unk5;
    u8 unk6;
    u8 unk7;
    u16 part;  /* +0x08: ItemList index, 0xFFFF for none */
    u16 index; /* +0x0A: node number; in node 0, the node count */
    MATRIX local; /* +0x0C */
    MATRIX world; /* +0x2C */
    u16 unk4C;
    u16 unk4E;
    u16 unk50;
    u16 unk52;
    u16 unk54;
    u16 unk56;
    u16 unk58;
    u16 unk5A;
    u32 unk5C;
    u32 unk60;
    u32 unk64;
    u32 model;     /* +0x68: func_8002CB54 output */
    u32 modelCopy; /* +0x6C */
    u32 unk70;
    u32 unk74;
    u32 unk78;
} BattlePartNode;

extern void func_8002CB54(u8* modelData, u32* out1, u32* out2);
extern void func_8002CC10(s16 x, s16 y);
extern void func_8002CC74(s16 x, s16 y);
extern void func_8002C8CC(u8* a0, void* a1, s32 a2);
extern void func_8009F708(void* set);
extern void* memcpy(void* dst, const void* src, u32 n);

/* func_8009EC4C.s: build a node set from (part, parent) u16 pairs; the list
 * runs while part is below list->count or is 0xFFFF.  Node 0 is a header
 * holding the node count; node k + 1 gets pair k: its parent pointer (none
 * for 0xFFFF), and for a real part the func_8002CB54 output for list item
 * `part`, set up through func_8002C8CC (after func_8002CC10/func_8002CC74
 * with a4..a7 when a3 is set) and copied to the second buffer.  Returns NULL
 * when there are no pairs or an allocation fails. */
BattlePartNode* func_8009EC4C(ItemList* list, u16* pairs, s32 a2, s32 a3, s16 a4, s16 a5,
                              s16 a6, s16 a7) {
    u16* p;
    s32 n;
    BattlePartNode* set;
    BattlePartNode* node;
    s32 idx;
    u16 part;
    u16 parent;

    HeapChangeCurrentUser(4, 0);
    p = pairs;
    for (n = 0; p[0] < list->count || p[0] == 0xFFFF; p += 2) {
        n++;
    }
    if (n == 0) {
        return NULL;
    }
    n++;
    set = HeapAlloc(n * sizeof(BattlePartNode), 0);
    if (set == NULL) {
        return NULL;
    }
    p = pairs;
    node = set + 1;
    idx = 1;
    part = p[0];
    parent = p[1];
    set->unk4 = 1;
    set->unk5 = 1;
    set->unk6 = 1;
    set->unk4C = 0x1000;
    set->unk4E = 0x1000;
    set->unk50 = 0x1000;
    set->parent = NULL;
    set->unk7 = 0;
    set->part = 0xFFFF;
    set->index = n;
    set->model = 0;
    set->modelCopy = 0;
    set->unk54 = 0;
    set->unk56 = 0;
    set->unk58 = 0;
    set->unk5C = 0;
    set->unk60 = 0;
    set->unk64 = 0;
    set->unk70 = 0;
    set->unk74 = 0;
    set->unk78 = 0;
    while (part < list->count || part == 0xFFFF) {
        if (parent == 0xFFFF) {
            node->parent = NULL;
        } else {
            node->parent = set + parent + 1;
        }
        node->index = idx++;
        node->unk4 = 1;
        node->unk5 = 1;
        node->unk7 = 1;
        node->unk4C = 0x1000;
        node->unk4E = 0x1000;
        node->unk50 = 0x1000;
        node->unk6 = 0;
        node->unk52 = 0;
        node->part = part;
        if (part != 0xFFFF) {
            func_8002CB54(list->items[part], &node->model, &node->modelCopy);
            if (node->model == 0) {
                func_8009F708(set);
                return NULL;
            }
            if (a3 != 0) {
                func_8002CC10(a4, a5);
                func_8002CC74(a6, a7);
            }
            func_8002C8CC(list->items[part], (void*)node->model, a2);
            memcpy((void*)node->modelCopy, (void*)node->model,
                   *(u32*)(list->items[part] + 0x34));
            node->unk54 = 0;
        } else {
            node->model = 0;
            node->modelCopy = 0;
            node->unk54 = 0;
        }
        node->unk56 = 0;
        node->unk58 = 0;
        node->unk5C = 0;
        node->unk60 = 0;
        node->unk64 = 0;
        node->unk70 = 0;
        node->unk74 = 0;
        node->unk78 = 0;
        node++;
        p += 2;
        part = p[0];
        parent = p[1];
    }
    return set;
}
/* func_8009EF3C.s, byte-exact: rebuild the matrices of a part-node set
 * for the 4.12 multiplier `scale`.  The header's world matrix gets its
 * position (+0x5C) and its RotMatrix / RotMatrixYXZ (+6) rotation (+0x54);
 * its local matrix is that times the scratchpad matrix diag(scale * the
 * +0x4C/+0x4E/+0x50 scales >> 12).  Each later node with +5 set rebuilds
 * its local rotation and clears +5, takes +4 = 1 from a parent whose +4 is
 * 1, and with +4 set takes its position and composes its world matrix with
 * the parent's (CompMatrix) or, without a parent, copies the local one.
 * Then +4 is cleared on every node after the header.  Returns the node
 * count.  The parameter is the walking `node` and `set` keeps the header:
 * the other way round, retail's two prologue copies come out swapped. */
u32 func_8009EF3C(BattlePartNode* node, s32 scale) {
    BattlePartNode* set = node;
    MATRIX* sm = (MATRIX*)0x1F800000;
    u32 count = set->index;
    u32 i;

    set->world.t[0] = set->unk5C;
    set->world.t[1] = set->unk60;
    set->world.t[2] = set->unk64;
    if (set->unk6) {
        RotMatrixYXZ((SVECTOR*)&set->unk54, &set->world);
    } else {
        RotMatrix((SVECTOR*)&set->unk54, &set->world);
    }
    sm->m[0][0] = (scale * (s16)node->unk4C) >> 12;
    sm->m[0][1] = 0;
    sm->m[0][2] = 0;
    sm->m[1][0] = 0;
    sm->m[1][1] = (scale * (s16)node->unk4E) >> 12;
    sm->m[1][2] = 0;
    sm->m[2][0] = 0;
    sm->m[2][1] = 0;
    sm->m[2][2] = (scale * (s16)node->unk50) >> 12;
    MulMatrix0(&node->world, sm, &node->local);
    node->local.t[0] = node->world.t[0];
    node->local.t[1] = node->world.t[1];
    node->local.t[2] = node->world.t[2];
    for (i = 1; i < count; i++) {
        node++;
        if (node->unk5) {
            if (node->unk6) {
                RotMatrixYXZ((SVECTOR*)&node->unk54, &node->local);
            } else {
                RotMatrix((SVECTOR*)&node->unk54, &node->local);
            }
            node->unk5 = 0;
        }
        if (node->parent != NULL && node->parent->unk4 == 1) {
            node->unk4 = 1;
        }
        if (node->unk4) {
            node->local.t[0] = node->unk5C;
            node->local.t[1] = node->unk60;
            node->local.t[2] = node->unk64;
            if (node->parent != NULL) {
                CompMatrix(&node->parent->world, &node->local, &node->world);
            } else {
                node->world = node->local;
            }
        }
    }
    for (i = 1; i < count; i++) {
        set++;
        set->unk4 = 0;
    }
    return count;
}

/* func_8009F1C4.s, byte-exact: func_8009EF3C's rebuild for the other model
 * kind (func_800AAA20 picks it when obj+0x37 is set).  The header is built
 * the same way.  Each later node first takes +5 = 1 and +4 = 1 from a
 * parent whose flag is 1.  With +5 set it rebuilds its local rotation,
 * multiplies it by diag(+0x4C/+0x4E/+0x50) and, with a parent, by
 * diag(0x1000000 / the parent's scales) (scratchpad matrix, MulMatrix0);
 * with +4 set it composes its world matrix as func_8009EF3C does.  +5 and
 * +4 are cleared on every node after the header at the end.  Returns the
 * node count.  The signed divisions carry aspsx's divide checks (the
 * BattleGcc263Div preset); the loop's scratchpad pointer is set at the top
 * of the +5 block, so it stays in a register across the calls. */
u32 func_8009F1C4(BattlePartNode* node, s32 scale) {
    BattlePartNode* set = node;
    MATRIX* sm = (MATRIX*)0x1F800000;
    u32 count = set->index;
    u32 i;

    set->world.t[0] = set->unk5C;
    set->world.t[1] = set->unk60;
    set->world.t[2] = set->unk64;
    if (set->unk6) {
        RotMatrixYXZ((SVECTOR*)&set->unk54, &set->world);
    } else {
        RotMatrix((SVECTOR*)&set->unk54, &set->world);
    }
    sm->m[0][0] = (scale * (s16)node->unk4C) >> 12;
    sm->m[0][1] = 0;
    sm->m[0][2] = 0;
    sm->m[1][0] = 0;
    sm->m[1][1] = (scale * (s16)node->unk4E) >> 12;
    sm->m[1][2] = 0;
    sm->m[2][0] = 0;
    sm->m[2][1] = 0;
    sm->m[2][2] = (scale * (s16)node->unk50) >> 12;
    MulMatrix0(&node->world, sm, &node->local);
    node->local.t[0] = node->world.t[0];
    node->local.t[1] = node->world.t[1];
    node->local.t[2] = node->world.t[2];
    for (i = 1; i < count; i++) {
        node++;
        if (node->parent != NULL) {
            if (node->parent->unk5 == 1) {
                node->unk5 = 1;
            }
            if (node->parent->unk4 == 1) {
                node->unk4 = 1;
            }
        }
        if (node->unk5) {
            MATRIX* m = (MATRIX*)0x1F800000;

            if (node->unk6) {
                RotMatrixYXZ((SVECTOR*)&node->unk54, &node->local);
            } else {
                RotMatrix((SVECTOR*)&node->unk54, &node->local);
            }
            m->m[0][0] = node->unk4C;
            m->m[0][1] = 0;
            m->m[0][2] = 0;
            m->m[1][0] = 0;
            m->m[1][1] = node->unk4E;
            m->m[1][2] = 0;
            m->m[2][0] = 0;
            m->m[2][1] = 0;
            m->m[2][2] = node->unk50;
            MulMatrix0(&node->local, m, &node->local);
            if (node->parent != NULL) {
                m->m[0][0] = 0x1000000 / (s16)node->parent->unk4C;
                m->m[0][1] = 0;
                m->m[0][2] = 0;
                m->m[1][0] = 0;
                m->m[1][1] = 0x1000000 / (s16)node->parent->unk4E;
                m->m[1][2] = 0;
                m->m[2][0] = 0;
                m->m[2][1] = 0;
                m->m[2][2] = 0x1000000 / (s16)node->parent->unk50;
                MulMatrix0(m, &node->local, &node->local);
            }
        }
        if (node->unk4) {
            node->local.t[0] = node->unk5C;
            node->local.t[1] = node->unk60;
            node->local.t[2] = node->unk64;
            if (node->parent != NULL) {
                CompMatrix(&node->parent->world, &node->local, &node->world);
            } else {
                node->world = node->local;
            }
        }
    }
    for (i = 1; i < count; i++) {
        set++;
        set->unk5 = 0;
        set->unk4 = 0;
    }
    return count;
}

#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif


/* func_8009F5B0.s */
void func_8009F5B0(void) {
}


/* ---- Port bodies (fo/bat3). ---- */
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2D6C;
#endif
#ifdef XENO_PC_PORT
/* func_8009E5C8.s: the Gear counterpart of func_8009A854: copy entry a0's
 * bytes (+0xE/+0x10/+0x11, 20-byte stride at D_800C34B0+0x5750) into the
 * Gear slot whose id matches its +0xF byte (slot 0, 1 or 3; the last match
 * wins, none keeps a1), record a0, mirror +0xC into g_GameState+0x2286[a0],
 * then patch every type-4 actor among the first three.  Retail stores +0x11
 * twice. */
void func_8009E5C8(u32 a0, u32 a1) {
    u32 idx = a0 & 0xFF;
    u8* t0 = D_800C34B0 + idx * 20 + 0x5750;
    u32 a2 = g_GameState[0x59C] * 0xA4;
    u32 id = t0[0xF];
    u32 s;
    u32 o;
    u32 i;

    if (g_GameState[0x98D + a2] == id) a1 = 0;
    if (g_GameState[0x995 + a2] == id) a1 = 1;
    if (g_GameState[0x99D + a2] == id) a1 = 3;
    s = a1 & 0xFF;
    o = s * 8;
    g_GameState[0x98A + o + a2] = t0[0xE];
    g_GameState[0x98C + o + a2] = t0[0x11];
    g_GameState[0x98B + o + a2] = t0[0x10];
    g_GameState[0x98C + o + a2] = t0[0x11];
    g_GameState[0x59C + 0x3E0 + a2 + s] = a0;
    g_GameState[0x2286 + idx] = t0[0xC];
    for (i = 0; i < 3; i++) {
        u32 a3 = i * 0x170;

        if (D_800C34B0[a3 + 0x56] == 4) {
            u32 x = o + a3;

            D_800C34B0[x + 0xB6] = t0[0xE];
            D_800C34B0[x + 0xB8] = t0[0x11];
            D_800C34B0[x + 0xB7] = t0[0x10];
            D_800C34B0[x + 0xB8] = t0[0x11];
            D_800C34B0[a3 + s + 0xA8] = a0;
        }
    }
}

static void bat3_dec_gs22b6(u32 id) {
    u8 v = g_GameState[0x22B6 + id];

    if (v != 0) {
        g_GameState[0x22B6 + id] = v - 1;
    }
}

/* func_8009E788.s: by the D_800C34B0+0x5FC2 mode (jtbl_800704CC), tick down
 * the g_GameState+0x22B6 counters of D_800D2D6C's +4 id (modes 0, 15), its
 * +7 id (modes 2, 17) or both (modes 3-14). */
void func_8009E788(void) {
    u32 mode = D_800C34B0[0x5FC2];

    if (mode >= 0x12) {
        return;
    }
    switch (mode) {
    case 0:
    case 15:
        bat3_dec_gs22b6(D_800D2D6C[4]);
        break;
    case 1:
    case 16:
        break;
    case 2:
    case 17:
        bat3_dec_gs22b6(D_800D2D6C[7]);
        break;
    default:
        bat3_dec_gs22b6(D_800D2D6C[4]);
        bat3_dec_gs22b6(D_800D2D6C[7]);
        break;
    }
}

/* func_8009E868.s: map (menu kind, bit mask) to the D_800C34B0+0x5FC7 command
 * code for kinds 0, 1 and 3; unknown pairs store nothing. */
void func_8009E868(u8 kind, u16 mask) {
    u32 code = 0;

    switch (kind) {
    case 0:
        switch (mask) {
        case 0x400: code = 0x24; break;
        case 0x200: code = 0x25; break;
        case 0x100: code = 0x26; break;
        case 0x080: code = 0x27; break;
        case 0x040: code = 0x28; break;
        case 0x020: code = 0x29; break;
        case 0x010: code = 0x2A; break;
        case 0x004: code = 0x2B; break;
        }
        break;
    case 1:
        switch (mask) {
        case 0x0020: code = 0x16; break;
        case 0x0002:
        case 0x0008: code = 0x19; break;
        case 0x0001:
        case 0x0004: code = 0x1A; break;
        case 0x0040: code = 0x15; break;
        case 0x0400: code = 0x2F; break;
        case 0x0800: code = 0x2E; break;
        case 0x1000: code = 0x2D; break;
        }
        break;
    case 3:
        switch (mask) {
        case 0x8000: code = 0x1B; break;
        case 0x4000: code = 0x1C; break;
        case 0x2000: code = 0x1D; break;
        case 0x1000: code = 0x1E; break;
        case 0x0400: code = 0x1F; break;
        case 0x0800: code = 0x20; break;
        case 0x0100: code = 0x21; break;
        case 0x0200: code = 0x22; break;
        }
        break;
    }
    if (code != 0) {
        D_800C34B0[0x5FC7] = code;
    }
}
#endif /* XENO_PC_PORT */
#ifdef XENO_PC_PORT
/* Instruction-faithful port bodies generated from the retail listings by
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/main64/<fn>.s --args N --jal-service
 * (one statement per MIPS instruction; include/battle_lift.h has the
 * register, memory, COP2, frame and call contract), committed as reviewed
 * source. */
#include "battle_lift.h"

/* func_8009EF3C.s (162 instructions): rebuild model a0's hierarchy matrices
 * for frame/scale word a1 (RotMatrix / RotMatrixYXZ per entry, MulMatrix0
 * scale, CompMatrix with the parent).  Every call goes through the
 * interpreter's jal path (--jal-service), so each main-exe callee gets the
 * binding an interpreted jal gets (RotMatrixYXZ's runtime host,
 * CompMatrix's address rule, ...). */
u32 func_8009EF3C(u32 arg0, u32 arg1) {
    static const char FN[] = "func_8009EF3C";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x38u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_sp = r_sp + 0xFFFFFFC8u; /* 8009EF3C addiu $sp, $sp, -0x38 */
    bl_sw(r_sp + 0x18u, r_s2); /* 8009EF40 sw $s2, 0x18($sp) */
    r_s2 = r_a0 + 0u; /* 8009EF44 addu $s2, $a0, $zero */
    bl_sw(r_sp + 0x20u, r_s4); /* 8009EF48 sw $s4, 0x20($sp) */
    r_s4 = r_s2 + 0u; /* 8009EF4C addu $s4, $s2, $zero */
    bl_sw(r_sp + 0x30u, r_ra); /* 8009EF50 sw $ra, 0x30($sp) */
    bl_sw(r_sp + 0x2Cu, r_s7); /* 8009EF54 sw $s7, 0x2C($sp) */
    bl_sw(r_sp + 0x28u, r_s6); /* 8009EF58 sw $s6, 0x28($sp) */
    bl_sw(r_sp + 0x24u, r_s5); /* 8009EF5C sw $s5, 0x24($sp) */
    bl_sw(r_sp + 0x1Cu, r_s3); /* 8009EF60 sw $s3, 0x1C($sp) */
    bl_sw(r_sp + 0x14u, r_s1); /* 8009EF64 sw $s1, 0x14($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 8009EF68 sw $s0, 0x10($sp) */
    r_v0 = bl_lw(r_s4 + 0x5Cu); /* 8009EF6C lw $v0, 0x5C($s4) */
    r_v1 = bl_lw(r_s4 + 0x60u); /* 8009EF70 lw $v1, 0x60($s4) */
    r_a0 = bl_lw(r_s4 + 0x64u); /* 8009EF74 lw $a0, 0x64($s4) */
    r_s6 = bl_lhu(r_s4 + 0xAu); /* 8009EF78 lhu $s6, 0xA($s4) */
    r_s1 = r_a1 + 0u; /* 8009EF7C addu $s1, $a1, $zero */
    bl_sw(r_s4 + 0x40u, r_v0); /* 8009EF80 sw $v0, 0x40($s4) */
    r_v0 = bl_lbu(r_s4 + 0x6u); /* 8009EF84 lbu $v0, 0x6($s4) */
    r_s0 = 0x1F800000u; /* 8009EF88 lui $s0, (0x1F800010 >> 16) */
    bl_sw(r_s4 + 0x44u, r_v1); /* 8009EF8C sw $v1, 0x44($s4) */
    /* 8009EF90 beqz $v0, .L8009EFAC */
    { int c_ = (r_v0 == 0); bl_sw(r_s4 + 0x48u, r_a0); if (c_) goto L_8009EFAC; }
    r_a0 = r_s4 + 0x54u; /* 8009EF98 addiu $a0, $s4, 0x54 */
    /* 8009EF9C jal RotMatrixYXZ */
    r_a1 = r_s4 + 0x2Cu;
    bl_jal(FN, 0x8004A92Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrixYXZ */
    /* 8009EFA4 j .L8009EFB8 */
    ; goto L_8009EFB8;
L_8009EFAC:
    r_a0 = r_s4 + 0x54u; /* 8009EFAC addiu $a0, $s4, 0x54 */
    /* 8009EFB0 jal RotMatrix */
    r_a1 = r_s4 + 0x2Cu;
    bl_jal(FN, 0x8003F738u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrix */
L_8009EFB8:
    r_v0 = bl_lh(r_s2 + 0x4Cu); /* 8009EFB8 lh $v0, 0x4C($s2) */
    ; /* 8009EFBC nop  */
    BL_MULT(r_s1, r_v0); /* 8009EFC0 mult $s1, $v0 */
    bl_sh(r_s0 + 0x2u, 0u); /* 8009EFC4 sh $zero, (0x1F800002 & 0xFFFF)($s0) */
    bl_sh(r_s0 + 0x4u, 0u); /* 8009EFC8 sh $zero, (0x1F800004 & 0xFFFF)($s0) */
    bl_sh(r_s0 + 0x6u, 0u); /* 8009EFCC sh $zero, (0x1F800006 & 0xFFFF)($s0) */
    r_v0 = lo; /* 8009EFD0 mflo $v0 */
    r_v0 = BL_SRA(r_v0, 12); /* 8009EFD4 sra $v0, $v0, 12 */
    bl_sh(r_s0, r_v0); /* 8009EFD8 sh $v0, (0x1F800000 & 0xFFFF)($s0) */
    r_v0 = bl_lh(r_s2 + 0x4Eu); /* 8009EFDC lh $v0, 0x4E($s2) */
    ; /* 8009EFE0 nop  */
    BL_MULT(r_s1, r_v0); /* 8009EFE4 mult $s1, $v0 */
    bl_sh(r_s0 + 0xAu, 0u); /* 8009EFE8 sh $zero, (0x1F80000A & 0xFFFF)($s0) */
    bl_sh(r_s0 + 0xCu, 0u); /* 8009EFEC sh $zero, (0x1F80000C & 0xFFFF)($s0) */
    bl_sh(r_s0 + 0xEu, 0u); /* 8009EFF0 sh $zero, (0x1F80000E & 0xFFFF)($s0) */
    r_v0 = lo; /* 8009EFF4 mflo $v0 */
    r_v0 = BL_SRA(r_v0, 12); /* 8009EFF8 sra $v0, $v0, 12 */
    bl_sh(r_s0 + 0x8u, r_v0); /* 8009EFFC sh $v0, (0x1F800008 & 0xFFFF)($s0) */
    r_v0 = bl_lh(r_s2 + 0x50u); /* 8009F000 lh $v0, 0x50($s2) */
    ; /* 8009F004 nop  */
    BL_MULT(r_s1, r_v0); /* 8009F008 mult $s1, $v0 */
    r_s5 = r_s2 + 0x2Cu; /* 8009F00C addiu $s5, $s2, 0x2C */
    r_a0 = r_s5 + 0u; /* 8009F010 addu $a0, $s5, $zero */
    r_a1 = r_s0 + 0u; /* 8009F014 addu $a1, $s0, $zero */
    r_s3 = r_s2 + 0xCu; /* 8009F018 addiu $s3, $s2, 0xC */
    r_a2 = r_s3 + 0u; /* 8009F01C addu $a2, $s3, $zero */
    r_v0 = lo; /* 8009F020 mflo $v0 */
    r_v0 = BL_SRA(r_v0, 12); /* 8009F024 sra $v0, $v0, 12 */
    /* 8009F028 jal MulMatrix0 */
    bl_sh(r_a1 + 0x10u, r_v0);
    bl_jal(FN, 0x8004920Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* MulMatrix0 */
    r_v0 = bl_lw(r_s2 + 0x40u); /* 8009F030 lw $v0, 0x40($s2) */
    r_v1 = bl_lw(r_s2 + 0x44u); /* 8009F034 lw $v1, 0x44($s2) */
    r_a0 = bl_lw(r_s2 + 0x48u); /* 8009F038 lw $a0, 0x48($s2) */
    r_s1 = 0u | 0x1u; /* 8009F03C ori $s1, $zero, 0x1 */
    bl_sw(r_s2 + 0x20u, r_v0); /* 8009F040 sw $v0, 0x20($s2) */
    r_v0 = (u32)(r_s1 < r_s6); /* 8009F044 sltu $v0, $s1, $s6 */
    bl_sw(r_s2 + 0x24u, r_v1); /* 8009F048 sw $v1, 0x24($s2) */
    /* 8009F04C beqz $v0, .L8009F174 */
    { int c_ = (r_v0 == 0); bl_sw(r_s2 + 0x28u, r_a0); if (c_) goto L_8009F174; }
    r_s7 = 0u | 0x1u; /* 8009F054 ori $s7, $zero, 0x1 */
    r_s0 = r_s5 + 0u; /* 8009F058 addu $s0, $s5, $zero */
    r_s5 = r_s2 + 0x54u; /* 8009F05C addiu $s5, $s2, 0x54 */
    r_s0 = r_s0 + 0x7Cu; /* 8009F060 addiu $s0, $s0, 0x7C */
L_8009F064:
    r_s3 = r_s3 + 0x7Cu; /* 8009F064 addiu $s3, $s3, 0x7C */
    r_s5 = r_s5 + 0x7Cu; /* 8009F068 addiu $s5, $s5, 0x7C */
    r_v0 = bl_lbu(r_s0 + 0xFFFFFFD9u); /* 8009F06C lbu $v0, -0x27($s0) */
    ; /* 8009F070 nop  */
    /* 8009F074 beqz $v0, .L8009F0A8 */
    { int c_ = (r_v0 == 0); r_s2 = r_s2 + 0x7Cu; if (c_) goto L_8009F0A8; }
    r_v0 = bl_lbu(r_s0 + 0xFFFFFFDAu); /* 8009F07C lbu $v0, -0x26($s0) */
    ; /* 8009F080 nop  */
    /* 8009F084 beqz $v0, .L8009F09C */
    { int c_ = (r_v0 == 0); r_a0 = r_s5 + 0u; if (c_) goto L_8009F09C; }
    /* 8009F08C jal RotMatrixYXZ */
    r_a1 = r_s3 + 0u;
    bl_jal(FN, 0x8004A92Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrixYXZ */
    /* 8009F094 j .L8009F0A8 */
    bl_sb(r_s0 + 0xFFFFFFD9u, 0u); goto L_8009F0A8;
L_8009F09C:
    /* 8009F09C jal RotMatrix */
    r_a1 = r_s3 + 0u;
    bl_jal(FN, 0x8003F738u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrix */
    bl_sb(r_s0 + 0xFFFFFFD9u, 0u); /* 8009F0A4 sb $zero, -0x27($s0) */
L_8009F0A8:
    r_v0 = bl_lw(r_s2); /* 8009F0A8 lw $v0, 0x0($s2) */
    ; /* 8009F0AC nop  */
    /* 8009F0B0 beqz $v0, .L8009F0CC */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_8009F0CC; }
    r_v0 = bl_lbu(r_v0 + 0x4u); /* 8009F0B8 lbu $v0, 0x4($v0) */
    ; /* 8009F0BC nop  */
    /* 8009F0C0 bne $v0, $s7, .L8009F0CC */
    { int c_ = (r_v0 != r_s7); ; if (c_) goto L_8009F0CC; }
    bl_sb(r_s0 + 0xFFFFFFD8u, r_s7); /* 8009F0C8 sb $s7, -0x28($s0) */
L_8009F0CC:
    r_v0 = bl_lbu(r_s0 + 0xFFFFFFD8u); /* 8009F0CC lbu $v0, -0x28($s0) */
    ; /* 8009F0D0 nop  */
    /* 8009F0D4 beqz $v0, .L8009F158 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_8009F158; }
    r_v0 = bl_lw(r_s0 + 0x30u); /* 8009F0DC lw $v0, 0x30($s0) */
    r_v1 = bl_lw(r_s0 + 0x34u); /* 8009F0E0 lw $v1, 0x34($s0) */
    r_a0 = bl_lw(r_s0 + 0x38u); /* 8009F0E4 lw $a0, 0x38($s0) */
    bl_sw(r_s0 + 0xFFFFFFF4u, r_v0); /* 8009F0E8 sw $v0, -0xC($s0) */
    bl_sw(r_s0 + 0xFFFFFFF8u, r_v1); /* 8009F0EC sw $v1, -0x8($s0) */
    bl_sw(r_s0 + 0xFFFFFFFCu, r_a0); /* 8009F0F0 sw $a0, -0x4($s0) */
    r_a0 = bl_lw(r_s2); /* 8009F0F4 lw $a0, 0x0($s2) */
    ; /* 8009F0F8 nop  */
    /* 8009F0FC beqz $a0, .L8009F118 */
    { int c_ = (r_a0 == 0); r_a0 = r_a0 + 0x2Cu; if (c_) goto L_8009F118; }
    r_a1 = r_s3 + 0u; /* 8009F104 addu $a1, $s3, $zero */
    /* 8009F108 jal CompMatrix */
    r_a2 = r_s0 + 0u;
    bl_jal(FN, 0x8004931Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* CompMatrix */
    /* 8009F110 j .L8009F15C */
    r_s1 = r_s1 + 0x1u; goto L_8009F15C;
L_8009F118:
    r_v0 = bl_lw(r_s0 + 0xFFFFFFE0u); /* 8009F118 lw $v0, -0x20($s0) */
    r_v1 = bl_lw(r_s0 + 0xFFFFFFE4u); /* 8009F11C lw $v1, -0x1C($s0) */
    r_a0 = bl_lw(r_s0 + 0xFFFFFFE8u); /* 8009F120 lw $a0, -0x18($s0) */
    r_a1 = bl_lw(r_s0 + 0xFFFFFFECu); /* 8009F124 lw $a1, -0x14($s0) */
    bl_sw(r_s0, r_v0); /* 8009F128 sw $v0, 0x0($s0) */
    bl_sw(r_s0 + 0x4u, r_v1); /* 8009F12C sw $v1, 0x4($s0) */
    bl_sw(r_s0 + 0x8u, r_a0); /* 8009F130 sw $a0, 0x8($s0) */
    bl_sw(r_s0 + 0xCu, r_a1); /* 8009F134 sw $a1, 0xC($s0) */
    r_v0 = bl_lw(r_s0 + 0xFFFFFFF0u); /* 8009F138 lw $v0, -0x10($s0) */
    r_v1 = bl_lw(r_s0 + 0xFFFFFFF4u); /* 8009F13C lw $v1, -0xC($s0) */
    r_a0 = bl_lw(r_s0 + 0xFFFFFFF8u); /* 8009F140 lw $a0, -0x8($s0) */
    r_a1 = bl_lw(r_s0 + 0xFFFFFFFCu); /* 8009F144 lw $a1, -0x4($s0) */
    bl_sw(r_s0 + 0x10u, r_v0); /* 8009F148 sw $v0, 0x10($s0) */
    bl_sw(r_s0 + 0x14u, r_v1); /* 8009F14C sw $v1, 0x14($s0) */
    bl_sw(r_s0 + 0x18u, r_a0); /* 8009F150 sw $a0, 0x18($s0) */
    bl_sw(r_s0 + 0x1Cu, r_a1); /* 8009F154 sw $a1, 0x1C($s0) */
L_8009F158:
    r_s1 = r_s1 + 0x1u; /* 8009F158 addiu $s1, $s1, 0x1 */
L_8009F15C:
    r_v0 = (u32)(r_s1 < r_s6); /* 8009F15C sltu $v0, $s1, $s6 */
    /* 8009F160 bnez $v0, .L8009F064 */
    { int c_ = (r_v0 != 0); r_s0 = r_s0 + 0x7Cu; if (c_) goto L_8009F064; }
    r_s0 = r_s0 + 0xFFFFFF84u; /* 8009F168 addiu $s0, $s0, -0x7C */
    r_s1 = 0u | 0x1u; /* 8009F16C ori $s1, $zero, 0x1 */
    r_v0 = (u32)(r_s1 < r_s6); /* 8009F170 sltu $v0, $s1, $s6 */
L_8009F174:
    /* 8009F174 beqz $v0, .L8009F194 */
    { int c_ = (r_v0 == 0); r_v0 = r_s6 + 0u; if (c_) goto L_8009F194; }
L_8009F17C:
    r_s4 = r_s4 + 0x7Cu; /* 8009F17C addiu $s4, $s4, 0x7C */
    r_s1 = r_s1 + 0x1u; /* 8009F180 addiu $s1, $s1, 0x1 */
    r_v0 = (u32)(r_s1 < r_s6); /* 8009F184 sltu $v0, $s1, $s6 */
    /* 8009F188 bnez $v0, .L8009F17C */
    { int c_ = (r_v0 != 0); bl_sb(r_s4 + 0x4u, 0u); if (c_) goto L_8009F17C; }
    r_v0 = r_s6 + 0u; /* 8009F190 addu $v0, $s6, $zero */
L_8009F194:
    r_ra = bl_lw(r_sp + 0x30u); /* 8009F194 lw $ra, 0x30($sp) */
    r_s7 = bl_lw(r_sp + 0x2Cu); /* 8009F198 lw $s7, 0x2C($sp) */
    r_s6 = bl_lw(r_sp + 0x28u); /* 8009F19C lw $s6, 0x28($sp) */
    r_s5 = bl_lw(r_sp + 0x24u); /* 8009F1A0 lw $s5, 0x24($sp) */
    r_s4 = bl_lw(r_sp + 0x20u); /* 8009F1A4 lw $s4, 0x20($sp) */
    r_s3 = bl_lw(r_sp + 0x1Cu); /* 8009F1A8 lw $s3, 0x1C($sp) */
    r_s2 = bl_lw(r_sp + 0x18u); /* 8009F1AC lw $s2, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 8009F1B0 lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 8009F1B4 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x38u; /* 8009F1B8 addiu $sp, $sp, 0x38 */
    /* 8009F1BC jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    return r_v0;
}

/* func_8009F1C4.s (251 instructions): the same rebuild for the other model
 * kind (800AAA20 picks it when obj+0x37 is set).  Every call goes through
 * the interpreter's jal path (--jal-service), as 8009EF3C's. */
u32 func_8009F1C4(u32 arg0, u32 arg1) {
    static const char FN[] = "func_8009F1C4";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x38u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_sp = r_sp + 0xFFFFFFC8u; /* 8009F1C4 addiu $sp, $sp, -0x38 */
    bl_sw(r_sp + 0x1Cu, r_s3); /* 8009F1C8 sw $s3, 0x1C($sp) */
    r_s3 = r_a0 + 0u; /* 8009F1CC addu $s3, $a0, $zero */
    bl_sw(r_sp + 0x28u, r_s6); /* 8009F1D0 sw $s6, 0x28($sp) */
    r_s6 = r_s3 + 0u; /* 8009F1D4 addu $s6, $s3, $zero */
    bl_sw(r_sp + 0x34u, r_ra); /* 8009F1D8 sw $ra, 0x34($sp) */
    bl_sw(r_sp + 0x30u, r_fp); /* 8009F1DC sw $fp, 0x30($sp) */
    bl_sw(r_sp + 0x2Cu, r_s7); /* 8009F1E0 sw $s7, 0x2C($sp) */
    bl_sw(r_sp + 0x24u, r_s5); /* 8009F1E4 sw $s5, 0x24($sp) */
    bl_sw(r_sp + 0x20u, r_s4); /* 8009F1E8 sw $s4, 0x20($sp) */
    bl_sw(r_sp + 0x18u, r_s2); /* 8009F1EC sw $s2, 0x18($sp) */
    bl_sw(r_sp + 0x14u, r_s1); /* 8009F1F0 sw $s1, 0x14($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 8009F1F4 sw $s0, 0x10($sp) */
    r_v0 = bl_lw(r_s6 + 0x5Cu); /* 8009F1F8 lw $v0, 0x5C($s6) */
    r_v1 = bl_lw(r_s6 + 0x60u); /* 8009F1FC lw $v1, 0x60($s6) */
    r_a0 = bl_lw(r_s6 + 0x64u); /* 8009F200 lw $a0, 0x64($s6) */
    r_fp = bl_lhu(r_s6 + 0xAu); /* 8009F204 lhu $fp, 0xA($s6) */
    r_s1 = r_a1 + 0u; /* 8009F208 addu $s1, $a1, $zero */
    bl_sw(r_s6 + 0x40u, r_v0); /* 8009F20C sw $v0, 0x40($s6) */
    r_v0 = bl_lbu(r_s6 + 0x6u); /* 8009F210 lbu $v0, 0x6($s6) */
    r_s0 = 0x1F800000u; /* 8009F214 lui $s0, (0x1F800010 >> 16) */
    bl_sw(r_s6 + 0x44u, r_v1); /* 8009F218 sw $v1, 0x44($s6) */
    /* 8009F21C beqz $v0, .L8009F238 */
    { int c_ = (r_v0 == 0); bl_sw(r_s6 + 0x48u, r_a0); if (c_) goto L_8009F238; }
    r_a0 = r_s6 + 0x54u; /* 8009F224 addiu $a0, $s6, 0x54 */
    /* 8009F228 jal RotMatrixYXZ */
    r_a1 = r_s6 + 0x2Cu;
    bl_jal(FN, 0x8004A92Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrixYXZ */
    /* 8009F230 j .L8009F244 */
    ; goto L_8009F244;
L_8009F238:
    r_a0 = r_s6 + 0x54u; /* 8009F238 addiu $a0, $s6, 0x54 */
    /* 8009F23C jal RotMatrix */
    r_a1 = r_s6 + 0x2Cu;
    bl_jal(FN, 0x8003F738u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrix */
L_8009F244:
    r_v0 = bl_lh(r_s3 + 0x4Cu); /* 8009F244 lh $v0, 0x4C($s3) */
    ; /* 8009F248 nop  */
    BL_MULT(r_s1, r_v0); /* 8009F24C mult $s1, $v0 */
    bl_sh(r_s0 + 0x2u, 0u); /* 8009F250 sh $zero, (0x1F800002 & 0xFFFF)($s0) */
    bl_sh(r_s0 + 0x4u, 0u); /* 8009F254 sh $zero, (0x1F800004 & 0xFFFF)($s0) */
    bl_sh(r_s0 + 0x6u, 0u); /* 8009F258 sh $zero, (0x1F800006 & 0xFFFF)($s0) */
    r_v0 = lo; /* 8009F25C mflo $v0 */
    r_v0 = BL_SRA(r_v0, 12); /* 8009F260 sra $v0, $v0, 12 */
    bl_sh(r_s0, r_v0); /* 8009F264 sh $v0, (0x1F800000 & 0xFFFF)($s0) */
    r_v0 = bl_lh(r_s3 + 0x4Eu); /* 8009F268 lh $v0, 0x4E($s3) */
    ; /* 8009F26C nop  */
    BL_MULT(r_s1, r_v0); /* 8009F270 mult $s1, $v0 */
    bl_sh(r_s0 + 0xAu, 0u); /* 8009F274 sh $zero, (0x1F80000A & 0xFFFF)($s0) */
    bl_sh(r_s0 + 0xCu, 0u); /* 8009F278 sh $zero, (0x1F80000C & 0xFFFF)($s0) */
    bl_sh(r_s0 + 0xEu, 0u); /* 8009F27C sh $zero, (0x1F80000E & 0xFFFF)($s0) */
    r_v0 = lo; /* 8009F280 mflo $v0 */
    r_v0 = BL_SRA(r_v0, 12); /* 8009F284 sra $v0, $v0, 12 */
    bl_sh(r_s0 + 0x8u, r_v0); /* 8009F288 sh $v0, (0x1F800008 & 0xFFFF)($s0) */
    r_v0 = bl_lh(r_s3 + 0x50u); /* 8009F28C lh $v0, 0x50($s3) */
    ; /* 8009F290 nop  */
    BL_MULT(r_s1, r_v0); /* 8009F294 mult $s1, $v0 */
    r_s2 = r_s3 + 0x2Cu; /* 8009F298 addiu $s2, $s3, 0x2C */
    r_a0 = r_s2 + 0u; /* 8009F29C addu $a0, $s2, $zero */
    r_a1 = r_s0 + 0u; /* 8009F2A0 addu $a1, $s0, $zero */
    r_s1 = r_s3 + 0xCu; /* 8009F2A4 addiu $s1, $s3, 0xC */
    r_a2 = r_s1 + 0u; /* 8009F2A8 addu $a2, $s1, $zero */
    r_v0 = lo; /* 8009F2AC mflo $v0 */
    r_v0 = BL_SRA(r_v0, 12); /* 8009F2B0 sra $v0, $v0, 12 */
    /* 8009F2B4 jal MulMatrix0 */
    bl_sh(r_a1 + 0x10u, r_v0);
    bl_jal(FN, 0x8004920Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* MulMatrix0 */
    r_v0 = bl_lw(r_s3 + 0x40u); /* 8009F2BC lw $v0, 0x40($s3) */
    r_v1 = bl_lw(r_s3 + 0x44u); /* 8009F2C0 lw $v1, 0x44($s3) */
    r_a0 = bl_lw(r_s3 + 0x48u); /* 8009F2C4 lw $a0, 0x48($s3) */
    r_s4 = 0u | 0x1u; /* 8009F2C8 ori $s4, $zero, 0x1 */
    bl_sw(r_s3 + 0x20u, r_v0); /* 8009F2CC sw $v0, 0x20($s3) */
    r_v0 = (u32)(r_s4 < r_fp); /* 8009F2D0 sltu $v0, $s4, $fp */
    bl_sw(r_s3 + 0x24u, r_v1); /* 8009F2D4 sw $v1, 0x24($s3) */
    /* 8009F2D8 beqz $v0, .L8009F554 */
    { int c_ = (r_v0 == 0); bl_sw(r_s3 + 0x28u, r_a0); if (c_) goto L_8009F554; }
    r_s7 = 0x1000000u; /* 8009F2E0 lui $s7, (0x1000000 >> 16) */
    r_s0 = r_s2 + 0u; /* 8009F2E4 addu $s0, $s2, $zero */
    r_s2 = r_s1 + 0u; /* 8009F2E8 addu $s2, $s1, $zero */
    r_s5 = r_s3 + 0x54u; /* 8009F2EC addiu $s5, $s3, 0x54 */
    r_s0 = r_s0 + 0x7Cu; /* 8009F2F0 addiu $s0, $s0, 0x7C */
L_8009F2F4:
    r_s2 = r_s2 + 0x7Cu; /* 8009F2F4 addiu $s2, $s2, 0x7C */
    r_s3 = r_s3 + 0x7Cu; /* 8009F2F8 addiu $s3, $s3, 0x7C */
    r_v0 = bl_lw(r_s3); /* 8009F2FC lw $v0, 0x0($s3) */
    ; /* 8009F300 nop  */
    /* 8009F304 beqz $v0, .L8009F340 */
    { int c_ = (r_v0 == 0); r_s5 = r_s5 + 0x7Cu; if (c_) goto L_8009F340; }
    r_v0 = bl_lbu(r_v0 + 0x5u); /* 8009F30C lbu $v0, 0x5($v0) */
    r_a3 = 0u | 0x1u; /* 8009F310 ori $a3, $zero, 0x1 */
    /* 8009F314 bne $v0, $a3, .L8009F320 */
    { int c_ = (r_v0 != r_a3); r_a3 = 0u | 0x1u; if (c_) goto L_8009F320; }
    bl_sb(r_s0 + 0xFFFFFFD9u, r_a3); /* 8009F31C sb $a3, -0x27($s0) */
L_8009F320:
    r_v0 = bl_lw(r_s3); /* 8009F320 lw $v0, 0x0($s3) */
    ; /* 8009F324 nop  */
    r_v0 = bl_lbu(r_v0 + 0x4u); /* 8009F328 lbu $v0, 0x4($v0) */
    r_a3 = 0u | 0x1u; /* 8009F32C ori $a3, $zero, 0x1 */
    /* 8009F330 bne $v0, $a3, .L8009F340 */
    { int c_ = (r_v0 != r_a3); ; if (c_) goto L_8009F340; }
    r_a3 = 0u | 0x1u; /* 8009F338 ori $a3, $zero, 0x1 */
    bl_sb(r_s0 + 0xFFFFFFD8u, r_a3); /* 8009F33C sb $a3, -0x28($s0) */
L_8009F340:
    r_v0 = bl_lbu(r_s0 + 0xFFFFFFD9u); /* 8009F340 lbu $v0, -0x27($s0) */
    ; /* 8009F344 nop  */
    /* 8009F348 beqz $v0, .L8009F4AC */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_8009F4AC; }
    r_v0 = bl_lbu(r_s0 + 0xFFFFFFDAu); /* 8009F350 lbu $v0, -0x26($s0) */
    ; /* 8009F354 nop  */
    /* 8009F358 beqz $v0, .L8009F374 */
    { int c_ = (r_v0 == 0); r_s1 = 0x1F800000u; if (c_) goto L_8009F374; }
    r_a0 = r_s5 + 0u; /* 8009F360 addu $a0, $s5, $zero */
    /* 8009F364 jal RotMatrixYXZ */
    r_a1 = r_s2 + 0u;
    bl_jal(FN, 0x8004A92Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrixYXZ */
    /* 8009F36C j .L8009F380 */
    ; goto L_8009F380;
L_8009F374:
    r_a0 = r_s5 + 0u; /* 8009F374 addu $a0, $s5, $zero */
    /* 8009F378 jal RotMatrix */
    r_a1 = r_s2 + 0u;
    bl_jal(FN, 0x8003F738u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotMatrix */
L_8009F380:
    r_v0 = bl_lhu(r_s0 + 0x20u); /* 8009F380 lhu $v0, 0x20($s0) */
    r_a0 = r_s2 + 0u; /* 8009F384 addu $a0, $s2, $zero */
    bl_sh(r_s1 + 0x2u, 0u); /* 8009F388 sh $zero, (0x1F800002 & 0xFFFF)($s1) */
    bl_sh(r_s1 + 0x4u, 0u); /* 8009F38C sh $zero, (0x1F800004 & 0xFFFF)($s1) */
    bl_sh(r_s1 + 0x6u, 0u); /* 8009F390 sh $zero, (0x1F800006 & 0xFFFF)($s1) */
    bl_sh(r_s1, r_v0); /* 8009F394 sh $v0, (0x1F800000 & 0xFFFF)($s1) */
    r_v0 = bl_lhu(r_s0 + 0x22u); /* 8009F398 lhu $v0, 0x22($s0) */
    r_a1 = r_s1 + 0u; /* 8009F39C addu $a1, $s1, $zero */
    bl_sh(r_s1 + 0xAu, 0u); /* 8009F3A0 sh $zero, (0x1F80000A & 0xFFFF)($s1) */
    bl_sh(r_s1 + 0xCu, 0u); /* 8009F3A4 sh $zero, (0x1F80000C & 0xFFFF)($s1) */
    bl_sh(r_s1 + 0xEu, 0u); /* 8009F3A8 sh $zero, (0x1F80000E & 0xFFFF)($s1) */
    bl_sh(r_s1 + 0x8u, r_v0); /* 8009F3AC sh $v0, (0x1F800008 & 0xFFFF)($s1) */
    r_v0 = bl_lhu(r_s0 + 0x24u); /* 8009F3B0 lhu $v0, 0x24($s0) */
    r_a2 = r_s2 + 0u; /* 8009F3B4 addu $a2, $s2, $zero */
    /* 8009F3B8 jal MulMatrix0 */
    bl_sh(r_s1 + 0x10u, r_v0);
    bl_jal(FN, 0x8004920Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* MulMatrix0 */
    r_v0 = bl_lw(r_s3); /* 8009F3C0 lw $v0, 0x0($s3) */
    ; /* 8009F3C4 nop  */
    /* 8009F3C8 beqz $v0, .L8009F4AC */
    { int c_ = (r_v0 == 0); r_a0 = r_s1 + 0u; if (c_) goto L_8009F4AC; }
    r_v0 = bl_lh(r_v0 + 0x4Cu); /* 8009F3D0 lh $v0, 0x4C($v0) */
    ; /* 8009F3D4 nop  */
    BL_DIV(r_s7, r_v0); /* 8009F3D8 div $zero, $s7, $v0 */
    /* 8009F3DC bnez $v0, .L8009F3E8 */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_8009F3E8; }
    bl_fail(FN, "BREAK", 7u); /* 8009F3E4 break 7 */
L_8009F3E8:
    r_at = 0u + 0xFFFFFFFFu; /* 8009F3E8 addiu $at, $zero, -0x1 */
    /* 8009F3EC bne $v0, $at, .L8009F400 */
    { int c_ = (r_v0 != r_at); r_at = 0x80000000u; if (c_) goto L_8009F400; }
    /* 8009F3F4 bne $s7, $at, .L8009F400 */
    { int c_ = (r_s7 != r_at); ; if (c_) goto L_8009F400; }
    bl_fail(FN, "BREAK", 6u); /* 8009F3FC break 6 */
L_8009F400:
    r_v0 = lo; /* 8009F400 mflo $v0 */
    bl_sh(r_s1 + 0x2u, 0u); /* 8009F404 sh $zero, (0x1F800002 & 0xFFFF)($s1) */
    bl_sh(r_s1 + 0x4u, 0u); /* 8009F408 sh $zero, (0x1F800004 & 0xFFFF)($s1) */
    bl_sh(r_s1 + 0x6u, 0u); /* 8009F40C sh $zero, (0x1F800006 & 0xFFFF)($s1) */
    bl_sh(r_s1, r_v0); /* 8009F410 sh $v0, (0x1F800000 & 0xFFFF)($s1) */
    r_v0 = bl_lw(r_s3); /* 8009F414 lw $v0, 0x0($s3) */
    ; /* 8009F418 nop  */
    r_v0 = bl_lh(r_v0 + 0x4Eu); /* 8009F41C lh $v0, 0x4E($v0) */
    ; /* 8009F420 nop  */
    BL_DIV(r_s7, r_v0); /* 8009F424 div $zero, $s7, $v0 */
    /* 8009F428 bnez $v0, .L8009F434 */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_8009F434; }
    bl_fail(FN, "BREAK", 7u); /* 8009F430 break 7 */
L_8009F434:
    r_at = 0u + 0xFFFFFFFFu; /* 8009F434 addiu $at, $zero, -0x1 */
    /* 8009F438 bne $v0, $at, .L8009F44C */
    { int c_ = (r_v0 != r_at); r_at = 0x80000000u; if (c_) goto L_8009F44C; }
    /* 8009F440 bne $s7, $at, .L8009F44C */
    { int c_ = (r_s7 != r_at); ; if (c_) goto L_8009F44C; }
    bl_fail(FN, "BREAK", 6u); /* 8009F448 break 6 */
L_8009F44C:
    r_v0 = lo; /* 8009F44C mflo $v0 */
    bl_sh(r_s1 + 0xAu, 0u); /* 8009F450 sh $zero, (0x1F80000A & 0xFFFF)($s1) */
    bl_sh(r_s1 + 0xCu, 0u); /* 8009F454 sh $zero, (0x1F80000C & 0xFFFF)($s1) */
    bl_sh(r_s1 + 0xEu, 0u); /* 8009F458 sh $zero, (0x1F80000E & 0xFFFF)($s1) */
    bl_sh(r_s1 + 0x8u, r_v0); /* 8009F45C sh $v0, (0x1F800008 & 0xFFFF)($s1) */
    r_v0 = bl_lw(r_s3); /* 8009F460 lw $v0, 0x0($s3) */
    ; /* 8009F464 nop  */
    r_v0 = bl_lh(r_v0 + 0x50u); /* 8009F468 lh $v0, 0x50($v0) */
    ; /* 8009F46C nop  */
    BL_DIV(r_s7, r_v0); /* 8009F470 div $zero, $s7, $v0 */
    /* 8009F474 bnez $v0, .L8009F480 */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_8009F480; }
    bl_fail(FN, "BREAK", 7u); /* 8009F47C break 7 */
L_8009F480:
    r_at = 0u + 0xFFFFFFFFu; /* 8009F480 addiu $at, $zero, -0x1 */
    /* 8009F484 bne $v0, $at, .L8009F498 */
    { int c_ = (r_v0 != r_at); r_at = 0x80000000u; if (c_) goto L_8009F498; }
    /* 8009F48C bne $s7, $at, .L8009F498 */
    { int c_ = (r_s7 != r_at); ; if (c_) goto L_8009F498; }
    bl_fail(FN, "BREAK", 6u); /* 8009F494 break 6 */
L_8009F498:
    r_v0 = lo; /* 8009F498 mflo $v0 */
    r_a1 = r_s2 + 0u; /* 8009F49C addu $a1, $s2, $zero */
    r_a2 = r_s2 + 0u; /* 8009F4A0 addu $a2, $s2, $zero */
    /* 8009F4A4 jal MulMatrix0 */
    bl_sh(r_a0 + 0x10u, r_v0);
    bl_jal(FN, 0x8004920Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* MulMatrix0 */
L_8009F4AC:
    r_v0 = bl_lbu(r_s0 + 0xFFFFFFD8u); /* 8009F4AC lbu $v0, -0x28($s0) */
    ; /* 8009F4B0 nop  */
    /* 8009F4B4 beqz $v0, .L8009F538 */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_8009F538; }
    r_v0 = bl_lw(r_s0 + 0x30u); /* 8009F4BC lw $v0, 0x30($s0) */
    r_v1 = bl_lw(r_s0 + 0x34u); /* 8009F4C0 lw $v1, 0x34($s0) */
    r_a0 = bl_lw(r_s0 + 0x38u); /* 8009F4C4 lw $a0, 0x38($s0) */
    bl_sw(r_s0 + 0xFFFFFFF4u, r_v0); /* 8009F4C8 sw $v0, -0xC($s0) */
    bl_sw(r_s0 + 0xFFFFFFF8u, r_v1); /* 8009F4CC sw $v1, -0x8($s0) */
    bl_sw(r_s0 + 0xFFFFFFFCu, r_a0); /* 8009F4D0 sw $a0, -0x4($s0) */
    r_a0 = bl_lw(r_s3); /* 8009F4D4 lw $a0, 0x0($s3) */
    ; /* 8009F4D8 nop  */
    /* 8009F4DC beqz $a0, .L8009F4F8 */
    { int c_ = (r_a0 == 0); r_a0 = r_a0 + 0x2Cu; if (c_) goto L_8009F4F8; }
    r_a1 = r_s2 + 0u; /* 8009F4E4 addu $a1, $s2, $zero */
    /* 8009F4E8 jal CompMatrix */
    r_a2 = r_s0 + 0u;
    bl_jal(FN, 0x8004931Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* CompMatrix */
    /* 8009F4F0 j .L8009F53C */
    r_s4 = r_s4 + 0x1u; goto L_8009F53C;
L_8009F4F8:
    r_v0 = bl_lw(r_s0 + 0xFFFFFFE0u); /* 8009F4F8 lw $v0, -0x20($s0) */
    r_v1 = bl_lw(r_s0 + 0xFFFFFFE4u); /* 8009F4FC lw $v1, -0x1C($s0) */
    r_a0 = bl_lw(r_s0 + 0xFFFFFFE8u); /* 8009F500 lw $a0, -0x18($s0) */
    r_a1 = bl_lw(r_s0 + 0xFFFFFFECu); /* 8009F504 lw $a1, -0x14($s0) */
    bl_sw(r_s0, r_v0); /* 8009F508 sw $v0, 0x0($s0) */
    bl_sw(r_s0 + 0x4u, r_v1); /* 8009F50C sw $v1, 0x4($s0) */
    bl_sw(r_s0 + 0x8u, r_a0); /* 8009F510 sw $a0, 0x8($s0) */
    bl_sw(r_s0 + 0xCu, r_a1); /* 8009F514 sw $a1, 0xC($s0) */
    r_v0 = bl_lw(r_s0 + 0xFFFFFFF0u); /* 8009F518 lw $v0, -0x10($s0) */
    r_v1 = bl_lw(r_s0 + 0xFFFFFFF4u); /* 8009F51C lw $v1, -0xC($s0) */
    r_a0 = bl_lw(r_s0 + 0xFFFFFFF8u); /* 8009F520 lw $a0, -0x8($s0) */
    r_a1 = bl_lw(r_s0 + 0xFFFFFFFCu); /* 8009F524 lw $a1, -0x4($s0) */
    bl_sw(r_s0 + 0x10u, r_v0); /* 8009F528 sw $v0, 0x10($s0) */
    bl_sw(r_s0 + 0x14u, r_v1); /* 8009F52C sw $v1, 0x14($s0) */
    bl_sw(r_s0 + 0x18u, r_a0); /* 8009F530 sw $a0, 0x18($s0) */
    bl_sw(r_s0 + 0x1Cu, r_a1); /* 8009F534 sw $a1, 0x1C($s0) */
L_8009F538:
    r_s4 = r_s4 + 0x1u; /* 8009F538 addiu $s4, $s4, 0x1 */
L_8009F53C:
    r_v0 = (u32)(r_s4 < r_fp); /* 8009F53C sltu $v0, $s4, $fp */
    /* 8009F540 bnez $v0, .L8009F2F4 */
    { int c_ = (r_v0 != 0); r_s0 = r_s0 + 0x7Cu; if (c_) goto L_8009F2F4; }
    r_s0 = r_s0 + 0xFFFFFF84u; /* 8009F548 addiu $s0, $s0, -0x7C */
    r_s4 = 0u | 0x1u; /* 8009F54C ori $s4, $zero, 0x1 */
    r_v0 = (u32)(r_s4 < r_fp); /* 8009F550 sltu $v0, $s4, $fp */
L_8009F554:
    /* 8009F554 beqz $v0, .L8009F57C */
    { int c_ = (r_v0 == 0); r_v0 = r_fp + 0u; if (c_) goto L_8009F57C; }
    r_a0 = r_s6 + 0x4u; /* 8009F55C addiu $a0, $s6, 0x4 */
L_8009F560:
    r_a0 = r_a0 + 0x7Cu; /* 8009F560 addiu $a0, $a0, 0x7C */
    r_s4 = r_s4 + 0x1u; /* 8009F564 addiu $s4, $s4, 0x1 */
    r_v0 = (u32)(r_s4 < r_fp); /* 8009F568 sltu $v0, $s4, $fp */
    bl_sb(r_a0 + 0x1u, 0u); /* 8009F56C sb $zero, 0x1($a0) */
    /* 8009F570 bnez $v0, .L8009F560 */
    { int c_ = (r_v0 != 0); bl_sb(r_a0, 0u); if (c_) goto L_8009F560; }
    r_v0 = r_fp + 0u; /* 8009F578 addu $v0, $fp, $zero */
L_8009F57C:
    r_ra = bl_lw(r_sp + 0x34u); /* 8009F57C lw $ra, 0x34($sp) */
    r_fp = bl_lw(r_sp + 0x30u); /* 8009F580 lw $fp, 0x30($sp) */
    r_s7 = bl_lw(r_sp + 0x2Cu); /* 8009F584 lw $s7, 0x2C($sp) */
    r_s6 = bl_lw(r_sp + 0x28u); /* 8009F588 lw $s6, 0x28($sp) */
    r_s5 = bl_lw(r_sp + 0x24u); /* 8009F58C lw $s5, 0x24($sp) */
    r_s4 = bl_lw(r_sp + 0x20u); /* 8009F590 lw $s4, 0x20($sp) */
    r_s3 = bl_lw(r_sp + 0x1Cu); /* 8009F594 lw $s3, 0x1C($sp) */
    r_s2 = bl_lw(r_sp + 0x18u); /* 8009F598 lw $s2, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 8009F59C lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 8009F5A0 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x38u; /* 8009F5A4 addiu $sp, $sp, 0x38 */
    /* 8009F5A8 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    return r_v0;
}
#endif /* XENO_PC_PORT */
