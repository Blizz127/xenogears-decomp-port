/* FieldLoad battle records: the runner extracts the actual production call,
 * its FieldLZSSDecompress wrapper and production LZSSDecompress unchanged.
 * Oracle: execute retail field 8007104C..70, its wrapper at 8007008C and the
 * complete SLUS decoder at 80032EB4 on the actual Map2 compressed section.
 *
 * The fixture represents the contiguous retail destination, including bytes
 * beyond +0x200 and guards beyond +0x230. Native global/alias ownership remains
 * outside this test; this test does not endorse the current host symbol sizes.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

typedef uint8_t u8;
typedef uint32_t u32;
enum { PSX_RAM_SIZE = 0x200000, MAP_SIZE = 123808, GUARD = 32, DEST_SIZE = 0x230 };
#define RETAIL_DEST 0x800658dcu
#define RETAIL_MAP  0x80100000u
static u8 ram[PSX_RAM_SIZE];
static u8 raw_map[MAP_SIZE];
static u32 map_words[MAP_SIZE / 4];
static struct Buffer { u8 before[GUARD], data[DEST_SIZE], after[GUARD]; } native, retail;
static void *D_8005A4E0 = map_words;
static u8 *const D_800658DC = native.data;
#ifndef FIELD_BATTLE_SOURCE
#error "Run through run_field_battle_record_retail_test.sh"
#endif
#include FIELD_BATTLE_SOURCE

static unsigned writes, first_write, last_write;
static u8 written[DEST_SIZE];

static u8 *address(u32 a, unsigned width)
{
    if (a >= RETAIL_DEST - GUARD &&
        (uint64_t)a + width <= RETAIL_DEST + DEST_SIZE + GUARD)
        return (u8 *)&retail + (a - (RETAIL_DEST - GUARD));
    if (a >= 0x80000000u && (uint64_t)a + width <= 0x80200000u)
        return ram + (a - 0x80000000u);
    return NULL;
}

static int rd(void *unused, u32 a, unsigned width, u32 *value)
{
    (void)unused;
    u8 *p = address(a, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i) *value |= (u32)p[i] << (i * 8);
    return 0;
}

static int wr(void *unused, u32 a, unsigned width, u32 value)
{
    (void)unused;
    u8 *p = address(a, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; ++i) p[i] = (u8)(value >> (i * 8));
    if (a >= RETAIL_DEST && (uint64_t)a + width <= RETAIL_DEST + DEST_SIZE) {
        for (unsigned i = 0; i < width; ++i) {
            unsigned offset = a - RETAIL_DEST + i;
            if (offset < first_write) first_write = offset;
            if (offset > last_write) last_write = offset;
            written[offset] = 1;
            ++writes;
        }
    }
    return 0;
}

static void hex_record(const u8 *record)
{
    for (unsigned i = 0; i < 32; ++i) fprintf(stderr, "%02x", record[i]);
}

static int compare(u8 fill, int relocate_source, const char *oracle_output)
{
    u8 *map = (u8 *)map_words;
    memcpy(map, raw_map, MAP_SIZE);
    u32 source_offset = map_words[0x148 / 4];
    if (relocate_source) {
        memmove(map + 0x2000, map + source_offset, MAP_SIZE - source_offset);
        source_offset = map_words[0x148 / 4] = 0x2000;
    }
    /* Map metadata is 528, while the compressed stream itself declares 530.
     * Neither native nor retail uses the wrapper's first argument as a cap. */
    assert(map_words[0x124 / 4] == 528);
    assert(*(u32 *)(map + source_offset) == 530);
    memcpy(ram + (RETAIL_MAP & 0x1fffff), map, MAP_SIZE);
    assert(!wr(NULL, 0x8005a4e0u, 4, RETAIL_MAP));
    memset(&native, fill, sizeof(native));
    memset(&retail, fill, sizeof(retail));
    memset(written, 0, sizeof(written));
    writes = last_write = 0;
    first_write = DEST_SIZE;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[29] = 0x801ff000;
    int result = PcPortMipsRun(&cpu, 0x8007104c, 0x80071070, 100000);
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "FIELD BATTLE RECORD FAIL oracle pc=%08x: %s\n", cpu.pc, cpu.error);
        exit(2);
    }
    assert(writes == 530 && first_write == 0 && last_write == 529);
    for (unsigned i = 0; i < DEST_SIZE; ++i) assert(written[i] == (i < 530));
    for (unsigned i = 0; i < GUARD; ++i)
        assert(retail.before[i] == fill && retail.after[i] == fill);
    for (unsigned i = 530; i < DEST_SIZE; ++i) assert(retail.data[i] == fill);
    if (oracle_output) {
        FILE *output = fopen(oracle_output, "wb");
        assert(output && fwrite(retail.data, 1, 530, output) == 530);
        assert(!fclose(output));
    }

    RunProductionFieldBattleCall();
    if (memcmp(&native, &retail, sizeof(native))) {
        unsigned different = 0, first = sizeof(native);
        for (unsigned i = 0; i < sizeof(native); ++i) {
            if (((u8 *)&native)[i] != ((u8 *)&retail)[i]) {
                if (first == sizeof(native)) first = i;
                ++different;
            }
        }
        fprintf(stderr, "FIELD BATTLE RECORD FAIL fill=%02x relocated_source=%d first_offset=%d "
                "different_bytes=%u retail_writes=%u; selector1 native=", fill, relocate_source,
                (int)first - GUARD, different, writes);
        hex_record(native.data + 32);
        fputs(" retail=", stderr);
        hex_record(retail.data + 32);
        fputc('\n', stderr);
        return 1;
    }
    return 0;
}

static void read_slice(FILE *image, u32 address, unsigned bytes, u32 file_base)
{
    assert(!fseek(image, address - file_base, SEEK_SET));
    assert(fread(ram + (address & 0x1fffff), 1, bytes, image) == bytes);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    FILE *disc = fopen("disc/disc1.bin", "rb");
    assert(disc);
    for (unsigned offset = 0; offset < MAP_SIZE; offset += 2048) {
        unsigned bytes = MAP_SIZE - offset < 2048 ? MAP_SIZE - offset : 2048;
        assert(!fseek(disc, (121096 + offset / 2048) * 2352L + 24, SEEK_SET));
        assert(fread(raw_map + offset, 1, bytes, disc) == bytes);
    }
    assert(!fclose(disc));
    FILE *image = fopen("disc/field.bin", "rb");
    assert(image);
    read_slice(image, 0x8007104c, 0x24, 0x8006faf0);
    read_slice(image, 0x8007008c, 0x24, 0x8006faf0);
    assert(!fclose(image));
    image = fopen("disc/SLUS_006.64", "rb");
    assert(image);
    read_slice(image, 0x80032eb4, 0xa0, 0x8000f800);
    assert(!fclose(image));
    const u8 fills[] = {0xa5, 0, 0x5a, 0xff};
    unsigned failures = 0, cases = 0;
    for (unsigned i = 0; i < sizeof(fills); ++i)
        for (int relocation = 0; relocation <= 1; ++relocation) {
            failures += compare(fills[i], relocation, cases == 0 ? argv[1] : NULL);
            ++cases;
        }
    if (failures) return 1;
    printf("FIELD BATTLE RECORD PASS %u cases: complete 530-byte retail output, all 16 encounter "
           "records and contiguous tail, surrounding guards, four fills and relocated source; "
           "production call/wrapper/decoder, actual retail instruction oracle\n", cases);
    return 0;
}
