#include "common.h"
#include "field/script_vm.h"
#include "field/actor.h"

void func_8009CF70(void) {}

// If the argument bit is set, treat the argument as an immediate,
// if not, the argument is loaded from the script VM's RAM.
int FieldScriptArgument1(int index, int mask) {
    if ((mask & 0x80) == 0) {
        return FieldScriptVMGetVariableValue(
            FieldScriptVMGetInstructionArgument(index) & 0xFFFF
        ); 
    }
    return FieldScriptVMGetInstructionArgumentS16(index);
}

int FieldScriptArgument2(int index, int mask) {
    if ((mask & 0x40) == 0) {
        return FieldScriptVMGetVariableValue(
            FieldScriptVMGetInstructionArgument(index) & 0xFFFF
        ); 
    }
    return FieldScriptVMGetInstructionArgumentS16(index);
}

int FieldScriptArgument3(int index, int mask) {
    if ((mask & 0x20) == 0) {
        return FieldScriptVMGetVariableValue(
            FieldScriptVMGetInstructionArgument(index) & 0xFFFF
        ); 
    }
    return FieldScriptVMGetInstructionArgumentS16(index);
}

int FieldScriptArgument4(int index, int mask) {
    if ((mask & 0x10) == 0) {
        return FieldScriptVMGetVariableValue(
            FieldScriptVMGetInstructionArgument(index) & 0xFFFF
        ); 
    }
    return FieldScriptVMGetInstructionArgumentS16(index);
}

int FieldScriptArgument5(int index, int mask) {
    if ((mask & 0x8) == 0) {
        return FieldScriptVMGetVariableValue(
            FieldScriptVMGetInstructionArgument(index) & 0xFFFF
        ); 
    }
    return FieldScriptVMGetInstructionArgumentS16(index);
}

int FieldScriptArgument6(int index, int mask) {
    if ((mask & 0x4) == 0) {
        return FieldScriptVMGetVariableValue(
            FieldScriptVMGetInstructionArgument(index) & 0xFFFF
        ); 
    }
    return FieldScriptVMGetInstructionArgumentS16(index);
}

int FieldScriptArgument7(int index, int mask) {
    if ((mask & 0x2) == 0) {
        return FieldScriptVMGetVariableValue(
            FieldScriptVMGetInstructionArgument(index) & 0xFFFF
        ); 
    }
    return FieldScriptVMGetInstructionArgumentS16(index);
}

int FieldScriptArgument8(int index, int mask) {
    if ((mask & 0x1) == 0) {
        return FieldScriptVMGetVariableValue(
            FieldScriptVMGetInstructionArgument(index) & 0xFFFF
        ); 
    }
    return FieldScriptVMGetInstructionArgumentS16(index);
}

void FieldScriptVMHandlerRandVariable(void) {
    int nValue;

    nValue = FieldScriptVMGetInstructionArgument(1) & 0xFFFF;
    FieldScriptMemoryWriteU16(nValue, rand());
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerMulVariableWithRand(void) {
    int nValue;
    int nRandom;
    int nArgument1;

    nRandom = rand();
    nArgument1 = FieldScriptVMGetArgument(3);
    nValue = (nRandom * (nArgument1 + 1)) >> 0xF;
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, nValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerRShiftVariable(void) {
    int nVariableValue;
    int nArgument;
    
    nArgument = FieldScriptVMGetInstructionArgument(1) & 0xFFFF;
    nVariableValue = FieldScriptVMGetVariableValue(nArgument);
    nVariableValue >>= FieldScriptVMGetArgument(3);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerLShiftVariable(void) {
    int nVariableValue;
    int nArgument;

    nArgument = FieldScriptVMGetInstructionArgument(1) & 0xFFFF;
    nVariableValue = FieldScriptVMGetVariableValue(nArgument);
    nVariableValue <<= FieldScriptVMGetArgument(3);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerIncVariable(void) {
    int nValue;

    nValue = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF) + 1;
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, nValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerDecVariable(void) {
    int nValue;

    nValue = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF) - 1;
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, nValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerVariableUnsetBit(void) {
    int nVariableValue;
    int nBit;

    nVariableValue = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF);
    nBit = FieldScriptArgument2(3, *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 5]);
    nVariableValue = nVariableValue & ~(1 << nBit);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerVariableXOR(void) {
    int nVariableValue;
    int nArgument;

    nVariableValue = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF);
    nArgument = FieldScriptArgument2(3, *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 5]);
    nVariableValue = nVariableValue ^ nArgument;
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerVariableOR(void) {
    int nVariableValue;
    int nArgument;

    nVariableValue = FieldScriptVMGetVariableValue((u_short) FieldScriptVMGetInstructionArgument(1));
    nArgument = FieldScriptArgument2(3, *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 5]);
    nVariableValue = nVariableValue | nArgument;
    FieldScriptMemoryWriteU16((u_short) FieldScriptVMGetInstructionArgument(1), nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerVariableAND(void) {
    int nVariableValue;
    int nArgument;

    nVariableValue = FieldScriptVMGetVariableValue((u_short) FieldScriptVMGetInstructionArgument(1));
    nArgument = FieldScriptArgument2(3, *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 5]);
    nVariableValue = nVariableValue & nArgument;
    FieldScriptMemoryWriteU16((u_short) FieldScriptVMGetInstructionArgument(1), nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerVariableSetBit(void) {
    int nVariableValue;
    int nBit;

    nVariableValue = FieldScriptVMGetVariableValue((u_short) FieldScriptVMGetInstructionArgument(1));
    nBit = FieldScriptArgument2(3, *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 5]);
    nVariableValue |= (1 << nBit);
    FieldScriptMemoryWriteU16((u_short) FieldScriptVMGetInstructionArgument(1), nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerVariableMul(void) {
    int nVariableValue;
    int nArgument;

    nVariableValue = FieldScriptVMGetVariableValue((u_short) FieldScriptVMGetInstructionArgument(1));
    nArgument = FieldScriptArgument2(3, *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 5]);
    nVariableValue = nVariableValue * nArgument;
    FieldScriptMemoryWriteU16((u_short) FieldScriptVMGetInstructionArgument(1), nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerVariableDiv(void) {
    int nVariableValue;
    int nArgument;
    int nVariableAddress;

    nVariableValue = FieldScriptVMGetVariableValue((u_short) FieldScriptVMGetInstructionArgument(1));
    nArgument = FieldScriptArgument2(3, *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 5]);
    
    // Handle division by zero
    if (nArgument == 0) {
        nArgument = 1;
    }

    nVariableValue /= nArgument;
    nVariableAddress = (u_short) FieldScriptVMGetInstructionArgument(1);
    FieldScriptMemoryWriteU16(nVariableAddress, nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerVariableSub(void) {
    int nVariableValue;
    int nArgument;

    nVariableValue = FieldScriptVMGetVariableValue((u_short) FieldScriptVMGetInstructionArgument(1));
    nArgument = FieldScriptArgument2(3, *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 5]);
    nVariableValue -= nArgument;
    FieldScriptMemoryWriteU16((u_short) FieldScriptVMGetInstructionArgument(1), nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerVariableAdd(void) {
    int nVariableValue;
    int nArgument;

    nVariableValue = FieldScriptVMGetVariableValue((u_short) FieldScriptVMGetInstructionArgument(1));
    nArgument = FieldScriptArgument2(3, *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 5]);
    nVariableValue += nArgument;
    FieldScriptMemoryWriteU16((u_short) FieldScriptVMGetInstructionArgument(1), nVariableValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerVariableSetFalse(void) {
    int nVariableAddress;

    nVariableAddress = (u_short) FieldScriptVMGetInstructionArgument(1);
    FieldScriptMemoryWriteU16(nVariableAddress, 0x0);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerVariableSetTrue(void) {
    int nVariableAddress;

    nVariableAddress = (u_short) FieldScriptVMGetInstructionArgument(1);
    FieldScriptMemoryWriteU16(nVariableAddress, 0x1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerVariableAssign(void) {
    int nVariableAddress;
    int nValue;

    nVariableAddress = FieldScriptVMGetInstructionArgument(1) & 0xFFFF;
    nValue = FieldScriptArgument2(3, *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 5]);
    FieldScriptMemoryWriteU16(nVariableAddress, nValue);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}
