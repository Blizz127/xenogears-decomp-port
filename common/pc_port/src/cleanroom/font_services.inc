void FontPrintf(char *format, ...)
{
    Font *active = *(Font *volatile *)&g_Font;
    if (active) {
        va_list arguments;
        va_start(arguments, format);
        func_80036718(0, format, arguments);
        va_end(arguments);
    }
}

void FontFree(void)
{
    Font *active = *(Font *volatile *)&g_Font;
    void *volatile *binding = &D_800593A0;
    if (active) {
        void *resource = *binding;
        *(FnPrintf_t *volatile *)&g_HeapDebugPrintfFn = FontPrintf;
        if (!resource) {
            HeapFree(active);
        }
        *(Font *volatile *)&g_Font = NULL;
    }
    *binding = NULL;
}
