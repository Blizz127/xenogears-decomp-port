void HeapDumpToFile(char *path)
{
    PCinit();
    int opened = PCcreate(path, 0);
    *(volatile int *)&g_HeapDebugDumpFileHandle = opened;
    FnPrintf_t *volatile *sink = &g_HeapDebugPrintfFn;
#ifdef _WIN32
    *sink = HeapFilePrintf;
#else
    *sink = (FnPrintf_t *)HeapWriteToDebugFile;
#endif
    HeapDebugDump(1, 0, 0, HEAP_DEBUG_PRINT_ALL);
    *sink = (FnPrintf_t *)func_800379C8;
    PCclose(opened);
}
