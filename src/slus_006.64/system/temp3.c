#include "common.h"
#include "main/game.h"
#include "system/menu.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/sound.h"
#include "psyq/libgpu.h"
#include "psyq/libcd.h"
#ifdef XENO_PC_PORT
#include "../../../pc_port/src/psx_memory.h"
#endif

#ifdef XENO_PC_PORT
#define REG_PIN(t, n, r) t n
#else
#define REG_PIN(t, n, r) register t n asm(r)
#endif


extern s32 D_8004F330;
extern s32 D_8004F334;
extern s32 g_GamePartySkinsInitialized;
extern s32 D_8004F320;
extern s32 g_GameSceneMapNum;
extern s32 D_8004F31C;
extern s32 D_8004F32C;
extern s32 D_8004F344;
extern void *D_8005A4A0;
extern void *D_8005A4BC;
extern StreamDataQueueEntry g_PartyStreamDataQueue[4];
extern int g_GamePartyMemberSkins[3];
extern int g_PartyIsWaitingForStreamData;
extern void* g_PartyDataBuffers[];
extern void* g_PartyStreamDataPointers[];

extern u8 *D_800592DC[];
extern u8* D_800592E0;
extern u8* D_800592D4;
extern u8* D_800592D8;

/* Small-data globals owned by this TU. Retail addresses exactly these via
 * %gp_rel (cc1 -G8 emits explicit %gp_rel for same-TU small definitions,
 * independent of the assembler -G flag); every other extern in this TU is
 * addressed absolutely under the Default preset. Do NOT move these to
 * headers or mark extern: their TU-local definition is what selects
 * %gp_rel. (D_800594D5/D6 stay extern: retail addresses them absolutely.) */
/* Retail temp3 is three TUs: temp3 proper (..0x8001B6C4), temp3b
 * (0x8001B6C4..0x8001B970) and temp3c (0x8001B970..), which
 * src/slus_006.64/system/temp3b.c / temp3c.c build by including this file
 * with TEMP3_PART_B / TEMP3_PART_C defined. Each owns the .sbss above that
 * it reaches %gp_rel; temp3b reaches temp3c's D_8005947C/D_800594F8 as
 * plain (absolute) externs. The port builds all three parts here. */
#if defined(XENO_PC_PORT) || defined(TEMP3_PART_C)
u8 D_8005946C;
u8 D_8005947C;
u8* D_80059480;
u8* D_800594AC;
u8 D_800594CC;
u8 D_800594D4;
u8 D_800594F8;
u8 D_8005954C;
s32 D_800595A0;
u8* D_800595A8;
u8* D_800595D0;
#endif
#if defined(XENO_PC_PORT) || defined(TEMP3_PART_B)
u8 D_8005959C;
#endif
#if !defined(XENO_PC_PORT) && defined(TEMP3_PART_B)
extern u8 D_8005947C;
extern u8 D_800594F8;
#endif

#if defined(XENO_PC_PORT) || (!defined(TEMP3_PART_B) && !defined(TEMP3_PART_C))

void func_8001A5CC(void) {
    s32 row, col;
    D_800592DC[0] = HeapAlloc(0x3480, 1);
    D_800592E0 = HeapAlloc(0x3480, 1);
    D_800592D4 = HeapAlloc(0x460, 1);
    D_800592D8 = HeapAlloc(0x460, 1);
    for (row = 0; row < 0x1C; row++) {
        for (col = 0; col < 0x28; col++) {
            D_800592D4[row * 0x28 + col] = 0;
            D_800592D8[row * 0x28 + col] = 0;
        }
    }
}

extern u8* D_800592D8;

void func_8001A684(s32 row, s32 col) {
    u8* pTable;
    if (row < 0) row = 0x1C;
    if (row >= 0x1D) row = 0;
    if (col < 0) col = 0x28;
    if (col >= 0x29) col = 0;
    pTable = D_800592D8 + ((row * 5) * 8 + col);
    pTable[0]++;
}

/* All heap-table globals below are plain externs addressed absolutely
 * (Default preset: maspsx force-appends -G0). D_800592DC is an array of
 * row pointers (retail indexes its address, never its value). */
extern int D_800592C8;

void func_8001A6E8(void* ot) {
    /* Renders marked cells of the D_800592D4 tile table as GPU packets into
     * per-row buffers, marks neighbours, ages the tables, then sprinkles up
     * to 0x14 random marks (plus one final random neighbourhood). Register
     * homes ($s0-$s7,$fp) match retail exactly. */
    REG_PIN(int, tidx, "$2");
    REG_PIN(u8*, pRow, "$23");
    REG_PIN(u8*, pPkt, "$18");
    int row;
    int col;
    REG_PIN(int, rm, "$22");
    REG_PIN(int, rp, "$21");
    REG_PIN(int, cm, "$17");
    REG_PIN(int, cp, "$16");
    int a2v;
    int fp;
    int a2base;
    u8 v;
    u32 pkt;

    /* First-statement global load: cc1 emits it before the prologue.
     * The row-table index scales inside the array access (sll, then the
     * lui/addu/lw triple through $at). */
    tidx = D_800592C8;
    fp = 0;
    row = 0;
    a2base = 0;
    pRow = D_800592DC[tidx];

    for (; row < 0x1C; a2base += 0x28, row++) {
        col = 0;
        rm = row - 1;
        rp = row + 1;
        a2v = a2base;
        pPkt = pRow + 8;
        for (; col < 0x28; col++) {
            u8* pD4;
            pD4 = D_800592D4;
            v = pD4[a2v + col];
            if (v == 0) {
                continue;
            }
            pPkt[-5] = 2;
            *(u32*)(pPkt - 4) = 0x70280000;
            pkt = (col << 3) | (row << 19);
            *(u32*)pPkt = pkt;
            AddPrim(ot, pRow);
            pPkt += 0xC;
            pRow += 0xC;
            fp++;
            { REG_PIN(int, a0rm, "$4") = rm; cm = col - 1; func_8001A684(a0rm, cm); }
            func_8001A684(rm, col);
            { REG_PIN(int, a0rm2, "$4") = rm; cp = col + 1; func_8001A684(a0rm2, cp); }
            func_8001A684(row, cm);
            func_8001A684(row, cp);
            func_8001A684(rp, cm);
            func_8001A684(rp, col);
            func_8001A684(rp, cp);
        }
    }
    row = 0;

    {
        REG_PIN(int, two, "$4") = 2;
        REG_PIN(u8, vv, "$2");
        for (; row < 0x460; row++) {
            vv = D_800592D8[row];
            if (vv != two) {
                D_800592D4[row] = (vv ^ 3u) < 1u;
            }
            D_800592D8[row] = 0;
        }
    }

    if (fp < 0x14) {
        int rB;
        fp = 0;
        {
            int rA;
            rA = rand();
            row = rA / 28;
            row = rA - row * 28;
            rB = rand();
            col = rB / 40;
            col = rB - col * 40;
        }
        do {
            int rvA;
            int rvB;
            int mA;
            int mB;
            int r1;
            int c1;
            rvA = rand();
            r1 = row - 1;
            row = r1 + rvA % 3;
            rvB = rand();
            c1 = col - 1;
            col = c1 + rvB % 3;
            if (row < 0) {
                row = 0x1C;
            }
            if (row >= 0x1D) {
                row = 0;
            }
            if (col < 0) {
                col = 0x28;
            }
            if (col >= 0x29) {
                col = 0;
            }
            {
                u8* pD4c;
                pD4c = D_800592D4;
                pD4c[row * 0x28 + col] = 1;
            }
            fp++;
        } while (fp < 0x14);
    }

    {
        REG_PIN(int, trm, "$16");
        REG_PIN(int, tcm, "$18");
        REG_PIN(int, tcp, "$17");
        int rA2;
        int rC;
        rA2 = rand();
        row = rA2 / 28;
        row = rA2 - row * 28;
        rC = rand();
        trm = row - 1;
        {
            REG_PIN(int, a0rm, "$4") = trm;
            col = rC / 40;
            col = rC - col * 40;
            tcm = col - 1;
            func_8001A684(a0rm, tcm);
        }
        func_8001A684(trm, col);
        { REG_PIN(int, a0rm3, "$4") = trm; tcp = col + 1; func_8001A684(a0rm3, tcp); }
        func_8001A684(row, tcm);
        func_8001A684(row, tcp);
        trm = row + 1;
        func_8001A684(trm, tcm);
        func_8001A684(trm, col);
        func_8001A684(trm, tcp);
    }
}

extern s32 D_8005A444[];
extern s32 D_8004F364;
extern s32 D_8004F328;
extern s32 D_8004F324;
extern s32 D_8004F2FC;
extern s32 D_8004F36C;
extern s32 D_8004F2F8;
extern s32 D_8004F31C;
extern s32 D_8004F320;
extern s32 D_8004F314;
extern s32 D_8004F310;
extern s32 D_8004F370;
extern s32 D_8004F35C;
extern s32 D_8004F358;
extern s32 D_8004F354;
extern s32 D_8004F350;
extern s32 D_8004F2F4;
extern s32 D_8004F344;
extern s32 D_8004F348;
extern s32 D_8004F304;
extern s32 D_8004F368;
extern s32 D_8004F300;
extern s32 D_8004F380;
extern s32 D_8004F37C;
extern s32 D_8004F378;
extern u8 D_8005942C;
extern u8 D_800594D0;
extern s16 D_8004F384;
extern s32 D_8004F318;
extern s32 D_8004F334;
extern s32 D_8004F33C;
extern s32 D_8004F338;
extern s32 D_8004F330;
extern s32 D_8004F32C;
extern s32 D_8004F340;
extern s32 D_8004F308;
extern s32 D_80062524[];
extern s32 D_8006F990[];
extern s32 g_GamePartySkinsInitialized;
extern s32 g_GameHasLoadedWDS;
extern s32 g_PartyIsWaitingForStreamData;

void func_8001AADC(void) {
    s32 i;
    s32* pSkins = (s32*)g_GamePartyMemberSkins;
    s32* pSlots = D_8005A444;
    s32* pD8006F990 = D_8006F990;
    s32* pMembers = (s32*)g_GamePartyMembers;

    D_8004F364 = 1;
    D_8004F328 = 0xFF;
    D_8004F324 = 0xFF;
    D_8004F2FC = 0;
    D_8004F36C = 0;
    D_8004F2F8 = 0;
    D_8004F31C = 0;
    D_8004F320 = 0;
    D_8004F314 = 0;
    D_8004F310 = 0;
    g_GamePartySkinsInitialized = 0;
    D_8004F370 = 0;
    D_8004F35C = 0;
    g_GameHasLoadedWDS = 0;
    g_PartyIsWaitingForStreamData = 0;
    D_8004F358 = 0;
    D_8004F354 = 0;
    D_8004F350 = 0;
    D_8004F2F4 = 0;
    D_8004F344 = 0;
    D_8004F348 = 0;
    D_8004F304 = 0;
    D_8004F368 = 0;
    D_8004F300 = 0;
    D_8004F380 = 0;
    D_8004F37C = 0;
    D_8004F378 = 0;
    D_8005942C = 0;
    D_800594D0 = 0;
    D_8004F384 = 0;
    D_8004F318 = 0;
    D_8004F334 = -1;
    g_GameSceneMapNum = -1;
    D_8004F33C = -1;
    D_8004F338 = -1;
    D_8004F330 = -1;
    D_8004F32C = -1;
    D_8004F340 = -1;
    D_8004F308 = -1;
    for (i = 0; i < 3; i++) {
        pSkins[i] = 0;
        pD8006F990[i] = 0;
        pSlots[i] = 0;
        pMembers[i] = 0;
    }
    /* Retail clears the four words ending at D_80062524 (0x80062518..0x80062524). */
    for (i = 3; i >= 0; i--) {
        D_80062524[i - 3] = 0;
    }
}

void GamePartySignalReinitialize(void) {
    g_GamePartySkinsInitialized = 0;
}

void func_8001ACA4(void) {
    D_8004F334 = -1;
    D_8004F330 = -1;
    HeapChangeCurrentUser(HEAP_USER_YOSI, 0);
    ArchiveSetIndex(4, 0);
    GamePartyStreamLoadSkinData(1);
}

s32 GameCharacterGetGearID(int characterIndex) {
    return g_pGameState->characters[characterIndex].gearId;
}

void GameWaitForCdData(void) {
    while (ArchiveDataSync()) {};
    ArchiveCdDataSync(0);
}

// Set GameState pointer to g_GameState, set party members accordingly 
// to it and stream load character skins for them.
void GamePartyCharactersInitializeSkins(void) {
    int i;
    int curPartyMemberIndex;
    int entryCount;
    void* pBuffer;

    g_pGameState = &g_GameState;
    curPartyMemberIndex = 0;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        g_GamePartyMembers[i] = CHARACTER_ID_NONE;
        if (g_pGameState->partyMembers[i] != CHARACTER_ID_NONE) {
            g_GamePartyMembers[curPartyMemberIndex++] = g_pGameState->partyMembers[i];
        }
    }
    
    for (i = 0, entryCount = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_GamePartyMembers[i] != CHARACTER_ID_NONE) {
            g_PartyStreamDataQueue[entryCount].archiveIndex = g_GamePartyMembers[i] + 5;
            g_GamePartyMemberSkins[i] = g_GamePartyMembers[i];
            pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_GamePartyMembers[i] + 5), 0x0);
            g_PartyStreamDataPointers[entryCount] = pBuffer;
            g_PartyStreamDataQueue[entryCount].pData = pBuffer;
            HeapPinBlock(g_PartyStreamDataPointers[entryCount]);
            entryCount++;
        }
    }
    g_PartyStreamDataQueue[entryCount].pData = NULL;
    g_PartyStreamDataQueue[entryCount].archiveIndex = 0;
    func_80029AFC(g_PartyStreamDataQueue, 0, 0);
    D_8004F31C = 1;
}

// Set GameState pointer to g_GameState, set party members accordingly 
// to it and stream load gear skins for them.
void GamePartyGearsInitializeSkins(void) {
    int i;
    int curPartyMemberIndex;
    int entryCount;
    int gearId;
    void* pBuffer;

    g_pGameState = &g_GameState;
    curPartyMemberIndex = 0;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        g_GamePartyMembers[i] = CHARACTER_ID_NONE;
        if (g_pGameState->partyMembers[i] != CHARACTER_ID_NONE) {
            g_GamePartyMembers[curPartyMemberIndex++] = g_pGameState->partyMembers[i];
        }
    }

    for (i = 0, entryCount = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_GamePartyMembers[i] != CHARACTER_ID_NONE) {
            gearId = GameCharacterGetGearID(g_GamePartyMembers[i]);
            if (gearId == 0xFF) {
                gearId = 0x0;
            }
            gearId += 0x10;
            g_PartyStreamDataQueue[entryCount].archiveIndex = gearId + 5;
            g_GamePartyMemberSkins[i] = gearId;
            pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(gearId + 5), 0x0);
            g_PartyStreamDataPointers[entryCount] = pBuffer;
            g_PartyStreamDataQueue[entryCount].pData = pBuffer;
            HeapPinBlock(g_PartyStreamDataPointers[entryCount]);
            entryCount++;
        }
    }
    g_PartyStreamDataQueue[entryCount].pData = NULL;
    g_PartyStreamDataQueue[entryCount].archiveIndex = 0;
    func_80029AFC(g_PartyStreamDataQueue, 0, 0);
    D_8004F31C = 2;
}

void GamePartySyncSkinData(void) {
    GameWaitForCdData();
    if (g_PartyIsWaitingForStreamData == 1) {
        GamePartySyncStreamedData();
        if (g_GamePartySkinsInitialized) return;
    } else if (g_GamePartySkinsInitialized) {
        GamePartyStreamLoadSkinData(0);
        return;
    }

    if (!g_GamePartySkinsInitialized) {
        if (!(g_GameSceneMapNum & 0xC000)) D_8004F320 = 0;
        else D_8004F320 = 1;
        
        g_PartyIsWaitingForStreamData = 0;
        if (D_8004F320 == 0) {
            if (D_8004F31C != 1) {
                GamePartyCharactersInitializeSkins();
                g_PartyIsWaitingForStreamData = 1;
            }
        } else if (D_8004F31C != 2) {
            GamePartyGearsInitializeSkins();
            g_PartyIsWaitingForStreamData = 1;
        }
    }
}

void GamePartyStreamLoadSkinData(int arg0) {
    int i;
    int entryIndex;

    HeapToggleErrorHandler(1);

    if (g_PartyIsWaitingForStreamData == 1) {
        GamePartySyncStreamedData();
    }
    GameWaitForCdData();
    
    for (i = 0, entryIndex = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_GamePartyMemberSkins[i] == CHARACTER_ID_NONE) {
            continue;
        }

        g_PartyStreamDataQueue[entryIndex].archiveIndex = g_GamePartyMemberSkins[i] + 5;
        g_PartyStreamDataPointers[entryIndex] = HeapAlloc(ArchiveDecodeAlignedSize(g_GamePartyMemberSkins[i] + 5), 0x1);
        g_PartyStreamDataQueue[entryIndex].pData = g_PartyStreamDataPointers[entryIndex];
        if (g_PartyStreamDataQueue[entryIndex].pData == 0) {
            for (i = 0; i < entryIndex; i++) {
                HeapUnpinBlock(g_PartyStreamDataPointers[i]);
                HeapFree(g_PartyStreamDataPointers[i]);
            }
            
            HeapToggleErrorHandler(0);
            return;
        }

        HeapPinBlock(g_PartyStreamDataPointers[entryIndex]);
        entryIndex++;
    }
        
    if (arg0 != 0) {
        D_8005A4A0 = HeapAlloc(ArchiveDecodeAlignedSize(0xA7), 0x1);
        g_PartyStreamDataQueue[entryIndex].pData = D_8005A4A0;
        if (g_PartyStreamDataQueue[entryIndex].pData != 0) {
            HeapPinBlock(g_PartyStreamDataQueue[entryIndex].pData);
            g_PartyStreamDataQueue[entryIndex].archiveIndex = 0xA7;
            entryIndex++;
            D_8004F344 = 1;
        }

        D_8005A4BC = HeapAlloc(ArchiveDecodeAlignedSize(0xA8), 0x1);
        g_PartyStreamDataQueue[entryIndex].pData = D_8005A4BC;
        if (g_PartyStreamDataQueue[entryIndex].pData != 0) {
            HeapPinBlock(g_PartyStreamDataQueue[entryIndex].pData);
            g_PartyStreamDataQueue[entryIndex].archiveIndex = 0xA8;
            entryIndex++;
            D_8004F32C = 0;
        }
    }

    g_PartyStreamDataQueue[entryIndex].pData = 0;
    g_PartyStreamDataQueue[entryIndex].archiveIndex = 0;
    func_80029AFC(g_PartyStreamDataQueue, 0, 0); // Start streaming the queued requests
    g_PartyIsWaitingForStreamData = 1;
    HeapToggleErrorHandler(0);
}

void GamePartySyncStreamedData(void) {
    int i;
    
    if (g_PartyIsWaitingForStreamData) {
        GameWaitForCdData();
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            HeapUnpinBlock(g_PartyDataBuffers[i]);
            if (g_GamePartyMembers[i] != CHARACTER_ID_NONE) {
                HeapUnpinBlock(g_PartyStreamDataPointers[i]);
                LZSSDecompress(g_PartyStreamDataPointers[i], g_PartyDataBuffers[i]);
                HeapFree(g_PartyStreamDataPointers[i]);
            }
        }
        g_PartyIsWaitingForStreamData = 0;
    }
}


extern int D_8005A4C0; // File size
extern void* D_8005A4E0; // File buffer

extern s32 D_8004F330;
extern s32 D_8004F334;

int func_8001B484(int fileIndex, s32 arg1) {
    if (D_8004F334 == arg1 && D_8004F330 == fileIndex) {
        return 0;
    }

    if (ArchiveDataSync() == 0) {
        ArchiveCdDataSync(0);
        if (D_8004F334 != -1) {
            HeapUnpinBlock(D_8005A4E0);
            HeapFree(D_8005A4E0);
        }
        func_8001B53C(fileIndex);
        D_8004F334 = arg1;
        D_8004F330 = fileIndex;
    }
    
    return -1;
}

void func_8001B53C(int index) {
    D_8005A4C0 = ArchiveDecodeAlignedSize(index + 0xB8);
    D_8005A4E0 = HeapAlloc(D_8005A4C0, 1);
    HeapPinBlock(D_8005A4E0);
    ArchiveReadFileToBuffer(index + 0xB8, D_8005A4E0, 0, CdlModeSpeed);
}

extern int g_GameHasLoadedWDS;
extern SoundWDSEntry* g_GameCurLoadedWDS;

void func_8001B5A8(void) {
    if (g_GameHasLoadedWDS == 1) {
        SoundFreeWdsEntry(g_GameCurLoadedWDS);
        g_GameHasLoadedWDS = 0;
    }
}

extern s32 D_8004F35C;
extern s32 D_8004F348;
extern s32 D_8004F2FC;
extern void* D_80062528;
extern void func_80039C4C(void* handle);
extern void func_800399D4(void* handle);

void func_8001B5E8(void) {
    if (D_8004F35C != 1) return;
    func_80039C4C(D_80062528);
    if (D_8004F348 == 0) {
        func_800399D4(D_80062528);
    } else {
        D_8004F2FC = (s32)D_80062528;
    }
    D_8004F35C = 0;
    D_8004F348 = 0;
}

extern s32 D_8004F338;
extern s32 D_8004F33C;
extern s32 D_8004F36C;
extern void func_8001B5E8(void);

void func_8001B66C(void) {
    if (D_8004F36C != 0) {
        func_8001B5E8();
        func_8001B5A8();
    }

    D_8004F33C = -1;
    D_8004F338 = -1;
    D_8004F36C = 0;
}

void func_8001B6BC(void) {}

#endif /* temp3 proper */

#if defined(XENO_PC_PORT) || defined(TEMP3_PART_B)
extern s32* D_8005917C;
extern u8 D_800C48EA;
extern u8 D_800D3338;
extern u16 D_8006F94E;
extern u16 D_8006F950;
extern u16 D_8006F952;
extern u16 D_8006F954;
extern void func_8003747C(void*);
extern void* FontLoadFont(int sx, int sy, int w, int h, s32 f1, s32 f2, s32 f3, s32 f4, s32 f5, s32 f6, s32 f7);
extern void func_80070F40(void);
extern void ChangeGameState(unsigned int state);
extern void MainLoop(int errorCode);

/* func_8001B6C4 (temp3b) reaches these two temp3c bytes absolutely, as
 * plain externs; temp3c's own functions reach them %gp_rel. */
#define T3_ABS7C D_8005947C
#define T3_ABSF8 D_800594F8

void func_8001B6C4(void) {
    s32 state;
    s32 next;
    D_8005959C = 1;
    ArchiveCdDataSync(0);
    ArchiveSetIndex(0xC, 0);
    if (D_8005917C[0] != -1) {
        func_8003747C(0x80200000);
        FontLoadFont(0x10, 0x10, 0x140, 0x100, 0x3E8, 0, 0x340, 0, 0x340, 0x20, 0);
    }
    func_8001B844();
    func_80070F40();

#ifdef XENO_PC_PORT
    /* The battle overlay owns these bytes in guest RAM. Its writes do not
     * update the native build's generated placeholders for overlay symbols. */
    state = *(u8*)PSX_ADDR(0x800C48EAu);
#else
    state = D_800C48EA;
#endif
    /* `state` is compared only; the selected value travels to the single
     * ChangeGameState call through `next` (which the allocator keeps in
     * $a0), matching retail's per-path `ori $a0` setup. */
    if (state == 1 || state == 0x40 || state == 0x21) {
#ifdef XENO_PC_PORT
        if (*(u8*)PSX_ADDR(0x800D3338u) == 0) {
#else
        if (D_800D3338 == 0) {
#endif
            if (T3_ABS7C == 0) {
                u16 tmp = D_8006F94E & 0x7FF;
                if (tmp < 0x400) {
                    next = 1;
                } else {
                    next = 3;
                }
            } else {
                next = 2;
            }
        } else {
            next = 6;
        }
    } else if (state == 0x81) {
        GamePartySignalReinitialize();
        D_8006F94E = 0x1EA;
        D_8006F950 = 0;
        D_8006F952 = 0;
        D_8006F954 = 0;
        next = 1;
    } else {
        /* Retail 0x8001B7D0 skips state selection for other result bytes. */
        goto battle_return;
    }
    ChangeGameState(next);

battle_return:
    if (T3_ABS7C == 0) {
        T3_ABSF8 = 1;
    }
    MainLoop(0);
}

extern u8 D_800C4A7C[];
#ifdef XENO_PC_PORT
/* Retail 0x800379D0 is exactly `jr ra; nop`. Unspecified arguments: retail
 * calls it with two, six, then one argument, leaving the other registers
 * untouched (all dead). */
void func_800379D0();
void func_800379D0() {
}
#else
extern void func_800379D0();
#endif
extern void func_8001B94C(DRAWENV* pDrawEnv);

void func_8001B844(void) {
    u8* pBase = D_800C4A7C;
    /* $v0 still holds call 2's 0x1000 (a void callee leaves it alone);
     * the third call reuses it. Spelled as an uninitialized $v0 pseudonym
     * so gcc emits no materialization of its own. */
#ifdef XENO_PC_PORT
    int siz = 0x1000;
#else
    REG_PIN(int, siz, "$2");
#endif
    ResetGraph(1);
    func_800379D0(0x300, 0);
    func_800379D0(8, 0x10, 0x140, 0xF0, 0, 0x1000);
    func_800379D0(siz);
    InitGeom();
    SetGeomOffset(0xA0, 0xB4);
    SetGeomScreen(0x200);
    SetDefDispEnv((DISPENV*)pBase, 0, 0xE0, 0x140, 0xE0);
    SetDefDrawEnv((DRAWENV*)(pBase - 0x5C), 0, 0, 0x140, 0xE0);
    SetDefDispEnv((DISPENV*)(pBase + 0x4070), 0, 0, 0x140, 0xE0);
    SetDefDrawEnv((DRAWENV*)(pBase + 0x4014), 0, 0xE0, 0x140, 0xE0);
    func_8001B94C((DRAWENV*)(pBase - 0x5C));
    func_8001B94C((DRAWENV*)(pBase + 0x4014));
}

void func_8001B94C(DRAWENV* pDrawEnv) {
    pDrawEnv->isbg = 1;
    pDrawEnv->dtd = 1;
    pDrawEnv->r0 = 60;
    pDrawEnv->g0 = 120;
    pDrawEnv->b0 = 120;
}

#endif /* temp3b */

#if defined(XENO_PC_PORT) || defined(TEMP3_PART_C)
/* New Game gamestate init (asm 8001B970): load the new-game save TEMPLATE
 * from archive index 0x10, file 3 (0x2358 bytes) into g_GameState, then
 * decode the template's 31 character-name records (0x14-byte blocks over
 * the first 0x26C; save-format u16 codes -> glyph pairs via func_80033B34,
 * terminator pair 0x0F,0x00), reset the sound-volume block, and set the
 * two mode bytes.  Retail's volume reset zeroes 20 halfwords DOWNWARD from
 * g_SoundVolumeController+6 -- i.e. the controller's first 8 bytes plus
 * 0x20 bytes of UNNAMED BSS below it (0x8005A3A0-0x8005A3BF, no symbol).
 * The port zeroes the controller part; the unnamed region has no host
 * symbol and is only ever nonzero after a return-to-title flow the port
 * does not have yet (fresh BSS is already zero) -- documented divergence,
 * revisit with the title-return flow. */
extern void* g_SystemDataEntries;
extern void func_80033B34(u16* src, u8* dst, s32 count);
extern void* memmove(u_char* pDst, u_char* pSrc, int size);

void func_8001B970(void) {
    void* buf;
    REG_PIN(u8*, base, "$17");
    u8 tmp[48];
    REG_PIN(u8*, q1, "$19");
    REG_PIN(u8*, q2, "$18");
    REG_PIN(s32, n, "$16");
    s32 k;

    ArchiveSetIndex(0x10, 0);
    HeapChangeCurrentUser(2, 0);
    buf = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);
    ArchiveReadFileToBuffer(3, buf, 0, 0x80);
    ArchiveCdDataSync(0);
    base = (u8*)&g_GameState;
    memmove(base, buf, 0x2358);
    HeapFree(buf);

    q1 = &tmp[0];
    q2 = &tmp[1];
    n = 0;
    for (; n < 0x26C; n += 0x14, base += 0x14) {
        s32 m;
        REG_PIN(int, f15, "$9");
        REG_PIN(u8*, g1, "$10");
        u8* p;
        u8* d2;
        u8* d1;
        k = 0;
        m = n;
        g1 = &((u8*)&g_GameState)[1];
        f15 = 0xF;
        p = base;
        d2 = q2;
        d1 = q1;
        for (; k < 0x14; d2 += 2, k += 2, d1 += 2) {
            u8 pv;
            u8* gp;
            *d1 = *p;
            gp = (u8*)((m + k) + (s32)g1);
            *d2 = *gp;
            pv = *p;
            p += 2;
            if (pv == f15 && *gp == 0) {
                break;
            }
        }
        func_80033B34((u16*)tmp, tmp + 24, k / 2);
        {
            s32 d = (s32)base;
            u8* s;
#ifdef XENO_PC_PORT
            s = tmp + 24;
#else
            /* Fresh rematerialization: a C tmp+24 here is CSE'd with the
             * call arg and kept in $s4 across the jal (a1 is clobbered). */
            asm volatile("addiu %0,$sp,0x28" : "=r" (s));
#endif
            {
                s32 e = (s32)base + 0x14;
                do {
                    *(u8*)d++ = *s++;
                } while (d < e);
            }
        }
    }

    {
        u16* pVol;
        k = 0x13;
        pVol = (u16*)((u8*)&g_SoundVolumeController + 6);
        for (; k >= 0; k--) {
            *pVol-- = 0;
        }
    }

    D_800594CC = 6;
    D_8005947C = 0;
}

extern u8 D_8006F9DE;
extern u8* D_80059470;
extern u8* D_8005949C;
extern u8* D_80059520;
extern s32 func_800379D8(s32 index, s32 variant,
                        u8** first, u8** second, u8** third);

s32 func_8001BB0C(void) {
    /* Retail 8001BB24 loads the byte index into a0; 8001BB3C puts the
     * third output address in the fifth argument slot, not vice versa. */
#ifdef XENO_PC_PORT
    /* Battle 80071130..54 writes the active 32-byte encounter to guest RAM.
     * Consume its +2 byte there; a separate native stub is not that record. */
    return func_800379D8(*(u8*)PSX_ADDR(0x8006F9DEu), 0,
                         &D_80059470, &D_80059520, &D_8005949C);
#else
    return func_800379D8(D_8006F9DE, 0, &D_80059470, &D_80059520, &D_8005949C);
#endif
}

/* Retail boot/reset helper used by func_8007954C exit 3 when D_800B0064 bit 7
 * is set. Byte-exact in the matching build; the same body serves the port. */
void func_8001BB50(void) {
    extern u8 D_800594D5;
    extern u8 D_800594D6;

    D_800594F8 = 1;
    D_8005946C = 0;
    func_8001B970();
    ArchiveCdDataSync(0);
    D_800594D4 = 0x88;
    D_800594D5 = 0x76;
    D_800594D6 = 0x54;
    D_800595A0 = 2;
}

extern s16 D_8006F9BC;
extern u8* D_8006F9C0;
extern s16 D_8006F9C4;
extern s16 D_8006F9CC;
extern u8* D_8006F9C8;
extern u32 D_8006F9D0;
extern s16 D_8006F9D4;
extern s32 D_8006F9D8;
extern u8 D_8004F388[];
extern void SoundAddSedsEntry(void* pData);
extern void func_80039DB8(s32 a0);

void func_8001BBAC(void) {
    s32 i;
    u8* p80;
    u8* pA8;
    u32 aBC;
    int addr;
#ifdef XENO_PC_PORT
    /* The retail queue is four 8-byte records. Native pointers need the
     * host StreamDataQueueEntry layout consumed by archive_port.c. Keep
     * this adapter alive through ArchiveDataSync, as on the retail path. */
    StreamDataQueueEntry queue[4];
#endif
    HeapChangeCurrentUser(2, NULL);
    ArchiveSetIndex(0xC, 0);
    p80 = HeapAlloc(4, 1);
    D_80059480 = p80;
#ifdef XENO_PC_PORT
    /* Retail 8001BBE4..8001BBF8 reserves down to guest 801E4000.
     * HeapAlloc returns a host pointer in the port; do the retail address
     * arithmetic in the guest domain, not on the host allocation address. */
    D_800594AC = HeapAlloc(PsxMemory_GuestAddr(D_80059480) + 0x7FE1C000u, 1);
#else
    D_800594AC = HeapAlloc((u32)p80 + 0x7FE1C000, 1);
#endif
    addr = 0x801E4000;
    D_800595D0 = HeapAlloc(ArchiveDecodeAlignedSize(2), 1);
    pA8 = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);
    /* Address laundered through an integer so the halfword store below
     * addresses it via register (shared with the queue call). The empty
     * barrier keeps the expansion from folding the store back to a
     * symbolic address; it emits no code of its own. */
    aBC = (u32)&D_8006F9BC;
    __asm__ volatile("" : "=r" (aBC) : "0" (aBC));
    *(u16*)aBC = 2;
    D_8006F9C4 = 3;
    D_800595A8 = pA8;
    D_8006F9C8 = pA8;
    D_8006F9CC = 4;
    D_8006F9D0 = addr;
    D_8006F9D4 = 0;
    D_8006F9D8 = 0;
    /* SW at 8001BC90 stores the first buffer; SW at 8001BC58 stores
     * the second HeapAlloc result. These are full pointers, not shorts. */
    D_8006F9C0 = D_800595D0;
#ifdef XENO_PC_PORT
    queue[0] = (StreamDataQueueEntry){2, 0, D_8006F9C0};
    queue[1] = (StreamDataQueueEntry){3, 0, D_8006F9C8};
    queue[2] = (StreamDataQueueEntry){4, 0, PSX_ADDR(D_8006F9D0)};
    queue[3] = (StreamDataQueueEntry){0, 0, NULL};
    func_80029AFC(queue, 0, 0x80);
#else
    func_80029AFC((u8*)aBC, 0, 0x80);
#endif
    while (ArchiveDataSync() == 3) {}
    SoundAddSedsEntry(D_800595D0);
    if (D_8005954C != 4) {
        i = 0;
        do {
#ifdef XENO_PC_PORT
            /* Retail 8001BCC8..8001BCF0 indexes initialized EXE data.
             * The generated D_8004F388 host symbol is only zero-filled BSS. */
            u8 val = ((u8*)PSX_ADDR(0x8004F388))[D_8005954C * 3 + i++];
#else
            u8 val = D_8004F388[D_8005954C * 3 + i++];
#endif
            if (val == 0xFF) {
                continue;
            }
            /* Retail 8001BD04 is LHU, followed by SLL 16. */
            func_80039DB8((u32)*(u16*)(D_800595D0 + 0x14) << 16 | val);
        } while (i < 3);
    }
}

u8 func_8001BD40(u8 min, u8 max) {
    REG_PIN(u8, mx, "$5") = max;
    int range;
    if (min == 0xFF) {
        return 0xFF;
    }
    if (mx == 0) {
        return 0;
    }
    if (min == mx) {
        return min;
    }
    range = mx - min;
    if (range >= 0xFF) {
        return (u8)(rand() & 0xFF);
    }
    {
#ifdef XENO_PC_PORT
        int rr = rand();
        int d = range + 1;
        return (u8)(((rr & 0xFF) % d + min) & 0xFF);
#else
        /* cc1 otherwise schedules addiu $v1,$s0,1 ahead of andi $v0,$v0,0xFF.
         * One asm block locks that order. Do not pin $2/$3: local register
         * asm reserves them for the whole function and collapses the min
         * copy/andi prologue. */
        int rr = rand();
        int d;
        asm volatile(
            "andi %0,%0,0xff\n\t"
            "addiu %1,%3,1"
            : "=r" (rr), "=r" (d)
            : "0" (rr), "r" (range));
        return (u8)((rr % d + min) & 0xFF);
#endif
    }
}

#endif /* temp3c */
