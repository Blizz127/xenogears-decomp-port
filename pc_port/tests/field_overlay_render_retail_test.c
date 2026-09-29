/* Reuse only the established fixture/bus/SDK boundaries. */
#define main overlay_initializer_fixture_main
#include "field_overlay_init_retail_test.c"
#undef main
#include "field/main.h"
extern void func_800A84C0(void);
extern s32 D_800AEB60;
extern u8 g_FieldBss_800AF85C[];
static u32 context[0x8120/4], expected_context[0x8120/4];
RenderContext* g_FieldCurRenderContext=(RenderContext*)context;
static s32 math_args[4], expected_math[4], angle_result;
static unsigned math_count;
long FieldGetVec2Magnitude(long x,long z) {
    math_args[0]=x;math_args[1]=z;++math_count;return 123;
}
int ratan2(int x,int y) {
    math_args[2]=x;math_args[3]=y;++math_count;return angle_result;
}
static int render_read(void* u,u32 a,unsigned w,u32* v) {
    uintptr_t lo=(uintptr_t)context;
    if(a>=lo && (uint64_t)a+w<=lo+sizeof(context)) {
        *v=0;for(unsigned i=0;i<w;++i)*v|=(u32)((u8*)(uintptr_t)a)[i]<<(8*i);
        return 0;
    }
    return read_bus(u,a,w,v);
}
static int render_write(void* u,u32 a,unsigned w,u32 v) {
    uintptr_t lo=(uintptr_t)context;
    if(a>=lo && (uint64_t)a+w<=lo+sizeof(context)) {
        for(unsigned i=0;i<w;++i)((u8*)(uintptr_t)a)[i]=(u8)(v>>(8*i));
        return 0;
    }
    return write_bus(u,a,w,v);
}
static int render_bridge(void* u,PcPortMipsCpu* c,u32 t) {
    if(t==0x80099a4c) c->gpr[2]=FieldGetVec2Magnitude((s32)c->gpr[4],(s32)c->gpr[5]);
    else if(t==0x8004b32c)c->gpr[2]=ratan2((s32)c->gpr[4],(s32)c->gpr[5]);
    else return bridge(u,c,t);
    return 1;
}
static void setup(unsigned page,unsigned frame,unsigned active) {
    reset();memset(context,0xa5,sizeof(context));context[0x80e4/4]=0xab123456;
    g_FieldCurRenderContextIndex=page;D_800AF278=active;
    D_800AEB60=frame;D_800AEB64=2;
    D_800AFC60=(u32)(uintptr_t)buffers[0];D_800AFC64=(u32)(uintptr_t)buffers[1];
    write_bus(NULL,0x800afc60,4,D_800AFC60);write_bus(NULL,0x800afc64,4,D_800AFC64);
    write_bus(NULL,0x800adb08,4,page);write_bus(NULL,0x800af278,4,active);
    write_bus(NULL,0x800aeb60,4,frame);write_bus(NULL,0x800aeb64,4,2);
    write_bus(NULL,0x800c426c,4,(u32)(uintptr_t)context);
    for(unsigned i=0;i<6;++i) {
        u32 values[6]={0xffed1234,0x7fffffff,0x80000001,0x00123456,0x80000001,0x7fffffff};
        unsigned off=(i<3?0x24:0x34)+(i%3)*4;
        memcpy(g_FieldBss_800AF85C+off,&values[i],4);
        write_bus(NULL,0x800af85c+off,4,values[i]);
    }
    u16 yaw=0x8a53;memcpy(g_FieldBss_800AF85C+0x132,&yaw,2);
    write_bus(NULL,0x800af98e,2,yaw);
    memset(math_args,0,sizeof(math_args));math_count=0;angle_result=-531;
}
int main(void) {
    FILE* f=fopen("disc/field.bin","rb");assert(f);
    assert(fread(ram+0x6faf0,1,260862,f)==260862);assert(!fclose(f));
    assert((uintptr_t)context+sizeof(context)<0x1000000);
    unsigned frames[]={0,1,15,16,31,32,0x7fffffff,0xffffffff};
    for(unsigned active=0;active<2;++active)for(unsigned page=0;page<2;++page)
    for(unsigned k=0;k<sizeof(frames)/sizeof(frames[0]);++k) {
        setup(page,frames[k],active);
        PcPortMipsBus bus={.read=render_read,.write=render_write,.bridge=render_bridge};
        PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&c,0x800a84c0,0xfffffffcu,200000)!=PC_PORT_MIPS_HALTED) {
            fprintf(stderr,"OVERLAY RENDER FAIL oracle: %s\n",c.error);return 1;
        }
        memcpy(expected,buffers,sizeof(expected));memcpy(expected_context,context,sizeof(context));
        memcpy(expected_math,math_args,sizeof(math_args));
        u32 frame_after,anim_after;read_bus(NULL,0x800aeb60,4,&frame_after);read_bus(NULL,0x800aeb64,4,&anim_after);
        assert(math_count==2*active);
        setup(page,frames[k],active);func_800A84C0();
        if(memcmp(expected,buffers,sizeof(expected)) || memcmp(expected_context,context,sizeof(context)) ||
           memcmp(expected_math,math_args,sizeof(math_args)) || math_count!=2*active ||
           (u32)D_800AEB60!=frame_after || (u32)D_800AEB64!=anim_after) {
            fprintf(stderr,"OVERLAY RENDER FAIL active=%u page=%u frame=%x\n",active,page,frames[k]);return 1;
        }
    }
    puts("OVERLAY RENDER PASS 32 cases: both buffers, full OT context, counters and math arguments");
    return 0;
}
