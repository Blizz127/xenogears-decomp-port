#include <stdint.h>

void func_8007FFE8(void)
{
    s16 index;

    for (index = 0; index < 4; ++index) {
        if (g_FieldTextBoxes[index].visibility == 0) {
            func_8007F6F8(index);
        }
    }
}

void func_800805F4(void)
{
    s16 index;

    for (index = 0; index < 4; ++index) {
        FieldTextBox *box = &g_FieldTextBoxes[index];
        s16 timer;

        if (box->visibility != 0) {
            continue;
        }
        if (box->windowOpenTimer == 0 && (box->_pad[8] & 4) == 0) {
            func_8007F6F8(index);
        }
        if (box->status == 0) {
            func_8007F6F8(index);
        }

        timer = box->windowOpenTimer;
        if (timer == INT16_MIN) {
            box->windowOpenTimer = INT16_MAX;
        } else if (timer != 0) {
            box->windowOpenTimer = timer - 1;
        }
    }
}
