void FieldMatrixResetTranslation(MATRIX *matrix)
{
    volatile MATRIX *target = matrix;
    target->t[2] = 0;
    target->t[1] = 0;
    target->t[0] = 0;
}

void FieldMatrixCreateWorldToScreen(void)
{
    FieldComputeSceneMatrices();
    SetRotMatrix(&g_Scene.worldToScreenMatrix);
    SetTransMatrix(&g_Scene.worldToScreenMatrix);
    if (g_FieldSystemMode == 0)
        func_802815B0();
}
