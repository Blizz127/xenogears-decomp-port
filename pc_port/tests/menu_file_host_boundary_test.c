/* Diagnostic witness only: accidental host-libc binding is NOT acceptance. */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
typedef int32_t s32;
#include "card_file.inc"
int main(void) {
    FILE *fixture=tmpfile(); assert(fixture);
    unsigned char input[512], output[512];
    for(unsigned i=0;i<sizeof input;++i) input[i]=(unsigned char)i;
    assert(fwrite(input,1,sizeof input,fixture)==sizeof input);
    assert(fflush(fixture)==0);
    char path[64]; snprintf(path,sizeof path,"/proc/self/fd/%d",fileno(fixture));
    int fd=open(path,O_RDONLY); assert(fd>=0);
    assert(read(fd,output,sizeof output)==sizeof output);
    assert(!memcmp(input,output,sizeof input)); assert(close(fd)==0);
    fd=open(path,3); assert(fd>=0);
    errno=0; assert(read(fd,output,sizeof output)==-1 && errno==EBADF);
    assert(close(fd)==0);
    memset(output,0xa5,sizeof output);
    assert(func_801C9038(path,output)==-1);
    for(unsigned i=0;i<sizeof output;++i) assert(output[i]==0xa5);
    assert(fclose(fixture)==0);
    puts("WITNESS: readable 512-byte fixture rejected by retail flags 3 through host libc; guest file backend UNRESOLVED");
}
