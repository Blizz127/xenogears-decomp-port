/* Ordinary field title overlay, native preview. See the public sibling
 * ShopMenuUpdateShopModeSelectionMenu for the atlas pairs and active ball;
 * func_800799D4 supplies archive directory 0x10 entry 1 and its field
 * snapshot at (704,256). All art, layout tables and strings come from the
 * user's installed disc. No movie playback is required to render this menu. */
#include <string.h>
#include "system/memory.h"

extern void* D_8005945C;
extern u8 D_801EA1D4[], D_801EA530[], D_801E9E84[];
extern u8 D_801E9A00[], D_801E9A2C[];
extern s32 ArchiveDecodeAlignedSize(u32);
extern void* LZSSHeapDecompress(void*, int);
extern void func_8002DD20(u32*);
extern s32 func_8002675C(u8*, s32, void*, s32, s32, s32, s32);
extern void* GetStringEntry(void*, s32);
extern s32 SystemRenderStringEntry(void*, void*, s32, s32);
extern void SystemTransferPaletteToVRAM(short, short);
extern int Vsync(int);
extern void GR_StoreFrameBufferImmediate(int, int, int, int);
extern int PsyX_XenoRenderedClipMisses(int, int, int, int);
extern void PcPort_QuickCheckpointNoticeSet(const char*);

enum { PREVIEW_TITLE_OPTIONS = 3,
       PREVIEW_TITLE_MAX_SPRITES = 28 };

static struct {
    u8* atlas;
    u32 atlasSize;
    u8 labelWidth[PREVIEW_TITLE_OPTIONS];
    int lastPage;
} pcPreviewTitleDraw;

static u32 PcPreviewTitleWord(const void* base, size_t word)
{
    u32 value;
    memcpy(&value, (const u8*)base + word * sizeof(value), sizeof(value));
    return value;
}

static u16 PcPreviewTitleHalf(const u8* address)
{
    u16 value;
    memcpy(&value, address, sizeof(value));
    return value;
}

static void PcPreviewTitleReleaseArchive(void)
{
    if (D_8005945C) {
        HeapFree(D_8005945C);
        D_8005945C = NULL;
    }
}

/* Read relative offsets without rewriting guest-width archive pointers. */
static void* PcPreviewTitleResource(u8* archive, u32 size, u32 slot, u32* outputSize)
{
    u32 offset;
    if (size < 4 || slot > PcPreviewTitleWord(archive, 0) || slot >= size / 4)
        return NULL;
    offset = PcPreviewTitleWord(archive, slot);
    if ((offset & 3) || offset > size - 4)
        return NULL;
    *outputSize = PcPreviewTitleWord(archive + offset, 0);
    if (!*outputSize || *outputSize > 0x200000)
        return NULL;
    return LZSSHeapDecompress(archive + offset, 1);
}

/* Match the system font pair packing: the second string occupies the other
 * palette bitplane. One 28-word by 13-row texture contains both strings.
 * Each two-string group occupies its own 128-pixel half of the texture page. */
static void PcPreviewTitlePrepareLabels(u8* strings, u32 size)
{
    SystemTransferPaletteToVRAM(0, 0x1D1);
    for (unsigned first = 0; first < PREVIEW_TITLE_OPTIONS; first += 2) {
        u32 pixels[0x100] = {0};
        RECT dest = {320 + (int)(first / 2) * 32, 0, 28, FONT_LETTER_HEIGHT};
        for (unsigned row = first; row < first + 2 && row < PREVIEW_TITLE_OPTIONS; row++) {
            unsigned id = D_801EA530[row];
            if (4 + 2 * id + 2 <= size && PcPreviewTitleHalf(strings + 4 + 2 * id) < size) {
                int width = SystemRenderStringEntry(
                    GetStringEntry(strings, id), pixels, 0x18, row % 2);
                if (width > 0 && width <= dest.w * 4)
                    pcPreviewTitleDraw.labelWidth[row] = (u8)width;
            }
        }
        LoadImage(&dest, (void*)pixels);
        DrawSync(0);
    }
}

static void PcPreviewTitleDrawBegin(void)
{
    u8* archive = D_8005945C;
    s32 size = archive ? ArchiveDecodeAlignedSize(1) : 0;
    memset(&pcPreviewTitleDraw, 0, sizeof(pcPreviewTitleDraw));
    pcPreviewTitleDraw.lastPage = -1;
    if (size > 0) {
        u32 dataSize = 0;
        u32* images = PcPreviewTitleResource(archive, size, 2, &dataSize);
        if (images) {
            func_8002DD20(images);
            DrawSync(0);
            HeapFree(images);
            pcPreviewTitleDraw.atlas = PcPreviewTitleResource(
                archive, size, 3, &pcPreviewTitleDraw.atlasSize);
        }
        u8* strings = PcPreviewTitleResource(archive, size, 4, &dataSize);
        if (strings) {
            PcPreviewTitlePrepareLabels(strings, dataSize);
            HeapFree(strings);
        }
    }
    PcPreviewTitleReleaseArchive();
    if (!pcPreviewTitleDraw.atlas)
        PcPort_QuickCheckpointNoticeSet("TITLE MENU GRAPHICS UNAVAILABLE");
}

/* The public native atlas builder writes alternating 40-byte POLY_FT4s,
 * two contexts per piece. Bound the run before calling it. DrawPrim consumes
 * the native packet directly, without encoding a host pointer in a PSX OT. */
static void PcPreviewTitleSprite(u32 id, int x, int y)
{
    POLY_FT4 packets[2 * PREVIEW_TITLE_MAX_SPRITES];
    const u32 size = pcPreviewTitleDraw.atlasSize;
    u8* atlas = pcPreviewTitleDraw.atlas;
    if (!atlas || size < 6 || id > (size - 6) / 2)
        return;
    u32 offset = PcPreviewTitleHalf(atlas + 4 + id * 2);
    if (offset > size - 4)
        return;
    unsigned pieces = PcPreviewTitleHalf(atlas + offset);
    if (pieces == 0 || pieces > PREVIEW_TITLE_MAX_SPRITES ||
        pieces > (size - offset - 4) / 28)
        return;
    func_8002675C(atlas, id, packets, 0, x, y, 0x1000);
    for (unsigned i = 0; i < pieces; i++)
        DrawPrim(&packets[2 * i]);
}

/* func_801D1EE0's settled title selection uses a 76 by 16 label bar.
 * The opaque green perimeter surrounds a semi-transparent yellow-to-black
 * gradient. The pointer and selected text are submitted on top. */
static void PcPreviewTitleHighlight(int cursorX, int cursorY)
{
    int left = cursorX + 20;
    int top = cursorY - 36;
    int right = left + 76;
    int bottom = top + 16;
    RECT window = {0, 0, 256, 256};
    DR_MODE mode;
    POLY_G4 fill;
    LINE_F3 topRight, leftBottom;

    SetDrawMode(&mode, 0, 0, GetTPage(0, 0, 320, 128), &window);
    DrawPrim(&mode);
    SetPolyG4(&fill);
    SetSemiTrans(&fill, 1);
    setRGB0(&fill, 128, 128, 0);
    setRGB1(&fill, 128, 128, 0);
    setRGB2(&fill, 0, 0, 0);
    setRGB3(&fill, 0, 0, 0);
    setXY4(&fill, left, top, right, top, left, bottom, right, bottom);
    DrawPrim(&fill);

    SetLineF3(&topRight);
    setRGB0(&topRight, 0, 64, 0);
    setXY3(&topRight, left, top, right, top, right, bottom);
    DrawPrim(&topRight);
    SetLineF3(&leftBottom);
    setRGB0(&leftBottom, 0, 64, 0);
    setXY3(&leftBottom, left, top, left, bottom, right, bottom);
    DrawPrim(&leftBottom);
}

static void PcPreviewTitleDrawChoice(unsigned choice)
{
    /* The disc supplies the anchor-relative geometry for every component of
     * the orbs and icons. 160/150 is the public selection-menu atlas origin. */
    for (unsigned row = 0; row < PREVIEW_TITLE_OPTIONS; row++) {
        u32 orb = PcPreviewTitleWord(D_801EA1D4, row * 2);
        u32 icon = PcPreviewTitleWord(D_801EA1D4, row * 2 + 1);
        PcPreviewTitleSprite(orb + (row == choice ? 0xD : 0), 160, 150);
        PcPreviewTitleSprite(icon, 160, 150);
    }

    /* The title picker positions both the pointer and its label using the
     * visible choice. The +7 title action offset belongs only to dispatch. */
    int px = (s16)PcPreviewTitleWord(D_801E9A00, choice);
    int py = (s16)PcPreviewTitleWord(D_801E9A2C, choice);
    if (pcPreviewTitleDraw.atlas)
        PcPreviewTitleHighlight(px, py);
    PcPreviewTitleSprite(MENU_TEX_POINTER_CURSOR, px, py);

    int width = pcPreviewTitleDraw.labelWidth[choice];
    if (width) {
        POLY_FT4 label;
        int x = px + (s16)PcPreviewTitleWord(D_801E9E84, choice) + 22;
        int y = py - 34;
        int u = (choice / 2) * 128;
        SetPolyFT4(&label);
        SetShadeTex(&label, 0);
        SetSemiTrans(&label, 0);
        setRGB0(&label, 128, 128, 128);
        setXY4(&label, x, y, x + width, y, x, y + FONT_LETTER_HEIGHT,
               x + width, y + FONT_LETTER_HEIGHT);
        setUV4(&label, u, 0, u + width, 0, u, FONT_LETTER_HEIGHT,
               u + width, FONT_LETTER_HEIGHT);
        label.tpage = GetTPage(0, 0, 320, 0);
        label.clut = GetClut(16 * (choice % 2), 0x1D1);
        DrawPrim(&label);
    }
}

static void PcPreviewTitleDrawFrame(int choice)
{
    RECT backdrop = {704, 256, 320, 224};
    GfxEnvironment* env = g_Menu->pGfxEnv == &g_Menu->gfxEnvs[0] ?
        &g_Menu->gfxEnvs[1] : &g_Menu->gfxEnvs[0];
    g_Menu->pGfxEnv = env;
    g_Menu->renderContext = g_Menu->renderContext == 0;
    DrawSync(0);
    Vsync(0);
    PutDrawEnv(&env->drawEnv);
    PutDispEnv(&env->dispEnv);
    MoveImage(&backdrop, 0, env->drawEnv.clip.y);
    if ((unsigned)choice < PREVIEW_TITLE_OPTIONS)
        PcPreviewTitleDrawChoice((unsigned)choice);
    pcPreviewTitleDraw.lastPage = env->drawEnv.clip.y;
}

static void PcPreviewTitleDrawEnd(void)
{
    /* The field caller always reads back (0,224), including on a quick
     * checkpoint handoff. Commit a pending host GL draw before that copy. */
    if (pcPreviewTitleDraw.lastPage >= 0) {
        RECT page = {0, pcPreviewTitleDraw.lastPage, 320, 224};
        DrawSync(0);
        if (!PsyX_XenoRenderedClipMisses(0, page.y, page.w, page.h))
            GR_StoreFrameBufferImmediate(0, page.y, page.w, page.h);
        if (page.y != 224)
            MoveImage(&page, 0, 224);
        DrawSync(0);
    }
    if (pcPreviewTitleDraw.atlas) {
        HeapFree(pcPreviewTitleDraw.atlas);
        pcPreviewTitleDraw.atlas = NULL;
    }
}
