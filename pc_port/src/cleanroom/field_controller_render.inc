#include <stdint.h>
#include <string.h>

/* Controller buffers and scaled coordinates are stored as 32-bit words. */
static int32_t field_controller_word(uint32_t bits)
{
    int32_t word;
    memcpy(&word, &bits, sizeof(word));
    return word;
}

void func_800796F4(void)
{
}

void func_8007995C(short width, short height, short x, short y,
                   int destination_x, int destination_y)
{
    RECT source = { x, y, width, height };
    MoveImage(&source, destination_x, destination_y);
    DrawSync(0);
}

void FieldRenderSyncAndFlush(void)
{
    FieldRenderSync();
    EnterCriticalSection();
    FlushCache();
    ExitCriticalSection();
}

void FieldSetControllerBuffers(void *first, void *second)
{
    g_pFieldControllerBuffer1 = field_controller_word((uint32_t)(uintptr_t)first);
    g_pFieldControllerBuffer2 = field_controller_word((uint32_t)(uintptr_t)second);
}

void func_8007ADA4(int left, int right, int top, int bottom)
{
    uint32_t x_speed = g_FieldMouseSpeedX;
    uint32_t y_speed = g_FieldMouseSpeedY;

    D_800C3A44 = field_controller_word((uint32_t)left * x_speed);
    D_800C3A50 = field_controller_word((uint32_t)right * x_speed);
    D_800C3A4C = field_controller_word((uint32_t)top * y_speed);
    D_800C3A54 = field_controller_word((uint32_t)bottom * y_speed);
}

void FieldSetMouseSpeed(u_short x, u_short y)
{
    g_FieldMouseSpeedX = x;
    g_FieldMouseSpeedY = y;
}
