/* SDL host side of the opt-in Port Dev Menu (port_dev_menu.c): keyboard
 * events (F8 open/close; arrows, Enter, Esc while open), the overlay, the
 * persistent CHEATS marker and the host functions the menu drives.  The
 * controller side is polled from port 1's active pad in PsyX_pad.cpp
 * (psycross_pad_select.patch), so the menu works on exactly the pad the
 * game uses, including Steam's virtual pad. */
#include "port_dev_menu.h"

void PsyX_TakeScreenshot();
void PsyX_TakeScreenshotPath(const char* path);
extern "C" int PsyX_HostFastForwardEnabled(void);
extern "C" void PsyX_HostFastForwardToggle(void);
extern "C" void PsyX_HostFastForwardToggleFrom(const char* source);
extern "C" int PcPort_PadConfirmCircle(void);
extern "C" void PcPort_PadConfirmToggle(void);
extern "C" int PcPort_FeiHd2dAssetPresent(void) __attribute__((weak));
extern "C" const char* PcPort_QuickCheckpointNotice(void) __attribute__((weak));

static int s_xenoDevMenuShotPending = 0;

static int PsyX_DevHostSpeed(void) { return PsyX_GetSpeedMultiplier(); }
static void PsyX_DevHostSpeedCycle(void) { PsyX_SetSpeedMultiplierFrom(PsyX_GetSpeedMultiplier() % 5 + 1, "dev-menu"); }
static void PsyX_DevHostFastForwardToggle(void) { PsyX_HostFastForwardToggleFrom("dev-menu"); }
static int PsyX_DevHostRecording(void) { return PsyX_IsRecording(); }
static void PsyX_DevHostRecordToggle(void) { PsyX_ToggleRecording(); }
/* Taken from the draw hook two presents later, once the menu is gone. */
static void PsyX_DevHostScreenshot(void) { s_xenoDevMenuShotPending = 2; }
static int PsyX_DevHostHd2dAvailable(void)
{
	return PcPort_FeiHd2dAssetPresent != NULL && PcPort_FeiHd2dEnabled != NULL &&
		PcPort_FeiHd2dAssetPresent();
}
static int PsyX_DevHostHd2dOn(void) { return PcPort_FeiHd2dEnabled ? PcPort_FeiHd2dEnabled() : 0; }
static void PsyX_DevHostHd2dToggle(void) { if (PcPort_FeiHd2dToggle) PcPort_FeiHd2dToggle(); }
static int PsyX_DevHostGod(void) { return PcPort_GodModeEnabled ? PcPort_GodModeEnabled() : 0; }
static int PsyX_DevHostBattles(void) { return PcPort_RandomBattlesEnabled ? PcPort_RandomBattlesEnabled() : 1; }

static void PsyX_PortDevMenuInstallHost()
{
	static const PcPortDevMenuHost host = {
		PsyX_HostFastForwardEnabled, PsyX_DevHostFastForwardToggle,
		PsyX_DevHostSpeed, PsyX_DevHostSpeedCycle,
		PsyX_DevHostRecording, PsyX_DevHostRecordToggle,
		PsyX_DevHostScreenshot,
		PsyX_DevHostHd2dAvailable, PsyX_DevHostHd2dOn, PsyX_DevHostHd2dToggle,
		PsyX_DevHostGod, PsyX_DevHostBattles,
		PcPort_PadConfirmCircle, PcPort_PadConfirmToggle,
	};
	static int installed;
	if (!installed) { PcPort_DevMenuSetHost(&host); installed = 1; }
}

static int PsyX_PortDevMenuHandleEvent(SDL_Event *event)
{
	int open;
	PsyX_PortDevMenuInstallHost();
	if (!PcPort_DevMenuEnabled()) return 0;
	open = PcPort_DevMenuOpen();
	if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
		int pressed = event->type == SDL_KEYDOWN;
		int key = event->key.keysym.scancode;
		if (key == SDL_SCANCODE_F8) {
			if (pressed && !event->key.repeat) PcPort_DevMenuKey(PC_DEV_OPEN, 1);
			return 1;
		}
		if (!open) return 0;
		/* While open the menu owns the keyboard: no game keys, no host keys. */
		if (!pressed || event->key.repeat) return 1;
		if (key == SDL_SCANCODE_UP) PcPort_DevMenuKey(PC_DEV_UP, 1);
		else if (key == SDL_SCANCODE_DOWN) PcPort_DevMenuKey(PC_DEV_DOWN, 1);
		else if (key == SDL_SCANCODE_LEFT) PcPort_DevMenuKey(PC_DEV_LEFT, 1);
		else if (key == SDL_SCANCODE_RIGHT) PcPort_DevMenuKey(PC_DEV_RIGHT, 1);
		else if (key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_KP_ENTER) PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);
		else if (key == SDL_SCANCODE_ESCAPE || key == SDL_SCANCODE_BACKSPACE) PcPort_DevMenuKey(PC_DEV_BACK, 1);
		return 1;
	}
	return open && (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP ||
		event->type == SDL_MOUSEWHEEL || event->type == SDL_TEXTINPUT);
}

static void PsyX_DevMenuDrawTextColor(const char* text, int x, int y, float r, float g, float b, int scale = 2);

/* Harness-only scripted menu keys (DEV_MENU_SPEC.md §9), inert unless set:
 * XENO_DEV_MENU_KEYS="present:KEY,..." with KEY one of F8 UP DOWN LEFT RIGHT ENTER ESC,
 * SDL:<scancode name> (a real key press pushed through SDL, e.g. SDL:F8)
 * or SHOT=<name>, which saves the presented frame WITH the overlay to
 * $XENO_DEV_MENU_SHOT_DIR/<name>.bmp.  `present` counts overlay draws. */
static void PsyX_DevMenuScript(int phase)
{
	static int parsed, count, presents;
	static struct { int at; char key[40]; } steps[64];
	int i;
	if (!parsed) {
		const char* env = getenv("XENO_DEV_MENU_KEYS");
		parsed = 1;
		while (env && *env && count < 64) {
			int at = 0, n = 0;
			if (sscanf(env, "%d:%39[^,]%n", &at, steps[count].key, &n) != 2) break;
			steps[count++].at = at;
			env += n;
			if (*env == ',') env++;
		}
		if (count) eprintinfo("[DEV_MENU] scripted keys: %d steps (harness)\n", count);
	}
	if (!count) return;
	if (phase == 0) presents++;
	for (i = 0; i < count; i++) {
		const char* k = steps[i].key;
		if (steps[i].at != presents) continue;
		if (phase == 0) {
			if (!strcmp(k, "F8")) PcPort_DevMenuKey(PC_DEV_OPEN, 1);
			else if (!strcmp(k, "UP")) PcPort_DevMenuKey(PC_DEV_UP, 1);
			else if (!strcmp(k, "DOWN")) PcPort_DevMenuKey(PC_DEV_DOWN, 1);
			else if (!strcmp(k, "LEFT")) PcPort_DevMenuKey(PC_DEV_LEFT, 1);
			else if (!strcmp(k, "RIGHT")) PcPort_DevMenuKey(PC_DEV_RIGHT, 1);
			else if (!strcmp(k, "ENTER")) PcPort_DevMenuKey(PC_DEV_ACCEPT, 1);
			else if (!strcmp(k, "ESC")) PcPort_DevMenuKey(PC_DEV_BACK, 1);
			else if (!strncmp(k, "SDL:", 4)) {
				/* A real key press through SDL's queue (same path as the
				 * keyboard: host hotkeys, dev-menu filter, game pad). */
				SDL_Event e;
				SDL_zero(e);
				e.type = SDL_KEYDOWN;
				e.key.state = SDL_PRESSED;
				e.key.keysym.scancode = SDL_GetScancodeFromName(k + 4);
				SDL_PushEvent(&e);
				e.type = SDL_KEYUP;
				e.key.state = SDL_RELEASED;
				SDL_PushEvent(&e);
			}
			if (strncmp(k, "SHOT=", 5)) eprintinfo("[DEV_MENU] scripted %s at present %d\n", k, presents);
		} else if (!strncmp(k, "SHOT=", 5)) {
			const char* dir = getenv("XENO_DEV_MENU_SHOT_DIR");
			char path[512];
			SDL_snprintf(path, sizeof path, "%s/%s.bmp", dir ? dir : ".", k + 5);
			PsyX_TakeScreenshotPath(path);
			eprintinfo("[DEV_MENU] screenshot %s (present %d)\n", path, presents);
		}
	}
}

static void PsyX_PortDevMenuDraw()
{
	GLboolean scissorEnabled, mask[4];
	GLint scissor[4];
	GLfloat clear[4];
	int i, cheats;
	const char* notice;

	if (s_xenoDevMenuShotPending > 0 && --s_xenoDevMenuShotPending == 0) {
		eprintwarn("Saving screenshot...\n");
		PsyX_TakeScreenshot();
	}
	PsyX_DevMenuScript(0);
	cheats = PcPort_DevMenuCheatsActive();
	notice = PcPort_QuickCheckpointNotice ? PcPort_QuickCheckpointNotice() : NULL;
	if (!PcPort_DevMenuOpen() && !cheats && !notice) { PsyX_DevMenuScript(1); return; }
	scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
	glGetBooleanv(GL_COLOR_WRITEMASK, mask);
	glGetIntegerv(GL_SCISSOR_BOX, scissor);
	glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
	glEnable(GL_SCISSOR_TEST); glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	if (cheats) {
		/* Persistent marker, top-right (GL y grows upward). */
		PsyX_HostToolbarClearRect(g_windowWidth - 92, g_windowHeight - 24, 86, 20, 0.55f, 0.08f, 0.08f);
		PsyX_DevMenuDrawTextColor("CHEATS", g_windowWidth - 86, g_windowHeight - 20, 1.0f, 0.95f, 0.95f);
	}
	if (notice) {
		/* Quick save/load notice (quick_checkpoint.c), bottom-left. */
		int w = (int)strlen(notice) * 12 + 16;
		PsyX_HostToolbarClearRect(8, 8, w, 28, 0.035f, 0.05f, 0.09f);
		PsyX_DevMenuDrawTextColor(notice, 16, 14, 0.98f, 0.85f, 0.45f);
	}
	if (PcPort_DevMenuOpen()) {
		PcPortDevMenuLayout layout;
		PcPort_DevMenuGetLayout(g_windowWidth, g_windowHeight, &layout);
		if (layout.visible_rows > 0) {
			const int scale = layout.text_scale, top = layout.top;
			const int rows = layout.visible_rows, rowHeight = layout.row_height;
			const int bottom = top - rowHeight * (rows + 3) - 4 * scale;
			char title[128];
			if (rows < PcPort_DevMenuRows())
				snprintf(title, sizeof title, "%s (%d-%d/%d)", PcPort_DevMenuTitle(),
					layout.first_row + 1, layout.first_row + rows, PcPort_DevMenuRows());
			else snprintf(title, sizeof title, "%s", PcPort_DevMenuTitle());
			PsyX_HostToolbarClearRect(4 * scale, bottom, g_windowWidth - 8 * scale,
				top + 10 * scale - bottom, 0.035f, 0.05f, 0.09f);
			PsyX_DevMenuDrawTextColor(title, 10 * scale, top, 0.98f, 0.85f, 0.45f, scale);
			for (i = 0; i < rows; i++) {
				int row = layout.first_row + i;
				int y = top - rowHeight * (i + 1);
				int selectable = PcPort_DevMenuRowSelectable(row);
				if (row == PcPort_DevMenuCursor())
					PsyX_HostToolbarClearRect(8 * scale, y - 2 * scale,
						g_windowWidth - 16 * scale, 12 * scale, 0.15f, 0.30f, 0.40f);
				if (selectable)
					PsyX_DevMenuDrawTextColor(PcPort_DevMenuRow(row), 14 * scale, y, 0.94f, 0.94f, 0.97f, scale);
				else
					PsyX_DevMenuDrawTextColor(PcPort_DevMenuRow(row), 14 * scale, y, 0.45f, 0.47f, 0.52f, scale);
			}
			const char *hint = PcPort_DevMenuRowAdjustable(PcPort_DevMenuCursor())
				? "LEFT/RIGHT ADJUST  A INCREASE  B BACK"
				: "DPAD/STICK MOVE  A PICK  B BACK";
			PsyX_DevMenuDrawTextColor(hint, 10 * scale, top - rowHeight * (rows + 1), 0.6f, 0.7f, 0.8f, scale);
			PsyX_DevMenuDrawTextColor("SELECT+START OR F8 CLOSE", 10 * scale, top - rowHeight * (rows + 2), 0.6f, 0.7f, 0.8f, scale);
			PsyX_DevMenuDrawTextColor(PcPort_DevMenuStatus(), 10 * scale, top - rowHeight * (rows + 3), 0.6f, 0.9f, 0.6f, scale);
		}
	}
	glClearColor(clear[0], clear[1], clear[2], clear[3]);
	glColorMask(mask[0], mask[1], mask[2], mask[3]);
	glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
	if (!scissorEnabled) glDisable(GL_SCISSOR_TEST);
	PsyX_DevMenuScript(1);
}

static void PsyX_DevMenuDrawTextColor(const char* text, int x, int y, float r, float g, float b, int scale)
{
	for (; *text != '\0'; ++text, x += 6 * scale) {
		char ch = (*text >= 'a' && *text <= 'z') ? (char)(*text - 32) : *text;
		const unsigned char* glyph = PsyX_HostToolbarGlyph(ch);
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
					PsyX_HostToolbarClearRect(x + start * scale, y + (6 - row) * scale,
						(column - start) * scale, scale, r, g, b);
			}
		}
	}
}
