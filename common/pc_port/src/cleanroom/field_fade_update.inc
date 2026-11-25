static int FieldFadeAdvanceChannel(int current, int delta)
{
    u32 sum = (u32)current + (u32)delta;

    if (sum >= 0x80000000u) {
        return 0;
    }
    if (sum >= 0x10000u) {
        return 0xff00;
    }
    return (int)sum;
}

void FieldFadeUpdate(int index)
{
    FieldFade *fade = &g_FieldEffects.fades[index];

    if (!fade->isVisible) {
        return;
    }

    if (fade->duration > 0) {
        fade->r0 = FieldFadeAdvanceChannel(fade->r0, fade->redDelta);
        fade->g0 = FieldFadeAdvanceChannel(fade->g0, fade->greenDelta);
        fade->b0 = FieldFadeAdvanceChannel(fade->b0, fade->blueDelta);
        --fade->duration;
        return;
    }

    fade->duration = 0;
    if (D_800ADC08 != 1 && fade->r0 == 0 && fade->g0 == 0 && fade->b0 == 0) {
        fade->isVisible = 0;
    }
}
