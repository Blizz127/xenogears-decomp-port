void SoundQueueSpuReadCommand(u32 address, void *data, u_long length,
                              SoundCommandCallback_t callback)
{
    SoundQueueTransferCommand(address, data, length, callback, 2);
}

void func_8003BC58(u32 address, void *data, u_long length,
                   SoundCommandCallback_t callback)
{
    SoundQueueTransferCommand(address, data, length, callback, 3);
}

void func_8003BC7C(u32 address, void *data, u_long length,
                   SoundCommandCallback_t callback)
{
    SoundQueueTransferCommand(address, data, length, callback, 4);
}

void SoundUnkDebugNoReturn_8003F4BC(void)
{
}

void func_80039D24(void)
{
}

void func_8003A2D4(void)
{
}

void func_8003A2DC(void)
{
}

void func_8003BDF4(void)
{
}

int func_8003977C(void)
{
    return 0;
}
