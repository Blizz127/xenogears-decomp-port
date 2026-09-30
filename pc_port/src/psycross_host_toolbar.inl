/* Host-only clickable controls drawn in pixels reserved above the emulated
 * framebuffer. The bar is presented to the player but deliberately rendered
 * after recording readback, so it never contaminates game captures. */
#include "psycross_host_toolbar_logic.h"
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

/* Cheats go through the cheat console (pc_port/src/cheat_console.c): queued
 * here on the host UI thread, executed on the game thread at the next
 * frame_tick.  Falls back to the direct call where the console is absent. */
static void PsyX_HostCheat(const char* command, void (*direct)(void))
{
#if defined(__GNUC__)
	if (PcPort_CheatQueue != NULL) {
		PcPort_CheatQueue(command);
		return;
	}
#endif
	if (direct != NULL)
		direct();
}

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

static int g_xenoHostToolbarActive = 0;
static int PsyX_PortDevMenuHandleEvent(SDL_Event *event);
static void PsyX_PortDevMenuDraw();
static PcPortHostToolbarAction g_xenoHostToolbarHover = PC_PORT_TOOLBAR_NONE;

static PcPortQuickUiState PsyX_HostToolbarQuickUiState()
{
#if defined(__GNUC__)
	if (PcPort_QuickCheckpointGetUiState != NULL)
		return (PcPortQuickUiState)PcPort_QuickCheckpointGetUiState();
#endif
	return PC_PORT_QUICK_UI_IDLE;
}

static int PsyX_HostToolbarContentHeight(int windowHeight)
{
	if (!g_xenoHostToolbarActive)
		return windowHeight;
	return windowHeight > PC_PORT_HOST_TOOLBAR_HEIGHT
		? windowHeight - PC_PORT_HOST_TOOLBAR_HEIGHT : 1;
}

static int PsyX_HostToolbarWarpY(int gameY)
{
	return g_xenoHostToolbarActive
		? gameY + PC_PORT_HOST_TOOLBAR_HEIGHT : gameY;
}

/* Pixels above the game content that the toolbar owns (0 when disabled);
 * the VRAM present path (psycross_present_vram_full_frame.patch) blits the
 * game image below them. */
extern "C" int PsyX_HostToolbarReservedHeight(void)
{
	return g_xenoHostToolbarActive ? PC_PORT_HOST_TOOLBAR_HEIGHT : 0;
}

static void PsyX_HostToolbarInitialise()
{
	const char* enabled = getenv("XENO_HOST_TOOLBAR");
	int width;
	int height;

	if (enabled != NULL && enabled[0] == '0' && enabled[1] == '\0')
		return;
	SDL_GetWindowSize(g_window, &width, &height);
	g_xenoHostToolbarActive = 1;
	width = width < 664 ? 664 : width;
	SDL_SetWindowMinimumSize(g_window, 664,
		240 + PC_PORT_HOST_TOOLBAR_HEIGHT);
	SDL_SetWindowSize(g_window, width,
		height + PC_PORT_HOST_TOOLBAR_HEIGHT);
	/* Keep renderer dimensions as the game content dimensions. The queued SDL
	 * resize event is normalized by PsyX_HostToolbarHandleEvent. */
	g_windowWidth = width;
	g_windowHeight = height;
	eprintinfo("Host toolbar enabled: Quick Save | Quick Load | Record | Speed (F11 cycles, Shift+F11 resets; hold Backspace for 5x)\n");
}

static void PsyX_HostToolbarDispatch(PcPortHostToolbarAction action)
{
	switch (action) {
	case PC_PORT_TOOLBAR_QUICK_SAVE:
#if defined(__GNUC__)
		PsyX_HostCheat("save", PcPort_QuickCheckpointRequestSave);
#endif
		break;
	case PC_PORT_TOOLBAR_QUICK_LOAD:
#if defined(__GNUC__)
		PsyX_HostCheat("load", PcPort_QuickCheckpointRequestLoad);
#endif
		break;
	case PC_PORT_TOOLBAR_FEI_HD2D:
#if defined(__GNUC__)
		if (PcPort_FeiHd2dToggle != NULL) PcPort_FeiHd2dToggle();
#endif
		break;
	case PC_PORT_TOOLBAR_RANDOM_BATTLES:
#if defined(__GNUC__)
		PsyX_HostCheat("encounters", PcPort_RandomBattlesToggle);
#endif
		break;
	case PC_PORT_TOOLBAR_GOD_MODE:
#if defined(__GNUC__)
		PsyX_HostCheat("god", PcPort_GodModeToggle);
#endif
		break;
	case PC_PORT_TOOLBAR_RECORD:
		PsyX_ToggleRecording();
		break;
	case PC_PORT_TOOLBAR_SPEED:
		PsyX_SetSpeedMultiplier(PsyX_GetSpeedMultiplier() % 5 + 1);
		break;
	default:
		break;
	}
}

static int PsyX_HostToolbarHandleEvent(SDL_Event* event)
{
	PcPortHostToolbarAction action;
	if (PsyX_PortDevMenuHandleEvent(event)) return 1;

	if (!g_xenoHostToolbarActive)
		return 0;
	if (event->type == SDL_WINDOWEVENT &&
		(event->window.event == SDL_WINDOWEVENT_RESIZED ||
		 event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
		event->window.data2 =
			PsyX_HostToolbarContentHeight(event->window.data2);
		return 0;
	}
	/* F10 is the keyboard half of the random-battle switch.  It lives here
	 * rather than in PsyX_main.cpp because pc_port/extern is git-ignored, and
	 * this handler sees every event first and can consume it. */
	if (event->type == SDL_KEYDOWN && event->key.repeat == 0 &&
		event->key.keysym.scancode == SDL_SCANCODE_F10) {
#if defined(__GNUC__)
		if (PcPort_RandomBattlesToggle != NULL || PcPort_CheatQueue != NULL) {
			PsyX_HostCheat("encounters", PcPort_RandomBattlesToggle);
			return 1;
		}
#endif
	}
	/* F12 is the keyboard half of the GOD switch, for the same reason F10 is
	 * the keyboard half of the battle switch: the toolbar's mouse path does
	 * not reach a headless or scripted session, so a driver had no way to turn
	 * party protection on before walking into a fight it cannot survive. */
	if (event->type == SDL_KEYDOWN && event->key.repeat == 0 &&
		event->key.keysym.scancode == SDL_SCANCODE_F12) {
#if defined(__GNUC__)
		if (PcPort_GodModeToggle != NULL || PcPort_CheatQueue != NULL) {
			PsyX_HostCheat("god", PcPort_GodModeToggle);
			return 1;
		}
#endif
	}
	if (event->type == SDL_MOUSEMOTION) {
		action = PcPort_HostToolbarHitTest(event->motion.x, event->motion.y);
		g_xenoHostToolbarHover = action;
		if (event->motion.y < PC_PORT_HOST_TOOLBAR_HEIGHT) {
			SDL_ShowCursor(SDL_ENABLE);
			return 1;
		}
		SDL_ShowCursor(SDL_DISABLE);
		event->motion.y -= PC_PORT_HOST_TOOLBAR_HEIGHT;
		return 0;
	}
	if (event->type == SDL_MOUSEBUTTONDOWN ||
		event->type == SDL_MOUSEBUTTONUP) {
		if (event->button.y < PC_PORT_HOST_TOOLBAR_HEIGHT) {
			if (event->type == SDL_MOUSEBUTTONUP &&
				event->button.button == SDL_BUTTON_LEFT) {
				action = PcPort_HostToolbarHitTest(
					event->button.x, event->button.y);
				PsyX_HostToolbarDispatch(action);
			}
			return 1;
		}
		event->button.y -= PC_PORT_HOST_TOOLBAR_HEIGHT;
	}
	return 0;
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

static void PsyX_HostToolbarDrawText(const char* text, int x, int y)
{
	const int scale = 2;
	for (; *text != '\0'; ++text, x += 12) {
		const unsigned char* glyph = PsyX_HostToolbarGlyph(*text);
		int row;
		if (glyph == NULL)
			continue;
		for (row = 0; row < 7; ++row) {
			int column = 0;
			while (column < 5) {
				int start;
				while (column < 5 && !(glyph[row] & (1 << (4 - column))))
					column++;
				start = column;
				while (column < 5 && (glyph[row] & (1 << (4 - column))))
					column++;
				if (column > start)
					PsyX_HostToolbarClearRect(x + start * scale,
						y + (6 - row) * scale,
						(column - start) * scale, scale,
						0.94f, 0.94f, 0.97f);
			}
		}
	}
}

#include "psycross_port_dev_menu.inl"

static void PsyX_HostToolbarDrawButton(int x, int width,
	PcPortHostToolbarAction action, const char* label, int labelX, int state)
{
	float shade = g_xenoHostToolbarHover == action ? 0.30f : 0.20f;
	float red = shade;
	float green = shade;
	float blue = shade + 0.04f;

	if (state == 1) {
		red = 0.64f;
		green = 0.43f;
		blue = 0.05f;
	} else if (state == 2) {
		red = 0.08f;
		green = 0.48f;
		blue = 0.18f;
	} else if (state == 3 ||
		(action == PC_PORT_TOOLBAR_RECORD && PsyX_IsRecording())) {
		red = 0.62f;
		green = 0.10f;
		blue = 0.12f;
	}
	PsyX_HostToolbarClearRect(x, g_windowHeight + 4, width, 26,
		0.48f, 0.48f, 0.53f);
	PsyX_HostToolbarClearRect(x + 1, g_windowHeight + 5, width - 2, 24,
		red, green, blue);
	PsyX_HostToolbarDrawText(label, labelX, g_windowHeight + 10);
}

static void PsyX_HostToolbarDraw()
{
	PcPortQuickUiState quickState;
	const char* saveLabel = "SAVE";
	const char* loadLabel = "LOAD";
	int saveLabelX = 24;
	int loadLabelX = 112;
	int saveState = 0;
	int loadState = 0;
	char speedLabel[] = "SPEED 1X";
	GLboolean scissorEnabled;
	GLboolean colorMask[4];
	GLint oldScissor[4];
	GLfloat oldClearColor[4];

	if (!g_xenoHostToolbarActive) {
		PsyX_PortDevMenuDraw();
		return;
	}
	scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
	glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
	glGetIntegerv(GL_SCISSOR_BOX, oldScissor);
	glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClearColor);
	glEnable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	quickState = PsyX_HostToolbarQuickUiState();
	if (quickState == PC_PORT_QUICK_UI_SAVE_PENDING) {
		saveLabel = "WAIT";
		saveState = 1;
	} else if (quickState == PC_PORT_QUICK_UI_SAVE_OK) {
		saveLabel = "SAVED";
		saveLabelX = 18;
		saveState = 2;
	} else if (quickState == PC_PORT_QUICK_UI_SAVE_ERROR) {
		saveLabel = "ERROR";
		saveLabelX = 18;
		saveState = 3;
	} else if (quickState == PC_PORT_QUICK_UI_LOAD_PENDING) {
		loadLabel = "WAIT";
		loadState = 1;
	} else if (quickState == PC_PORT_QUICK_UI_LOAD_OK) {
		loadLabel = "LOADED";
		loadLabelX = 100;
		loadState = 2;
	} else if (quickState == PC_PORT_QUICK_UI_LOAD_ERROR) {
		loadLabel = "ERROR";
		loadLabelX = 106;
		loadState = 3;
	}

	PsyX_HostToolbarClearRect(0, g_windowHeight, g_windowWidth,
		PC_PORT_HOST_TOOLBAR_HEIGHT, 0.07f, 0.07f, 0.09f);
	PsyX_HostToolbarDrawButton(8, 80, PC_PORT_TOOLBAR_QUICK_SAVE,
		saveLabel, saveLabelX, saveState);
	PsyX_HostToolbarDrawButton(96, 80, PC_PORT_TOOLBAR_QUICK_LOAD,
		loadLabel, loadLabelX, loadState);
	PsyX_HostToolbarDrawButton(184, 96, PC_PORT_TOOLBAR_RECORD,
		PsyX_IsRecording() ? "STOP" : "RECORD",
		PsyX_IsRecording() ? 208 : 196, 0);
	speedLabel[6] = '0' + PsyX_GetSpeedMultiplier();
	PsyX_HostToolbarDrawButton(288, 104, PC_PORT_TOOLBAR_SPEED,
		speedLabel, 294, PsyX_GetSpeedMultiplier() > 1 ? 1 : 0);

#if defined(__GNUC__)
	if (PcPort_GodModeEnabled != NULL) {
		int god = PcPort_GodModeEnabled();
		PsyX_HostToolbarDrawButton(578, 78, PC_PORT_TOOLBAR_GOD_MODE,
			"GOD", 606, god ? 3 : 0);
	}
	if (PcPort_RandomBattlesEnabled != NULL) {
		/* Green while encounters are live, red while they are suppressed. */
		int battles = PcPort_RandomBattlesEnabled();
		PsyX_HostToolbarDrawButton(472, 98, PC_PORT_TOOLBAR_RANDOM_BATTLES,
			"BATTLES", 496, battles ? 2 : 3);
	}
	if (PcPort_FeiHd2dEnabled != NULL) {
		int hd = PcPort_FeiHd2dEnabled();
		PsyX_HostToolbarDrawButton(400, 68, PC_PORT_TOOLBAR_FEI_HD2D,
			"HD2D", 412, hd ? 2 : 0);
	}
#endif

	glClearColor(oldClearColor[0], oldClearColor[1],
		oldClearColor[2], oldClearColor[3]);
	glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
	glScissor(oldScissor[0], oldScissor[1], oldScissor[2], oldScissor[3]);
	if (!scissorEnabled)
		glDisable(GL_SCISSOR_TEST);
	PsyX_PortDevMenuDraw();
}

static void PsyX_HostToolbarShutdown()
{
	g_xenoHostToolbarActive = 0;
	g_xenoHostToolbarHover = PC_PORT_TOOLBAR_NONE;
}

#else

static int PsyX_HostToolbarContentHeight(int height) { return height; }
static int PsyX_HostToolbarWarpY(int y) { return y; }
static void PsyX_HostToolbarInitialise() {}
static int PsyX_HostToolbarHandleEvent(SDL_Event*) { return 0; }
static void PsyX_HostToolbarDraw() {}
static void PsyX_HostToolbarShutdown() {}

#endif
