#include "common.h"
#include "psyq/libgpu.h"

#include <stdio.h>
#include <stdlib.h>

void func_800ACB90(void);

void* D_800AF784;

static u8 tim_data[16];
static u8 clear_data[0x200];
static s32 alloc_count;
static s32 free_count;
static s32 draw_sync_count;
static s32 field_load_count;
static s32 load_image_count;
static s32 failures;
static s32 checks;

static void require(int condition, const char* name) {
    checks++;
    if (!condition) {
        fprintf(stderr, "ASSERTION acb90.%s\n", name);
        failures++;
    }
}

void FieldLoadTIMWithClut(u_long* tim, short x, short y, short clut_x,
                          short clut_y, short clut_w, short clut_h) {
    field_load_count++;
    require((void*)tim == D_800AF784, "tim_pointer");
    require(x == 0x380, "tim_x");
    require(y == 0x100, "tim_y");
    require(clut_x == 0 && clut_y == 0x1FF, "clut_xy");
    require(clut_w == 0 && clut_h == 0, "clut_wh");
}

int DrawSync(int mode) {
    require(mode == 0, "draw_sync_mode");
    draw_sync_count++;
    return 0;
}

void* HeapAlloc(u_int size, u_int flags) {
    require(size == 0x200, "alloc_size");
    require(flags == 1, "alloc_flags");
    alloc_count++;
    return clear_data;
}

u_int HeapFree(void* ptr) {
    if (free_count == 0) {
        require(ptr == D_800AF784, "free_tim");
    } else {
        require(ptr == clear_data, "free_clear_buffer");
    }
    free_count++;
    return 0;
}

int LoadImage(RECT* rect, u_long* pixels) {
    s32 i;
    load_image_count++;
    require(rect->x == 0x3C0 && rect->y == 0x100, "clear_rect_xy");
    require(rect->w == 0x40 && rect->h == 4, "clear_rect_wh");
    require((void*)pixels == clear_data, "clear_buffer_pointer");
    for (i = 0; i < 0x80; i++) {
        require(((u32*)pixels)[i] == 0xFFFFFFFFu, "clear_buffer_word");
    }
    return 0;
}

int main(void) {
    D_800AF784 = tim_data;
    func_800ACB90();
    require(field_load_count == 1, "field_load_count");
    require(draw_sync_count == 2, "draw_sync_count");
    require(alloc_count == 1, "alloc_count");
    require(load_image_count == 1, "load_image_count");
    require(free_count == 2, "free_count");
    if (failures != 0) {
        fprintf(stderr, "FIELD ACB90 FAIL failures=%d\n", failures);
        return 1;
    }
    printf("FIELD ACB90 PASS checks=%d\n", checks);
    return 0;
}
