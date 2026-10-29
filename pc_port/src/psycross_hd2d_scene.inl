/* Positive scenery coverage for the optional HD-2D pass. Included by PsyX.
 * No depth claim: output is a full-drawable R8 certificate (exact byte 2).
 * Unknown fragments overwrite with protected4; retained/VRAM pixels stay0.
 * Replay the same triangles/shader/discards/depth/stencil into a separate FBO.
 * The default framebuffer and its VRAM feedback path remain untouched.
 * Staged implementation: actual-game parity/lifecycle validation is required. */
#include "hd2d_scene_buffers.h"
#include "hd2d_world_packets.h"
extern "C" {
#include "hd2d_integration.h"
}

static unsigned char* g_xenoSceneTags;
static GLuint g_xenoSceneFbo, g_xenoSceneCoverage, g_xenoSceneDs, g_xenoSceneVbo;
static int g_xenoSceneW, g_xenoSceneH, g_xenoSceneValid, g_xenoSceneOverride;
static int g_xenoSceneUploaded, g_xenoSceneShaderFailed;
static uint64_t g_xenoSceneEpoch, g_xenoSceneGeneration;
static int g_xenoSceneMap;
static GLuint g_xenoScenePrograms[16];
static unsigned g_xenoSceneProgramCount;
static char* g_xenoSceneShaderText[2];
#define XENO_SCENE_ATTRIB 5 /* Reserved by this renderer; a_extra remains 4. */

static int XenoSceneActive()
{
    /* CPU-only early gate; OFF never reaches a GL query/allocation/draw. */
    if (!PcPort_Hd2dOpeningActive()) return 0;
    if (!g_xenoSceneValid) return 0;
    if (g_xenoSceneEpoch != PcPort_Hd2dSceneEpoch() ||
        g_xenoSceneGeneration != PcPort_Hd2dWorldGeneration() ||
        g_xenoSceneMap != PcPort_Hd2dCurrentMap() || !PcPort_Hd2dWorldValid()) {
        g_xenoSceneValid = 0;
        return 0;
    }
    return 1;
}

void GR_Hd2dSceneBatchReset()
{
    g_xenoSceneOverride = 0;
    g_xenoSceneUploaded = 0;
    if (g_xenoSceneTags) memset(g_xenoSceneTags, 0, MAX_VERTEX_BUFFER_SIZE);
}
void GR_Hd2dScenePrimOverridden() { g_xenoSceneOverride = 1; }
void GR_Hd2dSceneTagPrim(int first, int end, int code, const void* packet)
{
    const int overridden = g_xenoSceneOverride;
    g_xenoSceneOverride = 0;
    if (!PcPort_Hd2dOpeningActive()) return;
    /* Classify every Seen packet, including skipped/zero-vertex primitives. */
    const unsigned coverage = PcPort_Hd2dWorldPacketClassify(packet, (unsigned)code, overridden);
    if (!XenoSceneActive()) return;
    /* Exact DR_MOVE emits no vertices. Its ordered flush may call ClearSplits,
     * resetting the batch beneath the caller's saved first index. Preserve
     * already-rasterized coverage and the VRAM rectangle's conservative zero;
     * every other backwards range still invalidates the scene. */
    if (code == 0x01 && end == 0 && first >= 0 && first <= MAX_VERTEX_BUFFER_SIZE &&
        g_xenoSceneTags && g_xenoSceneUploaded == 0) return;
    if (first < 0 || end < first || end > MAX_VERTEX_BUFFER_SIZE || !g_xenoSceneTags) {
        g_xenoSceneValid = 0;
        return;
    }
    memset(g_xenoSceneTags + first, coverage == 2 ? 2 : 4, (size_t)(end - first));
}

static const char* XenoSceneInject(int slot, const char* source, const char* addition)
{
    const char* at = strstr(source, "void main() {");
    if (!at) return NULL;
    const size_t head = (size_t)(at - source) + strlen("void main() {");
    const size_t n = strlen(source), extra = strlen(addition);
    if (n > SIZE_MAX - extra - 1) return NULL;
    char* next = (char*)malloc(n + extra + 1);
    if (!next) return NULL;
    memcpy(next, source, head);
    memcpy(next + head, addition, extra);
    memcpy(next + head + extra, source + head, n - head + 1);
    free(g_xenoSceneShaderText[slot]);
    g_xenoSceneShaderText[slot] = next;
    return next;
}
static int XenoSceneShaderSources(const char** vs, const char** fs,
                                  char* vd, size_t vc, char* fd, size_t fc)
{
#if defined(RENDERER_OGL) && !defined(ES2_SHADERS) && !defined(ES3_SHADERS)
    const char* vdecl = "\nin uint a_xenoScene;\nflat out uint v_xenoScene;\n";
    const char* fdecl = "\nflat in uint v_xenoScene;\nout vec4 xenoSceneCoverage;\n";
    if (strlen(vd) + strlen(vdecl) >= vc || strlen(fd) + strlen(fdecl) >= fc) {
        g_xenoSceneShaderFailed = 1;
        return 0;
    }
    const char* v = XenoSceneInject(0, vs[3], "\n v_xenoScene = a_xenoScene;\n");
    const char* f = XenoSceneInject(1, fs[4],
        "\n xenoSceneCoverage = vec4(float(v_xenoScene) / 255.0, 0.0, 0.0, 1.0);\n");
    if (!v || !f) { g_xenoSceneShaderFailed = 1; return 0; }
    /* Only swap sources after both allocations and all capacity checks pass. */
    strcat(vd, vdecl); strcat(fd, fdecl);
    vs[3] = v; fs[4] = f;
    return 1;
#else
    (void)vs; (void)fs; (void)vd; (void)vc; (void)fd; (void)fc;
    g_xenoSceneShaderFailed = 1;
    return 0;
#endif
}
static void XenoSceneBindProgram(GLuint program)
{
    /* Preserve the original program's existing attribute bindings. */
    glBindAttribLocation(program, XENO_SCENE_ATTRIB, "a_xenoScene");
    glBindFragDataLocation(program, 0, "fragColor");
    glBindFragDataLocation(program, 1, "xenoSceneCoverage");
}
static void XenoSceneProgramLinked(GLuint program)
{
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked || g_xenoSceneProgramCount == 16) { g_xenoSceneShaderFailed = 1; return; }
    g_xenoScenePrograms[g_xenoSceneProgramCount++] = program;
}
static int XenoSceneKnownProgram(GLuint program)
{
    for (unsigned i = 0; i < g_xenoSceneProgramCount; ++i)
        if (g_xenoScenePrograms[i] == program) return 1;
    return 0;
}
static void XenoSceneDestroyTargets()
{
    if (g_xenoSceneFbo) glDeleteFramebuffers(1, &g_xenoSceneFbo);
    if (g_xenoSceneCoverage) glDeleteTextures(1, &g_xenoSceneCoverage);
    if (g_xenoSceneDs) glDeleteRenderbuffers(1, &g_xenoSceneDs);
    g_xenoSceneFbo = g_xenoSceneCoverage = g_xenoSceneDs = 0;
    g_xenoSceneW = g_xenoSceneH = 0;
}
void GR_Hd2dSceneShutdown()
{
    /* Context teardown owns cleanup even when the master was turned OFF. */
    XenoSceneDestroyTargets();
    if (g_xenoSceneVbo) glDeleteBuffers(1, &g_xenoSceneVbo);
    g_xenoSceneVbo = 0;
    free(g_xenoSceneTags); g_xenoSceneTags = NULL;
    for (int i = 0; i < 2; ++i) {
        free(g_xenoSceneShaderText[i]); g_xenoSceneShaderText[i] = NULL;
    }
    g_xenoSceneValid = g_xenoSceneShaderFailed = g_xenoSceneUploaded = 0;
    g_xenoSceneProgramCount = 0;
    memset(g_xenoScenePrograms, 0, sizeof g_xenoScenePrograms);
    PcPort_Hd2dSceneBuffersReset();
    PcPort_Hd2dWorldReset();
}
static int XenoSceneTargets(int w, int h)
{
    if (g_xenoSceneFbo && w == g_xenoSceneW && h == g_xenoSceneH) return 1;
    GLint tex = 0, rb = 0, draw = 0, read = 0, unpack = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &rb);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpack);
    XenoSceneDestroyTargets();
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); /* NULL must mean no source data. */
    glGenTextures(1, &g_xenoSceneCoverage);
    glBindTexture(GL_TEXTURE_2D, g_xenoSceneCoverage);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenRenderbuffers(1, &g_xenoSceneDs);
    glBindRenderbuffer(GL_RENDERBUFFER, g_xenoSceneDs);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glGenFramebuffers(1, &g_xenoSceneFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_xenoSceneFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_xenoSceneCoverage, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, g_xenoSceneDs);
    const GLenum buffers[2] = { GL_NONE, GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(2, buffers);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpack);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
    if (!g_xenoSceneCoverage || !g_xenoSceneDs || !g_xenoSceneFbo || status != GL_FRAMEBUFFER_COMPLETE) {
        XenoSceneDestroyTargets();
        return 0;
    }
    g_xenoSceneW = w; g_xenoSceneH = h;
    return 1;
}
/* Copy actual default depth/stencil history, including effects of its clear
 * scissor and write masks. Only compatible 24/8, single-sample defaults enter. */
static void XenoSceneCopyTests()
{
    GLint draw = 0, read = 0;
    const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_xenoSceneFbo);
    glBlitFramebuffer(0, 0, g_xenoSceneW, g_xenoSceneH, 0, 0, g_xenoSceneW, g_xenoSceneH,
                      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
    if (scissor) glEnable(GL_SCISSOR_TEST);
}
/* Uses current scissor; never depends on the retail colour write mask. */
static void XenoSceneClearCoverage()
{
    GLint draw = 0;
    GLboolean mask[4];
    const GLfloat zero[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetBooleani_v(GL_COLOR_WRITEMASK, 1, mask);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_xenoSceneFbo);
    glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearBufferfv(GL_COLOR, 1, zero);
    glColorMaski(1, mask[0], mask[1], mask[2], mask[3]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
}
void GR_Hd2dSceneBegin()
{
    g_xenoSceneValid = 0;
    GR_Hd2dSceneBatchReset();
    if (!PcPort_Hd2dOpeningActive() || g_xenoSceneShaderFailed || g_dbg_wireframeMode) return;
    g_xenoSceneEpoch = PcPort_Hd2dSceneEpoch();
    g_xenoSceneGeneration = PcPort_Hd2dWorldGeneration();
    g_xenoSceneMap = PcPort_Hd2dCurrentMap();
    if (!PcPort_Hd2dWorldValid()) return;
    GLint draw = 0, bits = 0, stencil = 0, samples = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    if (draw) return;
    /* The legacy GL_DEPTH_BITS/STENCIL_BITS queries are unavailable in core
     * profiles. Query the actual default framebuffer attachment sizes. */
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH,
        GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &bits);
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_STENCIL,
        GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencil);
    glGetIntegerv(GL_SAMPLES, &samples);
    if (bits != 24 || stencil != 8 || samples) return;
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(g_window, &w, &h);
    if (w <= 0 || h <= 0 || !XenoSceneTargets(w, h)) return;
    if (!g_xenoSceneTags) g_xenoSceneTags = (unsigned char*)calloc(MAX_VERTEX_BUFFER_SIZE, 1);
    if (!g_xenoSceneTags) return;
    if (!g_xenoSceneVbo) {
        GLint vbo = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
        glGenBuffers(1, &g_xenoSceneVbo);
        glBindBuffer(GL_ARRAY_BUFFER, g_xenoSceneVbo);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
        GLint size = 0;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        if (size != MAX_VERTEX_BUFFER_SIZE) {
            glDeleteBuffers(1, &g_xenoSceneVbo); g_xenoSceneVbo = 0;
            return;
        }
    }
    const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
    glDisable(GL_SCISSOR_TEST);
    XenoSceneClearCoverage();
    if (scissor) glEnable(GL_SCISSOR_TEST);
    XenoSceneCopyTests();
    g_xenoSceneValid = 1;
}
void GR_Hd2dSceneClear()
{
    if (!XenoSceneActive()) return;
    GLint draw = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    if (draw) return;
    XenoSceneClearCoverage();
    XenoSceneCopyTests();
}
void GR_Hd2dSceneZeroRect(int x0, int y0, int x1, int y1)
{
    if (!XenoSceneActive()) return;
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > g_xenoSceneW) x1 = g_xenoSceneW;
    if (y1 > g_xenoSceneH) y1 = g_xenoSceneH;
    if (x1 <= x0 || y1 <= y0) return;
    GLint box[4];
    const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_SCISSOR_BOX, box);
    glEnable(GL_SCISSOR_TEST);
    glScissor(x0, y0, x1 - x0, y1 - y0);
    XenoSceneClearCoverage();
    glScissor(box[0], box[1], box[2], box[3]);
    if (!scissor) glDisable(GL_SCISSOR_TEST);
}
void GR_Hd2dSceneUpload(int count)
{
    g_xenoSceneUploaded = 0;
    if (!XenoSceneActive()) return;
    if (count < 0 || count > MAX_VERTEX_BUFFER_SIZE || !g_xenoSceneTags || !g_xenoSceneVbo) {
        g_xenoSceneValid = 0; return;
    }
    GLint vbo = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_xenoSceneVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count, g_xenoSceneTags);
    glEnableVertexAttribArray(XENO_SCENE_ATTRIB);
    glVertexAttribIPointer(XENO_SCENE_ATTRIB, 1, GL_UNSIGNED_BYTE, 1, (const void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    g_xenoSceneUploaded = count;
}
void GR_Hd2dSceneDraw(int first, int triangles, int overridden)
{
    if (!XenoSceneActive()) return;
    if (first < 0 || triangles < 0 || first > g_xenoSceneUploaded ||
        triangles > (g_xenoSceneUploaded - first) / 3) {
        g_xenoSceneValid = 0; return;
    }
    if (!triangles) return;
    GLint draw = 0, program = 0;
    GLboolean mask0[4], mask1[4];
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    if (draw) return;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetBooleani_v(GL_COLOR_WRITEMASK, 0, mask0);
    if (!XenoSceneKnownProgram((GLuint)program) || !mask0[0] || !mask0[1] || !mask0[2] || !mask0[3]) {
        g_xenoSceneValid = 0; return;
    }
    /* A DR override may skip the per-primitive hook. Certify none of this
     * split; zero its tag range before replay, including earlier positives. */
    if (overridden) {
        GLint vbo = 0;
        memset(g_xenoSceneTags + first, 4, (size_t)triangles * 3);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, g_xenoSceneVbo);
        glBufferSubData(GL_ARRAY_BUFFER, first, triangles * 3, g_xenoSceneTags + first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
    }
    const GLboolean blend = glIsEnabledi(GL_BLEND, 1);
    const GLboolean dither = glIsEnabled(GL_DITHER);
    const GLboolean logic = glIsEnabled(GL_COLOR_LOGIC_OP);
    if (logic) { g_xenoSceneValid = 0; return; }
    const GLboolean srgb = glIsEnabled(GL_FRAMEBUFFER_SRGB);
    glGetBooleani_v(GL_COLOR_WRITEMASK, 1, mask1);
    glDisablei(GL_BLEND, 1);
    glDisable(GL_DITHER); glDisable(GL_COLOR_LOGIC_OP); glDisable(GL_FRAMEBUFFER_SRGB);
    glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_xenoSceneFbo);
    GR_DrawTriangles(first, triangles);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
    glColorMaski(1, mask1[0], mask1[1], mask1[2], mask1[3]);
    if (blend) glEnablei(GL_BLEND, 1);
    if (dither) glEnable(GL_DITHER);
    if (logic) glEnable(GL_COLOR_LOGIC_OP);
    if (srgb) glEnable(GL_FRAMEBUFFER_SRGB);
}
void GR_Hd2dScenePresent(int path, int vramShown)
{
    if (!PcPort_Hd2dEnabled()) { g_xenoSceneValid = 0; return; }
    PcPort_Hd2dSceneBuffersOffer(NULL); /* Expire any older offer on every ON present. */
    if (path == 0 && !vramShown && XenoSceneActive()) {
        int w = 0, h = 0;
        GLint viewport[4], draw = 0;
        SDL_GL_GetDrawableSize(g_window, &w, &h);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
        if (!draw && w == g_xenoSceneW && h == g_xenoSceneH) {
            PcPortHd2dSceneBuffers b = {};
            b.coverage_tex = g_xenoSceneCoverage;
            b.drawable_width = w; b.drawable_height = h;
            b.viewport_x = viewport[0]; b.viewport_y = viewport[1];
            b.viewport_width = viewport[2]; b.viewport_height = viewport[3];
            b.field_id = g_xenoSceneMap; b.epoch = g_xenoSceneEpoch;
            b.flags = PC_PORT_HD2D_SCENE_COVERAGE_VALID;
            PcPort_Hd2dSceneBuffersOffer(&b);
        }
    }
    g_xenoSceneValid = 0;
}
