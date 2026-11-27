#include "field_party_masked_adjust.h"

static void fieldPartyRaiseGearHp(int slot, int amount)
{
    FieldPartyMemberIncreaseGearHp(slot, (unsigned int)amount);
}

static void fieldPartyLowerGearHp(int slot, int amount)
{
    FieldPartyMemberDecreaseGearHp(slot, (unsigned int)amount);
}

void FieldScriptVMHandlerIncreasePartyGearHp(void)
{
    fieldPartyAdjustSelected(fieldPartyRaiseGearHp);
}

void FieldScriptVMHandlerDecreasePartyGearHp(void)
{
    fieldPartyAdjustSelected(fieldPartyLowerGearHp);
}
