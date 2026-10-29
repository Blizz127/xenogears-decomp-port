/* Battle atlas glyph wrappers and POLY_FT4 construction. Original data,
 * frames and service calls use the memory bus; no CPU opcode is executed. */
#include "retail_data.h"
#define BATTLE_GLYPH_FULL_ADDRESS 0x80076a10u
#define BATTLE_GLYPH_HALF_ADDRESS 0x80076a6cu
#define BATTLE_GLYPH_RENDER_ADDRESS 0x8002675cu

#define GL_R8(a) battle_entry_read(entry,(a),1)
#define GL_R16(a) battle_entry_read(entry,(a),2)
#define GL_RS16(a) ((uint32_t)(int32_t)(int16_t)GL_R16(a))
#define GL_R32(a) battle_entry_read(entry,(a),4)
#define GL_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define GL_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define GL_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define GL_CALL(target,site,...) battle_entry_call(entry,(target),(site), \
    sizeof((uint32_t[]){__VA_ARGS__})/sizeof(uint32_t),(uint32_t[]){__VA_ARGS__})
#define GL_S0 (entry->cpu->gpr[16])
#define GL_S1 (entry->cpu->gpr[17])
#define GL_S2 (entry->cpu->gpr[18])
#define GL_S3 (entry->cpu->gpr[19])
#define GL_S4 (entry->cpu->gpr[20])
#define GL_S5 (entry->cpu->gpr[21])
#define GL_S6 (entry->cpu->gpr[22])
#define GL_S7 (entry->cpu->gpr[23])
#define GL_FP (entry->cpu->gpr[30])
#define GL_V0 (entry->cpu->gpr[2])
#define GL_V1 (entry->cpu->gpr[3])

/* Preserve the signed product's HI/LO before the rounding adjustment. */
static uint32_t battle_glyph_product(BattleEntryCall *entry,uint32_t left)
{
    uint64_t bits=(uint64_t)((int64_t)(int32_t)left*(int64_t)(int32_t)GL_S4);
    entry->cpu->hi=(uint32_t)(bits>>32);entry->cpu->lo=(uint32_t)bits;
    return (int32_t)entry->cpu->lo<0 ? entry->cpu->lo+0xfffu : entry->cpu->lo;
}

static uint32_t battle_glyph_asr12(uint32_t value)
{
    return (value>>12) | ((value&0x80000000u) ? 0xfff00000u : 0);
}

static void battle_glyph_wrapper_run(BattleEntryCall *entry,uint32_t target)
{
    uint32_t sp=entry->cpu->gpr[29]-0x28u;
    uint32_t index=entry->cpu->gpr[4],output=entry->cpu->gpr[5];
    /* The wrapper retains its output pointer in the guest t0 register. */
    entry->cpu->gpr[8]=output;
    uint32_t x=(uint32_t)(int32_t)(int16_t)entry->cpu->gpr[6];
    uint32_t y=(uint32_t)(int32_t)(int16_t)entry->cpu->gpr[7];
    entry->cpu->gpr[29]=sp;GL_V1=index;
    uint32_t table=GL_R32(0x800d2f5cu);
    GL_V0=target==BATTLE_GLYPH_FULL_ADDRESS ? 0x1000u : 0x800u;
    GL_W32(sp+0x14u,y);
    uint32_t slot=GL_R32(0x800ccb34u);
    GL_W32(sp+0x10u,x);GL_W32(sp+0x20u,entry->cpu->gpr[31]);
    GL_CALL(BATTLE_GLYPH_RENDER_ADDRESS,target+0x44u,table,index,output,slot,x,y,GL_V0);
    entry->cpu->gpr[31]=GL_R32(sp+0x20u);entry->cpu->gpr[29]=sp+0x28u;
}

static void battle_glyph_render_run(BattleEntryCall *entry)
{
    uint32_t table=entry->cpu->gpr[4],index=entry->cpu->gpr[5];
    uint32_t output=entry->cpu->gpr[6],slot=entry->cpu->gpr[7];
    /* Retail computes the descriptor-table address in guest a1. */
    entry->cpu->gpr[5]=table+(index<<1);
    uint32_t sp=entry->cpu->gpr[29]-0x50u;entry->cpu->gpr[29]=sp;
    GL_W32(sp+0x4cu,entry->cpu->gpr[31]);GL_W32(sp+0x48u,GL_FP);
    for(unsigned r=0;r<8;r++) GL_W32(sp+0x44u-r*4u,entry->cpu->gpr[23-r]);
    GL_W32(sp+0x24u,0);
    GL_V0=GL_R16(table+(index<<1)+4u);
    uint32_t coordinate=GL_R16(sp+0x60u);
    GL_W32(sp+0x5cu,slot);GL_W16(sp+0x10u,coordinate);
    coordinate=GL_R16(sp+0x64u);GL_V0+=table;
    GL_W32(sp+0x20u,GL_V0);GL_W16(sp+0x18u,coordinate);
    GL_V0=GL_RS16(GL_V0);GL_V1=GL_R16(sp+0x68u);
    if(GL_V0!=0) {
        GL_S4=GL_V1&0xffffu;GL_FP=4;GL_S7=output;
        uint32_t completed;
        do {
            GL_S1=GL_R32(sp+0x20u)+GL_FP;
            uint32_t px=battle_glyph_product(entry,GL_RS16(GL_S1+8u));
            entry->cpu->gpr[9]=GL_R32(sp+0x5cu);GL_S0=GL_S7+entry->cpu->gpr[9]*40u;
            uint32_t py=battle_glyph_product(entry,GL_RS16(GL_S1+10u));
            GL_S3=battle_glyph_asr12(px);
            uint32_t width=battle_glyph_product(entry,GL_RS16(GL_S1+4u));
            GL_S6=py>>12;
            uint32_t height=battle_glyph_product(entry,GL_RS16(GL_S1+6u));
            GL_S2=battle_glyph_asr12(width);GL_S5=height>>12;
            /* Preserve volatile values visible at the first GPU call. */
            entry->cpu->gpr[2]=height;entry->cpu->gpr[3]=width;entry->cpu->gpr[5]=py;
            GL_CALL(0x80043cb0u,0x80026864u,GL_S0);
            GL_CALL(0x80043bfcu,0x80026870u,GL_S0,0);
            GL_CALL(0x80043c24u,0x8002687cu,GL_S0,1);
            GL_CALL(0x80043a1cu,0x80026890u,GL_RS16(GL_S1+0x10u),0,
                    GL_RS16(GL_S1+0x16u),GL_RS16(GL_S1+0x18u));
            GL_W16(GL_S0+0x16u,GL_V0);
            GL_CALL(0x80043a58u,0x800268a4u,GL_RS16(GL_S1+0x12u),GL_RS16(GL_S1+0x14u));
            GL_W16(GL_S0+0xeu,GL_V0);
            uint32_t u=GL_R16(GL_S1),v=GL_R16(GL_S1+2u);
            uint32_t w=GL_R16(GL_S1+4u),flip=GL_R8(GL_S1+0x1au),h=GL_R16(GL_S1+6u);
            uint32_t x=GL_R16(sp+0x10u)+GL_S3;
            if(flip==0) {
                GL_V0=x;GL_V1=GL_S2+x;
                GL_W16(GL_S0+8u,GL_V0);GL_W16(GL_S0+0x10u,GL_V1);
                GL_W16(GL_S0+0x18u,GL_V0);GL_W16(GL_S0+0x20u,GL_V1);
            } else {
                u--;GL_V1=x;uint32_t right=GL_S2+x;
                GL_W16(GL_S0+8u,right);GL_W16(GL_S0+0x10u,GL_V1);
                GL_W16(GL_S0+0x18u,right);GL_W16(GL_S0+0x20u,GL_V1);
                if((int16_t)u<0) {u=0;w--;}
            }
            /* The vertical flag and frame Y are live after the X stores. */
            flip=GL_R8(GL_S1+0x1bu);
            uint32_t r4_value=flip ? (v-1u)<<16 : v-1u;
            uint32_t y=GL_R16(sp+0x18u)+GL_S6;
            if(flip==0) {
                GL_V0=y;GL_W16(GL_S0+0xau,GL_V0);GL_W16(GL_S0+0x12u,GL_V0);
                GL_V0+=GL_S5;GL_W16(GL_S0+0x1au,GL_V0);GL_W16(GL_S0+0x22u,GL_V0);
            } else {
                v--;GL_V1=y;GL_V0=GL_S5+y;
                GL_W16(GL_S0+0xau,GL_V0);GL_W16(GL_S0+0x12u,GL_V0);
                GL_W16(GL_S0+0x1au,GL_V1);GL_W16(GL_S0+0x22u,GL_V1);
                if((int16_t)v<0) {v=0;h--;}
            }
            entry->cpu->gpr[4]=r4_value;entry->cpu->gpr[5]=u;entry->cpu->gpr[6]=v;entry->cpu->gpr[7]=w;entry->cpu->gpr[8]=h;
            GL_V0=u+w;GL_V1=v+h;
            GL_W8(GL_S0+0xcu,u);GL_W8(GL_S0+0xdu,v);
            GL_W8(GL_S0+0x14u,GL_V0);GL_W8(GL_S0+0x15u,v);
            GL_W8(GL_S0+0x1cu,u);GL_W8(GL_S0+0x1du,GL_V1);
            GL_W8(GL_S0+0x24u,GL_V0);GL_W8(GL_S0+0x25u,GL_V1);
            uint32_t descriptor=GL_R32(sp+0x20u);GL_FP+=0x1cu;
            GL_V0=GL_RS16(descriptor);completed=GL_R32(sp+0x24u)+1u;
            GL_S7+=0x50u;GL_W32(sp+0x24u,completed);
        } while(completed!=GL_V0);
    }
    entry->cpu->gpr[9]=GL_R32(sp+0x20u);
    GL_V0=GL_RS16(entry->cpu->gpr[9]);
    entry->cpu->gpr[31]=GL_R32(sp+0x4cu);GL_FP=GL_R32(sp+0x48u);
    for(unsigned r=0;r<8;r++) entry->cpu->gpr[23-r]=GL_R32(sp+0x44u-r*4u);
    entry->cpu->gpr[29]=sp+0x50u;
}

#undef GL_R8
#undef GL_R16
#undef GL_RS16
#undef GL_R32
#undef GL_W8
#undef GL_W16
#undef GL_W32
#undef GL_CALL
#undef GL_S0
#undef GL_S1
#undef GL_S2
#undef GL_S3
#undef GL_S4
#undef GL_S5
#undef GL_S6
#undef GL_S7
#undef GL_FP
#undef GL_V0
#undef GL_V1

static int battle_glyphs_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    const char *identity;unsigned bytes;
    if(target==BATTLE_GLYPH_FULL_ADDRESS) {
        bytes=92;identity="22075ecff68520daca6dde56b3fb491d01dded56e381ba8b77b42b1950eb5e92";
    } else if(target==BATTLE_GLYPH_HALF_ADDRESS) {
        bytes=92;identity="054624538fe09bd63686c206be8d0f33ee1b42f44dd4d098eec19c6da6907443";
    } else if(target==BATTLE_GLYPH_RENDER_ADDRESS) {
        bytes=688;identity="400dc2a71aa7726c0fa7002e4a220faedbfae0107ad138fb4d1e192e22f2f1dd";
    } else return 0;
    if(runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime ||
       runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_GLYPHS");
    if(control && strcmp(control,"0")==0) return 0;
    const uint8_t *code=PSX_ADDR(target);
    if(target==BATTLE_GLYPH_RENDER_ADDRESS) {
        /* Main-executable text is not copied into game RAM. Reuse the
         * verified user-asset cache solely for identity; never execute it. */
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
        size_t offset=XENO_RD_SLUS_OFF(target);
        if(image==NULL || image->size<offset+bytes) return 0;
        code=image->bytes+offset;
    }
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=identity[i*2] ||
           file1_hex_digit(digest[i]&15u)!=identity[i*2+1]) return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0) return -1;
    if(target==BATTLE_GLYPH_RENDER_ADDRESS) battle_glyph_render_run(&entry);
    else battle_glyph_wrapper_run(&entry,target);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
