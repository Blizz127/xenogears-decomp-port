/* Run the same callback/context contract against the original MIPS bytes. */
#define func_8008C180 retail_remove
#define main contract_main
#include "field_party_remove_context_test.c"
#undef main
#undef func_8008C180
#include "battle_mips_adapter.h"
static unsigned char ram[0x200000];
static int memory(void* user,uint32_t address,unsigned width,uint32_t* value,int write) {
 (void)user;
 uintptr_t p=0;
 if(address>=0x80000000 && address+width<=0x80200000) p=(uintptr_t)(ram+(address&0x1fffff));
 struct {uint32_t guest;void* host;unsigned size;} ranges[]={
 {0x8005a444,D_8005A444,sizeof D_8005A444},
 {0x80062590,g_GamePartyMembers,sizeof g_GamePartyMembers},
 {0x8006fabc,g_GamePartyMemberSkins,sizeof g_GamePartyMemberSkins},
 {0x800afd1c,&D_800AFD1C,4},{0x800b00c0,&D_800B00C0,4}};
 for(unsigned i=0;i<sizeof ranges/sizeof ranges[0];i++)
  if(address>=ranges[i].guest && (uint64_t)address+width<=ranges[i].guest+ranges[i].size)
   p=(uintptr_t)ranges[i].host+address-ranges[i].guest;
 void** pointer=NULL;
 if(address==0x800afb10) pointer=(void**)&g_FieldActors;
 if(address==0x800b0078) pointer=(void**)&g_FieldScriptVMCurActor;
 if(address==0x800b06b8) pointer=&D_800B06B8;
 if(address==0x8005a414) pointer=&g_PartyDataBuffers[0];
 if(pointer){if(width!=4)return -1;if(write)*pointer=(void*)(uintptr_t)*value;else *value=(uint32_t)(uintptr_t)*pointer;return 0;}
 struct {void* ptr;size_t size;} host[]={{actors,sizeof actors},{data,sizeof data},{buffer,sizeof buffer}};
 for(unsigned i=0;i<sizeof host/sizeof host[0];i++)
  if(address>=(uintptr_t)host[i].ptr && (uint64_t)address+width<=(uintptr_t)host[i].ptr+host[i].size)p=address;
 if(!p)return -1;
 if(write) {for(unsigned i=0;i<width;i++)((u8*)p)[i]=*value>>(8*i);}
 else {*value=0;for(unsigned i=0;i<width;i++)*value|=(uint32_t)((u8*)p)[i]<<(8*i);}
 return 0;
}
static int rd(void*u,uint32_t a,unsigned w,uint32_t*v){return memory(u,a,w,v,0);}
static int wr(void*u,uint32_t a,unsigned w,uint32_t v){return memory(u,a,w,&v,1);}
static int bridge(void*u,PcPortMipsCpu*c,uint32_t t){
 (void)u;
 if(t==0x80080a74){func_80080A74(c->gpr[4]);return 1;}
 if(t==0x80076ac0){uint32_t a,b,d;require(!rd(0,c->gpr[29]+16,4,&a)&&!rd(0,c->gpr[29]+20,4,&b)&&!rd(0,c->gpr[29]+24,4,&d),"retail stack arguments");func_80076AC0(c->gpr[4],c->gpr[5],(void*)(uintptr_t)c->gpr[6],c->gpr[7],a,b,d);return 1;}
 return 0;
}
void retail_remove(s32 party){
 PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
 c.gpr[4]=party;c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffc;
 int result=PcPortMipsRun(&c,0x8008c180,0xfffffffc,1000);
 if(result){fprintf(stderr,"retail replay: %s\n",c.error);exit(1);}
}
int main(void){
 FILE*f=fopen("disc/field.bin","rb");require(f!=NULL,"retail file");
 require(fread(ram+0x6faf0,1,260862,f)==260862,"retail read");fclose(f);
 return contract_main();
}
