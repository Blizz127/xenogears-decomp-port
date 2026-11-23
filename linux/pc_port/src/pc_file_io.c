/* PsyQ PC host-file ABI (include/psyq/libsn.h): integer handles, byte counts.
 * Own the entire family so no handle can cross into PsyCross's FILE* API. */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

int PCinit(void) { return 0; }

int PCopen(char* name, int flags, int perms)
{
    static const int modes[] = {O_RDONLY, O_WRONLY, O_RDWR};
    (void)perms;
    if (flags < 0 || flags > 2) { errno = EINVAL; return -1; }
    return open(name, modes[flags]);
}

int PCcreate(char* name, int perms)
{
    (void)perms;
    return open(name, O_CREAT | O_TRUNC | O_RDWR, 0666);
}

int PCcreat(char* name, int perms) { return PCcreate(name, perms); }
int PCclose(int fd) { return close(fd); }

int PClseek(int fd, int offset, int mode)
{
    off_t position;
    if (mode < 0 || mode > 2) { errno = EINVAL; return -1; }
    position = lseek(fd, (off_t)offset, mode);
    if (position < 0) return -1;
    if (position > INT_MAX) { errno = EOVERFLOW; return -1; }
    return (int)position;
}

/* Retail PCread/PCwrite stop at the first short transfer, and return -1
 * on an error even if an earlier chunk completed. */
static int transfer(int fd, char* buffer, int length, int writing)
{
    int total = 0;
    if (length < 0) { errno = EINVAL; return -1; }
    while (total < length) {
        int chunk = length - total;
        ssize_t count;
        if (chunk > 0x8000) chunk = 0x8000;
        do {
            count = writing ? write(fd, buffer + total, (size_t)chunk)
                            : read(fd, buffer + total, (size_t)chunk);
        } while (count < 0 && errno == EINTR);
        if (count < 0) return -1;
        total += (int)count;
        if (count < chunk) break;
    }
    return total;
}

int PCread(int fd, char* buffer, int length)
{
    return transfer(fd, buffer, length, 0);
}

int PCwrite(int fd, char* buffer, int length)
{
    return transfer(fd, buffer, length, 1);
}
