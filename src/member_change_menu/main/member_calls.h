/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#ifndef XENO_MEMBER_CHANGE_CALLS_H
#define XENO_MEMBER_CHANGE_CALLS_H

/* Game services used by the member-change overlay. */
int ArchiveCdDataSync(int mode);
int ArchiveDecodeAlignedSize(unsigned int entryIndex);
s32 ArchiveReadFileToBuffer(s32 entry, void *destination, u32 argument, u32 flags);
int ArchiveSetIndex(int directoryIndex, int entryIndex);
int ControllerGetType(int controllerIndex);
int ControllerPopState(void);
void ControllerResetState(void);
void GameCheckAndHandleSoftReset(void);
void *GetStringEntry(void *arg0, s32 arg1);
void *HeapAlloc(u_int bytes, u_int flags);
u_int HeapFree(void *payload);
void *LZSSHeapDecompress(void *pCompressed, int flags);
unsigned int ResolveArchiveEntryPointers(u32 *pFile);
void SoundAddSedsEntry(SoundFile *record);
void SoundEnableAllSpuChannels(void);
void SoundMuteAllSpuChannels(void);
s32 SystemRenderStringEntry(void *pString, void *pWork, s32 height, s32 flag);
void SystemTransferPaletteToVRAM(short x, short y);
int Vsync(int mode);
void func_80026338(u8 *atlas, s32 index, u32 *count, s32 *depth, s32 *paletteX, s32 *paletteY,
    s32 *textureX, s32 *textureY);
s32 func_800263E4(u8 *table, s32 index, void *packets, s32 buffer, s32 x, s32 y, s32 scale,
    s32 flipX, s32 flipY);
s32 func_8002675C(u8 *table, s32 index, void *polys, s32 renderCtx, s32 x, s32 y, s32 scale);
void func_8002DD20(u32 *pList);
s32 func_80036410(void);
void func_8003852C(void *pSed);
void func_80039DB8(s32 packedId);
void func_8003A094(SoundFile *pFile);

/* Entry points shared by the native menu providers. */
void MemberChangeMenuDrawCursors(void);
void MemberChangeMenuFree(void);
void MemberChangeMenuFreeCursors(void);
void MemberChangeMenuFreeWindow(u_char index);
void MemberChangeMenuInitialize(void);
void MemberChangeMenuInitializeBackground(void);
void MemberChangeMenuInitializeCursors(u_char mode);
void MemberChangeMenuInitializeScrollBar(unsigned char slot, u_short x, u_short y,
    u_short width, u_short height);
void MemberChangeMenuInitializeWindow(u_char slot, u_short x, u_short y, u_short width,
    u_short height, u_char animate, u8 option, int depth, u_char scroll);
void MemberChangeMenuInitializeWindowBorderCorners(unsigned char slot, u_short x, u_short y,
    u_short width, u_short height);
void MemberChangeMenuInitializeWindowBorders(void);
void MemberChangeMenuInitializeWindowGraphics(u_char index);
u_short MemberChangeMenuIsCharacterFlagSet(u_short value, u_char maskIndex);
void MemberChangeMenuLoadResources(void);
void MemberChangeMenuMain(void);
void MemberChangeMenuMainLoop(void);
void MemberChangeMenuParseNumberToString(unsigned int number);
void MemberChangeMenuPlaySoundEffect(u_char soundEffectId);
void MemberChangeMenuPollInput(void);
void MemberChangeMenuRender(void);
void MemberChangeMenuRenderBackgroundDim(void);
void MemberChangeMenuRenderBottomWindowBorder(int slot);
void MemberChangeMenuRenderCharacter(MenuCharacter *character, u_char flag);
void MemberChangeMenuRenderCharacters(void);
void MemberChangeMenuRenderLeftWindowBorder(int slot);
void MemberChangeMenuRenderRightWindowBorder(int slot);
void MemberChangeMenuRenderScrollBar(int slot);
void MemberChangeMenuRenderTopWindowBorder(int slot);
void MemberChangeMenuRenderWindowBackground(int slot);
void MemberChangeMenuRenderWindowBorderCorners(int slot);
void MemberChangeMenuRenderWindows(void);
void MemberChangeMenuResetRenderContext(void);
void MemberChangeMenuSetCharacterHpAndMpStrings(MenuCharacter *character, u_char characterIndex,
    u_char localIndex, int *x, int *y, int verticalSpacing);
void MemberChangeMenuSetCharacterLevelStrings(MenuCharacter *character, u_char characterIndex,
    u_char localIndex, int *x, int *y, int verticalSpacing);
void MemberChangeMenuSetCharacters(u_char isInitialization);
void MemberChangeMenuSetCursorToCharacter(u_char isBenchedCharWindowActive, int characterIndex,
    u_char cursorIndex);
void MemberChangeMenuSetManager(u_char isInitialization);
void MemberChangeMenuSetSelectionMenu(u_char isInitialization);
void MemberChangeMenuSetVertices(SVECTOR *vertices, u_short x, u_short y, u_short width,
    u_short height);
void MemberChangeMenuSetWindow(u_char index, u_short x, u_short y, u_short width,
    u_short height, u_char option, int zIndex, u_char hasScrollBar);
void MemberChangeMenuSetWindowBorderBottom(u_char index, u_short x, u_short y, u_short width,
    u_short height);
void MemberChangeMenuSetWindowBorderLeft(u_char index, u_short x, u_short y, u_short height);
void MemberChangeMenuSetWindowBorderPrimitive(POLY_FT4 *primitive);
void MemberChangeMenuSetWindowBorderRight(u_char index, u_short x, u_short y, u_short width,
    u_short height);
void MemberChangeMenuSetWindowBorderTop(u_char index, u_short x, u_short y, u_short width);
u_char MemberChangeMenuSwapCharacters(u8 direction, s32 current, s32 offset,
    u8 selectedDirection, s32 selected, s32 selectedOffset);
void MemberChangeMenuUpdateAndRender(void);
void MemberChangeMenuUpdateCharacter(MenuCharacter *pCharacter, u_char characterIndex,
    u_char localIndex, int *pPositionsX, int *pPositionsY, int verticalSpacing,
    u_char isPartyCharacter);
void MemberChangeMenuUpdateCharacters(u_char offset);
void MemberChangeMenuUpdateWindows(void);
void func_801C5034(u_char setting);
void func_801C5160(u_char isInitialization);
void func_801C51C4(u_char setting);
void func_801C5228(u_char isInitialization);
void func_801C5724(void);
void func_801C57A0(MenuString *string, int index, s32 yOffset, u32 attributes);
void func_801C59E0(MenuString *records, u8 *indices, s32 offset, s32 count);
void func_801C5B90(void);
void func_801C5CF4(void);
void func_801C5D24(POLY_G4 *pPoly, u_char red, u_char green, u_char blue);
void func_801C7DA8(void);
void func_801C7E38(void);
void func_801C7EC8(void);
void func_801C846C(void);
void func_801C95A0(s32 character, s32 slot);
void func_801C9908(void);
void func_801C9A08(MenuCharacter *character, u_char characterIndex, u_char localIndex,
    int *positionsX, int *positionsY, int verticalSpacing);

#endif
