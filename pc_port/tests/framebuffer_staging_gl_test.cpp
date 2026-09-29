/* Visible OpenGL regression for the production framebuffer-copy helpers.
 * This is a hardware-adapter fixture, not a game scene or retail acceptance. */
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

#define USE_OPENGL 1
#define USE_FRAMEBUFFER_BLIT 1
#define RENDERER_OGL 1
#define VRAM_WIDTH 1024
#define VRAM_HEIGHT 512
#define VRAM_FORMAT GL_RG
#define VRAM_INTERNAL_FORMAT GL_RG32F
using u_int = unsigned int;
using uint = unsigned int;
using ushort = unsigned short;
using u_char = unsigned char;
struct RECT16 { short x, y, w, h; };
struct TestDrawEnv { RECT16 clip; };
static TestDrawEnv activeDrawEnv = {{0, 0, 320, 224}};

static unsigned short vram[VRAM_WIDTH * VRAM_HEIGHT];
static GLuint g_fbTexture, g_glBlitFramebuffer, g_glVRAMFramebuffer;
static GLuint g_vramTexture, g_vramTexturesDouble[2];
static int g_vramTextureIdx, vram_need_update;
static int g_xeno_vram_fb_synced, g_lastBoundTexture;
static int g_windowWidth = 640, g_windowHeight = 448;
static RECT16 g_PreviousFramebuffer;
static void GR_XenoReadBackbufferToVRAM(int, int, int, int);

/* Extracted unchanged from the currently built renderer by the runner. */
#include "framebuffer_staging_body.inc"

static int failures;
static void check_gl(const char* phase)
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::fprintf(stderr, "FRAMEBUFFER STAGING FAIL %s GL=%04x\n", phase, error);
        ++failures;
    }
}

static void check_capture(const char* label, int y, int w, int h,
                          unsigned char r, unsigned char g, unsigned char b)
{
    /* Model rendering is represented only by an explicit GL clear color.
     * The framebuffer ownership/readback functions themselves are production. */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(r / 255.f, g / 255.f, b / 255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    GR_StoreFrameBuffer(0, y, w, h);
    check_gl(label);
    const unsigned short expected = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10);
    size_t mismatches = 0;
    for (int row = 0; row < h; ++row)
        for (int col = 0; col < w; ++col)
            if ((vram[(y + row) * VRAM_WIDTH + col] & 0x7fff) != expected)
                ++mismatches;
    if (mismatches) {
        std::fprintf(stderr, "FRAMEBUFFER STAGING FAIL %s y=%d size=%dx%d "
                     "expected=%04x actual=%04x mismatches=%zu\n", label, y, w, h,
                     expected, vram[y * VRAM_WIDTH] & 0x7fff, mismatches);
        ++failures;
    } else {
        std::printf("FRAMEBUFFER STAGING PASS %s y=%d pixels=%d\n", label, y, w*h);
    }
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL init: %s\n", SDL_GetError());
        return 2;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("Xenogears framebuffer adapter test (not game)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_windowWidth, g_windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext context = window ? SDL_GL_CreateContext(window) : nullptr;
    if (!context) {
        std::fprintf(stderr, "SDL GL context: %s\n", SDL_GetError());
        SDL_Quit();
        return 2;
    }
    std::printf("GL renderer: %s\n", glGetString(GL_RENDERER));
    glGenTextures(1, &g_fbTexture);
    glBindTexture(GL_TEXTURE_2D, g_fbTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VRAM_WIDTH, VRAM_HEIGHT,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &g_glBlitFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, g_glBlitFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_fbTexture, 0);
    glGenTextures(2, g_vramTexturesDouble);
    for (GLuint texture : g_vramTexturesDouble) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, VRAM_INTERNAL_FORMAT, VRAM_WIDTH, VRAM_HEIGHT,
                     0, VRAM_FORMAT, GL_UNSIGNED_BYTE, vram);
    }
    g_vramTexture = g_vramTexturesDouble[0];
    glGenFramebuffers(1, &g_glVRAMFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, g_glVRAMFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_vramTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    check_gl("setup");

    {
        /* These are packed PS1 RGB555 pixels, not RGBA texture channels.
         * Test the displayed result, independently of capture freshness. */
        const unsigned short colors[] = {0x001f, 0x03e0, 0x7c00, 0x7fff, 0x8000, 0xffff};
        for (unsigned short color : colors) {
            for (int i = 0; i < VRAM_WIDTH * VRAM_HEIGHT; ++i) vram[i] = color;
            vram_need_update = 1;
            GR_MaterializeFramebufferRect(0, 0, 320, 224);
            unsigned char pixel[4] = {};
            glReadPixels(g_windowWidth/2, g_windowHeight/2, 1, 1,
                         GL_RGBA, GL_UNSIGNED_BYTE, pixel);
            check_gl("materialize-color");
            const unsigned char expected[] = {
                static_cast<unsigned char>((color & 31) * 255 / 31),
                static_cast<unsigned char>(((color >> 5) & 31) * 255 / 31),
                static_cast<unsigned char>(((color >> 10) & 31) * 255 / 31)};
            if (std::memcmp(pixel, expected, 3)) {
                std::fprintf(stderr,"FRAMEBUFFER MATERIALIZE FAIL color=%04x "
                    "actual=%u,%u,%u expected=%u,%u,%u\n", color,
                    pixel[0],pixel[1],pixel[2],expected[0],expected[1],expected[2]);
                ++failures;
            }
        }
        for (int page : {0, 256}) {
            activeDrawEnv.clip.y = page;
            for (int row = 0; row < 224; ++row)
                for (int col = 0; col < 320; ++col)
                    vram[(page + row) * VRAM_WIDTH + col] =
                        (col % 32) | ((row % 32) << 5) | (((col + row) % 32) << 10);
            vram_need_update = 1;
            GR_MaterializeFramebufferRect(0, page, 320, 224);
            unsigned char displayed[640 * 448 * 4];
            glReadPixels(0, 0, 640, 448, GL_RGBA, GL_UNSIGNED_BYTE, displayed);
            check_gl("materialize-ramp");
            size_t mismatches = 0;
            for (int row = 0; row < 224; ++row)
                for (int col = 0; col < 320; ++col) {
                    const unsigned char* pixel = &displayed[((447 - row*2)*640 + col*2)*4];
                    if (pixel[0] != (col % 32)*255/31 ||
                        pixel[1] != (row % 32)*255/31 ||
                        pixel[2] != ((col + row) % 32)*255/31) ++mismatches;
                }
            if (mismatches) {
                std::fprintf(stderr,"FRAMEBUFFER MATERIALIZE FAIL ramp page=%d mismatches=%zu\n",
                             page,mismatches);
                ++failures;
            } else std::printf("FRAMEBUFFER MATERIALIZE PASS ramp page=%d pixels=71680\n",page);
        }
    }

    {
        /* GR_StoreFrameBuffer maps the full draw clip to the window.
         * A partial restore must use the inverse mapping, leaving all
         * other pixels untouched; it must not stretch to the full window. */
        for (int page : {0, 256}) {
            activeDrawEnv.clip = {0, static_cast<short>(page), 320, 224};
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_SCISSOR_TEST);
            glClearColor(0, 0, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            for (int row=page+20; row<page+60; ++row)
                for (int col=30; col<80; ++col) vram[row*VRAM_WIDTH+col]=0x03e0;
            vram_need_update=1;
            glEnable(GL_SCISSOR_TEST);
            glScissor(0,0,1,1); /* unrelated polygon state must not crop the copy */
            GR_MaterializeFramebufferRect(30,page+20,50,40);
            if(!glIsEnabled(GL_SCISSOR_TEST)) {
                std::fputs("FRAMEBUFFER MATERIALIZE FAIL scissor state lost\n",stderr);
                ++failures;
            }
            glDisable(GL_SCISSOR_TEST);
            unsigned char displayed[640*448*4];
            glReadPixels(0,0,640,448,GL_RGBA,GL_UNSIGNED_BYTE,displayed);
            check_gl("materialize-placement");
            size_t mismatches=0;
            for(int y=0;y<448;++y) for(int x=0;x<640;++x) {
                const bool copied=x>=60 && x<160 && y>=328 && y<408;
                const unsigned char* pixel=&displayed[(y*640+x)*4];
                if(pixel[0]!=0 || pixel[1]!=(copied?255:0) ||
                   pixel[2]!=(copied?0:255)) ++mismatches;
            }
            if(mismatches) {
                std::fprintf(stderr,"FRAMEBUFFER MATERIALIZE FAIL placement page=%d mismatches=%zu\n",
                             page,mismatches);
                ++failures;
            }
        }
    }

    {
        /* Intro DR_MOVE/StoreImage can pass RECT.x < 0. GPU wraps 10-bit X. */
        std::memset(vram, 0, sizeof(vram));
        u_int src[64 * 8];
        for (int i = 0; i < 64 * 8; ++i)
            src[i] = 0x0000F8u; /* R=0xF8 -> 5-bit 0x1F after the copy helper */
        GR_CopyRGBAFramebufferToVRAM(src, -32, 256, 64, 8, 1, 0);
        const unsigned short expected = 0x001f;
        const int wrap_x = (-32) & (VRAM_WIDTH - 1);
        size_t mismatches = 0;
        for (int row = 0; row < 8; ++row)
            for (int col = 0; col < 64; ++col) {
                const int vx = (wrap_x + col) & (VRAM_WIDTH - 1);
                if ((vram[(256 + row) * VRAM_WIDTH + vx] & 0x7fff) != expected)
                    ++mismatches;
            }
        /* Span 992..1023 then 0..31; x=32 on this row must stay empty. */
        if ((vram[256 * VRAM_WIDTH + 32] & 0x7fff) != 0)
            ++mismatches;
        if (mismatches) {
            std::fprintf(stderr,
                "FRAMEBUFFER COPY WRAP FAIL x=-32 wrap_x=%d mismatches=%zu\n",
                wrap_x, mismatches);
            ++failures;
        } else {
            std::printf("FRAMEBUFFER COPY WRAP PASS x=-32 dest_x=%d pixels=%d\n",
                        wrap_x, 64 * 8);
        }
    }

    check_capture("before-materialize", 0, 320, 224, 0, 248, 0);
    for (int i = 0; i < 6; ++i) {
        int y = (i & 1) ? 256 : 0;
        int w = i == 2 ? 64 : 320;
        int h = i == 2 ? 32 : 224;
        activeDrawEnv.clip.y = y;
        GR_MaterializeFramebufferRect(0, y, w, h);
        check_gl("materialize");
        check_capture("after-materialize", y, w, h,
                      (i & 1) ? 248 : 16, 64, (i & 1) ? 8 : 240);
    }
    SDL_GL_SwapWindow(window);
    SDL_PumpEvents();
    SDL_Delay(250);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return failures ? 1 : 0;
}
