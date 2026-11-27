extern int PsyX_XenoRenderedClipMisses(int, int, int, int);
extern void GR_StoreFrameBufferImmediate(int, int, int, int);
extern void FieldRenderSync(void);

void func_800A476C(int x, int y)
{
    RECT source = {0, 0, 320, 224};
    SetGeomScreen(512);
    if (PsyX_XenoRenderedClipMisses(source.x, source.y, source.w, source.h) == 0)
        GR_StoreFrameBufferImmediate(source.x, source.y, source.w, source.h);
    MoveImage(&source, x, y);
    FieldRenderSync();
}
