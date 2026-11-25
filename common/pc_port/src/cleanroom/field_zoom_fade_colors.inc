void func_800A5600(u8 intensity)
{
    const volatile int *context = &g_FieldCurRenderContextIndex;
    for (int index = 0; index < 10; index += 2) {
        volatile POLY_FT4 *pair = &g_FieldZoomFadeEffect.polygons[index];
        pair[1 - (*context & 1)].r0 = intensity;
        pair[1 - (*context & 1)].g0 = intensity;
        pair[1 - (*context & 1)].b0 = intensity;
    }
}
