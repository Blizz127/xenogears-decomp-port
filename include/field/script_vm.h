#ifndef _XENO_FIELD_SCRIPT_VIRTUAL_MACHINE_H
#define _XENO_FIELD_SCRIPT_VIRTUAL_MACHIEN_H

#define FIELD_SCRIPT_VM_VAR_UNSIGNED -1
#define FIELD_SCRIPT_VM_VAR_SIGNED 0

#define FIELD_SCRIPT_VM_COND_EQUAL 0x0
#define FIELD_SCRIPT_VM_COND_NOT_EQUAL2 0x1
#define FIELD_SCRIPT_VM_COND_LT 0x2
#define FIELD_SCRIPT_VM_COND_LT2 0x3
#define FIELD_SCRIPT_VM_COND_GTE 0x4
#define FIELD_SCRIPT_VM_COND_GTE2 0x5
#define FIELD_SCRIPT_VM_COND_AND 0x6
#define FIELD_SCRIPT_VM_COND_NOT_EQUAL 0x7
#define FIELD_SCRIPT_VM_COND_OR 0x8
#define FIELD_SCRIPT_VM_COND_AND2 0x9
#define FIELD_SCRIPT_VM_COND_NAND 0xA

// Addresses / Slot IDs of global field script variables
#define SCRIPT_VAR_SCENARIO_FLAG 0x0
#define SCRIPT_VAR_EVENT_TIME 0xA
#define SCRIPT_VAR_SYSTEM_TIME_LOW 0xC // Seconds and minutes
#define SCRIPT_VAR_SYSTEM_TIME_HIGH 0xE // Hours

typedef void (*ScriptVMHandler)(void);

// Is Yielding maybe?
extern int D_800B00C0;
extern char D_8006FD84;
extern int D_800ADBE0;
extern int D_800ADBE4;
extern int D_800ADBEC;
extern int D_800ADB1C;

extern int FieldScriptVMGetInstructionArgument(int);
extern int FieldScriptVMGetVariableSign(int);
extern int FieldScriptVMGetVariableValue(int);
extern short FieldScriptVMGetInstructionArgumentS16(int);
extern ScriptVMHandler g_FieldScriptVMHandlers[];
extern ScriptVMHandler g_FieldScriptVMHandlers2[];
#endif
