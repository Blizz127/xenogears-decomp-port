#include <stdint.h>

void FieldDistortionFree(void)
{
    volatile FieldDistortion *view = &g_FieldEffects.distortion;
    volatile int *state = &D_800ADB24;
    int allocated = *state;
    view->isActive = 0;
    if (allocated) {
        (void)HeapFree((void *)(uintptr_t)view->buffer0);
        (void)HeapFree((void *)(uintptr_t)view->buffer1);
        (void)HeapFree((void *)(uintptr_t)view->buffer2);
        (void)HeapFree((void *)(uintptr_t)view->buffer3);
        *state = 0;
    }
}
