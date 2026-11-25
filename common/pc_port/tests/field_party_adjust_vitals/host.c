#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Synthetic state for testing the host interfaces. */
typedef uint16_t u16;
enum { MAX_PARTY_MEMBERS = 3, MAX_GAME_CHARACTERS = 11 };
typedef struct {
    uint32_t prefixGuard;
    u16 maxMp, hp, maxHp, mp;
    uint32_t suffixGuard;
} GameCharacter;
typedef struct { GameCharacter characters[MAX_GAME_CHARACTERS]; } GameState;
extern GameState *g_pGameState;
extern int g_GamePartyMembers[MAX_PARTY_MEMBERS];

typedef uint32_t u32;

_Static_assert(sizeof(int) == 4 && sizeof(u16) == 2, "vital arithmetic widths");


#ifdef BUILD_COMMANDS
#ifndef VITALS_IMPLEMENTATION
#define VITALS_IMPLEMENTATION "../../src/cleanroom/field_party_adjust_vitals.inc"
#endif
#include VITALS_IMPLEMENTATION
#else

typedef void Adjust(int slot, int amount);
Adjust FieldPartyMemberIncreaseHp, FieldPartyMemberDecreaseHp;
Adjust FieldPartyMemberIncreaseMp, FieldPartyMemberDecreaseMp;

enum Operation { HP_UP, HP_DOWN, MP_UP, MP_DOWN };
static Adjust *const operations[] = {
    FieldPartyMemberIncreaseHp, FieldPartyMemberDecreaseHp,
    FieldPartyMemberIncreaseMp, FieldPartyMemberDecreaseMp
};
GameState *g_pGameState;
int g_GamePartyMembers[MAX_PARTY_MEMBERS];

static GameState primary, alternative, expected, alternativeBefore;
static unsigned cases;

static u16 *quantity(GameCharacter *member, enum Operation op)
{
    return op == HP_UP || op == HP_DOWN ? &member->hp : &member->mp;
}

static u16 *maximum(GameCharacter *member, enum Operation op)
{
    return op == HP_UP || op == HP_DOWN ? &member->maxHp : &member->maxMp;
}

struct Access { void *address; size_t width; int store; };
static struct Access expectedAccess[8];
static size_t accessCount, consumed;
static int switchSlot, switchTo;
static unsigned long observedAccesses;

static void expect(void *address, size_t width, int store)
{
    assert(accessCount < sizeof expectedAccess / sizeof expectedAccess[0]);
    expectedAccess[accessCount++] = (struct Access){address, width, store};
}

#ifdef OBSERVE_ACCESSES
static void observe(void *address, size_t width, int store)
{
    assert(consumed < accessCount);
    const struct Access *item = &expectedAccess[consumed++];
    assert(item->address == address && item->width == width && item->store == store);
    ++observedAccesses;
    /* The store callback runs just before the first quantity write. Change the
     * external world here to distinguish a second slot read from a cached ID,
     * and a retained state pointer from a second state-pointer read. */
    if (switchSlot >= 0 && consumed == 4) {
        assert(store);
        g_GamePartyMembers[switchSlot] = switchTo;
        g_pGameState = &alternative;
    }
}

void __tsan_init(void) {}
void __tsan_func_entry(void *caller) { (void)caller; }
void __tsan_func_exit(void) {}
#define ACCESS(width) \
    void __tsan_read##width(void *p) { observe(p, width, 0); } \
    void __tsan_volatile_read##width(void *p) { observe(p, width, 0); } \
    void __tsan_write##width(void *p) { observe(p, width, 1); } \
    void __tsan_volatile_write##width(void *p) { observe(p, width, 1); }
ACCESS(1)
ACCESS(2)
ACCESS(4)
ACCESS(8)
#undef ACCESS
#endif

static int signedAmount(uint32_t amount)
{
    return amount <= INT32_MAX ? (int)amount : (int)((int64_t)amount - INT64_C(4294967296));
}

static void check(enum Operation op, int slot, int identity, u16 value, u16 limit,
                  uint32_t amount, int change, u16 secondValue, u16 secondLimit)
{
    const int restored = op == HP_UP || op == MP_UP;
    const int next = change ? (identity + 3) % MAX_GAME_CHARACTERS : identity;
    int rosterExpected[MAX_PARTY_MEMBERS];

    memset(&primary, (unsigned char)(value ^ 0xa7u), sizeof primary);
    memset(&alternative, (unsigned char)(limit ^ 0x53u), sizeof alternative);
    for (int i = 0; i < MAX_PARTY_MEMBERS; ++i)
        g_GamePartyMembers[i] = (identity + 1 + i) % MAX_GAME_CHARACTERS;
    g_GamePartyMembers[slot] = identity;
    g_pGameState = &primary;

    *quantity(&primary.characters[identity], op) = value;
    *maximum(&primary.characters[identity], op) = limit;
    if (change) {
        *quantity(&primary.characters[next], op) = secondValue;
        *maximum(&primary.characters[next], op) = secondLimit;
    }
    memcpy(&expected, &primary, sizeof expected);
    memcpy(&alternativeBefore, &alternative, sizeof alternativeBefore);
    memcpy(rosterExpected, g_GamePartyMembers, sizeof rosterExpected);
    rosterExpected[slot] = next;
    accessCount = consumed = 0;
    switchSlot = change ? slot : -1;
    switchTo = next;
    u16 *current = quantity(&primary.characters[identity], op);
    expect(&g_GamePartyMembers[slot], sizeof g_GamePartyMembers[slot], 0);
    expect(&g_pGameState, sizeof g_pGameState, 0);
    expect(current, sizeof *current, 0);
    expect(current, sizeof *current, 1);
    if (restored) {
        *quantity(&expected.characters[identity], op) = (u16)((uint32_t)value + amount);
        expect(&g_GamePartyMembers[slot], sizeof g_GamePartyMembers[slot], 0);
        expect(maximum(&primary.characters[next], op), sizeof(u16), 0);
        expect(quantity(&primary.characters[next], op), sizeof(u16), 0);
        u16 *newCurrent = quantity(&expected.characters[next], op);
        u16 newLimit = *maximum(&expected.characters[next], op);
        if (*newCurrent > newLimit) {
            expect(quantity(&primary.characters[next], op), sizeof(u16), 1);
            *newCurrent = newLimit;
        }
    } else {
        uint32_t remaining = (uint32_t)value - amount;
        *quantity(&expected.characters[identity], op) =
            signedAmount(remaining) > 0 ? (u16)remaining : 1;
    }
    operations[op](slot, signedAmount(amount));
#ifdef OBSERVE_ACCESSES
    assert(consumed == accessCount);
#endif
    assert(!memcmp(&expected, &primary, sizeof primary));
    assert(!memcmp(&alternativeBefore, &alternative, sizeof alternative));
    assert(!memcmp(rosterExpected, g_GamePartyMembers, sizeof rosterExpected));
    assert(g_pGameState == (change ? &alternative : &primary));
    ++cases;
}

int main(void)
{
    static const u16 values[] = {0, 1, 2, 255, 32767, 32768, 65534, 65535};
    static const uint32_t amounts[] = {0, 1, 2, 65535, 65536, 65537,
        0x7fffffffu, 0x80000000u, 0x80000001u, 0xffff0000u, 0xfffffffeu, 0xffffffffu};
    for (enum Operation op = HP_UP; op <= MP_DOWN; ++op) {
        for (int slot = 0; slot < MAX_PARTY_MEMBERS; ++slot) {
            for (int member = 0; member < MAX_GAME_CHARACTERS; ++member) {
                for (size_t v = 0; v < sizeof values / sizeof values[0]; ++v) {
                    for (size_t a = 0; a < sizeof amounts / sizeof amounts[0]; ++a) {
                        u16 limit = values[(a + slot + member) % (sizeof values / sizeof values[0])];
                        check(op, slot, member, values[v], limit, amounts[a], 0, 0, 0);
                    }
                }
#ifdef OBSERVE_ACCESSES
                for (u16 second = 2; second <= 4; ++second)
                    check(op, slot, member, 65530, 5, 7, 1, second, 3);
#endif
            }
        }
    }
    printf("%u party vital cases, %lu observed accesses\n", cases, observedAccesses);
    return 0;
}
#endif
