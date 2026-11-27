#ifndef PC_PORT_BATTLE_WIN32_HOST_H
#define PC_PORT_BATTLE_WIN32_HOST_H

#include <stddef.h>
#include <stdint.h>

/* The battle runtime must not mix the Windows SDK's types with PsyQ's. */
void *PcPort_Win32MainImageSymbol(const char *name);
size_t PcPort_Win32DataSymbolSize(const char *name);
int PcPort_Win32HostAddressValid(uintptr_t address, unsigned width, int write);
uint32_t PcPort_Win32ThreadId(void);
int PcPort_Win32BiosConsoleWrite(const void *bytes, unsigned length);

#endif
