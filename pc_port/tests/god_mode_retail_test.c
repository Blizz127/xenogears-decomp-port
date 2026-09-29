/* Runs the disc damage routine; only its two external callees are fixtures. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "god_mode.h"
static unsigned char ram[0x200000], overlay[0x80000];
static size_t overlay_size;
static unsigned deaths;
static int rd(void *p, uint32_t a, unsigned w, uint32_t *v) {
    (void)p; a &= 0x1fffffffu;
    if (a > sizeof(ram)-w) return -1;
    *v=0; for(unsigned i=0;i<w;i++) *v |= (uint32_t)ram[a+i]<<(i*8);
    return 0;
}
static int wr(void *p, uint32_t a, unsigned w, uint32_t v) {
    (void)p; a &= 0x1fffffffu;
    if (a > sizeof(ram)-w) return -1;
    for(unsigned i=0;i<w;i++) ram[a+i]=(unsigned char)(v>>(i*8));
    return 0;
}
static uint32_t get(uint32_t a,unsigned w) {uint32_t v; assert(!rd(NULL,a,w,&v)); return v;}
static void put(uint32_t a,unsigned w,uint32_t v) {assert(!wr(NULL,a,w,v));}
static int bridge(void *p,PcPortMipsCpu *c,uint32_t t) {
    (void)p; PcPort_GodModeBeforeGuest(c,t);
    if(t==0x80089c08u) {c->gpr[2]=1u<<c->gpr[4]; return 1;}
    if(t==0x800883acu) {deaths++; return 1;}
    return 0;
}
static void run(unsigned gear,unsigned slot,unsigned kind,unsigned amount,unsigned on,unsigned row) {
    memset(ram,0,sizeof(ram)); memcpy(ram+0x6faf0,overlay,overlay_size); deaths=0;
    if((unsigned)PcPort_GodModeEnabled()!=on) PcPort_GodModeToggle();
    unsigned stride=slot*0x170u;
    uint32_t hp=(gear?0x800ccdecu:0x800ccd34u)+stride;
    put(0x800d2dccu+slot,1,1); put(0x800c3eb8u+slot*28,1,gear);
    put(0x800c4000u+row*72+slot,1,kind); put(0x800c3fe8u+row*72+slot*2,2,amount);
    put(hp,gear?4:2,100); put(hp+(gear?4:2),gear?4:2,1000);
    PcPortMipsBus bus={0}; bus.read=rd;bus.write=wr;bus.bridge=bridge;
    PcPortMipsCpu c; PcPortMipsCpuInit(&c,&bus);
    c.gpr[4]=row;c.gpr[29]=0x801ff000;c.gpr[31]=0x80010000;
    int result=PcPortMipsRun(&c,0x80085618,0x80010000,10000);
    if(result) fprintf(stderr,"MIPS %d: %s\n",result,c.error);
    assert(result==0);
    int damage=(kind==0||kind==5||kind==7||kind==8);
    int signed_amount=(!gear&&(amount&0x8000u))?(int)amount-65536:(int)amount;
    int blocked=on&&slot<3&&damage&&signed_amount>0;
    int expected=100;
    if(damage&&!blocked) expected-=signed_amount;
    if(kind==2) expected+=(int)amount;
    if(expected<0) expected=0;
    assert(get(hp,gear?4:2)==(unsigned)expected);
    assert((get(0x800ccd64+stride,2)&0x8000u)==(expected==0?0x8000u:0));
    assert(deaths==(unsigned)(slot>=3&&expected==0));
    assert(get(0x800c3fe8u+row*72+slot*2,2)==(blocked?0:amount));
}
int main(void) {
    assert(!PcPort_GodModeEnabled());
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    overlay_size=fread(overlay,1,sizeof(overlay),f); assert(feof(f)); fclose(f);
    const unsigned kinds[]={0,1,2,3,4,5,6,7,8,9,10,11};
    for(unsigned on=0;on<2;on++) for(unsigned gear=0;gear<2;gear++)
    for(unsigned slot=0;slot<11;slot++) for(unsigned k=0;k<sizeof(kinds)/sizeof(kinds[0]);k++)
    for(unsigned r=0;r<2;r++) {
        run(gear,slot,kinds[k],25,on,r*7);
        run(gear,slot,kinds[k],150,on,r*7);
    }
    run(0,0,0,65511,1,0); /* signed healing in foot damage row */
    run(1,0,0,65511,1,0); /* unsigned Gear damage */
    PcPort_GodModeToggle(); assert(!PcPort_GodModeEnabled());
    run(0,0,0,150,0,0); run(1,0,0,150,0,0);
    puts("God-mode retail damage tests: PASS (foot/Gear, all slots, toggle, healing, lethal damage)");
}
