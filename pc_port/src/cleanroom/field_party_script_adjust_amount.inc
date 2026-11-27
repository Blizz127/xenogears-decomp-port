#include "field_party_masked_adjust.h"

void FieldScriptVMHandlerDecreasePartyHp(void)
{
    fieldPartyAdjustSelected(FieldPartyMemberDecreaseHp);
}

void FieldScriptVMHandlerIncreasePartyMp(void)
{
    fieldPartyAdjustSelected(FieldPartyMemberIncreaseMp);
}

void FieldScriptVMHandlerDecreasePartyMp(void)
{
    fieldPartyAdjustSelected(FieldPartyMemberDecreaseMp);
}
