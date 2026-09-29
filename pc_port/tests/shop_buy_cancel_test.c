/* Execute the production Buy caller, row builder, selected-item and Stored
 * routines together. Other helpers are isolated; ordinary Back exits without
 * a transaction. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "main/game.h"
static SystemMenu menu;
static MenuShop shop;
static MenuUnk6 resources;
static MenuShopItem items[256];
static u8 work[0x618], text[16];
SystemMenu* g_Menu=&menu;
static void require(int ok, const char* reason) {
 if(!ok) {fprintf(stderr,"SHOP BUY CANCEL FAIL: %s\n",reason);exit(1);}
}
void* HeapAlloc(u_int size,u_int flags) { (void)flags; require(size<=sizeof(work),"allocation size");return work; }
u_int HeapFree(void* p) { require(p==work,"free work");return 0; }
void* GetWeaponName(s32 id) {(void)id;return text;}
void* GetAccessoryName(s32 id) {(void)id;return text;}
void* GetItemName(s32 id) {(void)id;return text;}
void* GetStringEntry(void* bundle,s32 id) {(void)bundle;(void)id;return text;}
s32 SystemRenderStringEntry(void* str,void* buffer,s32 width,s32 plane) {(void)str;(void)width;(void)plane;require(buffer==work,"text buffer");return 16;}
void func_80033B34(u16* src,u8* dst,s32 n) {memcpy(dst,src,n*2);}
int LoadImage(RECT* rect,u_long* data) {(void)rect;require((void*)data==work,"upload");return 0;}
int DrawSync(int mode) {(void)mode;return 0;}
void func_801C5A7C(MenuString* p,s32 i,s32 off,u8 f) {(void)p;(void)i;(void)off;(void)f;}
void ShopMenuSetVertices(SVECTOR* p,u16 x,u16 y,u16 w,u16 h) {(void)p;(void)x;(void)y;(void)w;(void)h;}
s32 func_8002675C(void* a,s32 id,POLY_FT4* p,s32 ctx,s32 x,s32 y,s32 scale) {(void)a;(void)id;(void)p;(void)ctx;(void)x;(void)y;(void)scale;return 1;}
void func_801C5040(POLY_FT4* p,short x,short y,u_char u,u_char v,short w,short h) {(void)p;(void)x;(void)y;(void)u;(void)v;(void)w;(void)h;}
u_short ShopMenuGetCharacterEquippedItemFlags(u_char id,u_char type) {(void)id;(void)type;return 0;}
u_short ShopMenuIsCharacterFlagSet(u_short flags,u_char id) {return flags & (1u<<id);}
void func_801CE480(s32* d,u8* c,u8 id,u8 type,u8 character) {(void)d;(void)c;(void)id;(void)type;(void)character;require(0,"consumable has no stat preview");}
void ShopMenuSetStatChangeColor(int n,POLY_FT4* p,u_char c) {(void)n;(void)p;(void)c;require(0,"consumable has no stat colors");}
s32 D_801D21CC[9];
GameState g_GameState;
int D_801D1F50 = 8;
s32 D_801D1FD4[8], D_801D1FE8[8];
u16 D_801D2260;
static MenuManager manager;
static MenuSelectionMenu selection;
static unsigned updates;
extern u_char ShopMenuBuyMenu(void);
void func_801CCE1C(void* output, u8 id) { (void)output; (void)id; }
void ShopMenuUpdateAndRender(void) { ++updates; menu.input = MENU_INPUT_BACK; }
#define NOOP(name) void name(void) {}
NOOP(ShopMenuInitializeArrowCursor)
NOOP(ShopMenuUpdateScrollBarHandle)
NOOP(ShopMenuUpdateArrowCursor)
NOOP(func_801CBC88)
NOOP(func_801CBCF0)
NOOP(ShopMenuInitializeWindow)
NOOP(ShopMenuStartOpenMenuTransition)
NOOP(ShopMenuUpdateCharacterPortraits)
NOOP(ShopMenuUpdateBuyMenuExplanationGraphics)
NOOP(ShopMenuUpdateGoldGraphics)
NOOP(ShopMenuPlaySoundEffect)
NOOP(ShopMenuInitializePointerCursors)
NOOP(ShopMenuFreePointerCursors)
NOOP(ShopMenuParseNumberToString)
int ShopMenuConfirmationWindow(void) { require(0,"unexpected confirmation"); return 0; }
int main(void) {
 menu.pShop=&shop; menu.unk330=&resources; menu.pManager=&manager;
 menu.pSelectionMenu=&selection; menu.unk2DC=text;
 resources.pItemsData=items; items[1].price=20;
 for (unsigned i=0;i<8;i++) { menu.shopItemIDs[i]=1; menu.shopItemTypes[i]=2; }
 g_GameState.gold=300;
 require(ShopMenuBuyMenu()==TRUE,"normal cancel returns");
 require(updates==2,"one frame and exit update");
 require(g_GameState.gold==300,"cancel preserves gold");
 for(unsigned i=0;i<MAX_SHOP_ITEMS;i++) require(shop.curItemQuantities[i]==0,"no purchase");
 require(D_801D2260==0 && shop.strItemDesc.width==16,"production selected/stored chain");
 puts("SHOP BUY CANCEL PASS: production caller, row builder, selected and stored chain");
 return 0;
}
