int ArchiveClearStreamFileSections(void)
{
    int *file = (int *)g_ArchiveCurStreamFile;
    volatile ArchiveStreamFileSectionHeader *first;
    volatile ArchiveStreamFileSectionHeader *section;
    int count;
    int i;

    if (file == 0)
        return -1;

    count = *file;
    first = (volatile ArchiveStreamFileSectionHeader *)(file + 1);
    section = first;
    for (i = 0; i < count; ++i) {
        section->state = 0;
        section->id = 0;
        section->size = 0;
        section->unk2 = 0;
        ++section;
    }
    first->size = (unsigned short)count;
    return count;
}
