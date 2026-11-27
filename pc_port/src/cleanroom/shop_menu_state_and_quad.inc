void func_801C5040(POLY_FT4* pPoly, short x, short y, u_char u, u_char v,
                   short width, short height)
{
    volatile POLY_FT4* quad = pPoly;

    quad->x0 = x;
    quad->y0 = y;
    quad->y1 = y;
    quad->x2 = x;
    quad->u0 = u;
    quad->u2 = u;
    quad->x1 = (short)(x + width);
    quad->y2 = (short)(y + height);
    quad->x3 = (short)(x + width);
    quad->y3 = (short)(y + height);
    quad->v0 = v;
    quad->u1 = (u_char)(u + width);
    quad->v1 = v;
    quad->v2 = (u_char)(v + height);
    quad->u3 = (u_char)(u + width);
    quad->v3 = (u_char)(v + height);
}

u_short ShopMenuIsCharacterFlagSet(u_short value, u_char maskIndex)
{
    return D_801D21F0[maskIndex] & value;
}

u_short ShopMenuGetCharacterBitMask(int maskIndex)
{
    return D_801D21F0[(u_char)maskIndex];
}

void ShopMenuSetManager(u_char isInitialization)
{
    if (isInitialization) {
        MenuManager* manager = HeapAlloc(sizeof(*manager), 0);
        g_Menu->pManager = manager;
        bzero((u8*)manager, sizeof(*manager));
    } else {
        HeapFree(g_Menu->pManager);
    }
}

void ShopMenuSelectionMenuManager(u_char isInitialization)
{
    if (isInitialization) {
        MenuSelectionMenu* selection = HeapAlloc(sizeof(*selection), 0);
        g_Menu->pSelectionMenu = selection;
        bzero((u8*)selection, sizeof(*selection));
    } else {
        HeapFree(g_Menu->pSelectionMenu);
    }
}

void ShopMenuUnk354Manager(u_char isInitialization)
{
    if (isInitialization) {
        MenuUnk5* work = HeapAlloc(sizeof(*work), 0);
        g_Menu->unk354 = work;
        bzero((u8*)work, sizeof(*work));
    } else {
        HeapFree(g_Menu->unk354);
    }
}

void ShopMenuUnk348Manager(u_char isInitialization)
{
    if (isInitialization) {
        MenuUnk1* work = HeapAlloc(sizeof(*work), 0);
        g_Menu->unk348 = work;
        bzero((u8*)work, sizeof(*work));
    } else {
        HeapFree(g_Menu->unk348);
    }
}

void ShopMenuUnk1E20Manager(u_char isInitialization)
{
    if (isInitialization) {
        MenuUnk7* work = HeapAlloc(sizeof(*work), 0);
        g_Menu->unk1E20 = work;
        bzero((u8*)work, sizeof(*work));
    } else {
        HeapFree(g_Menu->unk1E20);
    }
}

void ShopMenuShopManager(u_char isInitialization)
{
    if (isInitialization) {
        MenuShop* shop = HeapAlloc(sizeof(*shop), 0);
        g_Menu->pShop = shop;
        bzero((u8*)shop, sizeof(*shop));
    } else {
        HeapFree(g_Menu->pShop);
    }
}

void ShopMenuResetRenderContext(void)
{
    g_Menu->renderContext = 0;
}
