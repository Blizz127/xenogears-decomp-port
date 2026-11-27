void ShopMenuLoadResources(void)
{
    u32 *table = PcPort_LoadCommonShopResources();
    void *entries = LZSSHeapDecompress((void *)(uintptr_t)table[7], 0);
    g_Menu->pShopEntries = entries;
    HeapFree(table);
}
