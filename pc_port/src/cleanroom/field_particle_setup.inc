static short fieldParticleSignedHalf(int value)
{
    unsigned int bits = (unsigned int)value & 0xffffu;
    return (short)((int)(bits & 0x7fffu) - (int)(bits & 0x8000u));
}

static signed char fieldParticleSignedByte(int value)
{
    unsigned int bits = (unsigned int)value & 0xffu;
    return (signed char)((int)(bits & 0x7fu) - (int)(bits & 0x80u));
}

static int fieldParticleMode(int offset)
{
    const unsigned char *script = g_FieldScriptVMCurScriptData;
    return script[g_FieldScriptVMCurActor->scriptInstructionPointer + offset];
}

static void fieldParticleFinish(unsigned int instructionLength)
{
    /* The VM charges a wrapping machine word held in a signed int. */
    unsigned int *count = (unsigned int *)&g_FieldScriptMaxInstructionCount;
    *count += 4u;
    g_FieldScriptVMCurActor->scriptInstructionPointer =
        (unsigned short)(g_FieldScriptVMCurActor->scriptInstructionPointer + instructionLength);
}

/* Decode before looking up the bank. Commit this field before the next read
 * because the script and its mode byte can share storage with a template. */
#define FIELD_PARTICLE_STORE(member, convert, expression)                         \
    do {                                                                          \
        int componentValue = (expression);                                        \
        unsigned int bankIndex = (unsigned int)D_800B2384.bankIndex;              \
        if (bankIndex < NUM_PARTICLE_BANKS) {                                     \
            g_FieldDefaultParticleBanks[bankIndex].member = convert(componentValue); \
        }                                                                         \
    } while (0)

void FieldScriptInitializeParticleBank(void)
{
    int index = FieldScriptVMGetArgument(1);
    D_800B2384.bankIndex = index;
    if ((unsigned int)index < NUM_PARTICLE_BANKS) {
        ParticleBank *bank = &g_FieldDefaultParticleBanks[index];
        bank->speedMultiplier = 1;
        bank->targetActorID = fieldParticleSignedHalf(D_800B2374);
        bank->unk0 = 0;
        D_800ADB40 = bank->targetActorID;
        bank->rotAngle = 0;

        FIELD_PARTICLE_STORE(max, fieldParticleSignedHalf, FieldScriptVMGetArgument(3));
        FIELD_PARTICLE_STORE(swait, (unsigned short), FieldScriptVMGetArgument(5));
        FIELD_PARTICLE_STORE(ewait, (unsigned short), FieldScriptVMGetArgument(7));
        if ((unsigned int)D_800B2384.bankIndex < NUM_PARTICLE_BANKS) {
            func_8008861C();
        }
    }
    fieldParticleFinish(9);
}

void FieldScriptSetParticleBankPosition(void)
{
    FIELD_PARTICLE_STORE(pos.vx, fieldParticleSignedHalf, FieldScriptArgument1(1, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(pos.vy, fieldParticleSignedHalf, FieldScriptArgument2(3, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(pos.vz, fieldParticleSignedHalf, FieldScriptArgument3(5, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(epos.vx, fieldParticleSignedHalf, FieldScriptArgument4(7, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(epos.vy, fieldParticleSignedHalf, FieldScriptArgument5(9, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(epos.vz, fieldParticleSignedHalf, FieldScriptArgument6(11, fieldParticleMode(13)));
    fieldParticleFinish(14);
}

void FieldScriptSetParticleBankPhysics(void)
{
    FIELD_PARTICLE_STORE(speed, (int), FieldScriptArgument1(1, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(gravity.vx, fieldParticleSignedHalf, FieldScriptArgument2(3, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(gravity.vy, fieldParticleSignedHalf, FieldScriptArgument3(5, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(gravity.vz, fieldParticleSignedHalf, FieldScriptArgument4(7, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(srange, (unsigned short), FieldScriptArgument5(9, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(erange, (unsigned short), FieldScriptArgument6(11, fieldParticleMode(13)));
    fieldParticleFinish(14);
}

void FieldScriptSetParticleBankParameters(void)
{
    unsigned int flags;
    FIELD_PARTICLE_STORE(pswait, (unsigned short), FieldScriptVMGetArgument(1));
    FIELD_PARTICLE_STORE(pewait, (unsigned short), FieldScriptVMGetArgument(3));
    FIELD_PARTICLE_STORE(shape, fieldParticleSignedHalf, FieldScriptVMGetArgument(5));
    flags = (unsigned int)FieldScriptVMGetArgument(7);
    flags |= (unsigned int)FieldScriptVMGetArgument(9) << 1;
    flags |= (unsigned int)D_800B2378;
    FIELD_PARTICLE_STORE(flags, (unsigned short), (int)(flags & 0xffffu));
    FIELD_PARTICLE_STORE(unk72, fieldParticleSignedHalf, D_800B237C);
    FIELD_PARTICLE_STORE(unk74, fieldParticleSignedHalf, D_800B2380);
    fieldParticleFinish(11);
}

void FieldScriptSetParticleBankScale(void)
{
    FIELD_PARTICLE_STORE(scale.vx, fieldParticleSignedHalf, FieldScriptArgument1(1, fieldParticleMode(9)));
    FIELD_PARTICLE_STORE(scale.vy, fieldParticleSignedHalf, FieldScriptArgument2(3, fieldParticleMode(9)));
    FIELD_PARTICLE_STORE(scale.vz, (short), 0);
    FIELD_PARTICLE_STORE(scaleDelta.vx, fieldParticleSignedHalf, FieldScriptArgument3(5, fieldParticleMode(9)));
    FIELD_PARTICLE_STORE(scaleDelta.vy, fieldParticleSignedHalf, FieldScriptArgument4(7, fieldParticleMode(9)));
    FIELD_PARTICLE_STORE(scaleDelta.vz, (short), 0);
    fieldParticleFinish(10);
}

void FieldScriptSetParticleBankColor(void)
{
    FIELD_PARTICLE_STORE(color.r, (unsigned char), FieldScriptArgument1(1, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(color.g, (unsigned char), FieldScriptArgument2(3, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(color.b, (unsigned char), FieldScriptArgument3(5, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(colorDelta.r, fieldParticleSignedByte, FieldScriptArgument4(7, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(colorDelta.g, fieldParticleSignedByte, FieldScriptArgument5(9, fieldParticleMode(13)));
    FIELD_PARTICLE_STORE(colorDelta.b, fieldParticleSignedByte, FieldScriptArgument6(11, fieldParticleMode(13)));
    fieldParticleFinish(14);
}

#undef FIELD_PARTICLE_STORE
