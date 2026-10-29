/* Mock GL verifies upload state restoration and packet identity without a GPU. */
#include "../src/hd2d/hd2d_sprite_transport.c"
#include <assert.h>
static int enabled, calls, links, opening=1, uploads, deletes;
static GLuint next_texture=43;
static void *linked_ot;
static GLint state[6]={17,19,8,11,13,15}, saved[6], tw, th;
static int Index(GLenum name) {
    switch(name) {
    case GL_TEXTURE_BINDING_2D:return 0; case GL_PIXEL_UNPACK_BUFFER_BINDING:return 1;
    case GL_UNPACK_ALIGNMENT:return 2; case GL_UNPACK_ROW_LENGTH:return 3;
    case GL_UNPACK_SKIP_ROWS:return 4; case GL_UNPACK_SKIP_PIXELS:return 5;
    default:assert(0);return 0;
    }
}
int PcPort_Hd2dEnabled(void) { return enabled; }
int PcPort_Hd2dOpeningActive(void) { assert(enabled); return opening; }
int PcPort_Hd2dCurrentMap(void) { return 7; }
void PcPort_AddPrimDomainAware(void *ot, void *prim) { links++; linked_ot=ot; ((P_TAG *)prim)->addr=42; }
void glGetIntegerv(GLenum n, GLint *v) { calls++; *v=state[Index(n)]; }
void glBindTexture(GLenum target, GLuint texture) { assert(target==GL_TEXTURE_2D); calls++; state[0]=texture; }
void glBindBuffer(GLenum target, GLuint buffer) { assert(target==GL_PIXEL_UNPACK_BUFFER); calls++; state[1]=buffer; }
void glPixelStorei(GLenum name, GLint value) { calls++; state[Index(name)]=value; }
void glGenTextures(GLsizei n, GLuint *v) { assert(n==1); calls++; *v=next_texture++; }
void glTexParameteri(GLenum target, GLenum name, GLint value) {
    assert(target==GL_TEXTURE_2D); calls++;
    if (name==GL_TEXTURE_MIN_FILTER || name==GL_TEXTURE_MAG_FILTER) assert(value==GL_NEAREST);
    else assert(value==GL_CLAMP_TO_EDGE);
}
void glTexImage2D(GLenum target, GLint level, GLint format, GLsizei w, GLsizei h,
    GLint border, GLenum input, GLenum type, const void *pixels) {
    assert(target==GL_TEXTURE_2D && !level && format==GL_RGBA8 && !border && input==GL_RGBA && type==GL_UNSIGNED_BYTE && pixels);
    assert(!state[1] && state[2]==1 && !state[3] && !state[4] && !state[5]); calls++; uploads++; tw=w; th=h;
}
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum name, GLint *value) {
    assert(target==GL_TEXTURE_2D && !level); calls++; *value=name==GL_TEXTURE_WIDTH?tw:th;
}
void glDeleteTextures(GLsizei n, const GLuint *v) { (void)n;(void)v;calls++;deletes++; }
int main(void)
{
    POLY_FT4 packet={0}, before, other;
    PcPortHd2dRetailQuad q={{-8,9,-8,9},{12,12,45,45},{1,2,1,2},{3,3,4,4},9,8,(void *)99,&packet};
    PcPortHd2dPartMapping m={123,7,0,2,0,9,8,{1,2,1,2},{3,3,4,4},0,0,16,32};
    PcPortHd2dPartProfile p={64,64,&m,1,"pc_port/assets/hd2d/test.rgba"};
    unsigned tex=0; float uv[4]={0}; unsigned char pixels[64*64*4]={0}; FILE *f;
    packet.code=0x2e; packet.r0=128;packet.g0=72;packet.b0=16;
    setXY4(&packet,-8,12,9,12,-8,45,9,45);setUV4(&packet,1,3,2,3,1,4,2,4);
    packet.tpage=9;packet.clut=8;before=packet;
    assert(!PcPort_Hd2dSpriteSubmitPart((void *)1,(void *)1,(void *)1));
    assert(!PcPort_Hd2dSpriteTextureOverride((void *)1,0,0,0,0,0,0,&tex,uv));
    assert(!calls && !links);
    f=fopen(p.rgba_path,"wb");assert(f);assert(fwrite(pixels,1,sizeof pixels,f)==sizeof pixels);assert(!fclose(f));
    enabled=1;memcpy(saved,state,sizeof state);
    assert(PcPort_Hd2dSpriteSubmitPart(&p,&m,&q));assert(links==1 && linked_ot==q.ot);
    assert(!memcmp(state,saved,sizeof state));
    /* Only linkage may differ; all retail geometry, colour and blend remain exact. */
    ((P_TAG *)&before)->addr=42;assert(!memcmp(&before,&packet,sizeof packet));
    /* Presenting the prior frame and walking an unrelated OT must preserve
     * the newly built sprite's registration until its own GPU query. */
    PcPort_Hd2dSpritePresentDone();
    PcPort_Hd2dSpriteGpuWalkDone();
    assert(PcPort_Hd2dSpriteTextureOverride(&packet,9,8,1,3,2,4,&tex,uv));
    assert(tex==43 && uv[0]==.5f/64 && uv[2]==15.5f/64);
    other=packet;assert(!PcPort_Hd2dSpriteTextureOverride(&other,9,8,1,3,2,4,&tex,uv));
    packet.r0++;assert(!PcPort_Hd2dSpriteTextureOverride(&packet,9,8,1,3,2,4,&tex,uv));packet.r0--;
    PcPort_Hd2dSpriteGpuWalkDone();assert(!PcPort_Hd2dSpriteTextureOverride(&packet,9,8,1,3,2,4,&tex,uv));
    assert(PcPort_Hd2dSpriteSubmitPart(&p,&m,&q));
    opening=0;assert(!PcPort_Hd2dSpriteTextureOverride(&packet,9,8,1,3,2,4,&tex,uv));
    PcPort_Hd2dSpriteGpuWalkDone();opening=1;
    /* Simulate a registered FT4 encountered with a DR texture bound: the
     * renderer records its address, deliberately skips TextureOverride, then
     * the completed walk retires it without accepting the replacement. */
    assert(PcPort_Hd2dSpriteSubmitPart(&p,&m,&q));
    enabled=0;PcPort_Hd2dSpritePacketSeen(q.poly);enabled=1;
    PcPort_Hd2dSpriteGpuWalkDone(); /* OFF observation did not mark it seen */
    PcPortHd2dSpriteDiagnostics pending_stats;
    assert(PcPort_Hd2dSpriteDiagnosticsGet(&pending_stats) && pending_stats.pending==1);
    PcPort_Hd2dSpritePacketSeen(NULL);
    PcPort_Hd2dSpritePacketSeen(q.poly);
    PcPort_Hd2dSpriteGpuWalkDone();
    {
        PcPortHd2dSpriteDiagnostics stats;
        assert(PcPort_Hd2dSpriteDiagnosticsGet(&stats));
        assert(stats.registered==3 && stats.accepted==1 && stats.queries==5 &&
            stats.snapshot_miss==1 && stats.no_identity==2 && stats.premature_presents==1 &&
            stats.gpu_walks==5 && stats.no_scope==1 && stats.expired_unconsumed==2 && !stats.pending);
        enabled=0;assert(!PcPort_Hd2dSpriteDiagnosticsGet((void *)1));
    }
    /* Multiple immutable profiles interleave in one OT without changing any
     * texture already referenced by a queued split. */
    enabled=1;
    PcPortHd2dPartProfile second=p, extra[MAX_CACHED_ATLASES], bad=p, missing=p;
    POLY_FT4 packet2=packet;
    PcPortHd2dRetailQuad q2=q;q2.poly=&packet2;
    second.rgba_path="pc_port/assets/hd2d/second.rgba";
    memset(pixels,7,sizeof pixels);
    f=fopen(second.rgba_path,"wb");assert(f);assert(fwrite(pixels,1,sizeof pixels,f)==sizeof pixels);assert(!fclose(f));
    assert(uploads==1);
    assert(PcPort_Hd2dSpriteSubmitPart(&p,&m,&q));
    assert(PcPort_Hd2dSpriteSubmitPart(&second,&m,&q2));
    assert(uploads==2 && !memcmp(state,saved,sizeof state));
    unsigned first_id=0,second_id=0;
    assert(PcPort_Hd2dSpriteTextureOverride(&packet2,9,8,1,3,2,4,&second_id,uv));
    assert(PcPort_Hd2dSpriteTextureOverride(&packet,9,8,1,3,2,4,&first_id,uv));
    assert(first_id==43 && second_id==44 && first_id!=second_id);
    assert(PcPort_Hd2dSpriteTextureOverride(&packet2,9,8,1,3,2,4,&tex,uv) && tex==second_id);
    assert(PcPort_Hd2dSpriteTextureOverride(&packet,9,8,1,3,2,4,&tex,uv) && tex==first_id);
    PcPort_Hd2dSpriteGpuWalkDone();
    assert(PcPort_Hd2dSpriteSubmitPart(&second,&m,&q2));assert(uploads==2);
    PcPort_Hd2dSpriteReset(); /* Reset expires packets, keeps both GL objects. */
    bad.width=0;missing.rgba_path="pc_port/assets/hd2d/missing.rgba";
    int old_calls=calls,old_links=links;
    assert(!PcPort_Hd2dSpriteSubmitPart(&bad,&m,&q));
    assert(!PcPort_Hd2dSpriteSubmitPart(&missing,&m,&q));
    assert(calls==old_calls && links==old_links && uploads==2);
    for(unsigned i=0;i<MAX_CACHED_ATLASES-2;++i) {
        extra[i]=p;
        assert(PcPort_Hd2dSpriteSubmitPart(&extra[i],&m,&q));
        PcPort_Hd2dSpriteReset();
    }
    assert(uploads==MAX_CACHED_ATLASES);
    extra[MAX_CACHED_ATLASES-2]=p;old_calls=calls;old_links=links;
    assert(!PcPort_Hd2dSpriteSubmitPart(&extra[MAX_CACHED_ATLASES-2],&m,&q));
    assert(calls==old_calls && links==old_links); // full cache no GL/link mutations
    assert(PcPort_Hd2dSpriteSubmitPart(&p,&m,&q));
    assert(PcPort_Hd2dSpriteSubmitPart(&second,&m,&q2)); // cache reuse still works at capacity
    assert(uploads==MAX_CACHED_ATLASES);
    old_calls=calls;old_links=links;enabled=0;
    assert(!PcPort_Hd2dSpriteSubmitPart((void*)1,(void*)1,(void*)1));
    assert(!PcPort_Hd2dSpriteTextureOverride((void*)1,0,0,0,0,0,0,(void*)1,uv));
    PcPort_Hd2dSpriteReset();assert(calls==old_calls && links==old_links && !deletes);
    return 0;
}
