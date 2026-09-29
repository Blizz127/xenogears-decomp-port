/* Production-linked certificate for retail world-map modal pause support. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "system/controller.h"
#include "world_map_frame_driver_712d0.h"
#include "world_map_pause.h"

typedef struct TestRect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} TestRect;

typedef struct InputSample {
    int popped;
    u16 c1;
    u16 c2;
    u16 c1_released;
    u16 c2_released;
    u16 c1_once;
    u16 c2_once;
} InputSample;

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

u_short g_C1ButtonState;
u_short g_C1ButtonStatePressedOnce;
u_short g_C1ButtonStateReleased;
u_short g_C2ButtonState;
u_short g_C2ButtonStatePressedOnce;
u_short g_C2ButtonStateReleased;

int D_80059488;
short g_SoundControlFlags;
static _Alignas(2) u8 s_spu_registers[24u * 0x10u + 16u];
void *g_pSoundSpuRegisters = s_spu_registers;

/* Only referenced by the deliberate native-twin mutant. */
u8 g_GfxPauseLettersCompressed[16];
static u8 s_decoded_image[32];

static char s_events[256];
static size_t s_event_len;
static int s_draw_sync_calls;
static int s_vsync_calls;
static int s_upload_calls;
static int s_free_calls;
static int s_enable_calls;
static int s_move_calls;
static int s_disp_calls;
static int s_draw_env_calls;
static void *s_decompress_source;
static int s_decompress_flags;
static void *s_upload_image;
static s32 s_upload_args[6];
static void *s_freed_pointer;
static TestRect s_move_rects[4];
static int s_move_x[4];
static int s_move_y[4];
static void *s_disp_envs[4];
static void *s_draw_envs[4];
static InputSample s_samples[16];
static size_t s_sample_count;
static size_t s_sample_index;
static int s_pop_zero_count;
static int s_controller_type_values[8];
static size_t s_controller_type_count;
static size_t s_controller_type_index;

static void write_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 read_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 read_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 read_native_u16(const u8 *base, size_t offset)
{
    u16 value;
    memcpy(&value, base + offset, sizeof(value));
    return value;
}

static void write_native_u16(u8 *base, size_t offset, u16 value)
{
    memcpy(base + offset, &value, sizeof(value));
}

static int assertion_true(const char *name, int condition)
{
    if (condition == 0) {
        fprintf(stderr, "ASSERTION %s\n", name);
        return 0;
    }
    return 1;
}

static void append_event(char event)
{
    if (s_event_len + 1u < sizeof(s_events)) {
        s_events[s_event_len++] = event;
        s_events[s_event_len] = '\0';
    }
}

static void reset_observers(void)
{
    memset(s_events, 0, sizeof(s_events));
    s_event_len = 0u;
    s_draw_sync_calls = 0;
    s_vsync_calls = 0;
    s_upload_calls = 0;
    s_free_calls = 0;
    s_enable_calls = 0;
    s_move_calls = 0;
    s_disp_calls = 0;
    s_draw_env_calls = 0;
    s_decompress_source = NULL;
    s_decompress_flags = -1;
    s_upload_image = NULL;
    memset(s_upload_args, 0xA5, sizeof(s_upload_args));
    s_freed_pointer = NULL;
    memset(s_move_rects, 0, sizeof(s_move_rects));
    memset(s_move_x, 0, sizeof(s_move_x));
    memset(s_move_y, 0, sizeof(s_move_y));
    memset(s_disp_envs, 0, sizeof(s_disp_envs));
    memset(s_draw_envs, 0, sizeof(s_draw_envs));
    memset(s_samples, 0, sizeof(s_samples));
    s_sample_count = 0u;
    s_sample_index = 0u;
    s_pop_zero_count = 0;
    memset(s_controller_type_values, 0, sizeof(s_controller_type_values));
    s_controller_type_count = 0u;
    s_controller_type_index = 0u;
}

void *LZSSHeapDecompress(void *source, int flags)
{
    s_decompress_source = source;
    s_decompress_flags = flags;
    append_event('D');
    return s_decoded_image;
}

s32 func_8002DDE4(void *image, s32 texture_mode, s32 texture_x,
                  s32 texture_y, s32 clut_mode, s32 clut_x, s32 clut_y)
{
    s_upload_calls++;
    s_upload_image = image;
    s_upload_args[0] = texture_mode;
    s_upload_args[1] = texture_x;
    s_upload_args[2] = texture_y;
    s_upload_args[3] = clut_mode;
    s_upload_args[4] = clut_x;
    s_upload_args[5] = clut_y;
    append_event('U');
    return 0;
}

unsigned int HeapFree(void *pointer)
{
    s_free_calls++;
    s_freed_pointer = pointer;
    append_event('F');
    return 0u;
}

int DrawSync(int mode)
{
    (void)mode;
    s_draw_sync_calls++;
    append_event('S');
    return 0;
}

static int test_vsync(int mode)
{
    (void)mode;
    s_vsync_calls++;
    D_80059488 += 17;
    append_event('V');
    return 0;
}

/* The port has used both spellings at different retail seams. */
int VSync(int mode) { return test_vsync(mode); }
int Vsync(int mode) { return test_vsync(mode); }

int MoveImage(void *rect_pointer, int x, int y)
{
    const TestRect *rect = (const TestRect *)rect_pointer;
    if ((size_t)s_move_calls <
            sizeof(s_move_rects) / sizeof(s_move_rects[0])) {
        s_move_rects[s_move_calls] = *rect;
        s_move_x[s_move_calls] = x;
        s_move_y[s_move_calls] = y;
    }
    s_move_calls++;
    append_event('I');
    return 0;
}

void PutDispEnv(void *env)
{
    if ((size_t)s_disp_calls < sizeof(s_disp_envs) / sizeof(s_disp_envs[0]))
        s_disp_envs[s_disp_calls] = env;
    s_disp_calls++;
    append_event('P');
    /* Prove that the draw-env index is reloaded after PutDispEnv. */
    if (s_disp_calls == 2)
        write_u32(UINT32_C(0x8009D7F0), 0u);
}

void PutDrawEnv(void *env)
{
    if ((size_t)s_draw_env_calls <
            sizeof(s_draw_envs) / sizeof(s_draw_envs[0]))
        s_draw_envs[s_draw_env_calls] = env;
    s_draw_env_calls++;
    append_event('R');
}

int ControllerPopState(void)
{
    const InputSample *sample;
    if (s_sample_index >= s_sample_count) {
        s_pop_zero_count++;
        return 0;
    }
    sample = &s_samples[s_sample_index++];
    if (sample->popped == 0) {
        s_pop_zero_count++;
#if defined(W34N20_MUTANT_WAIT_DROP_RELEASED_SOURCE)
        /* Keep the deliberately broken START waiter bounded.  This direct
         * wake-up cannot satisfy the independent released-source assertion. */
        write_u16(UINT32_C(0x8009BD10), UINT16_C(0x0800));
#endif
        return 0;
    }
    g_C1ButtonState = sample->c1;
    g_C2ButtonState = sample->c2;
    g_C1ButtonStateReleased = sample->c1_released;
    g_C2ButtonStateReleased = sample->c2_released;
    g_C1ButtonStatePressedOnce = sample->c1_once;
    g_C2ButtonStatePressedOnce = sample->c2_once;
    return sample->popped;
}

int ControllerGetType(int port)
{
    (void)port;
    if (s_controller_type_index >= s_controller_type_count)
        return 1;
    return s_controller_type_values[s_controller_type_index++];
}

void SoundEnableAllSpuChannels(void)
{
    s_enable_calls++;
    append_event('E');
    /* Make the exit display and draw environment reload observable. */
    write_u32(UINT32_C(0x8009D7F0), 1u);
}

static int test_graphics_pause_letters(void)
{
    int ok = 1;

    reset_observers();
    GraphicsDrawPauseLetters(136, 100);
    ok &= assertion_true("graphics.guest_static_source",
                         s_decompress_source ==
                             PSX_ADDR(UINT32_C(0x8004FBD8)) &&
                         s_decompress_source != g_GfxPauseLettersCompressed &&
                         s_decompress_flags == 0);
    ok &= assertion_true("graphics.retail_upload_args",
                         s_upload_calls == 1 &&
                         s_upload_image == s_decoded_image &&
                         s_upload_args[0] == 1 &&
                         s_upload_args[1] == 136 &&
                         s_upload_args[2] == 100 &&
                         s_upload_args[3] == 0 &&
                         s_upload_args[4] == 0 &&
                         s_upload_args[5] == 0);
    ok &= assertion_true("graphics.call_order_and_free",
                         strcmp(s_events, "DUSF") == 0 &&
                         s_draw_sync_calls == 1 && s_free_calls == 1 &&
                         s_freed_pointer == s_decoded_image);
    return ok;
}

static int test_sound_mute(void)
{
    u32 voice;
    int ok = 1;
    int all_voices = 1;
    int adsr_low_preserved = 1;
    int untouched_fields = 1;

    memset(s_spu_registers, 0xA5, sizeof(s_spu_registers));
    for (voice = 0u; voice < 24u; voice++) {
        size_t base = (size_t)voice * 0x10u;
        write_native_u16(s_spu_registers, base,
                         (u16)(UINT16_C(0x1000) + (u16)voice));
        write_native_u16(s_spu_registers, base + 2u,
                         (u16)(UINT16_C(0x2000) + (u16)voice));
        write_native_u16(s_spu_registers, base + 4u,
                         (u16)(UINT16_C(0x3000) + (u16)voice));
        write_native_u16(s_spu_registers, base + 6u,
                         (u16)(UINT16_C(0x4000) + (u16)voice));
        write_native_u16(s_spu_registers, base + 8u,
                         (u16)(UINT16_C(0xAB00) + (u16)(voice + 1u)));
        write_native_u16(s_spu_registers, base + 0xAu,
                         (u16)(UINT16_C(0x5000) + (u16)voice));
        write_native_u16(s_spu_registers, base + 0xCu,
                         (u16)(UINT16_C(0x6000) + (u16)voice));
        write_native_u16(s_spu_registers, base + 0xEu,
                         (u16)(UINT16_C(0x7000) + (u16)voice));
    }
    g_SoundControlFlags = (short)0x1205;
    SoundMuteAllSpuChannels();
    ok &= assertion_true("mute.control_flag",
                         (u16)g_SoundControlFlags == UINT16_C(0x1245));
    for (voice = 0u; voice < 24u; voice++) {
        size_t base = (size_t)voice * 0x10u;
        all_voices &= read_native_u16(s_spu_registers, base) == 0u;
        all_voices &= read_native_u16(s_spu_registers, base + 2u) == 0u;
        all_voices &= read_native_u16(s_spu_registers, base + 4u) == 0u;
        all_voices &= read_native_u16(s_spu_registers, base + 0xAu) ==
                      UINT16_C(0x1FDF);
        adsr_low_preserved &=
            read_native_u16(s_spu_registers, base + 8u) ==
            (u16)(UINT16_C(0x7F00) + (u16)(voice + 1u));
        untouched_fields &=
            read_native_u16(s_spu_registers, base + 6u) ==
                (u16)(UINT16_C(0x4000) + (u16)voice) &&
            read_native_u16(s_spu_registers, base + 0xCu) ==
                (u16)(UINT16_C(0x6000) + (u16)voice) &&
            read_native_u16(s_spu_registers, base + 0xEu) ==
                (u16)(UINT16_C(0x7000) + (u16)voice);
    }
    ok &= assertion_true("mute.all_24_voices", all_voices);
    ok &= assertion_true("mute.adsr1_preserves_low_byte",
                         adsr_low_preserved);
    ok &= assertion_true("mute.write_set_exact", untouched_fields &&
                         s_spu_registers[24u * 0x10u] == UINT8_C(0xA5));
    return ok;
}

static void load_start_samples(void)
{
    static const InputSample samples[] = {
        {1, 0x0001u, 0x0002u, 0x0004u, 0x0008u, 0x0010u, 0x0020u},
        {1, 0x0040u, 0x0080u, 0x0100u, 0x0200u, 0x0400u, 0x0800u},
        {0, 0u, 0u, 0u, 0u, 0u, 0u},
        {1, 0x1000u, 0x2000u, 0x0800u, 0x4000u, 0x8000u, 0x0001u},
        {0, 0u, 0u, 0u, 0u, 0u, 0u},
        /* Wrong-mask mutant needs a third round in order to terminate. */
        {1, 0x0003u, 0x0005u, 0x0100u, 0x0007u, 0x0009u, 0x000Bu},
        {0, 0u, 0u, 0u, 0u, 0u, 0u}
    };
    memcpy(s_samples, samples, sizeof(samples));
    s_sample_count = sizeof(samples) / sizeof(samples[0]);
}

static int test_start_wait(void)
{
    int ok = 1;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(s_spu_registers, 0xA5, sizeof(s_spu_registers));
    reset_observers();
    load_start_samples();
    write_u32(UINT32_C(0x8009D7F0), 0u);
    write_u16(UINT32_C(0x8009BD10), UINT16_C(0x0800));
    write_u16(UINT32_C(0x8009BD14), UINT16_C(0x1111));
    write_u16(UINT32_C(0x8009BD18), UINT16_C(0x2222));
    write_u16(UINT32_C(0x8009BD1C), UINT16_C(0x3333));
    write_u16(UINT32_C(0x8009CD4C), UINT16_C(0x4444));
    write_u16(UINT32_C(0x8009CD50), UINT16_C(0x5555));
    D_80059488 = 0x10203040;
    g_SoundControlFlags = 0;
    wm_8007634C();

    ok &= assertion_true("wait.entry_copy_to_zero",
                         s_move_calls >= 1 &&
                         s_move_rects[0].x == 0 &&
                         s_move_rects[0].y == 216 &&
                         s_move_rects[0].w == 320 &&
                         s_move_rects[0].h == 216 &&
                         s_move_x[0] == 0 && s_move_y[0] == 0);
    ok &= assertion_true("wait.start_mask_and_round_count",
                         s_pop_zero_count == 2 && s_sample_index == 5u &&
                         s_upload_calls == 2);
    ok &= assertion_true("wait.start_clear_each_round",
                         read_u16(UINT32_C(0x8009CD4C)) == 0x1000u &&
                         read_u16(UINT32_C(0x8009CD50)) == 0x2000u &&
                         read_u16(UINT32_C(0x8009BD10)) == 0x0800u &&
                         read_u16(UINT32_C(0x8009BD14)) == 0x4000u &&
                         read_u16(UINT32_C(0x8009BD18)) == 0x8000u &&
                         read_u16(UINT32_C(0x8009BD1C)) == 0x0001u);
    ok &= assertion_true("wait.sound_reenabled", s_enable_calls == 1);
    ok &= assertion_true("wait.timer_restored",
                         D_80059488 == 0x10203040 && s_vsync_calls == 4);
    ok &= assertion_true("wait.final_copy_to_backbuffer",
                         s_move_calls == 2 &&
                         s_move_rects[1].x == 0 &&
                         s_move_rects[1].y == 0 &&
                         s_move_rects[1].w == 320 &&
                         s_move_rects[1].h == 216 &&
                         s_move_x[1] == 0 && s_move_y[1] == 216);
    ok &= assertion_true("wait.final_env_reloads_and_stride",
                         s_disp_calls == 2 && s_draw_env_calls == 2 &&
                         s_disp_envs[0] == PSX_ADDR(UINT32_C(0x8009BC9C)) &&
                         s_draw_envs[0] == PSX_ADDR(UINT32_C(0x8009BC40)) &&
                         s_disp_envs[1] == PSX_ADDR(UINT32_C(0x8009BC9C)) &&
                         s_draw_envs[1] == PSX_ADDR(UINT32_C(0x8009BBC8)) &&
                         read_u32(UINT32_C(0x8009D7F0)) == 0u);
    ok &= assertion_true("wait.draw_vsync_cadence",
                         s_draw_sync_calls == 6 && s_vsync_calls == 4);
    return ok;
}

static void load_disconnect_samples(void)
{
    static const InputSample samples[] = {
        {1, 0x0011u, 0x0022u, 0x0033u, 0x0044u, 0x0055u, 0x0066u},
        {0, 0u, 0u, 0u, 0u, 0u, 0u},
        {1, 0x0111u, 0x0222u, 0x0333u, 0x0444u, 0x0555u, 0x0666u},
        {0, 0u, 0u, 0u, 0u, 0u, 0u}
    };
    memcpy(s_samples, samples, sizeof(samples));
    s_sample_count = sizeof(samples) / sizeof(samples[0]);
    s_controller_type_values[0] = 0;
    s_controller_type_values[1] = 1;
    s_controller_type_count = 2u;
}

static int test_disconnect_wait(void)
{
    int ok = 1;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(s_spu_registers, 0xA5, sizeof(s_spu_registers));
    reset_observers();
    load_disconnect_samples();
    write_u32(UINT32_C(0x8009D7F0), 1u);
    D_80059488 = 12345;
    g_SoundControlFlags = 0;
    wm_80076594();

    ok &= assertion_true("wait.disconnect_two_rounds",
                         s_controller_type_index == 2u &&
                         s_pop_zero_count == 2 &&
                         s_sample_index == 4u && s_upload_calls == 2);
    ok &= assertion_true("wait.release_source_accumulates",
                         read_u16(UINT32_C(0x8009BD10)) == 0x0333u);
    ok &= assertion_true("wait.disconnect_final_inputs",
                         read_u16(UINT32_C(0x8009CD4C)) == 0x0111u &&
                         read_u16(UINT32_C(0x8009CD50)) == 0x0222u &&
                         read_u16(UINT32_C(0x8009BD14)) == 0x0444u &&
                         read_u16(UINT32_C(0x8009BD18)) == 0x0555u &&
                         read_u16(UINT32_C(0x8009BD1C)) == 0x0666u);
    ok &= assertion_true("wait.disconnect_no_entry_copy",
                         s_move_calls == 1 && s_move_y[0] == 216);
    ok &= assertion_true("wait.disconnect_timer_restored",
                         D_80059488 == 12345);
    return ok;
}

static void set_driver_common_guard(void)
{
    write_u32(UINT32_C(0x8009C178), 0u);
    write_u32(UINT32_C(0x8009D804), 0u);
    write_u32(UINT32_C(0x8009D554), 1u);
    write_u32(UINT32_C(0x8009D80C), 0u);
}

static int test_driver_start_guard(void)
{
    int ok = 1;
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(s_spu_registers, 0xA5, sizeof(s_spu_registers));
    reset_observers();
    set_driver_common_guard();
    write_u16(UINT32_C(0x8009BD10), UINT16_C(0x0800));
    /* Both masks are present so the START-mask mutant remains bounded here;
     * the direct modal test independently distinguishes the retail mask. */
    s_samples[0] = (InputSample){1, 1u, 2u, 0x0900u, 4u, 5u, 6u};
    s_samples[1] = (InputSample){0, 0u, 0u, 0u, 0u, 0u, 0u};
    s_sample_count = 2u;
    s_controller_type_values[0] = 1;
    s_controller_type_count = 1u;
    wm_712d0_run_pause_lanes();
    ok &= assertion_true("driver.start_guard_calls_modal",
                         s_upload_calls == 1 && s_enable_calls == 1 &&
                         s_controller_type_index == 1u);
    return ok;
}

static int test_driver_disconnect_guard(void)
{
    int ok = 1;
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(s_spu_registers, 0xA5, sizeof(s_spu_registers));
    reset_observers();
    set_driver_common_guard();
    write_u16(UINT32_C(0x8009BD10), 0u);
    s_samples[0] = (InputSample){1, 1u, 2u, 3u, 4u, 5u, 6u};
    s_samples[1] = (InputSample){0, 0u, 0u, 0u, 0u, 0u, 0u};
    s_sample_count = 2u;
    /* Driver sees disconnected; modal then sees reconnected. */
    s_controller_type_values[0] = 0;
    s_controller_type_values[1] = 1;
    s_controller_type_count = 2u;
    wm_712d0_run_pause_lanes();
    ok &= assertion_true("driver.disconnect_guard_calls_modal",
                         s_upload_calls == 1 && s_enable_calls == 1 &&
                         s_controller_type_index == 2u);
    return ok;
}

int main(void)
{
    int ok = 1;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    ok &= test_graphics_pause_letters();
    ok &= test_sound_mute();
    ok &= test_start_wait();
    ok &= test_disconnect_wait();
    ok &= test_driver_start_guard();
    ok &= test_driver_disconnect_guard();
    if (ok == 0)
        return EXIT_FAILURE;
    puts("W34N20 PAUSE CERTIFICATE PASS");
    return EXIT_SUCCESS;
}
