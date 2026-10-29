#define GL_GLEXT_PROTOTYPES
#include <SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include "xg_plat/hd2d.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
static const int W=32,H=24;
static unsigned bit(int e) { return 1u<<e; }
static std::vector<unsigned char> pixels() {
    std::vector<unsigned char> p(W*H*4);
    glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,p.data()); return p;
}
static void gray() {
    glDisable(GL_SCISSOR_TEST); glColorMask(1,1,1,1); glDisable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0.5f,0.5f,0.5f,1); glClear(GL_COLOR_BUFFER_BIT);
}
static GLuint texture(GLint format,GLenum type,const void *data) {
    GLuint t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,format,W,H,0,GL_RED,type,data); return t;
}
int main() {
    XgHd2dConfig c; XgHd2d_DefaultConfig(&c);
    XgHd2dFrame f={}; f.field_active=1; f.field_id=2; f.width=W; f.height=H;
    // No GL context exists. These must bypass without a single GL call.
    assert(XgHd2d_Present(&c,&f)==0);
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;++e) c.effect[e].enabled=1;
    assert(XgHd2d_Present(&c,&f)==0);
    c.enabled=1; assert(XgHd2d_Present(&c,&f)==0); // empty allowlist
    c.map_count=1; c.maps[0]=2; f.field_id=490;
    assert(XgHd2d_Present(&c,&f)==0);
    f.field_id=2; f.field_active=0; assert(XgHd2d_Present(&c,&f)==0);
    f.field_active=1;
    XgHd2dPlan p=XgHd2d_Plan(&c,&f);
    assert(p.active==0); // every effect requires certified sprite/UI coverage
    assert(p.missing_depth & bit(XG_HD2D_DOF));
    assert(p.missing_protection & bit(XG_HD2D_BLOOM));
    f.protection_texture=1; f.protection_verified=1; // policy-only token, no GL inspection yet
    c.effect[XG_HD2D_GRADE].intensity=NAN;
    assert(!(XgHd2d_Plan(&c,&f).active&bit(XG_HD2D_GRADE)));
    c.effect[XG_HD2D_GRADE].intensity=4;
    assert(XgHd2d_Plan(&c,&f).intensity[XG_HD2D_GRADE]==1);
    c.map_count=XG_HD2D_MAX_MAPS+1; assert(XgHd2d_Present(&c,&f)==0); c.map_count=1;
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_Window *window=SDL_CreateWindow("hd2d regression",0,0,W,H,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(!window) { std::fprintf(stderr,"GL window: %s\n",SDL_GetError()); return 1; }
    SDL_GLContext context=SDL_GL_CreateContext(window); assert(context);
    const GLubyte *vendor=glGetString(GL_VENDOR), *renderer=glGetString(GL_RENDERER);
    const GLubyte *version=glGetString(GL_VERSION);
    assert(vendor && renderer && version);
    std::printf("hd2d: video=%s GL vendor=%s renderer=%s version=%s\n",
                SDL_GetCurrentVideoDriver(),vendor,renderer,version);
    std::fflush(stdout);
    glReadBuffer(GL_BACK); glDrawBuffer(GL_BACK);
    gray(); const auto baseline=pixels();
    std::vector<unsigned char> initialCoverage(W*H,0);
    f.protection_texture=texture(GL_R8,GL_UNSIGNED_BYTE,initialCoverage.data());
    c.enabled=0; assert(XgHd2d_Present(&c,&f)==0); assert(pixels()==baseline);
    c.enabled=1;
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;++e) c.effect[e].enabled=0;
    assert(XgHd2d_Present(&c,&f)==0); assert(pixels()==baseline);
    c.effect[XG_HD2D_GRADE]={1,1};
    assert(XgHd2d_Present(&c,&f)==1); assert(glGetError()==GL_NO_ERROR);
    const auto graded=pixels(); assert(graded!=baseline);
    // An explicit truth table checks the rendered result, independent of the
    // shader predicate. The same bytes retain the legacy binary interpretation.
    struct CoverageCase { unsigned char value; bool coverageEditable, binaryEditable; };
    const CoverageCase coverageCases[]={
        {0,false,true}, {16,false,false}, {2,true,false},
        {6,false,false}, {18,false,false}, {1,false,false},
        {4,false,false}, {8,false,false}, {13,false,false}, {255,false,false}
    };
    const int coverageCaseCount=sizeof(coverageCases)/sizeof(coverageCases[0]);
    for(int i=0;i<W*H;++i) initialCoverage[i]=coverageCases[i%coverageCaseCount].value;
    glBindTexture(GL_TEXTURE_2D,f.protection_texture);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_RED,GL_UNSIGNED_BYTE,initialCoverage.data());
    for(auto mode : {XG_HD2D_PROTECTION_BINARY, XG_HD2D_PROTECTION_COVERAGE_BITS}) {
        f.protection_mode=mode;
        gray(); assert(XgHd2d_Present(&c,&f)==1); const auto actual=pixels();
        for(int i=0;i<W*H;++i) {
            const auto &test=coverageCases[i%coverageCaseCount];
            const bool editable=mode==XG_HD2D_PROTECTION_BINARY ? test.binaryEditable : test.coverageEditable;
            const auto &expected=editable ? graded : baseline;
            assert(std::memcmp(&actual[i*4],&expected[i*4],4)==0);
        }
    }
    assert(glGetError()==GL_NO_ERROR);
    f.protection_mode=XG_HD2D_PROTECTION_BINARY;
    std::fill(initialCoverage.begin(),initialCoverage.end(),0);
    glBindTexture(GL_TEXTURE_2D,f.protection_texture);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_RED,GL_UNSIGNED_BYTE,initialCoverage.data());
    gray();
    // State preservation includes separate FBOs, texture/sampler bindings,
    // viewport, write masks, and enabled capabilities.
    GLuint tex[3], sam[3], vao, fb[2], unpack;
    glGenTextures(3,tex); glGenSamplers(3,sam); glGenVertexArrays(1,&vao);
    glGenFramebuffers(2,fb); glGenBuffers(1,&unpack);
    for(int i=0;i<3;++i) { glActiveTexture(GL_TEXTURE0+i); glBindTexture(GL_TEXTURE_2D,tex[i]); glBindSampler(i,sam[i]); }
    glActiveTexture(GL_TEXTURE5); glBindVertexArray(vao); glViewport(3,4,8,9);
    glBindFramebuffer(GL_READ_FRAMEBUFFER,fb[0]); glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fb[1]);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack); glColorMask(0,1,0,1);
    const GLenum caps[]={GL_BLEND,GL_DEPTH_TEST,GL_STENCIL_TEST,GL_CULL_FACE,GL_SCISSOR_TEST,
        GL_FRAMEBUFFER_SRGB,GL_DITHER,GL_RASTERIZER_DISCARD,GL_COLOR_LOGIC_OP,GL_SAMPLE_ALPHA_TO_COVERAGE,GL_SAMPLE_COVERAGE};
    for(GLenum cap:caps) glEnable(cap);
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    assert(XgHd2d_Present(&c,&f)==1); assert(glGetError()==GL_NO_ERROR);
    GLint v=0,view[4]; GLboolean mask[4];
    glGetIntegerv(GL_ACTIVE_TEXTURE,&v); assert(v==GL_TEXTURE5);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&v); assert(v==GLint(vao));
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&v); assert(v==GLint(fb[0]));
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&v); assert(v==GLint(fb[1]));
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&v); assert(v==GLint(unpack));
    glGetIntegerv(GL_VIEWPORT,view); assert(view[0]==3&&view[1]==4&&view[2]==8&&view[3]==9);
    glGetBooleanv(GL_COLOR_WRITEMASK,mask); assert(!mask[0]&&mask[1]&&!mask[2]&&mask[3]);
    GLint poly[2]; glGetIntegerv(GL_POLYGON_MODE,poly); assert(poly[0]==GL_LINE);
    for(GLenum cap:caps) assert(glIsEnabled(cap));
    for(int i=0;i<3;++i) {
        glActiveTexture(GL_TEXTURE0+i); glGetIntegerv(GL_TEXTURE_BINDING_2D,&v); assert(v==GLint(tex[i]));
        glGetIntegeri_v(GL_SAMPLER_BINDING,i,&v); assert(v==GLint(sam[i]));
    }
    for(GLenum cap:caps) glDisable(cap);
    glBindFramebuffer(GL_FRAMEBUFFER,0); glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); glColorMask(1,1,1,1);
    // Every protected pixel survives all supported effects byte-for-byte.
    std::vector<unsigned char> protection(W*H,0);
    for(int y=5;y<19;++y) for(int x=12;x<20;++x) protection[y*W+x]=255;
    std::vector<float> depth(W*H,20.0f);
    f.protection_texture=texture(GL_R8,GL_UNSIGNED_BYTE,protection.data()); f.protection_verified=1;
    f.linear_depth_texture=texture(GL_R32F,GL_FLOAT,depth.data()); f.linear_depth_verified=1;
    f.shadow_count=f.light_count=f.particle_count=1;
    f.shadows[0]={0.5f,0.25f,0.2f,0.1f}; f.lights[0]={0.5f,0.5f,0.8f,1}; f.particles[0]={0.2f,0.5f,4,0.1f};
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;++e) c.effect[e]={1,0.7f};
    gray(); assert(XgHd2d_Present(&c,&f)==1); auto rendered=pixels();
    assert(rendered!=baseline);
    for(int i=0;i<W*H;++i) if(protection[i])
        assert(std::memcmp(&rendered[i*4],&baseline[i*4],4)==0);
    assert(glGetError()==GL_NO_ERROR);
    // Linear-depth fog must depend on depth values, not vertical screen position.
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;++e) c.effect[e].enabled=0;
    c.effect[XG_HD2D_FOG]={1,1}; c.fog_start=2; c.fog_end=10;
    for(int y=0;y<H;++y) for(int x=0;x<W;++x) depth[y*W+x]=x<W/2?1.0f:20.0f;
    glBindTexture(GL_TEXTURE_2D,f.linear_depth_texture);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_RED,GL_FLOAT,depth.data());
    gray(); assert(XgHd2d_Present(&c,&f)==1); rendered=pixels();
    assert(std::memcmp(&rendered[(2*W+2)*4],&baseline[(2*W+2)*4],4)==0);
    assert(std::memcmp(&rendered[(2*W+W-3)*4],&baseline[(2*W+W-3)*4],4)!=0);
    // Isolate each spatial effect on a high-contrast source. Neither bloom nor
    // DOF may borrow protected foreground pixels into its neighborhood.
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;++e) c.effect[e].enabled=0;
    std::fill(depth.begin(),depth.end(),20.0f);
    glBindTexture(GL_TEXTURE_2D,f.linear_depth_texture);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_RED,GL_FLOAT,depth.data());
    for(int effect : {XG_HD2D_DOF, XG_HD2D_BLOOM}) {
        gray(); glEnable(GL_SCISSOR_TEST); glScissor(3,3,4,4);
        glClearColor(1,1,1,1); glClear(GL_COLOR_BUFFER_BIT); glDisable(GL_SCISSOR_TEST);
        auto before=pixels(); c.effect[effect]={1,1};
        assert(XgHd2d_Present(&c,&f)==1); auto after=pixels(); assert(before!=after);
        for(int i=0;i<W*H;++i) if(protection[i])
            assert(std::memcmp(&after[i*4],&before[i*4],4)==0);
        c.effect[effect].enabled=0;
    }
    c.effect[XG_HD2D_FOG]={1,1};
    // Declared-but-wrong depth format must fail closed after real GL inspection.
    f.linear_depth_texture=f.protection_texture; gray();
    assert(XgHd2d_Present(&c,&f)==0); assert(pixels()==baseline);
    // Explicit OT proxy mode retains the linear fixtures above unchanged.
    f.depth_mode=XG_HD2D_DEPTH_OT_ORDER; f.protection_mode=XG_HD2D_PROTECTION_COVERAGE_BITS;
    f.order_scale=65535.0f/4095.0f;
    std::vector<unsigned short> order(W*H,4095);
    f.linear_depth_texture=texture(GL_R16,GL_UNSIGNED_SHORT,order.data());
    std::fill(protection.begin(),protection.end(),2); // map coverage stays editable
    const unsigned char kinds[]={0,16,6,18,1,4,8,13,255};
    const int protectedKindCount=sizeof(kinds)/sizeof(kinds[0]);
    for(int k=0;k<protectedKindCount;++k) protection[(6+k)*W+14]=kinds[k];
    glBindTexture(GL_TEXTURE_2D,f.protection_texture);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_RED,GL_UNSIGNED_BYTE,protection.data());
    // All effects preserve unknown, mixed and sprite/UI/override coverage exactly.
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;++e) c.effect[e]={1,.7f};
    c.order_focus=.8f; c.order_range=.2f;
    gray();assert(XgHd2d_Present(&c,&f)==1);rendered=pixels();
    for(int k=0;k<protectedKindCount;++k)
        assert(!std::memcmp(&rendered[((6+k)*W+14)*4],&baseline[((6+k)*W+14)*4],4));
    assert(rendered!=baseline && !(XgHd2d_Plan(&c,&f).active&bit(XG_HD2D_PARTICLES)));
    // A bright protected patch must not feed bloom into neighboring map
    // pixels. Exact map2 is the positive control for the same scene and effect.
    const auto sparseProtection=protection;
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;++e) c.effect[e].enabled=0;
    c.effect[XG_HD2D_BLOOM]={1,1};
    for(int kind : {0,16,2,6,18}) {
        std::fill(protection.begin(),protection.end(),2);
        for(int y=5;y<19;++y) for(int x=12;x<20;++x) protection[y*W+x]=kind;
        glBindTexture(GL_TEXTURE_2D,f.protection_texture);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_RED,GL_UNSIGNED_BYTE,protection.data());
        gray(); glEnable(GL_SCISSOR_TEST); glScissor(12,5,8,14);
        glClearColor(1,1,1,1); glClear(GL_COLOR_BUFFER_BIT); glDisable(GL_SCISSOR_TEST);
        const auto before=pixels();
        assert(XgHd2d_Present(&c,&f)==1); const auto after=pixels();
        if(kind==2) {
            bool changedNeighbor=false;
            for(int y=0;y<H;++y) for(int x=0;x<W;++x)
                if(x<12 || x>=20 || y<5 || y>=19)
                    changedNeighbor |= std::memcmp(&after[(y*W+x)*4],&before[(y*W+x)*4],4)!=0;
            assert(changedNeighbor);
        } else {
            assert(after==before);
        }
    }
    protection=sparseProtection;
    glBindTexture(GL_TEXTURE_2D,f.protection_texture);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_RED,GL_UNSIGNED_BYTE,protection.data());
    assert(glGetError()==GL_NO_ERROR);
    // Larger raw OT slots are nearer: zero ordered distance is valid, zero
    // source slot is invalid; fog is ordinal and independent of image y.
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;++e)c.effect[e].enabled=0;
    c.effect[XG_HD2D_FOG]={1,1};c.order_fog_start=.2f;c.order_fog_end=.8f;
    for(int y=0;y<H;++y)for(int x=0;x<W;++x)order[y*W+x]=x<8?4095:(x<16?1:0);
    glBindTexture(GL_TEXTURE_2D,f.linear_depth_texture);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_RED,GL_UNSIGNED_SHORT,order.data());
    gray();assert(XgHd2d_Present(&c,&f)==1);rendered=pixels();
    assert(!std::memcmp(&rendered[(2*W+2)*4],&baseline[(2*W+2)*4],4)); // nearest distance0
    assert(std::memcmp(&rendered[(2*W+10)*4],&baseline[(2*W+10)*4],4)); // far ordinal
    assert(!std::memcmp(&rendered[(2*W+20)*4],&baseline[(2*W+20)*4],4)); // invalid slot0
    // Full drawable attachments sampled at exact viewport offset, not local0.
    f.x=4;f.y=3;f.width=16;f.height=12;f.auxiliary_width=W;f.auxiliary_height=H;
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;++e)c.effect[e].enabled=0;
    c.effect[XG_HD2D_GRADE]={1,1};
    std::fill(protection.begin(),protection.end(),2);protection[9*W+10]=1;
    glBindTexture(GL_TEXTURE_2D,f.protection_texture);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,W,H,GL_RED,GL_UNSIGNED_BYTE,protection.data());
    gray();assert(XgHd2d_Present(&c,&f)==1);rendered=pixels();
    assert(!std::memcmp(&rendered[(9*W+10)*4],&baseline[(9*W+10)*4],4));
    assert(std::memcmp(&rendered[(9*W+11)*4],&baseline[(9*W+11)*4],4));
    assert(!std::memcmp(&rendered[0],&baseline[0],4)); // outside viewport untouched
    f.protection_mode=static_cast<XgHd2dProtectionMode>(99);
    gray();assert(XgHd2d_Present(&c,&f)==0);assert(pixels()==baseline);
    f.protection_mode=XG_HD2D_PROTECTION_COVERAGE_BITS;
    f.auxiliary_width=10;assert(XgHd2d_Present(&c,&f)==0);f.auxiliary_width=W;
    assert(glGetError()==GL_NO_ERROR);
    c.enabled=0; gray(); assert(XgHd2d_Present(&c,&f)==0); assert(pixels()==baseline);
    XgHd2d_Shutdown(); assert(glGetError()==GL_NO_ERROR);
    SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit();
    puts("hd2d: policy, shader, output, coverage eligibility, protected bloom, linear depth and GL state PASS");
}
