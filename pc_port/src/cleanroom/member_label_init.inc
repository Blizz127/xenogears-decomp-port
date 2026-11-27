void func_801C5B90(void) {
    SystemTransferPaletteToVRAM(0, 465);
    void *work = HeapAlloc(910, 0);
    MenuString *labels = g_Menu->unk4E0;
    labels[0].pVramBuffer = work;
    func_801C59E0(labels, D_801CB400, 0, 4);
    func_801C5724();
}
