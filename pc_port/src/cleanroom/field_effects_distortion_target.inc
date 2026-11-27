static s32 fieldDistortionStep(int target, int current, int frames)
{
    u32 difference = ((u32)target << 16) - (u32)current;
    /* Widen the signed numerator so INT32_MIN divided by -1 also wraps. */
    long long signedDifference = (long long)(difference & 0x7fffffffu)
                                 - (long long)(difference & 0x80000000u);
    return (s32)(u32)(signedDifference / frames);
}

void FieldDistortionSetTarget(int t1, int t2, int t3, int t4, int t5, int t6, int duration)
{
    int frames = duration == 0 ? 1 : duration;
    volatile FieldDistortion *distortion = &g_FieldEffects.distortion;
    int d1 = fieldDistortionStep(t1, distortion->v1, frames);
    int d2 = fieldDistortionStep(t2, distortion->v2, frames);
    int d3 = fieldDistortionStep(t3, distortion->v3, frames);
    int d4 = fieldDistortionStep(t4, distortion->v4, frames);
    int d5 = fieldDistortionStep(t5, distortion->v5, frames);
    int d6 = fieldDistortionStep(t6, distortion->v6, frames);
    distortion->duration = (s16)(u16)frames;
    distortion->delta1 = d1;
    distortion->delta2 = d2;
    distortion->delta3 = d3;
    distortion->delta4 = d4;
    distortion->delta5 = d5;
    distortion->delta6 = d6;
}
