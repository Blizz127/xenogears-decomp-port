void FieldSwapRenderContext(void)
{
    unsigned int nextIndex = ((unsigned int)g_FieldCurRenderContextIndex & 1u) ^ 1u;

    g_FieldCurRenderContextIndex = (int)nextIndex;
    g_FieldCurRenderContext = &g_FieldRenderContexts[nextIndex];
    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(&g_FieldCurRenderContext->drawEnvs[0]);
}
