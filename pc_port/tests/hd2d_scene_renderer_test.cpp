/* Real SDL/OpenGL test of the included production coverage renderer and
 * production one-shot adapter. Geometry/provenance/bridge are bounded test
 * inputs; this is not a production scene, CLUT, or fixed-UV validation. */
#define GL_GLEXT_PROTOTYPES
#include <SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "hd2d_scene_buffers.h"
#include "hd2d_world_packets.h"
extern "C" {
#include "hd2d_integration.h"
}

#define RENDERER_OGL 1
#define MAX_VERTEX_BUFFER_SIZE 256
static SDL_Window *g_window;
static int g_dbg_wireframeMode;
static unsigned renderer_gl_calls, replay_calls, classify_calls, checks;
static int enabled, opening = 1, map_id = 14, world_valid = 1;
static uint64_t epoch = 10, world_generation = 20;
static int width = 32, height = 24;
struct Token { unsigned coverage; };
static Token scenery = {2}, unknown = {0};

extern "C" int PcPort_Hd2dEnabled(void) { return enabled; }
extern "C" int PcPort_Hd2dOpeningActive(void) { return enabled && opening; }
extern "C" int PcPort_Hd2dCurrentMap(void) { return map_id; }
extern "C" uint64_t PcPort_Hd2dSceneEpoch(void) { return epoch; }
extern "C" uint64_t PcPort_Hd2dWorldGeneration(void) { return world_generation; }
extern "C" int PcPort_Hd2dWorldValid(void) { return world_valid; }
extern "C" void PcPort_Hd2dWorldReset(void) { ++world_generation; }
extern "C" unsigned PcPort_Hd2dWorldPacketClassify(const void *p, unsigned, int override_on)
{
    ++classify_calls;
    return p && !override_on ? static_cast<const Token *>(p)->coverage : 0;
}
static void GR_DrawTriangles(int first, int triangles);
static void note_gl() { ++renderer_gl_calls; }

/* GL_CALL_WRAPPERS_BEGIN -- every GL call in the included renderer is counted. */
#define glBindAttribLocation(...) (note_gl(), glBindAttribLocation(__VA_ARGS__))
#define glBindBuffer(...) (note_gl(), glBindBuffer(__VA_ARGS__))
#define glBindFragDataLocation(...) (note_gl(), glBindFragDataLocation(__VA_ARGS__))
#define glBindFramebuffer(...) (note_gl(), glBindFramebuffer(__VA_ARGS__))
#define glBindRenderbuffer(...) (note_gl(), glBindRenderbuffer(__VA_ARGS__))
#define glBindTexture(...) (note_gl(), glBindTexture(__VA_ARGS__))
#define glBlitFramebuffer(...) (note_gl(), glBlitFramebuffer(__VA_ARGS__))
#define glBufferData(...) (note_gl(), glBufferData(__VA_ARGS__))
#define glBufferSubData(...) (note_gl(), glBufferSubData(__VA_ARGS__))
#define glCheckFramebufferStatus(...) (note_gl(), glCheckFramebufferStatus(__VA_ARGS__))
#define glClearBufferfv(...) (note_gl(), glClearBufferfv(__VA_ARGS__))
#define glColorMaski(...) (note_gl(), glColorMaski(__VA_ARGS__))
#define glDeleteBuffers(...) (note_gl(), glDeleteBuffers(__VA_ARGS__))
#define glDeleteFramebuffers(...) (note_gl(), glDeleteFramebuffers(__VA_ARGS__))
#define glDeleteRenderbuffers(...) (note_gl(), glDeleteRenderbuffers(__VA_ARGS__))
#define glDeleteTextures(...) (note_gl(), glDeleteTextures(__VA_ARGS__))
#define glDisable(...) (note_gl(), glDisable(__VA_ARGS__))
#define glDisablei(...) (note_gl(), glDisablei(__VA_ARGS__))
#define glDrawBuffers(...) (note_gl(), glDrawBuffers(__VA_ARGS__))
#define glEnable(...) (note_gl(), glEnable(__VA_ARGS__))
#define glEnableVertexAttribArray(...) (note_gl(), glEnableVertexAttribArray(__VA_ARGS__))
#define glEnablei(...) (note_gl(), glEnablei(__VA_ARGS__))
#define glFramebufferRenderbuffer(...) (note_gl(), glFramebufferRenderbuffer(__VA_ARGS__))
#define glFramebufferTexture2D(...) (note_gl(), glFramebufferTexture2D(__VA_ARGS__))
#define glGenBuffers(...) (note_gl(), glGenBuffers(__VA_ARGS__))
#define glGenFramebuffers(...) (note_gl(), glGenFramebuffers(__VA_ARGS__))
#define glGenRenderbuffers(...) (note_gl(), glGenRenderbuffers(__VA_ARGS__))
#define glGenTextures(...) (note_gl(), glGenTextures(__VA_ARGS__))
#define glGetBooleani_v(...) (note_gl(), glGetBooleani_v(__VA_ARGS__))
#define glGetBufferParameteriv(...) (note_gl(), glGetBufferParameteriv(__VA_ARGS__))
#define glGetFramebufferAttachmentParameteriv(...) (note_gl(), glGetFramebufferAttachmentParameteriv(__VA_ARGS__))
#define glGetIntegerv(...) (note_gl(), glGetIntegerv(__VA_ARGS__))
#define glGetProgramiv(...) (note_gl(), glGetProgramiv(__VA_ARGS__))
#define glIsEnabled(...) (note_gl(), glIsEnabled(__VA_ARGS__))
#define glIsEnabledi(...) (note_gl(), glIsEnabledi(__VA_ARGS__))
#define glReadBuffer(...) (note_gl(), glReadBuffer(__VA_ARGS__))
#define glRenderbufferStorage(...) (note_gl(), glRenderbufferStorage(__VA_ARGS__))
#define glScissor(...) (note_gl(), glScissor(__VA_ARGS__))
#define glTexImage2D(...) (note_gl(), glTexImage2D(__VA_ARGS__))
#define glTexParameteri(...) (note_gl(), glTexParameteri(__VA_ARGS__))
#define glVertexAttribIPointer(...) (note_gl(), glVertexAttribIPointer(__VA_ARGS__))
#define SDL_GL_GetDrawableSize(...) (note_gl(), SDL_GL_GetDrawableSize(__VA_ARGS__))
/* GL_CALL_WRAPPERS_END */
#include "../src/psycross_hd2d_scene.inl"
/* GL_CALL_UNWRAPPERS_BEGIN */
#undef glBindAttribLocation
#undef glBindBuffer
#undef glBindFragDataLocation
#undef glBindFramebuffer
#undef glBindRenderbuffer
#undef glBindTexture
#undef glBlitFramebuffer
#undef glBufferData
#undef glBufferSubData
#undef glCheckFramebufferStatus
#undef glClearBufferfv
#undef glColorMaski
#undef glDeleteBuffers
#undef glDeleteFramebuffers
#undef glDeleteRenderbuffers
#undef glDeleteTextures
#undef glDisable
#undef glDisablei
#undef glDrawBuffers
#undef glEnable
#undef glEnableVertexAttribArray
#undef glEnablei
#undef glFramebufferRenderbuffer
#undef glFramebufferTexture2D
#undef glGenBuffers
#undef glGenFramebuffers
#undef glGenRenderbuffers
#undef glGenTextures
#undef glGetBooleani_v
#undef glGetBufferParameteriv
#undef glGetFramebufferAttachmentParameteriv
#undef glGetIntegerv
#undef glGetProgramiv
#undef glIsEnabled
#undef glIsEnabledi
#undef glReadBuffer
#undef glRenderbufferStorage
#undef glScissor
#undef glTexImage2D
#undef glTexParameteri
#undef glVertexAttribIPointer
#undef SDL_GL_GetDrawableSize
/* GL_CALL_UNWRAPPERS_END */

static void check(const char *name, bool condition)
{
    ++checks;
    if (!condition) {
        std::fprintf(stderr, "ASSERTION %s (GL error=%x, SDL=%s)\n",
                     name, glGetError(), SDL_GetError());
        std::exit(1);
    }
}
static void GR_DrawTriangles(int first, int triangles)
{
    ++replay_calls;
    glDrawArrays(GL_TRIANGLES, first, triangles * 3);
}
static void clean_gl() { check("no-GL-error", glGetError() == GL_NO_ERROR); }
static void default_framebuffer_witness()
{
    GLint draw = -1, depth = -1, stencil = -1, samples = -1;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    check("default-framebuffer-query-target", draw == 0);
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH,
        GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depth);
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_STENCIL,
        GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencil);
    glGetIntegerv(GL_SAMPLES, &samples);
    const GLenum error = glGetError();
    std::printf("scene-renderer: default draw=%d depth=%d stencil=%d samples=%d "
                "query_error=%x\n", draw, depth, stencil, samples, error);
    std::fflush(stdout);
    check("default-depth24-stencil8-single-sample-query",
          error == GL_NO_ERROR && depth == 24 && stencil == 8 && samples == 0);
}

static GLuint program, vao, vertex_buffer;
static GLint color_uniform, discard_uniform;
struct Vertex { float x, y, z; };
static std::vector<Vertex> vertices;
static std::vector<unsigned char> read_pixels(bool coverage);

static GLuint shader(GLenum type, const char **sources, int count)
{
    GLuint object = glCreateShader(type);
    glShaderSource(object, count, sources, NULL);
    glCompileShader(object);
    GLint valid = 0;
    glGetShaderiv(object, GL_COMPILE_STATUS, &valid);
    if (!valid) {
        char log[4096];
        glGetShaderInfoLog(object, sizeof log, NULL, log);
        std::fprintf(stderr, "%s\n", log);
    }
    check("injected-shader-compiles", valid != 0);
    return object;
}
static void make_program()
{
    char vd[1024] = "in vec3 a_position;\n";
    char fd[1024] = "out vec4 fragColor;\nuniform vec4 u_color;\nuniform vec4 u_discard;\n";
    const char *vs[] = {"#version 330 core\n", vd, "",
        "void main() { gl_Position=vec4(a_position,1.0); }\n"};
    const char *fs[] = {"#version 330 core\n", fd, "", "",
        "void main() { if (gl_FragCoord.x>=u_discard.x && gl_FragCoord.x<u_discard.z && "
        "gl_FragCoord.y>=u_discard.y && gl_FragCoord.y<u_discard.w) discard; "
        "fragColor=u_color; }\n"};
    check("actual-shader-injection", XenoSceneShaderSources(vs, fs, vd, sizeof vd, fd, sizeof fd));
    GLuint v = shader(GL_VERTEX_SHADER, vs, 4);
    GLuint f = shader(GL_FRAGMENT_SHADER, fs, 5);
    program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    glBindAttribLocation(program, 0, "a_position");
    XenoSceneBindProgram(program);
    glLinkProgram(program);
    XenoSceneProgramLinked(program);
    check("actual-program-registration", !g_xenoSceneShaderFailed && XenoSceneKnownProgram(program));
    glDeleteShader(v);
    glDeleteShader(f);
    glUseProgram(program);
    color_uniform = glGetUniformLocation(program, "u_color");
    discard_uniform = glGetUniformLocation(program, "u_discard");
    check("fixture-uniforms", color_uniform >= 0 && discard_uniform >= 0);
    glUniform4f(color_uniform, .25f, .5f, .75f, 1.f);
    glUniform4f(discard_uniform, 0, 0, 0, 0);
}
static int quad(int x0, int y0, int x1, int y1, float z, Token *token,
                bool overridden = false)
{
    const int first = static_cast<int>(vertices.size());
    const float l = 2.f * x0 / width - 1.f, r = 2.f * x1 / width - 1.f;
    const float b = 2.f * y0 / height - 1.f, t = 2.f * y1 / height - 1.f;
    vertices.insert(vertices.end(), {{l,b,z},{r,b,z},{r,t,z},{l,b,z},{r,t,z},{l,t,z}});
    if (overridden) GR_Hd2dScenePrimOverridden();
    GR_Hd2dSceneTagPrim(first, static_cast<int>(vertices.size()), 0x2c, token);
    return first;
}
static void upload()
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL);
    glEnableVertexAttribArray(0);
    GR_Hd2dSceneUpload(static_cast<int>(vertices.size()));
}
static void draw(int first, int overridden = 0)
{
    glDrawArrays(GL_TRIANGLES, first, 6); /* Ordinary retail colour/depth/stencil. */
    const auto retail = read_pixels(false);
    GR_Hd2dSceneDraw(first, 2, overridden); /* Actual auxiliary replay. */
    check("auxiliary-replay-never-changes-retail-colour", retail == read_pixels(false));
}
static void neutral()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
    glViewport(0, 0, width, height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0xff);
    glClearDepth(1);
    glClearStencil(0);
    glClearColor(.125f, .25f, .375f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glUseProgram(program);
    glUniform4f(discard_uniform, 0, 0, 0, 0);
    glUniform4f(color_uniform, .25f, .5f, .75f, 1.f);
    vertices.clear();
}
static std::vector<unsigned char> read_pixels(bool coverage)
{
    GLint old_read;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &old_read);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, coverage ? g_xenoSceneFbo : 0);
    glReadBuffer(coverage ? GL_COLOR_ATTACHMENT0 : GL_BACK);
    std::vector<unsigned char> data(static_cast<size_t>(width * height) * (coverage ? 1u : 4u));
    glReadPixels(0, 0, width, height, coverage ? GL_RED : GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(old_read));
    return data;
}
static void uniform_coverage(unsigned char expected)
{
    const auto data = read_pixels(true);
    check("uniform-full-drawable-coverage",
          std::all_of(data.begin(), data.end(), [=](unsigned char v) { return v == expected; }));
}
static std::vector<GLint> state()
{
    std::vector<GLint> out;
    const GLenum scalar[] = {GL_DRAW_FRAMEBUFFER_BINDING, GL_READ_FRAMEBUFFER_BINDING,
        GL_CURRENT_PROGRAM, GL_VERTEX_ARRAY_BINDING, GL_ARRAY_BUFFER_BINDING,
        GL_PIXEL_UNPACK_BUFFER_BINDING, GL_RENDERBUFFER_BINDING, GL_ACTIVE_TEXTURE,
        GL_TEXTURE_BINDING_2D, GL_DEPTH_FUNC, GL_STENCIL_FUNC, GL_STENCIL_REF,
        GL_STENCIL_VALUE_MASK, GL_STENCIL_WRITEMASK, GL_STENCIL_FAIL,
        GL_STENCIL_PASS_DEPTH_FAIL, GL_STENCIL_PASS_DEPTH_PASS};
    for (GLenum name : scalar) { GLint v; glGetIntegerv(name, &v); out.push_back(v); }
    for (GLenum name : {GL_VIEWPORT, GL_SCISSOR_BOX}) {
        GLint v[4]; glGetIntegerv(name, v); out.insert(out.end(), v, v + 4);
    }
    for (GLuint i = 0; i < 2; ++i) {
        GLboolean m[4]; glGetBooleani_v(GL_COLOR_WRITEMASK, i, m);
        for (GLboolean v : m) out.push_back(v);
        out.push_back(glIsEnabledi(GL_BLEND, i));
    }
    for (GLenum name : {GL_SCISSOR_TEST, GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE,
                        GL_DITHER, GL_COLOR_LOGIC_OP, GL_FRAMEBUFFER_SRGB})
        out.push_back(glIsEnabled(name));
    GLboolean write; glGetBooleanv(GL_DEPTH_WRITEMASK, &write); out.push_back(write);
    /* Attribute5 belongs to the producer. Verify the retail position attribute. */
    for (GLenum name : {GL_VERTEX_ATTRIB_ARRAY_ENABLED, GL_VERTEX_ATTRIB_ARRAY_SIZE,
                        GL_VERTEX_ATTRIB_ARRAY_TYPE, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING}) {
        GLint v; glGetVertexAttribiv(0, name, &v); out.push_back(v);
    }
    return out;
}
static XgHd2dFrame frame()
{
    XgHd2dFrame f = {};
    f.field_active = 1; f.field_id = map_id; f.width = width; f.height = height;
    return f;
}
static void off_checks()
{
    const unsigned calls = renderer_gl_calls, classifications = classify_calls;
    GR_Hd2dSceneBegin();
    GR_Hd2dScenePrimOverridden();
    GR_Hd2dSceneTagPrim(-1, -1, -1, reinterpret_cast<const void *>(uintptr_t(1)));
    GR_Hd2dSceneUpload(-1);
    GR_Hd2dSceneDraw(-1, -1, 1);
    GR_Hd2dSceneClear();
    GR_Hd2dSceneZeroRect(0, 0, 4, 4);
    GR_Hd2dScenePresent(0, 0);
    check("OFF-performs-zero-GL-calls", calls == renderer_gl_calls);
    check("OFF-does-not-classify", classifications == classify_calls);
}
static void allocation_and_raster()
{
    neutral();
    const auto retail_before = read_pixels(false);
    GLuint texture, unpack, rb, read_fbo;
    glGenTextures(1, &texture); glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, texture);
    glGenBuffers(1, &unpack); glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpack);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, 4, NULL, GL_STATIC_DRAW);
    glGenRenderbuffers(1, &rb); glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glGenFramebuffers(1, &read_fbo); glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);
    glEnable(GL_SCISSOR_TEST); glScissor(3, 4, 5, 6);
    glColorMaski(0, GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
    glColorMaski(1, GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    glEnablei(GL_BLEND, 1); glEnable(GL_DITHER); glEnable(GL_FRAMEBUFFER_SRGB);
    const auto before = state();
    GR_Hd2dSceneBegin();
    if (!g_xenoSceneValid || !g_xenoSceneFbo || !g_xenoSceneCoverage ||
        !g_xenoSceneDs || !g_xenoSceneVbo)
        std::fprintf(stderr,
            "scene-renderer: allocation valid=%d size=%dx%d fbo=%u coverage=%u "
            "ds=%u vbo=%u shader_failed=%d programs=%u tags=%p\n",
            g_xenoSceneValid, g_xenoSceneW, g_xenoSceneH, g_xenoSceneFbo,
            g_xenoSceneCoverage, g_xenoSceneDs, g_xenoSceneVbo,
            g_xenoSceneShaderFailed, g_xenoSceneProgramCount,
            static_cast<void *>(g_xenoSceneTags));
    check("initial-target-allocation-valid", g_xenoSceneValid && g_xenoSceneFbo &&
          g_xenoSceneCoverage && g_xenoSceneDs && g_xenoSceneVbo);
    check("allocation-and-clear-preserve-state", before == state());
    check("begin-preserves-retained-retail-colour", retail_before == read_pixels(false));
    uniform_coverage(0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glDeleteTextures(1, &texture); glDeleteBuffers(1, &unpack);
    glDeleteRenderbuffers(1, &rb); glDeleteFramebuffers(1, &read_fbo);
    glDisable(GL_SCISSOR_TEST); glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    const int world = quad(0, 0, width, height, 0, &scenery);
    const int protected_quad = quad(4, 4, 12, 12, 0, &unknown);
    const int primitive_override = quad(12, 4, 20, 12, 0, &scenery, true);
    const int discarded = quad(20, 4, 28, 12, 0, &unknown);
    const int split_override = quad(4, 14, 12, 22, 0, &scenery);
    upload();
    const auto before_draw = state();
    draw(world);
    check("auxiliary-draw-preserves-retail-state", before_draw == state());
    uniform_coverage(2);
    /* A blended unknown still overwrites coverage with protection. */
    glEnablei(GL_BLEND, 0); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUniform4f(color_uniform, 1, 0, 0, .5f);
    draw(protected_quad);
    glDisablei(GL_BLEND, 0);
    draw(primitive_override);
    glUniform4f(discard_uniform, 20, 4, 24, 12);
    draw(discarded);
    glUniform4f(discard_uniform, 0, 0, 0, 0);
    draw(split_override, 1);
    const auto coverage = read_pixels(true);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        const bool protected_pixel =
            (y >= 4 && y < 12 && x >= 4 && x < 20) ||
            (y >= 4 && y < 12 && x >= 24 && x < 28) ||
            (y >= 14 && y < 22 && x >= 4 && x < 12);
        check("exact-raster-world-protection-discard",
              coverage[static_cast<size_t>(y * width + x)] == (protected_pixel ? 4 : 2));
    }
    const auto retail = read_pixels(false);
    glEnable(GL_SCISSOR_TEST); glScissor(1, 2, 3, 4);
    const auto before_zero = state();
    GR_Hd2dSceneZeroRect(18, 11, 10, 5);
    check("VRAM-zero-preserves-state", before_zero == state());
    check("VRAM-zero-preserves-retail-colour", retail == read_pixels(false));
    const auto zeroed = read_pixels(true);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        const size_t at = static_cast<size_t>(y * width + x);
        check("VRAM-rect-uncertified0",
              zeroed[at] == (x >= 10 && x < 18 && y >= 5 && y < 11 ? 0 : coverage[at]));
    }
    glDisable(GL_SCISSOR_TEST);
    GR_Hd2dScenePresent(0, 0);
    XgHd2dFrame f = frame();
    check("real-adapter-accepts-coverage-only",
          PcPort_Hd2dSceneBuffersConsume(0, width, height, &f) == 1 &&
          f.protection_verified && f.protection_texture == g_xenoSceneCoverage &&
          !f.linear_depth_verified && !f.linear_depth_texture);
    f = frame();
    check("real-adapter-consumes-once", !PcPort_Hd2dSceneBuffersConsume(0, width, height, &f));
    GR_Hd2dScenePresent(0, 0);
    f = frame();
    check("renderer-present-consumes-scene-once",
          !PcPort_Hd2dSceneBuffersConsume(0, width, height, &f));
    clean_gl();
}

static void depth_stencil_sequence()
{
    neutral();
    glEnable(GL_SCISSOR_TEST); glScissor(0, 0, width / 2, height);
    glClearDepth(.2); glClearStencil(3);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    GR_Hd2dSceneBegin();
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glDepthMask(GL_TRUE);
    glEnable(GL_STENCIL_TEST); glStencilFunc(GL_EQUAL, 0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    const int world = quad(0, 0, width, height, 0, &scenery);
    const int farther = quad(0, 0, width, height, .6f, &unknown);
    const int nearer = quad(0, 0, width, height, -.5f, &unknown);
    upload();
    draw(world); /* Left fails inherited stencil/depth; right writes stencil1. */
    auto data = read_pixels(true);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
        check("inherited-default-depth-stencil",
              data[static_cast<size_t>(y * width + x)] == (x < width / 2 ? 0 : 2));
    glStencilFunc(GL_EQUAL, 1, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    draw(farther); /* Right fails updated depth; left fails updated stencil. */
    check("auxiliary-depth-history-matches-retail", data == read_pixels(true));
    draw(nearer); /* Only right passes both current tests. */
    data = read_pixels(true);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
        check("auxiliary-stencil-history-matches-retail",
              data[static_cast<size_t>(y * width + x)] == (x < width / 2 ? 0 : 4));
    /* Match the actual retail clear hook: colour+depth under a scissor,
     * retaining stencil. The producer copies the real resulting test state. */
    glEnable(GL_SCISSOR_TEST); glScissor(width / 2, 0, width / 2, height / 2);
    glClearDepth(1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const auto before = state();
    GR_Hd2dSceneClear();
    check("scissored-clear-preserves-state", before == state());
    data = read_pixels(true);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
        check("scissored-clear-leaves-other-coverage",
              data[static_cast<size_t>(y * width + x)] ==
              (x < width / 2 || y < height / 2 ? 0 : 4));
    glDisable(GL_SCISSOR_TEST);
    draw(farther); /* Cleared lower-right depth now permits this draw. */
    data = read_pixels(true);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x)
        check("retail-clear-refreshes-auxiliary-depth",
              data[static_cast<size_t>(y * width + x)] == (x < width / 2 ? 0 : 4));
    clean_gl();
}

static void dr_move_batch_reset_sequence()
{
    neutral(); GR_Hd2dSceneBegin();
    quad(0, 0, width, height, 0, &scenery);
    quad(4, 4, 10, 10, 0, &unknown);
    upload(); draw(0); draw(6);
    const int first_before_parse = static_cast<int>(vertices.size());
    const auto prior_coverage = read_pixels(true);
    const auto retail = read_pixels(false);
    const auto before_reset = state();
    /* ParsePrimitive(0x01) calls DrawAllSplits, whose ClearSplits resets the
     * CPU batch after its real draws. Exercise that actual renderer boundary,
     * plus the real conservative VRAM-rectangle hook; no MoveImage emulation. */
    GR_Hd2dSceneBatchReset();
    vertices.clear();
    GR_Hd2dSceneZeroRect(12, 6, 20, 14);
    const auto zeroed = read_pixels(true);
    const unsigned classifications = classify_calls, calls = renderer_gl_calls;
    GR_Hd2dSceneTagPrim(first_before_parse, 0, 0x01, &unknown);
    check("DR_MOVE-reset-keeps-prior-coverage-valid", g_xenoSceneValid);
    check("DR_MOVE-still-classifies-exactly-once", classify_calls == classifications + 1);
    check("DR_MOVE-tag-does-not-touch-GL", renderer_gl_calls == calls);
    check("DR_MOVE-reset-keeps-batch-empty", g_xenoSceneUploaded == 0 &&
          std::all_of(g_xenoSceneTags, g_xenoSceneTags + MAX_VERTEX_BUFFER_SIZE,
                      [](unsigned char value) { return value == 0; }));
    check("DR_MOVE-tag-keeps-zeroed-coverage", zeroed == read_pixels(true));
    check("DR_MOVE-coverage-hooks-preserve-retail-colour", retail == read_pixels(false));
    check("DR_MOVE-coverage-hooks-preserve-state", before_reset == state());
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        const size_t at = static_cast<size_t>(y * width + x);
        check("DR_MOVE-conservative-rect-only",
              zeroed[at] == (x >= 12 && x < 20 && y >= 6 && y < 14 ? 0 : prior_coverage[at]));
    }
    /* A later batch starts at vertex0 but must retain previous world/UI/VRAM
     * certificates outside its actual fragments. */
    quad(2, 14, 8, 20, 0, &unknown);
    upload(); draw(0);
    const auto later = read_pixels(true);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        const size_t at = static_cast<size_t>(y * width + x);
        check("DR_MOVE-following-batch-raster-protection",
              later[at] == (x >= 2 && x < 8 && y >= 14 && y < 20 ? 4 : zeroed[at]));
    }
    GR_Hd2dScenePresent(0, 0);
    XgHd2dFrame f = frame();
    check("DR_MOVE-scene-offers-coverage-after-later-batch",
          PcPort_Hd2dSceneBuffersConsume(0, width, height, &f) == 1 &&
          f.protection_verified && f.protection_texture == g_xenoSceneCoverage &&
          !f.linear_depth_verified && !f.linear_depth_texture);
    f = frame();
    check("DR_MOVE-coverage-offer-still-one-shot",
          !PcPort_Hd2dSceneBuffersConsume(0, width, height, &f));

    struct Rejected { int first, end, code; bool reset; };
    const Rejected invalid[] = {
        {-1, 0, 0x01, true}, {MAX_VERTEX_BUFFER_SIZE + 1, 0, 0x01, true},
        {6, -1, 0x01, true}, {6, MAX_VERTEX_BUFFER_SIZE + 1, 0x01, true},
        {6, 3, 0x01, true}, {6, 0, 0x2c, true}, {6, 0, 0xff, true},
        {6, 0, 0x00, true}, {6, 0, 0x11, true},
        {6, 0, 0x01, false}, /* No observed upload reset: no exception. */
    };
    for (const auto &bad : invalid) {
        neutral(); GR_Hd2dSceneBegin();
        quad(0, 0, width, height, 0, &scenery);
        upload(); draw(0);
        uniform_coverage(2);
        if (bad.reset) GR_Hd2dSceneBatchReset();
        const auto before = read_pixels(false);
        const unsigned classified = classify_calls;
        GR_Hd2dSceneTagPrim(bad.first, bad.end, bad.code, &unknown);
        check("DR_MOVE-exception-does-not-admit-malformed-or-geometry-ranges", !g_xenoSceneValid);
        check("rejected-range-still-classifies", classify_calls == classified + 1);
        check("rejected-range-does-not-modify-retail-colour", before == read_pixels(false));
        GR_Hd2dScenePresent(0, 0);
        f = frame();
        check("rejected-range-never-offers-prior-positive",
              !PcPort_Hd2dSceneBuffersConsume(0, width, height, &f));
    }
    neutral(); GR_Hd2dSceneBegin();
    quad(0, 0, width, height, 0, &scenery);
    upload(); draw(0); GR_Hd2dSceneBatchReset();
    ++world_generation;
    GR_Hd2dSceneTagPrim(6, 0, 0x01, &unknown);
    check("DR_MOVE-exception-cannot-revive-stale-generation", !g_xenoSceneValid);
    GR_Hd2dScenePresent(0, 0); f = frame();
    check("stale-DR_MOVE-never-offers-prior-positive",
          !PcPort_Hd2dSceneBuffersConsume(0, width, height, &f));

    neutral(); GR_Hd2dSceneBegin();
    GR_Hd2dSceneTagPrim(0, 0, 0x01, &unknown);
    check("empty-DR_MOVE-stays-valid", g_xenoSceneValid);
    uniform_coverage(0);
    clean_gl();
}

static void lifecycle_checks()
{
    for (int rejection = 0; rejection < 8; ++rejection) {
        neutral();
        GR_Hd2dSceneBegin();
        quad(0, 0, width, height, 0, &scenery);
        upload();
        draw(0);
        uniform_coverage(2);
        if (rejection == 0) ++epoch;
        if (rejection == 1) ++world_generation;
        if (rejection == 2) world_valid = 0;
        if (rejection == 3) opening = 0;
        if (rejection == 4) {
            glEnable(GL_COLOR_LOGIC_OP);
            GR_Hd2dSceneDraw(0, 2, 0);
            check("logic-operation-fails-closed", !g_xenoSceneValid);
            glDisable(GL_COLOR_LOGIC_OP);
        }
        if (rejection == 5) GR_Hd2dSceneUpload(MAX_VERTEX_BUFFER_SIZE + 1);
        GR_Hd2dScenePresent(rejection == 6 ? 1 : 0, rejection == 7);
        XgHd2dFrame f = frame();
        check("invalid-scene-never-offers-stale-positive",
              !PcPort_Hd2dSceneBuffersConsume(0, width, height, &f));
        world_valid = 1; opening = 1;
    }
    neutral(); GR_Hd2dSceneBegin(); GR_Hd2dScenePresent(0, 0);
    ++epoch;
    XgHd2dFrame f = frame();
    check("offer-expires-on-post-offer-epoch-change",
          !PcPort_Hd2dSceneBuffersConsume(0, width, height, &f));

    neutral(); GR_Hd2dSceneBegin();
    const GLuint old_texture = g_xenoSceneCoverage;
    const GLuint old_fbo = g_xenoSceneFbo, old_ds = g_xenoSceneDs;
    /* An SDL resize is a real drawable-size change; no synthetic depth. */
    SDL_SetWindowSize(g_window, 40, 28);
    SDL_PumpEvents();
    SDL_GL_GetDrawableSize(g_window, &width, &height);
    check("SDL-drawable-resized", width == 40 && height == 28);
    neutral(); GR_Hd2dSceneBegin();
    check("resize-recreates-matching-targets",
          g_xenoSceneValid && g_xenoSceneW == width && g_xenoSceneH == height);
    uniform_coverage(0);
    /* Drivers may recycle object names immediately; inspect new dimensions. */
    GLint tex_width = 0, tex_height = 0, binding = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    glBindTexture(GL_TEXTURE_2D, g_xenoSceneCoverage);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tex_width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &tex_height);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(binding));
    check("resized-texture-real-dimensions", tex_width == width && tex_height == height);
    if (old_texture != g_xenoSceneCoverage) check("old-texture-deleted", !glIsTexture(old_texture));
    if (old_fbo != g_xenoSceneFbo) check("old-fbo-deleted", !glIsFramebuffer(old_fbo));
    if (old_ds != g_xenoSceneDs) check("old-depth-stencil-deleted", !glIsRenderbuffer(old_ds));

    enabled = 0;
    const GLuint texture = g_xenoSceneCoverage, fbo = g_xenoSceneFbo;
    const GLuint ds = g_xenoSceneDs, vbo = g_xenoSceneVbo;
    off_checks();
    check("OFF-keeps-existing-targets-without-touching-GL",
          texture == g_xenoSceneCoverage && fbo == g_xenoSceneFbo);
    const uint64_t prior_generation = world_generation;
    GR_Hd2dSceneShutdown(); /* Teardown explicitly owns GL cleanup even OFF. */
    check("shutdown-deletes-owned-resources",
          !glIsTexture(texture) && !glIsFramebuffer(fbo) &&
          !glIsRenderbuffer(ds) && !glIsBuffer(vbo));
    check("shutdown-resets-CPU-generation-and-pointers",
          world_generation != prior_generation && !g_xenoSceneTags &&
          !g_xenoSceneFbo && !g_xenoSceneCoverage && !g_xenoSceneDs &&
          !g_xenoSceneVbo && !g_xenoSceneProgramCount);
    off_checks();
    clean_gl();
}

int main()
{
    off_checks(); /* No SDL or GL context exists yet. */
    check("OFF-never-allocates-targets", !g_xenoSceneTags && !g_xenoSceneFbo);
    check("SDL-init", SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    g_window = SDL_CreateWindow("coverage renderer fixture", 0, 0, width, height,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    check("SDL-GL-window", g_window != NULL);
    SDL_GLContext context = SDL_GL_CreateContext(g_window);
    check("SDL-GL-context", context != NULL);
    std::printf("scene-renderer: video=%s vendor=%s renderer=%s GL=%s\n",
                SDL_GetCurrentVideoDriver(), glGetString(GL_VENDOR),
                glGetString(GL_RENDERER), glGetString(GL_VERSION));
    std::fflush(stdout);
    default_framebuffer_witness();
    make_program();
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vertex_buffer); glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_BUFFER_SIZE * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL);
    glEnableVertexAttribArray(0);
    enabled = 1;
    allocation_and_raster();
    depth_stencil_sequence();
    dr_move_batch_reset_sequence();
    lifecycle_checks();
    glDeleteBuffers(1, &vertex_buffer); glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    SDL_GL_DeleteContext(context);
    /* A new context must rebuild every resource and program registration. */
    context = SDL_GL_CreateContext(g_window);
    check("SDL-recreated-context", context != NULL);
    default_framebuffer_witness();
    make_program();
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vertex_buffer); glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    enabled = 1;
    ++epoch;
    neutral(); GR_Hd2dSceneBegin();
    quad(0, 0, width, height, 0, &scenery);
    upload(); draw(0); uniform_coverage(2);
    check("context-recreation-valid", g_xenoSceneValid && g_xenoSceneProgramCount == 1);
    GR_Hd2dSceneShutdown();
    glDeleteBuffers(1, &vertex_buffer); glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    clean_gl();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    std::printf("HD2D scene renderer PASS checks=%u replay_draws=%u; "
                "real GL raster/state/lifecycle, synthetic scene inputs only\n", checks, replay_calls);
    return 0;
}
