/* The animation byte stack grows downward in the packed sprite record. */
enum {
    ANIM_STACK_CURSOR = 0x8c,
    ANIM_STACK_BYTES = 0x8e
};

static inline int animStackOffset(const unsigned char *record)
{
    unsigned int cursor = record[ANIM_STACK_CURSOR];
    return cursor < 128 ? (int)cursor : (int)cursor - 256;
}

static inline unsigned int animPopBytes(SpriteData *sprite, unsigned int width)
{
    unsigned char *record = (unsigned char *)sprite;
    const unsigned char *top = record + ANIM_STACK_BYTES + animStackOffset(record);
    unsigned int value = 0;

    for (unsigned int i = 0; i < width; ++i)
        value |= (unsigned int)top[i] << (8 * i);
    record[ANIM_STACK_CURSOR] += width;
    return value;
}

static inline void animPushBytes(SpriteData *sprite, unsigned int width, unsigned int value)
{
    unsigned char *record = (unsigned char *)sprite;

    record[ANIM_STACK_CURSOR] -= width;
    for (unsigned int i = 0; i < width; ++i) {
        /* A write can alias the cursor. Resolve it for each emitted byte. */
        int offset = ANIM_STACK_BYTES + animStackOffset(record) + (int)i;
        record[offset] = (unsigned char)(value >> (8 * i));
    }
}

u_char AnimScriptStackPopU8(SpriteData *sprite)
{
    return (u_char)animPopBytes(sprite, 1);
}

s32 AnimScriptStackPopU16(SpriteData *sprite)
{
    int value = (int)animPopBytes(sprite, 2);
    return value < 0x8000 ? value : value - 0x10000;
}

s32 AnimScriptStackPopU24(SpriteData *sprite)
{
    return (s32)animPopBytes(sprite, 3);
}

void AnimScriptStackPushU8(SpriteData *sprite, u8 value)
{
    animPushBytes(sprite, 1, value);
}

void AnimScriptStackPushU16(SpriteData *sprite, u16 value)
{
    animPushBytes(sprite, 2, value);
}

void AnimScriptStackPushU24(SpriteData *sprite, s32 value)
{
    animPushBytes(sprite, 3, (unsigned int)value);
}
