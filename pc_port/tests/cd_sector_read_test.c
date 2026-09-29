#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "iso9660.h"
#define eprinterr(...) ((void)0)
#include "vfile.inc"
static VFILE g_imageFile;
enum {RM_DATA,RM_XA_AUDIO};
static int readMode;
#define COMMAND_QUEUE_SIZE 128
static struct {unsigned mode;unsigned char*p;unsigned processed,count;} g_cdComQueue[COMMAND_QUEUE_SIZE];
#include "sector_read.inc"
static unsigned char raw[2352*3],out[2336*3+32];
static unsigned cases;
static void fixture(unsigned length,unsigned audio,unsigned polling,unsigned disk) {
    memset(out,0xa5,sizeof out);memset(g_cdComQueue,0,sizeof g_cdComQueue);
    for(unsigned i=0;i<COMMAND_QUEUE_SIZE;++i)g_cdComQueue[i].processed=1;
    FILE*f=NULL;
    if(disk) {
        f=tmpfile();assert(f);assert(fwrite(raw,1,length,f)==length);rewind(f);
        g_imageFile=(VFILE){f,NULL,NULL,0};
    } else g_imageFile=(VFILE){NULL,raw,raw,length};
    readMode=audio?RM_XA_AUDIO:RM_DATA;
    assert(CdRead(3,(u_long*)(out+16),0)==1);
    int status;
    do {status=CdReadSync(polling,NULL);} while(status>0);
    assert(status==(length==sizeof raw?0:-1));
    unsigned complete=length/2352, payload=audio?2336:2048,offset=audio?16:24;
    for(unsigned i=0;i<sizeof out;++i) {
        unsigned char expected=0xa5;
        if(i>=16 && i<16+complete*payload) {
            unsigned j=i-16;expected=raw[(j/payload)*2352+offset+j%payload];
        }
        assert(out[i]==expected);
    }
    assert(g_cdComQueue[0].processed==1);
    assert(g_cdComQueue[0].p==out+16+complete*payload);
    assert(g_cdComQueue[0].count==3-complete);
    if(f)assert(fclose(f)==0);
    ++cases;
}
int main(void) {
    assert(sizeof(Sector)==2352 && sizeof(AudioSector)==2352);
    for(unsigned i=0;i<sizeof raw;++i)raw[i]=(i*73+(i>>3))&255;
    /* Every truncation boundary, including headers, payload, EDC and ECC. */
    for(unsigned n=0;n<=sizeof raw;++n)for(unsigned a=0;a<2;++a)
    for(unsigned poll=0;poll<2;++poll)fixture(n,a,poll,0);
    const unsigned sizes[]={0,1,15,16,23,24,2047,2048,2071,2072,2351,2352,2353,4703,4704,7055,7056};
    for(unsigned i=0;i<sizeof sizes/sizeof sizes[0];++i)
    for(unsigned a=0;a<2;++a)for(unsigned p=0;p<2;++p)fixture(sizes[i],a,p,1);
    /* Virtual fread returns complete elements, but consumes/copies the final
     * partial element just like stdio; zero/overflow requests do not advance. */
    for(unsigned size=1;size<=32;++size)for(unsigned n=0;n<100;++n) {
        unsigned char b[128];memset(b,0xa5,sizeof b);VFILE v={NULL,raw,raw,n};
        size_t got=vfread(b,size,3,&v),bytes=n<size*3?n:size*3;
        assert(got==bytes/size && v.curPtr==raw+bytes);
        assert(!memcmp(b,raw,bytes));for(unsigned i=bytes;i<sizeof b;++i)assert(b[i]==0xa5);
        ++cases;
    }
    VFILE v={NULL,raw,raw,sizeof raw};
    assert(vfread(out,0,SIZE_MAX,&v)==0 && v.curPtr==raw);
    assert(vfread(out,SIZE_MAX,2,&v)==0 && v.curPtr==raw);
    printf("PASS %u sector/element fixtures; complete-sector copies only\n",cases+2);
}
