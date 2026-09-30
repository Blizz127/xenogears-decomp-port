#ifndef _XENO_GAME_H
#define _XENO_GAME_H

#define MAX_GAME_CHARACTERS 11
#define MAX_GAME_GEARS 20
#define MAX_PARTY_MEMBERS 3
#define MAX_BENCHED_PARTY_MEMBERS 6
#define CHARACTER_ID_NONE 0xFF

#define MAX_ITEM_QUANTITY 99
#define MAX_INVENTORY_WEAPONS 100
#define MAX_INVENTORY_ACCESSORIES 200
#define MAX_INVENTORY_ITEMS 150

#define MAX_GOLD_AMOUNT 9999999


typedef struct {
    /* 0x0  */ undefined8 field_0x0[0x4C];
    /* 0x4C */ u_short hp;
    /* 0x4E */ u_short maxHp;
    /* 0x50 */ u_short mp;
    /* 0x52 */ u_short maxMp;
    /* 0x54 */ undefined32 field_0x54;
    /* 0x58 */ u_char attack;
    /* 0x59 */ u_char defense;
    /* 0x5A */ u_char agility;
    /* 0x5B */ u_char ether;
    /* 0x5C */ u_char etherDefence;
    /* 0x5D */ undefined8 field_0x5D;
    /* 0x5E */ u_char hitPercentage;
    /* 0x5F */ u_char evadePercentage;
    /* 0x60 */ undefined16 field_0x60;
    /* 0x62 */ u_char level;
    /* 0x63 */ undefined8 unk63;
    /* 0x64 */ undefined8 unk64[0x6];
    /* 0x6A */ undefined8 unk6A[0x5]; // Equipped weapons?
    /* 0x6F */ undefined8 unk6F[0x5]; // Equipped weapons?    
    /* 0x74 */ undefined8 unk74[0x3]; // Equipped accessories?
    /* 0x77 */ undefined8 unk77;
    /* 0x78 */ u_char unk78; // PROVISIONAL: 0..200 gauge; ordinary items raise/
                             // lower it via func_801E31C0 (effectFlags bit 1,
                             // amount = statEffectFlags low byte, sign = bit 15;
                             // clamped 0..0xC8). Exact game meaning undetermined.
    /* 0x79 */ undefined8 unk79[0x27];
    /* 0xA0 */ u_char gearId;
    /* 0xA1 */ undefined8 field_0xA1[0x3];
} GameCharacter; // size: 0xA4


typedef struct {
    /* 0x0  */ undefined8 field_0x0[0x10];
    /* 0x10 */ short ether;
    /* 0x12 */ short maxEther;
    /* 0x14 */ undefined8 field_0x14[0x24];
    /* 0x38 */ int fuel;
    /* 0x3C */ int maxFuel;
    /* 0x40 */ undefined8 field_0x40[0x20];
    /* 0x60 */ u_int hp;
    /* 0x64 */ u_int maxHp;
    /* 0x68 */ undefined8 field_0x68[0x3C];
} GameGear; // size: 0xA4

typedef struct {
    /* 0x0    */ undefined8 field_0x0[0x26C];
    /* 0x26C  */ GameCharacter characters[MAX_GAME_CHARACTERS];
    /* 0x978  */ GameGear gears[MAX_GAME_GEARS];
    /* 0x1648 */ undefined8 unk1648[0x206];
    /* 0x184E */ undefined16 unk184E;
    /* 0x1850 */ undefined8 unk1850[0x2];
    /* 0x1852 */ undefined16 unk1852;
    /* 0x1854 */ undefined8 unk1854[0xD0];
    /* 0x1924 */ u_int gold;
    /* 0x1928 */ undefined8 unk1928[0x408];
    /* 0x1D30 */ undefined16 unk1D30;
    /* 0x1D32 */ u_short FrMask;
    /* 0x1D34 */ undefined8 partyMembers[MAX_PARTY_MEMBERS];
    /* 0x1D37 */ undefined8 unk1D37;

    // Inventory
    /* 0x1D38 */ u_char weaponQuantities[MAX_INVENTORY_WEAPONS];
    /* 0x1D9C */ u_char weaponIDs[MAX_INVENTORY_WEAPONS];
    /* 0x1E00 */ u_char accessoryQuantities[MAX_INVENTORY_ACCESSORIES];
    /* 0x1EC8 */ u_char accessoryIDs[MAX_INVENTORY_ACCESSORIES];
    /* 0x1F90 */ u_char itemQuantities[MAX_INVENTORY_ITEMS];
    /* 0x2026 */ u_char itemIDs[MAX_INVENTORY_ITEMS];

    /* PROVISIONAL fourth/fifth inventory families (the unk20BC split).
     * Same {quantities[N], ids[N]} tiling as weapons/accessories/items;
     * bulk-populated by func_801E5058 (IDs 1..bound-1, quantity 10) and
     * scanned by the field misc11 count-empty/find-by-id loops at these
     * offsets.  Likely the gear-side inventories; exact categories
     * undetermined -- rename when a reader proves them. */
    /* 0x20BC */ u_char unk20BCQuantities[0x64];
    /* 0x2120 */ u_char unk2120IDs[0x64];
    /* 0x2184 */ u_char unk2184Quantities[0x96];
    /* 0x221A */ u_char unk221AIDs[0x96];
    /* 0x22B0 */ undefined8 unk22B0;
    /* 0x22B1 */ undefined8 gearRide[MAX_PARTY_MEMBERS];
    /* 0x22B4 */ undefined8 unk22B4[0x2];
    /* 0x22B6 */ undefined16 unk22B6;
    /* 0x22B8 */
} GameState; // size: unknown

extern GameState g_GameState;
extern GameState* g_pGameState;
extern int g_GamePartyMembers[MAX_PARTY_MEMBERS];

#endif