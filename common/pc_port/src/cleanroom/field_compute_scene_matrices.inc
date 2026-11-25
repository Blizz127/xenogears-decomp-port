void FieldComputeSceneMatrices(void)
{
    extern void FieldMatrixCopy(MATRIX *dest, MATRIX *source);

    MATRIX composed;
    long flag;
    VECTOR scale;
    int uniformScale;

    RotMatrix(&g_Scene.camRotation, &g_Scene.camRotationMatrix);
    FieldMatrixResetTranslation(&g_Scene.camRotationMatrix);
    CompMatrix(&g_Scene.camRotationMatrix, &g_Scene.viewMatrix, &composed);
    FieldMatrixCopy(&g_Scene.viewMatrix, &composed);

    RotMatrix(&g_Scene.worldRotation, &g_Scene.worldRotationMatrix);
    FieldMatrixResetTranslation(&g_Scene.worldRotationMatrix);
    RotMatrix(&g_Scene.worldRotation, &g_Scene.worldToScreenMatrix);
    MulMatrix2(&g_Scene.viewMatrix, &g_Scene.worldToScreenMatrix);

    SetRotMatrix(&g_Scene.viewMatrix);
    SetTransMatrix(&g_Scene.viewMatrix);
    RotTrans(&g_Scene.worldTranslation, (VECTOR *)g_Scene.worldToScreenMatrix.t, &flag);

    uniformScale = g_WorldScale;
    scale.vx = uniformScale;
    scale.vy = uniformScale;
    scale.vz = uniformScale;
    ScaleMatrix(&g_Scene.worldToScreenMatrix, &scale);
    SetRotMatrix(&g_Scene.worldToScreenMatrix);
    SetTransMatrix(&g_Scene.worldToScreenMatrix);
}
