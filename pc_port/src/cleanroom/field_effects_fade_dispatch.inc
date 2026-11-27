void FieldFadeInitializePrimitives(int index);
void FieldFadeUpdate(int index);
void FieldFadeDraw(u_long *orderingTable, int renderContext);

void FieldFadeInitialize(void)
{
    FieldFadeInitializePrimitives(0);
    FieldFadeInitializePrimitives(1);
}

void FieldFadeUpdateAndDraw(u_long *orderingTable)
{
    if (*(volatile int *)&D_800ADC04 == 2) {
        FieldFadeUpdate(0);
        FieldFadeUpdate(1);
    }
    int context = *(volatile int *)&g_FieldCurRenderContextIndex;
    FieldFadeDraw(orderingTable, context);
}
