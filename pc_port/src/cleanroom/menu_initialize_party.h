#ifndef XENO_MENU_INITIALIZE_PARTY_H
#define XENO_MENU_INITIALIZE_PARTY_H

static inline SystemMenu *menu_initialize_current(void) {
    return *(SystemMenu *volatile *)&g_Menu;
}

static inline void menu_initialize_party(u_short (*isAvailable)(u_short, u_char),
                                         void (*loadResources)(void)) {
    menu_initialize_current()->menu1Choice = 4;
    menu_initialize_current()->unk337 = 255;
    menu_initialize_current()->unk326 = 60;
    menu_initialize_current()->unk334 = 0;
    menu_initialize_current()->unk335 = 0;

    u16 first = *(volatile u16 *)&g_GameState.unk1D30;
    u16 second = *(volatile u16 *)&g_GameState.FrMask;
    u16 mask = first & second & 0x7ff;
    for (unsigned index = 0; index < 16; ++index) {
        u16 selected = isAvailable(mask, index);
        menu_initialize_current()->availableCharacters[index] = selected != 0;
    }
    for (unsigned index = 0; index < MAX_PARTY_MEMBERS; ++index) {
        u8 id = g_GameState.partyMembers[index];
        SystemMenu *owner = menu_initialize_current();
        if (id < sizeof(owner->availableCharacters) && owner->availableCharacters[id]) {
            owner->pManager->currentCharacterIDs[index] = id;
        } else {
            if (id != 255)
                owner = menu_initialize_current();
            owner->pManager->currentCharacterIDs[index] = 255;
        }
    }
    loadResources();
}

#endif
