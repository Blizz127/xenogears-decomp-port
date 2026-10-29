/* Composition benchmark only: synthetic scene, not an in-game performance gate. */
#define GL_GLEXT_PROTOTYPES
#include <SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include "xg_plat/hd2d.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static int number(const char *text, int low, int high) {
    char *end;
    long value = std::strtol(text, &end, 10);
    return *text && !*end && value >= low && value <= high ? int(value) : 0;
}
int main(int argc, char **argv) {
    int width=1280, height=720, samples=120;
    if (argc!=1 && argc!=4 && argc!=5) return 2;
    const bool ordered=argc==5;
    if(ordered && std::strcmp(argv[4],"ot")) return 2;
    if (argc>=4) {
        width=number(argv[1],64,4096); height=number(argv[2],64,4096);
        samples=number(argv[3],3,1000);
        if (!width || !height || !samples) return 2;
    }
    if (SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_Window *window=SDL_CreateWindow("HD2D composition benchmark",0,0,width,height,
        SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    SDL_GLContext context=window ? SDL_GL_CreateContext(window) : NULL;
    if (!context) { std::fprintf(stderr,"SDL GL: %s\n",SDL_GetError()); SDL_Quit(); return 1; }
    SDL_GL_SetSwapInterval(0);
    std::printf("vendor=%s\nrenderer=%s\nversion=%s\n",
        glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION));
    int drawableWidth,drawableHeight;
    SDL_GL_GetDrawableSize(window,&drawableWidth,&drawableHeight);
    if (width!=drawableWidth || height!=drawableHeight) {
        std::fprintf(stderr,"Drawable size differs; use unscaled display for comparison\n");
        SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit(); return 1;
    }
    std::vector<float> depth(width*height);
    std::vector<unsigned char> protection(width*height,ordered ? 2 : 0);
    std::vector<unsigned short> order(ordered ? width*height : 0);
    for (int y=0;y<height;++y) for(int x=0;x<width;++x) {
        depth[y*width+x]=10.0f+float(x)*200.0f/width;
        if(ordered) order[y*width+x]=y==0 ? 0 :
            static_cast<unsigned short>(1+(width-1-x)*4094/(width-1));
        if (x>width/3 && x<width*2/3 && y>height/3 && y<height*2/3) {
            if(ordered) {
                const unsigned char coverage[]={1,4,8};
                protection[y*width+x]=coverage[(x-width/3)*3/(width/3+1)];
            } else protection[y*width+x]=255;
        }
    }
    GLuint textures[2]; glGenTextures(2,textures);
    glBindTexture(GL_TEXTURE_2D,textures[0]);
    if(ordered) {
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexImage2D(GL_TEXTURE_2D,0,GL_R16,width,height,0,GL_RED,GL_UNSIGNED_SHORT,order.data());
    } else glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,width,height,0,GL_RED,GL_FLOAT,depth.data());
    glBindTexture(GL_TEXTURE_2D,textures[1]);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width,height,0,GL_RED,GL_UNSIGNED_BYTE,protection.data());
    XgHd2dConfig config; XgHd2d_DefaultConfig(&config);
    // Synthetic map ID is a benchmark fixture, not a production allowlist.
    config.map_count=1; config.maps[0]=2;
    config.focus_depth=70; config.focus_range=20; config.fog_start=20; config.fog_end=210;
    for (int e=0;e<XG_HD2D_EFFECT_COUNT;++e) config.effect[e]={1,0.5f};
    XgHd2dFrame frame={}; frame.field_active=1; frame.field_id=2;
    frame.width=width; frame.height=height;
    frame.linear_depth_texture=textures[0]; frame.protection_texture=textures[1];
    frame.linear_depth_verified=frame.protection_verified=1;
    if(ordered) {
        frame.depth_mode=XG_HD2D_DEPTH_OT_ORDER;
        frame.protection_mode=XG_HD2D_PROTECTION_COVERAGE_BITS;
        frame.order_scale=65535.0f/4095.0f;
        frame.auxiliary_width=width;frame.auxiliary_height=height;
        std::puts("mode=ot_order raw_R16_slots=0..4095 zero_background larger_nearer physical_particles=disabled");
        std::puts("scope=synthetic_composition_only not_in_game_or_whole_gpu_performance");
    }
    frame.light_count=frame.shadow_count=frame.particle_count=XG_HD2D_MAX_ITEMS;
    for (int i=0;i<XG_HD2D_MAX_ITEMS;++i) {
        float x=float(i+1)/(XG_HD2D_MAX_ITEMS+1);
        frame.lights[i]={x,0.4f,0.1f,0.8f};
        frame.shadows[i]={x,0.3f,0.025f,0.012f};
        frame.particles[i]={x,0.7f,15.0f,0.003f};
    }
    if(ordered) {
        config.enabled=1;
        const unsigned expected=((1u<<XG_HD2D_EFFECT_COUNT)-1u)&~(1u<<XG_HD2D_PARTICLES);
        if(XgHd2d_Plan(&config,&frame).active!=expected) {
            std::fputs("OT fixture did not enable exactly the supported effects\n",stderr);
            glDeleteTextures(2,textures);SDL_GL_DeleteContext(context);
            SDL_DestroyWindow(window);SDL_Quit();return 1;
        }
        config.enabled=0;
    }
    std::printf("scenario,width,height,samples,median_ms,p95_ms\n");
    int result=0;
    for (int enabled=0;enabled<=1 && !result;++enabled) {
        config.enabled=enabled;
        std::vector<double> times;
        for (int i=-30;i<samples;++i) {
            auto start=std::chrono::steady_clock::now();
            glClearColor(0.6f,0.5f,0.4f,1); glClear(GL_COLOR_BUFFER_BIT);
            int present=XgHd2d_Present(&config,&frame);
            glFinish();
            if (present!=enabled || glGetError()!=GL_NO_ERROR) {
                std::fprintf(stderr,"Present failed: %s\n",XgHd2d_LastError()); result=1; break;
            }
            if(i>=0) times.push_back(std::chrono::duration<double,std::milli>(
                std::chrono::steady_clock::now()-start).count());
        }
        if (!result) {
            std::sort(times.begin(),times.end());
            std::printf("%s,%d,%d,%d,%.6f,%.6f\n",enabled ? (ordered ? "ot_order_supported_effects" : "all_effects") : "off",
                width,height,samples,times[samples/2],times[(samples*95-1)/100]);
        }
    }
    XgHd2d_Shutdown(); glDeleteTextures(2,textures);
    SDL_GL_DeleteContext(context); SDL_DestroyWindow(window); SDL_Quit();
    return result;
}
