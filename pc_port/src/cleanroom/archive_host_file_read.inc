char *ArchiveReadHddFile(char *path, int *fileSize)
{
    int fd = -1;
    for (int attempt = 0; attempt < 4 && fd == -1; ++attempt)
        fd = PCopen(path, 0, 0);
    if (fd == -1)
        return (char *)0;

    int length = PClseek(fd, 0, 2);
    if (fileSize)
        *fileSize = length;
    (void)PClseek(fd, 0, 0);
    char *data = HeapAlloc((unsigned int)length, 0);

    if (data) {
        int readResult = 0;
        for (int attempt = 0; attempt < 4 && readResult == 0; ++attempt)
            readResult = PCread(fd, data, length);
        if (readResult == 0) {
            (void)HeapFree(data);
            data = (char *)0;
        }
    }

    for (int attempt = 0; attempt < 4; ++attempt) {
        if (PCclose(fd) == 0)
            return data;
    }
    if (data)
        (void)HeapFree(data);
    return (char *)0;
}
