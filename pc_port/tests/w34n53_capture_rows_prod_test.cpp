#include <stdint.h>
#include <stdio.h>
#include <string.h>

void PsyX_FlipCaptureRows(unsigned char* pixels, int width, int height);

static int s_failures;

static void check(const char* name, bool condition)
{
    if (!condition) {
        fprintf(stderr, "W34N53_ASSERT_FAIL:%s\n", name);
        s_failures++;
    }
}

static void fill_rows(unsigned char* pixels, int width, int height)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int channel = 0; channel < 4; channel++) {
                pixels[((y * width + x) * 4) + channel] =
                    (unsigned char)(y * 48 + x * 8 + channel);
            }
        }
    }
}

static bool row_equals(const unsigned char* lhs, const unsigned char* rhs,
                       int width)
{
    return memcmp(lhs, rhs, (size_t)width * 4) == 0;
}

static void test_even_height(void)
{
    struct GuardedPixels {
        uint32_t before[4];
        unsigned char pixels[3 * 4 * 4];
        uint32_t after[4];
    } fixture;
    unsigned char original[sizeof(fixture.pixels)];
    const size_t row_bytes = 3u * 4u;

    memset(&fixture, 0, sizeof(fixture));
    for (int i = 0; i < 4; i++) {
        fixture.before[i] = 0x13579bdfu + (uint32_t)i;
        fixture.after[i] = 0x2468ace0u + (uint32_t)i;
    }
    fill_rows(fixture.pixels, 3, 4);
    memcpy(original, fixture.pixels, sizeof(original));

    PsyX_FlipCaptureRows(fixture.pixels, 3, 4);

    check("even.outer_rows",
          row_equals(fixture.pixels, original + row_bytes * 3u, 3)
          && row_equals(fixture.pixels + row_bytes * 3u, original, 3));
    check("even.interior_rows",
          row_equals(fixture.pixels + row_bytes, original + row_bytes * 2u, 3)
          && row_equals(fixture.pixels + row_bytes * 2u,
                        original + row_bytes, 3));
    check("even.pixel_stride",
          fixture.pixels[3] == original[row_bytes * 3u + 3u]
          && fixture.pixels[row_bytes + 11u]
                 == original[row_bytes * 2u + 11u]);
    check("even.write_bounds",
          fixture.before[0] == 0x13579bdfu
          && fixture.before[3] == 0x13579be2u
          && fixture.after[0] == 0x2468ace0u
          && fixture.after[3] == 0x2468ace3u);

    PsyX_FlipCaptureRows(fixture.pixels, 3, 4);
    check("even.involution",
          memcmp(fixture.pixels, original, sizeof(original)) == 0);
}

static void test_odd_height(void)
{
    unsigned char pixels[2 * 5 * 4];
    unsigned char original[sizeof(pixels)];
    const size_t row_bytes = 2u * 4u;

    fill_rows(pixels, 2, 5);
    memcpy(original, pixels, sizeof(original));
    PsyX_FlipCaptureRows(pixels, 2, 5);

    check("odd.outer_rows",
          row_equals(pixels, original + row_bytes * 4u, 2)
          && row_equals(pixels + row_bytes * 4u, original, 2));
    check("odd.middle_unchanged",
          row_equals(pixels + row_bytes * 2u,
                     original + row_bytes * 2u, 2));
}

static void test_degenerate_inputs(void)
{
    unsigned char pixel[4] = { 1u, 2u, 3u, 4u };

    PsyX_FlipCaptureRows(NULL, 1, 1);
    PsyX_FlipCaptureRows(pixel, 0, 1);
    PsyX_FlipCaptureRows(pixel, 1, 1);
    check("degenerate.no_write",
          pixel[0] == 1u && pixel[1] == 2u
          && pixel[2] == 3u && pixel[3] == 4u);
}

int main(void)
{
    test_even_height();
    test_odd_height();
    test_degenerate_inputs();

    if (s_failures != 0) {
        fprintf(stderr, "W34N53 CAPTURE ROW CERTIFICATE FAIL count=%d\n",
                s_failures);
        return 1;
    }

    puts("W34N53 CAPTURE ROW CERTIFICATE PASS");
    return 0;
}
