#include "menu_initialize_party.h"

void ShopMenuInitialize(void) {
    menu_initialize_party(ShopMenuIsCharacterFlagSet, ShopMenuLoadResources);
}
