/* The on-disc callback table stores 32-bit words. Each host builder receives its declared argument types. */

static s32 ModelBuild8002CDCC(u32 source, u32 command, s32 shade)
{
    return func_8002CDCC((u8 *)(uintptr_t)source, (u8 *)(uintptr_t)command, shade);
}

static s32 ModelBuild8002CF34(u32 source, u32 command, s32 shade)
{
    (void)command;
    (void)shade;
    return func_8002CF34((s32 *)(uintptr_t)source);
}

static s32 ModelBuild8002CF58(u32 source, u32 command, s32 shade)
{
    return func_8002CF58((u8 *)(uintptr_t)source, (u8 *)(uintptr_t)command, shade);
}

static s32 ModelBuild8002D0C0(u32 source, u32 command, s32 shade)
{
    (void)command;
    (void)shade;
    return func_8002D0C0((s32 *)(uintptr_t)source);
}

static s32 ModelBuild8002D0E4(u32 source, u32 command, s32 shade)
{
    (void)command;
    (void)shade;
    return func_8002D0E4((u8 *)(uintptr_t)source);
}

static s32 ModelBuild8002D180(u32 source, u32 command, s32 shade)
{
    return func_8002D180((u8 *)(uintptr_t)source, (s16 *)(uintptr_t)command, shade);
}

static s32 ModelBuild8002D244(u32 source, u32 command, s32 shade)
{
    (void)shade;
    return func_8002D244((u8 *)(uintptr_t)source, (s16 *)(uintptr_t)command);
}

static s32 ModelBuild8002D530(u32 source, u32 command, s32 shade)
{
    return func_8002D530((u8 *)(uintptr_t)source, (s16 *)(uintptr_t)command, shade);
}

static s32 ModelBuild8002D6AC(u32 source, u32 command, s32 shade)
{
    return func_8002D6AC((u8 *)(uintptr_t)source, (s16 *)(uintptr_t)command, shade);
}

static s32 ModelBuild8002D77C(u32 source, u32 command, s32 shade)
{
    (void)shade;
    return func_8002D77C((u8 *)(uintptr_t)source, (s16 *)(uintptr_t)command);
}

static s32 ModelBuild8002D814(u32 source, u32 command, s32 shade)
{
    return func_8002D814((u8 *)(uintptr_t)source, (s16 *)(uintptr_t)command, shade);
}

static s32 ModelBuild8002D984(u32 source, u32 command, s32 shade)
{
    (void)command;
    (void)shade;
    return func_8002D984((u8 *)(uintptr_t)source);
}

static s32 ModelBuild8002DA14(u32 source, u32 command, s32 shade)
{
    (void)shade;
    return func_8002DA14((u8 *)(uintptr_t)source, (s16 *)(uintptr_t)command);
}

static s32 ModelBuild8002DAFC(u32 source, u32 command, s32 shade)
{
    (void)source;
    (void)command;
    (void)shade;
    return func_8002DAFC();
}
