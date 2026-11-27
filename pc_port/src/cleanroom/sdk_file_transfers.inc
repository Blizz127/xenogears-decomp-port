/* Host descriptor transfers used by the file loader and debug dump callers.
 * libsn_pcread_retail_test.c specifies the maximum transaction size and the
 * first-short-transfer rule. pc_file_io_test.c pins the shared binary ABI. */
#include <errno.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

static int pc_file_transfer(int fd, char *bytes, int length, int writing)
{
    enum { transaction_limit = 0x8000 };
    int offset = 0;

    if (length == 0)
        return 0;
    if (length < 0) {
        errno = EINVAL;
        return -1;
    }
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }
    if (bytes == NULL) {
#ifdef _WIN32
        errno = EINVAL;
#else
        errno = EFAULT;
#endif
        return -1;
    }

    while (offset < length) {
        int count = length - offset;
        if (count > transaction_limit)
            count = transaction_limit;
#ifdef _WIN32
        int result = writing ? _write(fd, bytes + offset, (unsigned)count)
                             : _read(fd, bytes + offset, (unsigned)count);
#else
        ssize_t result = writing ? write(fd, bytes + offset, (size_t)count)
                                 : read(fd, bytes + offset, (size_t)count);
#endif
        if (result < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        offset += (int)result;
        if (result < count)
            break;
    }
    return offset;
}

int PCread(int fd, char *bytes, int length)
{
    return pc_file_transfer(fd, bytes, length, 0);
}

int PCwrite(int fd, char *bytes, int length)
{
    return pc_file_transfer(fd, bytes, length, 1);
}
