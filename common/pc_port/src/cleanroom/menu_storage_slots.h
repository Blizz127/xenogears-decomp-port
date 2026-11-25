#ifndef XENO_MENU_STORAGE_SLOTS_H
#define XENO_MENU_STORAGE_SLOTS_H

static inline void menu_set_32c_storage(u_char setting) {
    if (setting == 0) {
        HeapFree(g_Menu->unk32C);
        return;
    }
    MenuUnk2 *storage = HeapAlloc(sizeof(*storage), 0);
    g_Menu->unk32C = storage;
    bzero((u8 *)storage, sizeof(*storage));
}

static inline void menu_set_330_storage(u_char setting) {
    if (setting == 0) {
        HeapFree(g_Menu->unk330);
        return;
    }
    MenuUnk6 *storage = HeapAlloc(sizeof(*storage), 0);
    g_Menu->unk330 = storage;
    bzero((u8 *)storage, sizeof(*storage));
}

#endif
