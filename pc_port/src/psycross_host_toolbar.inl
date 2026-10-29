/* Host overlay hooks.  The old clickable toolbar strip (SAVE | LOAD | RECORD |
 * SPEED | HD2D | BATTLES | GOD and its reserved 34 px) is gone: those
 * functions live on host keys (F9-F12, F6/Backspace) and in the opt-in dev
 * menu (psycross_port_dev_menu.inl).  The game image fills the whole window;
 * these hooks stay as identity functions for the PsyCross patches that call
 * them.  The overlay is drawn after recording readback, so it never enters
 * game captures.  The glyph renderer below is the dev menu's text. */
#include "quick_checkpoint_request.h"
#include "file_menu_notice.h"
#if defined(__GNUC__)
extern "C" int PcPort_GodModeEnabled(void) __attribute__((weak));
extern "C" void PcPort_GodModeToggle(void) __attribute__((weak));
extern "C" int PcPort_RandomBattlesEnabled(void) __attribute__((weak));
extern "C" void PcPort_RandomBattlesToggle(void) __attribute__((weak));
extern "C" int PcPort_FeiHd2dEnabled(void) __attribute__((weak));
extern "C" void PcPort_FeiHd2dToggle(void) __attribute__((weak));
extern "C" void PcPort_CheatQueue(const char* line) __attribute__((weak));
#endif

/* The native File/card screen has not been implemented.  The policy (dispatch
 * the notice off the game thread, never call the modal dialog inline) lives in
 * pc_port/src/file_menu_notice.c so it can be unit tested; this file supplies
 * the SDL-backed platform hooks.
 *
 * SDL_ShowSimpleMessageBox is MODAL: it runs the native dialog and does not
 * return until the user dismisses it.  Calling it from the field menu loop is a
 * hard hang whenever the dialog cannot be surfaced - with no window manager
 * mapping the zenity window onto the game's display, the game thread never
 * returns, no frame is drawn and no input is ever read again.  That is exactly
 * what the Blackmoon Forest route hit: selecting File froze the field at
 * (-496,0,-1276) with two byte-identical screenshots 25 s apart and the stack
 * parked in SDL_Zenity_ShowMessageBox. */
static void PsyX_FileMenuNoticeDialog(const char *title, const char *text)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title, text, g_window);
}

static void *PsyX_FileMenuNoticeAlloc(unsigned long size)
{
    return SDL_malloc((size_t)size);
}

static void PsyX_FileMenuNoticeFree(void *memory)
{
    SDL_free(memory);
}

static void PsyX_FileMenuNoticeLog(const char *message)
{
    eprintinfo("%s", message);
}

static int PsyX_FileMenuNoticeTrampoline(void *context)
{
    PcPort_FileMenuNoticeThreadMain(context);
    return 0;
}

static int PsyX_FileMenuNoticeStartThread(void (*entry)(void *), void *context)
{
    SDL_Thread *thread;

    (void)entry; /* the host always runs the policy's thread body */
    thread = SDL_CreateThread(PsyX_FileMenuNoticeTrampoline,
                              "fileMenuNotice", context);
    if (thread == NULL) {
        eprinterr("File menu notice thread failed: %s\n", SDL_GetError());
        return 0;
    }
    SDL_DetachThread(thread);
    return 1;
}

/* Installed for the field menu; the policy checks every hook for NULL. */
void *(*PcPort_FileMenuNoticeAlloc)(unsigned long size) =
    PsyX_FileMenuNoticeAlloc;
void (*PcPort_FileMenuNoticeFree)(void *memory) = PsyX_FileMenuNoticeFree;
void (*PcPort_FileMenuNoticeLog)(const char *message) = PsyX_FileMenuNoticeLog;
int (*PcPort_FileMenuNoticeStartThread)(void (*entry)(void *), void *context) =
    PsyX_FileMenuNoticeStartThread;
void (*PcPort_FileMenuNoticeDialog)(const char *title, const char *text) =
    PsyX_FileMenuNoticeDialog;

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && \
	(defined(RENDERER_OGL) || defined(RENDERER_OGLES))

static int PsyX_PortDevMenuHandleEvent(SDL_Event *event);
static void PsyX_PortDevMenuDraw();

static int PsyX_HostToolbarContentHeight(int windowHeight) { return windowHeight; }
static int PsyX_HostToolbarWarpY(int gameY) { return gameY; }

/* No pixels are reserved above the game image any more. */
extern "C" int PsyX_HostToolbarReservedHeight(void) { return 0; }

static void PsyX_PortDevMenuInstallHost();
static void PsyX_HostToolbarInitialise() { PsyX_PortDevMenuInstallHost(); }

static int PsyX_HostToolbarHandleEvent(SDL_Event* event)
{
	return PsyX_PortDevMenuHandleEvent(event);
}

static const unsigned char* PsyX_HostToolbarGlyph(char ch)
{
	static const unsigned char glyphA[7] = {14, 17, 17, 31, 17, 17, 17};
	static const unsigned char glyphB[7] = {30, 17, 17, 30, 17, 17, 30};
	static const unsigned char glyphJ[7] = {7, 2, 2, 2, 18, 18, 12};
	static const unsigned char glyphK[7] = {17, 18, 20, 24, 20, 18, 17};
	static const unsigned char glyphM[7] = {17, 27, 21, 21, 17, 17, 17};
	static const unsigned char glyphQ[7] = {14, 17, 17, 17, 21, 18, 13};
	static const unsigned char glyphU[7] = {17, 17, 17, 17, 17, 17, 14};
	static const unsigned char glyphY[7] = {17, 17, 10, 4, 4, 4, 4};
	static const unsigned char glyphZ[7] = {31, 1, 2, 4, 8, 16, 31};
	static const unsigned char glyphDash[7] = {0, 0, 0, 31, 0, 0, 0};
	static const unsigned char glyphLParen[7] = {2, 4, 8, 8, 8, 4, 2};
	static const unsigned char glyphRParen[7] = {8, 4, 2, 2, 2, 4, 8};
	static const unsigned char glyphGreater[7] = {8, 4, 2, 1, 2, 4, 8};
	static const unsigned char glyphColon[7] = {0, 12, 12, 0, 12, 12, 0};
	static const unsigned char glyphSlash[7] = {1, 1, 2, 4, 8, 16, 16};
	static const unsigned char glyphPlus[7] = {0, 4, 4, 31, 4, 4, 0};
	static const unsigned char glyphDot[7] = {0, 0, 0, 0, 0, 12, 12};
	static const unsigned char glyphApos[7] = {4, 4, 8, 0, 0, 0, 0};
	static const unsigned char glyphPercent[7] = {25, 25, 2, 4, 8, 19, 19};
	static const unsigned char glyphC[7] = {14, 17, 16, 16, 16, 17, 14};
	static const unsigned char glyphD[7] = {30, 17, 17, 17, 17, 17, 30};
	static const unsigned char glyphE[7] = {31, 16, 16, 30, 16, 16, 31};
	static const unsigned char glyphF[7] = {31, 16, 16, 30, 16, 16, 16};
	static const unsigned char glyphG[7] = {14, 17, 16, 23, 17, 17, 14};
	static const unsigned char glyphH[7] = {17, 17, 17, 31, 17, 17, 17};
	static const unsigned char glyphN[7] = {17, 25, 25, 21, 19, 19, 17};
	static const unsigned char glyphI[7] = {31, 4, 4, 4, 4, 4, 31};
	static const unsigned char glyphL[7] = {16, 16, 16, 16, 16, 16, 31};
	static const unsigned char glyphO[7] = {14, 17, 17, 17, 17, 17, 14};
	static const unsigned char glyphP[7] = {30, 17, 17, 30, 16, 16, 16};
	static const unsigned char glyphR[7] = {30, 17, 17, 30, 20, 18, 17};
	static const unsigned char glyphS[7] = {15, 16, 16, 14, 1, 1, 30};
	static const unsigned char glyphT[7] = {31, 4, 4, 4, 4, 4, 4};
	static const unsigned char glyphV[7] = {17, 17, 17, 17, 17, 10, 4};
	static const unsigned char glyphW[7] = {17, 17, 17, 17, 21, 21, 10};
	static const unsigned char glyphX[7] = {17, 17, 10, 4, 10, 17, 17};
	static const unsigned char digits[10][7] = {
		{14, 17, 19, 21, 25, 17, 14},
		{4, 12, 4, 4, 4, 4, 14},
		{14, 17, 1, 2, 4, 8, 31},
		{30, 1, 1, 14, 1, 1, 30},
		{2, 6, 10, 18, 31, 2, 2},
		{31, 16, 16, 30, 1, 1, 30},
		{14, 16, 16, 30, 17, 17, 14},
		{31, 1, 2, 4, 8, 8, 8},
		{14, 17, 17, 14, 17, 17, 14},
		{14, 17, 17, 15, 1, 1, 14}
	};
	if (ch >= '0' && ch <= '9') return digits[ch - '0'];

	switch (ch) {
	case 'A': return glyphA;
	case 'B': return glyphB;
	case 'J': return glyphJ;
	case 'K': return glyphK;
	case 'M': return glyphM;
	case 'Q': return glyphQ;
	case 'U': return glyphU;
	case 'Y': return glyphY;
	case 'Z': return glyphZ;
	case '-': return glyphDash;
	case '(': return glyphLParen;
	case ')': return glyphRParen;
	case '>': return glyphGreater;
	case ':': return glyphColon;
	case '/': return glyphSlash;
	case '+': return glyphPlus;
	case '.': return glyphDot;
	case '\'': return glyphApos;
	case '%': return glyphPercent;
	case 'C': return glyphC;
	case 'D': return glyphD;
	case 'E': return glyphE;
	case 'F': return glyphF;
	case 'G': return glyphG;
	case 'H': return glyphH;
	case 'N': return glyphN;
	case 'I': return glyphI;
	case 'L': return glyphL;
	case 'O': return glyphO;
	case 'P': return glyphP;
	case 'R': return glyphR;
	case 'S': return glyphS;
	case 'T': return glyphT;
	case 'V': return glyphV;
	case 'W': return glyphW;
	case 'X': return glyphX;
	default: return NULL;
	}
}

static void PsyX_HostToolbarClearRect(int x, int y, int width, int height,
	float red, float green, float blue)
{
	glScissor(x, y, width, height);
	glClearColor(red, green, blue, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

#include "psycross_port_dev_menu.inl"

static void PsyX_HostToolbarDraw()
{
	PsyX_PortDevMenuDraw();
}

static void PsyX_HostToolbarShutdown() {}

#else

static int PsyX_HostToolbarContentHeight(int height) { return height; }
static int PsyX_HostToolbarWarpY(int y) { return y; }
static void PsyX_HostToolbarInitialise() {}
static int PsyX_HostToolbarHandleEvent(SDL_Event*) { return 0; }
static void PsyX_HostToolbarDraw() {}
static void PsyX_HostToolbarShutdown() {}

#endif
