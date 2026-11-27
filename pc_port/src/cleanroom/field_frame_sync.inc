void FieldRenderSync(void)
{
    DrawSync(0);
    Vsync(0);
}

void FieldUpdateDeltaTime(void)
{
    const int elapsed = Vsync(1);

    *(volatile int *)&g_FrameDeltaTime = elapsed;
}
