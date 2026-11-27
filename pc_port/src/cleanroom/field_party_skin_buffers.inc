#include <stddef.h>

extern void *g_PartyDataBuffers[3];

void FieldPartyAllocateSkinDataBuffers(void)
{
    void *volatile *slots = g_PartyDataBuffers;
    void *third;
    void *first_to_pin;

    HeapChangeCurrentUser(8, NULL);
    slots[0] = HeapAlloc(0x14000, 0);
    slots[1] = HeapAlloc(0x14000, 0);
    third = HeapAlloc(0x14000, 0);
    first_to_pin = slots[0];
    slots[2] = third;
    HeapPinBlock(first_to_pin);
    HeapPinBlock(slots[1]);
    HeapPinBlock(slots[2]);
}

void FieldPartyFreeSkinDataBuffers(void)
{
    void *volatile *slots = g_PartyDataBuffers;

    for (int i = 0; i < 3; ++i) {
        HeapUnpinBlock(slots[i]);
    }
    for (int i = 0; i < 3; ++i) {
        HeapFree(slots[i]);
    }
}
