/* Packet identity transport: retail packets/UVs/colour/OT stay untouched. */
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include "common.h"
#include "psyq/libgpu.h"
#include "guest_prim_link.h"
#include "hd2d_sprite_parts.h"
#include "hd2d_integration.h"
#include "hd2d_sprite_diagnostics.h"
#include "host_only_bss.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PENDING_PARTS 16384
static struct PendingPart {
    const POLY_FT4 *packet;
    int seen, consumed;
    POLY_FT4 snapshot;
    const PcPortHd2dPartProfile *profile;
    PcPortHd2dPartMapping mapping;
} pending[MAX_PENDING_PARTS] HOST_ONLY_BSS;
static size_t pending_count;
#define MAX_CACHED_ATLASES 8
/* Immutable stable profiles own distinct textures for the context lifetime.
 * Never overwrite/delete a cached texture that queued GPU splits may use. */
static struct { const PcPortHd2dPartProfile *profile; GLuint texture; }
    atlas_cache[MAX_CACHED_ATLASES];
static size_t atlas_count;
static GLuint FindTexture(const PcPortHd2dPartProfile *profile)
{
    size_t i;
    for(i=0;i<atlas_count;++i)
        if(atlas_cache[i].profile==profile) return atlas_cache[i].texture;
    return 0;
}

static PcPortHd2dSpriteDiagnostics diagnostics;
static int diagnostic_requested = -1;
static void DiagnosticSummary(void)
{
    if (!PcPort_Hd2dEnabled() || diagnostic_requested != 1) return;
    fprintf(stderr,"[hd2d-sprite-gpu] registered=%llu queries=%llu accepted=%llu no_scope=%llu no_texture=%llu no_identity=%llu snapshot_miss=%llu metadata_miss=%llu uv_miss=%llu premature_presents=%llu gpu_walks=%llu expired_unconsumed=%llu pending=%u\n",
        (unsigned long long)diagnostics.registered,(unsigned long long)diagnostics.queries,
        (unsigned long long)diagnostics.accepted,(unsigned long long)diagnostics.no_scope,
        (unsigned long long)diagnostics.no_texture,(unsigned long long)diagnostics.no_identity,
        (unsigned long long)diagnostics.snapshot_miss,(unsigned long long)diagnostics.metadata_miss,
        (unsigned long long)diagnostics.uv_miss,(unsigned long long)diagnostics.premature_presents,
        (unsigned long long)diagnostics.gpu_walks,(unsigned long long)diagnostics.expired_unconsumed,(unsigned)pending_count);
}
static int DiagnosticEnabled(void)
{
    if (diagnostic_requested < 0) {
        const char *value=getenv("XENO_HD2D_SPRITE_DIAG");
        diagnostic_requested=value && !strcmp(value,"1");
        if (diagnostic_requested && atexit(DiagnosticSummary)) diagnostic_requested=0;
    }
    return diagnostic_requested;
}
int PcPort_Hd2dSpriteDiagnosticsGet(PcPortHd2dSpriteDiagnostics *out)
{
    if (!PcPort_Hd2dEnabled()) return 0;
    if (!DiagnosticEnabled() || !out) return 0;
    *out=diagnostics; out->pending=(unsigned)pending_count;
    return 1;
}
void PcPort_Hd2dSpriteReset(void) { pending_count=0; }
/* Field builds the NEXT OT before Vsync presents the PREVIOUS one. Therefore
 * presentation is not a packet lifetime boundary. Retire only consumed entries. */
void PcPort_Hd2dSpritePresentDone(void)
{
    size_t i, remaining=0;
    if (!PcPort_Hd2dEnabled()) return;
    for (i=0;i<pending_count;++i)
        if (!pending[i].consumed) pending[remaining++]=pending[i];
    pending_count=remaining;
    if (DiagnosticEnabled()) {
        static uint64_t reported_registered, reported_accepted;
        if (remaining) diagnostics.premature_presents++;
        if (diagnostics.registered!=reported_registered || diagnostics.accepted!=reported_accepted) {
            DiagnosticSummary();
            reported_registered=diagnostics.registered;
            reported_accepted=diagnostics.accepted;
        }
    }
}
/* Exact-address CPU observation happens before the renderer's DR-bound skip.
 * No packet dereference, texture lookup or output mutation is performed. */
void PcPort_Hd2dSpritePacketSeen(const void *packet)
{
    size_t i;
    if (!PcPort_Hd2dEnabled()) return;
    for (i=0;i<pending_count;++i)
        if (pending[i].packet == packet) pending[i].seen=1;
}
/* Retire only exact registered packet addresses encountered in this OT walk.
 * Unrelated independently queued OTs never invalidate unseen registrations. */
void PcPort_Hd2dSpriteGpuWalkDone(void)
{
    size_t i, remaining=0;
    int diag;
    if (!PcPort_Hd2dEnabled()) return;
    diag=DiagnosticEnabled();
    if (diag) diagnostics.gpu_walks++;
    for (i=0;i<pending_count;++i) {
        if (pending[i].seen) {
            if (diag && !pending[i].consumed) diagnostics.expired_unconsumed++;
        } else pending[remaining++]=pending[i];
    }
    pending_count=remaining;
}

static int PacketMatchesQuad(const POLY_FT4 *p, const PcPortHd2dRetailQuad *q)
{
    const short x[4] = {p->x0,p->x1,p->x2,p->x3}, y[4] = {p->y0,p->y1,p->y2,p->y3};
    const unsigned char u[4] = {p->u0,p->u1,p->u2,p->u3}, v[4] = {p->v0,p->v1,p->v2,p->v3};
    return (p->code & 0xfc) == 0x2c && p->tpage == q->tpage && p->clut == q->clut &&
        !memcmp(x,q->x,sizeof x) && !memcmp(y,q->y,sizeof y) &&
        !memcmp(u,q->u,sizeof u) && !memcmp(v,q->v,sizeof v);
}
static GLuint LoadTexture(const PcPortHd2dPartProfile *p)
{
    FILE *file;
    unsigned char *pixels;
    size_t bytes;
    GLint binding, unpack_buffer, alignment, row_length, skip_rows, skip_pixels, width=0, height=0;
    GLuint texture=0;
    texture=FindTexture(p);
    if(texture) return texture;
    if(atlas_count==MAX_CACHED_ATLASES || !PcPort_Hd2dPartProfileValidate(p)) return 0;
    bytes = (size_t)p->width * p->height * 4;
    file = fopen(p->rgba_path,"rb");
    if (!file) return 0;
    pixels = malloc(bytes);
    if (!pixels) { fclose(file); return 0; }
    if (fread(pixels,1,bytes,file) != bytes || fgetc(file) != EOF || ferror(file)) {
        free(pixels); fclose(file); return 0;
    }
    if (fclose(file)) { free(pixels); return 0; }
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&unpack_buffer);
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH,&row_length);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS,&skip_rows);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&skip_pixels);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glGenTextures(1,&texture);
    if (texture) {
        glBindTexture(GL_TEXTURE_2D,texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,p->width,p->height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&height);
    }
    glBindTexture(GL_TEXTURE_2D,binding);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack_buffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,row_length);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,skip_rows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,skip_pixels);
    free(pixels);
    if (!texture || width != p->width || height != p->height) {
        if (texture) glDeleteTextures(1,&texture);
        return 0;
    }
    atlas_cache[atlas_count].profile=p;
    atlas_cache[atlas_count++].texture=texture;
    return texture;
}
int PcPort_Hd2dSpriteSubmitPart(const PcPortHd2dPartProfile *p,
    const PcPortHd2dPartMapping *m, const PcPortHd2dRetailQuad *q)
{
    size_t i;
    struct PendingPart *entry;
    if (!PcPort_Hd2dEnabled()) return 0;
    if (!PcPort_Hd2dOpeningActive() || !p || !m || !q || !q->poly || !q->ot ||
        pending_count == MAX_PENDING_PARTS || !PacketMatchesQuad(q->poly,q) || !LoadTexture(p)) return 0;
    for (i=0;i<pending_count;++i) if (pending[i].packet == q->poly) {
        if (!memcmp(q->poly,&pending[i].snapshot,sizeof pending[i].snapshot)) {
            /* Ambiguous duplicate linkage/reuse fails closed and removes the
             * old token so the retail fallback cannot gain stale replacement. */
            pending[i]=pending[--pending_count];
            return 0;
        }
        /* The new producer matched a verified profile before reaching this API.
         * Changed bytes prove reuse; overwrite its old registration safely. */
        pending[i]=pending[--pending_count];
        break;
    }
    entry = pending + pending_count++;
    entry->packet = q->poly; entry->seen=entry->consumed=0; entry->profile=p; entry->mapping=*m;
    if (DiagnosticEnabled()) diagnostics.registered++;
    PcPort_AddPrimDomainAware(q->ot,q->poly);
    /* Snapshot after linkage, including domain-aware packet tag. */
    memcpy(&entry->snapshot,q->poly,sizeof entry->snapshot);
    return 1;
}
int PcPort_Hd2dSpriteTextureOverride(const void *packet, int tpage, int clut,
    int u0, int v0, int u1, int v1, unsigned *texture, float uv[4])
{
    size_t i;
    if (!PcPort_Hd2dEnabled()) return 0;
    int diag=DiagnosticEnabled();
    if (diag) diagnostics.queries++;
    for (i=0;i<pending_count;++i)
        if (pending[i].packet == packet) pending[i].seen=1;
    if (!PcPort_Hd2dOpeningActive() || !packet || !texture || !uv) {
        if (diag) diagnostics.no_scope++;
        return 0;
    }
    if (!atlas_count) { if (diag) diagnostics.no_texture++; return 0; }
    for (i=0;i<pending_count;++i) {
        struct PendingPart *e = pending+i;
        const PcPortHd2dPartMapping *m = &e->mapping;
        int j, min_u=255, min_v=255, max_u=0, max_v=0;
        GLuint selected_texture;
        if (e->packet != packet) continue;
        e->seen=1;
        if (memcmp(packet,&e->snapshot,sizeof e->snapshot)) {
            if (diag) diagnostics.snapshot_miss++;
            return 0;
        }
        selected_texture=FindTexture(e->profile);
        if(!selected_texture) { if(diag) diagnostics.no_texture++; return 0; }
        if (tpage != m->tpage || clut != m->clut || m->map != PcPort_Hd2dCurrentMap()) {
            if (diag) diagnostics.metadata_miss++;
            return 0;
        }
        for (j=0;j<4;++j) {
            if (m->u[j]<min_u) min_u=m->u[j];
            if (m->u[j]>max_u) max_u=m->u[j];
            if (m->v[j]<min_v) min_v=m->v[j];
            if (m->v[j]>max_v) max_v=m->v[j];
        }
        if (u0 != min_u || v0 != min_v || u1 != max_u || v1 != max_v || u0 == u1 || v0 == v1) {
            if (diag) diagnostics.uv_miss++;
            return 0;
        }
        e->consumed=1;
        if (diag) {
            diagnostics.accepted++;
            if (diagnostics.accepted==1)
                fprintf(stderr,"[hd2d-sprite-gpu] first_consumed texture=%u map=%d actor=%d frame=%d part=%d pending=%u\n",
                    selected_texture,m->map,m->actor,m->frame,m->part,(unsigned)pending_count);
        }
        *texture=selected_texture;
        uv[0]=(m->x+.5f)/e->profile->width;
        uv[1]=(m->y+.5f)/e->profile->height;
        uv[2]=(m->x+m->width-.5f)/e->profile->width;
        uv[3]=(m->y+m->height-.5f)/e->profile->height;
        return 1;
    }
    if (diag) diagnostics.no_identity++;
    return 0;
}
