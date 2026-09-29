#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#define CD_SECTOR_SIZE_MODE2 2352
#define DISC_CUE_FILENAME "disc.cue"
#define eprinterr(...) ((void)0)
#define eprintinfo(...) ((void)0)
#define eprintwarn(...) ((void)0)
static unsigned opens,closes,seek_calls,open_attempts,fail_close;
static unsigned fail_seek;
static int fail_tell;
static unsigned read_calls,fail_read;
static FILE *tracked_open(const char*p,const char*m) {++open_attempts;FILE*f=fopen(p,m);if(f)++opens;return f;}
static int tracked_close(FILE*f) {++closes;int r=fclose(f);return fail_close?EOF:r;}
static int tracked_seek(FILE*f,long n,int origin) {
    if(++seek_calls==fail_seek)return -1;
    return fseek(f,n,origin);
}
static long tracked_tell(FILE*f) {return fail_tell?-1:ftell(f);}
static int tracked_getc(FILE*f) {if(++read_calls==fail_read)return EOF;return fgetc(f);}
static int tracked_error(FILE*f) {return (fail_read && read_calls==fail_read)||ferror(f);}
#define fopen tracked_open
#define fclose tracked_close
#define fseek tracked_seek
#define ftell tracked_tell
#define fgetc tracked_getc
#define ferror tracked_error
#include "vfile.inc"
static VFILE g_imageFile;
static int g_cdSectorSize=2352,g_cdNumFrames,g_cdCurrentTrack=1,g_cdCurrentSector;
#include "image_config.inc"
#include "image_available.inc"
#define COMMAND_QUEUE_SIZE 128
static struct {unsigned processed;} g_cdComQueue[COMMAND_QUEUE_SIZE];
#include "image_open.inc"
static unsigned shutdown_step,shutdown_ready;
static void _eCdControlF_Pause(void) {assert(shutdown_step++==0);assert(PsyX_IsCDImageInit()==(int)shutdown_ready);}
static void _eCdSpoolerJoin(void) {assert(shutdown_step++==1);assert(PsyX_IsCDImageInit()==(int)shutdown_ready);}
#include "image_shutdown.inc"
#undef fopen
#undef fclose
#undef fseek
#undef ftell
#undef fgetc
#undef ferror
static void reset(const char *path) {
    assert(opens==closes);
    memset(&g_imageFile,0,sizeof g_imageFile);
    PsyX_CDFS_Init(path,1,2352);
    assert(!PsyX_IsCDImageInit() && !PsyX_CD_CheckImageAvailable());
    g_cdNumFrames=0;g_cdSectorSize=2352;seek_calls=fail_seek=0;fail_tell=0;
    read_calls=fail_read=0;
    fail_close=0;
    memset(g_cdComQueue,0xa5,sizeof g_cdComQueue);
}
static void failed_init(void) {
    assert(CdInit()==0);assert(g_imageFile.fp==NULL && g_imageFile.basePtr==NULL);
    assert(!PsyX_IsCDImageInit() && !PsyX_CD_CheckImageAvailable());
    assert(opens==closes && g_cdNumFrames==0);
    assert(g_cdComQueue[0].processed==0xa5a5a5a5u);
}
static unsigned cue_case(const void*text,size_t size,int expected,unsigned read_failure) {
    FILE*f=fopen(DISC_CUE_FILENAME,"wb");assert(f);
    assert(fwrite(text,1,size,f)==size && fclose(f)==0);
    reset("");g_cdCurrentTrack=77;fail_read=read_failure;
    unsigned before=open_attempts;
    assert(CdInit()==expected);
    assert(PsyX_IsCDImageInit()==expected && PsyX_CD_CheckImageAvailable()==expected);
    if(expected) {assert(g_cdCurrentTrack==1);tracked_close(g_imageFile.fp);g_imageFile.fp=NULL;}
    else {
        assert(open_attempts==before+1); /* Never open data from invalid CUE. */
        assert(g_imageFile.fp==NULL && g_cdNumFrames==0);
        assert(g_cdCurrentTrack==77 && g_cdSectorSize==2352 && !g_cdImageBinaryFileName[0]);
    }
    assert(opens==closes && unlink(DISC_CUE_FILENAME)==0);return 1;
}
static unsigned cue_cases(const char*path) {
    char good[4096],text[8192];unsigned cases=0;
    int len=snprintf(good,sizeof good,"FILE \"%s\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\n",path);
    assert(len>0 && len<(int)sizeof good);
    for(int n=0;n<=len;++n)cases+=cue_case(good,n,n>=len-1,0);
    for(unsigned n=1;n<=(unsigned)len+1;++n)cases+=cue_case(good,len,0,n);
    const char*bad[]={"REM only\n","FILE","FILE \"unterminated","FILE x WAVE\n",
        "TRACK 01 MODE2/2352\n","FILE x BINARY\nTRACK 02 MODE2/2352\nINDEX 01 00:00:00\n",
        "FILE x BINARY\nTRACK 01 MODE1/2352\nINDEX 01 00:00:00\n",
        "FILE x BINARY\nTRACK 01 MODE2/2048\nINDEX 01 00:00:00\n"};
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];++i)cases+=cue_case(bad[i],strlen(bad[i]),0,0);
    const char*extra[]={"TRACK 02 AUDIO\n","INDEX 02 00:00:01\n","PREGAP 00:02:00\n",
        "FILE other.bin BINARY\n","GARBAGE\n","INDEX 01 00:00:00\n"};
    for(unsigned i=0;i<sizeof extra/sizeof extra[0];++i) {
        int n=snprintf(text,sizeof text,"%s%s",good,extra[i]);cases+=cue_case(text,n,0,0);
    }
    memcpy(text,good,len);text[10]=0;cases+=cue_case(text,len,0,0);
    memset(text,'X',sizeof text);cases+=cue_case(text,sizeof text,0,0);
    int n=snprintf(text,sizeof text," \tREM comment\r\n%sREM trailing",good);
    cases+=cue_case(text,n,1,0);
    n=snprintf(text,sizeof text,"FILE \"%s\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:01\n",path);
    cases+=cue_case(text,n,0,0);
    FILE*spaced=fopen("image with spaces.bin","wb");assert(spaced);
    assert(ftruncate(fileno(spaced),4704)==0 && fclose(spaced)==0);
    const char*spacecue="FILE \"image with spaces.bin\" BINARY\r\nTRACK 1 MODE2/2352\r\nINDEX 1 00:00:00";
    cases+=cue_case(spacecue,strlen(spacecue),1,0);assert(unlink("image with spaces.bin")==0);
    FILE*f=fopen(DISC_CUE_FILENAME,"wb");assert(f);
    assert(fwrite(good,1,len,f)==(size_t)len && fclose(f)==0);
    reset("");fail_close=1;assert(CdInit()==0 && !g_imageFile.fp && !g_cdImageBinaryFileName[0]);
    assert(opens==closes && unlink(DISC_CUE_FILENAME)==0);++cases;
    return cases;
}
int main(int argc,char **argv) {
    char path[]="/var/tmp/xeno-image-open.XXXXXX";int fd=mkstemp(path);assert(fd>=0);
    reset("/var/tmp/xeno-image-open-absent/no.bin");failed_init();
    const long sizes[]={0,1,2351,2352,4704,4705};
    unsigned cases=1;
    for(unsigned i=0;i<sizeof sizes/sizeof sizes[0];++i) {
        assert(ftruncate(fd,sizes[i])==0);reset(path);
        if(sizes[i]<2352)failed_init();
        else {
            assert(CdInit()==1 && g_cdNumFrames==sizes[i]/2352);
            assert(PsyX_IsCDImageInit() && PsyX_CD_CheckImageAvailable());
            FILE *first=g_imageFile.fp;unsigned before=opens;
            PsyX_CDFS_Init(path,1,2352);
            assert(!PsyX_IsCDImageInit() && !PsyX_CD_CheckImageAvailable());
            assert(CdInit()==1 && g_imageFile.fp==first && opens==before);
            assert(g_cdComQueue[127].processed==1);
            tracked_close(g_imageFile.fp);g_imageFile.fp=NULL;
        }
        ++cases;
    }
    assert(ftruncate(fd,4704)==0);
    for(unsigned n=1;n<=2;++n) {reset(path);fail_seek=n;failed_init();++cases;}
    reset(path);fail_tell=1;failed_init();++cases;
    for(int size=-1;size<=0;++size) {reset(path);g_cdSectorSize=size;failed_init();++cases;}
    assert(ftruncate(fd,(off_t)INT_MAX+1)==0);reset(path);failed_init();++cases;
    /* Borrowed memory must remain borrowed, and invalid lengths must not form
     * out-of-bounds end pointers before validation. */
    static u_char bytes[4704];
    for(unsigned n=0;n<=4704;n+=2352) {
        reset("_MEMORY");PsyX_CDFS_Init_Mem((const u_int*)bytes,n,1,2352);
        assert(!PsyX_IsCDImageInit() && !PsyX_CD_CheckImageAvailable());
        unsigned before=opens;
        assert(CdInit()==(n!=0));assert(g_imageFile.basePtr==bytes && g_imageFile.fp==NULL);
        assert(PsyX_IsCDImageInit()==(n!=0) && PsyX_CD_CheckImageAvailable()==(n!=0));
        assert(g_imageFile.curPtr==bytes && opens==before && opens==closes);
        ++cases;
    }
    reset("_MEMORY");PsyX_CDFS_Init_Mem((const u_int*)bytes,-1,1,2352);
    assert(CdInit()==0 && g_imageFile.curPtr==bytes);++cases;
    assert(!PsyX_IsCDImageInit() && !PsyX_CD_CheckImageAvailable());
    reset("_MEMORY");PsyX_CDFS_Init_Mem((const u_int*)bytes,sizeof bytes,1,2352);
    assert(CdInit()==1 && PsyX_CD_CheckImageAvailable());
    g_cdSectorSize=0;
    assert(CdInit()==0 && !PsyX_IsCDImageInit() && !PsyX_CD_CheckImageAvailable());++cases;
    g_cdSectorSize=2352;
    assert(CdInit()==1 && PsyX_CD_CheckImageAvailable());
    PsyX_CDFS_Init_Mem((const u_int*)bytes,sizeof bytes,1,2352);
    assert(!PsyX_IsCDImageInit() && !PsyX_CD_CheckImageAvailable());
    assert(CdInit()==1 && PsyX_CD_CheckImageAvailable());++cases;
    for(shutdown_ready=1;;shutdown_ready=0) {
        shutdown_step=0;PsyX_CD_Shutdown();
        assert(shutdown_step==2 && !PsyX_IsCDImageInit() && !PsyX_CD_CheckImageAvailable());
        assert(g_imageFile.basePtr==bytes && opens==closes);++cases;
        if(!shutdown_ready)break;
    }
    assert(CdInit()==1 && PsyX_CD_CheckImageAvailable());
    g_UseCDImage=0;
    assert(CdInit()==1 && !PsyX_IsCDImageInit() && !PsyX_CD_CheckImageAvailable());++cases;
    assert(ftruncate(fd,4704)==0);
    cases+=cue_cases(path);
    for(unsigned missing=0;missing<2;++missing) {
        FILE*cue=fopen(DISC_CUE_FILENAME,"wb");assert(cue);
        assert(fprintf(cue,"FILE \"%s\" BINARY\n  TRACK 01 MODE2/2352\n  INDEX 01 00:00:00\n",
            missing?"/var/tmp/xeno-image-open-absent/no.bin":path)>0);
        assert(fclose(cue)==0);reset("");
        if(missing)failed_init();
        else {assert(CdInit()==1 && g_cdNumFrames==2);tracked_close(g_imageFile.fp);g_imageFile.fp=NULL;}
        assert(unlink(DISC_CUE_FILENAME)==0);++cases;
    }
    assert(close(fd)==0 && unlink(path)==0);
    if(argc==2) {
        unsigned char expected[32],actual[32];
        FILE*reference=fopen(argv[1],"rb");assert(reference);
        assert(fread(expected,1,sizeof expected,reference)==sizeof expected);
        assert(fseek(reference,0,SEEK_END)==0);long size=ftell(reference);
        assert(size>0 && fclose(reference)==0);
        reset(argv[1]);assert(CdInit()==1 && g_cdNumFrames==size/2352);
        assert(vfread(actual,1,sizeof actual,&g_imageFile)==sizeof actual);
        assert(!memcmp(actual,expected,sizeof actual));
        tracked_close(g_imageFile.fp);g_imageFile.fp=NULL;
        assert(opens==closes);++cases;
        printf("PASS supplied image: %ld bytes, %d frames, first 32 bytes agree\n",size,g_cdNumFrames);
    }
    printf("PASS %u image initialization fixtures; real stdio, injected seek/tell failures\n",cases);
}
