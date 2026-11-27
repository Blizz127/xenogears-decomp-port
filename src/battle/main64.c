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
