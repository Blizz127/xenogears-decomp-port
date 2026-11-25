void MemberChangeMenuPlaySoundEffect(u_char soundEffectId)
{
    u32 bank = g_Menu->unk2E4->sedId;
    func_80039DB8((bank << 16) | soundEffectId);
}

void MemberChangeMenuFreeWindow(u_char index)
{
    MenuWindow* window;
    MenuWindowParameters* parameters;
    uintptr_t slot;

    *(volatile u8*)((uintptr_t)g_Menu->pManager->shouldRenderWindow + index) = 0;
    *(volatile u8*)((uintptr_t)g_Menu->pManager->unk27 + index) = 0;

    slot = (uintptr_t)g_Menu->windows + (uintptr_t)index * sizeof(window);
    __builtin_memcpy(&window, (const void*)slot, sizeof(window));
    HeapFree(window);

    slot = (uintptr_t)g_Menu->windowParameters + (uintptr_t)index * sizeof(parameters);
    __builtin_memcpy(&parameters, (const void*)slot, sizeof(parameters));
    HeapFree(parameters);
}

void MemberChangeMenuRenderBackgroundDim(void)
{
    SystemMenu* menu = g_Menu;
    GfxEnvironment* graphics = menu->pGfxEnv;
    u32 context = (u32)menu->renderContext;
    uintptr_t base = (uintptr_t)menu->unk348->polysDimEffect;
    s32 offset = (s32)(context * (u32)sizeof(POLY_F4));

    AddPrim(&graphics->ot[8], (void*)(base + offset));

    menu = g_Menu;
    graphics = menu->pGfxEnv;
    context = (u32)menu->renderContext;
    base = (uintptr_t)menu->unk348->drawModeDimEffect;
    offset = (s32)(context * (u32)sizeof(DR_MODE));
    AddPrim(&graphics->ot[8], (void*)(base + offset));
}

void func_801C5CF4(void)
{
    *(volatile u8*)&g_Menu->pManager->unk4 = 0;
    *(volatile u8*)&g_Menu->pManager->unk3 = 0;
}

void func_801C5D24(POLY_G4* pPoly, u_char red, u_char green, u_char blue)
{
    volatile POLY_G4* colors = pPoly;

    SetPolyG4(pPoly);
    colors->r0 = red;
    colors->g0 = green;
    colors->b0 = blue;
    colors->r1 = red;
    colors->g1 = green;
    colors->b1 = blue;
    colors->r2 = 0;
    colors->g2 = 0;
    colors->b2 = 0;
    colors->r3 = 0;
    colors->g3 = 0;
    colors->b3 = 0;
}

void func_801C7DA8(void)
{
    int slot;
    for (slot = 0; slot < 4; ++slot) {
        SystemMenu* menu = g_Menu;
        if (menu->pManager->unk34[slot]) {
            MenuString* record = &menu->unk4E0[slot];
            uintptr_t primitive = (uintptr_t)record->polys +
                                  (uintptr_t)record->renderContext * sizeof(POLY_FT4);
            AddPrim(&menu->pGfxEnv->ot[4], (void*)primitive);
        }
    }
}

void func_801C7E38(void)
{
    int slot;
    for (slot = 0; slot < 8; ++slot) {
        SystemMenu* menu = g_Menu;
        if (menu->pManager->unkC[slot]) {
            MenuString* record = &menu->unk6E0[slot];
            uintptr_t primitive = (uintptr_t)record->polys +
                                  (uintptr_t)record->renderContext * sizeof(POLY_FT4);
            AddPrim(&menu->pGfxEnv->ot[4], (void*)primitive);
        }
    }
}

void func_801C846C(void)
{
    func_801C7DA8();
    func_801C7E38();
    func_801C7EC8();
}
