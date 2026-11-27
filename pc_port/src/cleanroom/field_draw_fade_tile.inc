extern void GR_MaterializeFramebufferRect(int, int, int, int);

static int field_fade_active_bank(void) {
    return *(volatile int *)&g_FieldCurRenderContextIndex;
}

static void *field_fade_bank(void *base, unsigned stride, int index) {
    return (void *)((uintptr_t)base + (intptr_t)(s32)((u32)index * stride));
}

void func_80079784(int color) {
    unsigned char shade = (unsigned char)((u32)color * 4u);
    FieldClearAndSwapOTag();
    volatile TILE *paint = field_fade_bank(D_800AFE54, sizeof(TILE), field_fade_active_bank());
    paint->b0 = shade;
    paint->g0 = shade;
    paint->r0 = shade;

    int bank = field_fade_active_bank();
    TILE *tile = field_fade_bank(D_800AFE54, sizeof(TILE), bank);
    DR_MODE *mode = field_fade_bank(D_800AFE24, sizeof(DR_MODE), bank);
    u32 *head = FIELD_CTX_OT1(g_FieldCurRenderContext);
    addPrim(head, tile);
    addPrim(head, mode);
    FieldRenderSync();
    MoveImage(&D_800AFE4C, 0, (s32)((u32)field_fade_active_bank() << 8));
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
    GR_MaterializeFramebufferRect(0, (s32)((u32)field_fade_active_bank() << 8),
                                  D_800AFE4C.w, D_800AFE4C.h);
    DrawOTag((u_long *)(FIELD_CTX_OT1(g_FieldCurRenderContext) + 1));
}
