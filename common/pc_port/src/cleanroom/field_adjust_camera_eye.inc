void func_80073684(VECTOR* eye, VECTOR* at)
{
    MATRIX rotation;
    VECTOR delta;
    VECTOR rotated;
    volatile VECTOR* currentAt = at;
    volatile VECTOR* currentEye = eye;

    PushMatrix();
    RotMatrix(&g_Scene.sceneAngle, &rotation);
    delta.vx = (s32)((u32)at->vx - (u32)eye->vx);
    delta.vy = (s32)((u32)at->vy - (u32)eye->vy);
    delta.vz = (s32)((u32)at->vz - (u32)eye->vz);
    ApplyMatrixLV(&rotation, &delta, &rotated);

    /* With overlapping vectors the eye X store can change the next target Z. */
    currentEye->vx = (s32)((u32)currentAt->vx + (u32)rotated.vx);
    currentEye->vz = (s32)((u32)currentAt->vz + (u32)rotated.vz);
    PopMatrix();
}
