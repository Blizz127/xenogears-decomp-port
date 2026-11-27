extern void *LZSSDecompress(void *compressed, void *destination);

void FieldLZSSDecompress(void *context, void *compressed, void *destination)
{
    (void)context;
    (void)LZSSDecompress(compressed, destination);
}
