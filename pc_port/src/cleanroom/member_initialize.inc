#include "menu_initialize_party.h"

void MemberChangeMenuInitialize(void) {
    menu_initialize_party(MemberChangeMenuIsCharacterFlagSet, MemberChangeMenuLoadResources);
}
