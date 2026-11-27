#include <stdint.h>

extern u32 FieldScriptVMGetActorIndex(int);
extern short FieldScriptVMGetInstructionArgumentS16(int);
extern int FieldScriptArgument1(int, int);
extern int FieldScriptArgument2(int, int);
extern int FieldScriptVMGetArgument(int);
extern int fieldActorCommandRandom(void) __asm__("rand");
extern void func_8009E574(s32, s32);
extern void func_8009E810(s16);
extern void func_8009F5F4(void);
extern void func_8009FD10(int);
extern int D_800B00C0;

static volatile ActorData *fieldCommandActor(void)
{
    return *(ActorData *volatile *)&g_FieldScriptVMCurActor;
}

static u8 fieldCommandByte(unsigned offset)
{
    volatile ActorData *actor = fieldCommandActor();
    const volatile u8 *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    return script[(unsigned)actor->scriptInstructionPointer + offset];
}

void FieldScriptVMHandlerHideActorById(void)
{
    if (FieldScriptVMGetActorIndex(1) != 255) {
        int index = FieldScriptVMGetActorIndex(1);
        volatile FieldActor *target = &g_FieldActors[index];
        target->status |= 0x20;
    }
    fieldCommandActor()->scriptInstructionPointer += 2;
}

void func_8009E248(void)
{
    int x = FieldScriptVMGetInstructionArgumentS16(1);
    int z = FieldScriptVMGetInstructionArgumentS16(3);
    func_8009E574(x, z);
    func_8009E810(FieldScriptVMGetInstructionArgumentS16(5));
    volatile ActorData *actor = fieldCommandActor();
    actor->scriptFlags.flags |= 1u << 18;
    actor->scriptInstructionPointer += 7;
}

void func_8009E2C8(void)
{
    int value = FieldScriptArgument1(1, fieldCommandByte(3));
    func_8009E810((s16)value);
    volatile ActorData *actor = fieldCommandActor();
    actor->scriptFlags.flags |= 1u << 18;
    actor->scriptInstructionPointer += 4;
}

static void fieldCommandPosition(unsigned selectorOffset, unsigned length)
{
    int x = FieldScriptArgument1(1, fieldCommandByte(selectorOffset));
    int z = FieldScriptArgument2(3, fieldCommandByte(selectorOffset));
    func_8009E574(x, z);
    volatile ActorData *actor = fieldCommandActor();
    actor->flags &= ~(1u << 21);
    actor->scriptFlags.flags &= ~(1u << 16);
    actor->scriptInstructionPointer += length;
}

void func_8009E35C(void)
{
    volatile ActorData *actor = fieldCommandActor();
    const volatile u8 *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    actor->walkmeshId = script[(unsigned)actor->scriptInstructionPointer + 5u];
    fieldCommandPosition(6, 7);
}

void func_8009E4BC(void)
{
    fieldCommandPosition(5, 6);
}

void func_8009F424(void)
{
    volatile ActorData *actor = fieldCommandActor();
    u16 counter = (u16)(actor->unk102 + 1);
    actor->unk102 = (s16)counter;
    s16 yaw = actor->rotation.vy;
    if ((counter & 15u) == 0) {
        int noise = fieldActorCommandRandom();
        actor = fieldCommandActor();
        int turn = (noise & 1) ? -512 : 512;
        yaw = (s16)((actor->rotation.vy + turn) & 4095);
    }
    D_800B00C0 = 1;
    actor->rotation.vx = yaw;
    actor->scriptInstructionPointer++;
}

void func_8009F5A8(void)
{
    u16 previous = fieldCommandActor()->scriptInstructionPointer;
    func_8009F5F4();
    D_800B00C0 = 1;
    fieldCommandActor()->scriptInstructionPointer = previous;
}

static void fieldCommandSetGearRide(u8 enabled)
{
    int slot = FieldScriptVMGetArgument(1);
    if (slot > 2) {
        slot = 2;
    }
    volatile u8 *record = (volatile u8 *)g_pGameState;
    record[(intptr_t)__builtin_offsetof(GameState, gearRide) + slot] = enabled;
    func_8009FD10(slot);
    fieldCommandActor()->scriptInstructionPointer += 3;
}

void FieldScriptPartyMemberRideGear(void)
{
    fieldCommandSetGearRide(1);
}

void FieldScriptPartyMemberDisembarkGear(void)
{
    fieldCommandSetGearRide(0);
}
