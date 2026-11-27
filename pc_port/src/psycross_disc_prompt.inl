/* The original insert-disc dialog owns these host controls only while its
 * validator is waiting. The same game thread presents and pumps SDL there. */
extern "C" int PcPort_ToggleDiscLid(int targetDisc, int reinsert) __attribute__((weak));
extern "C" int PcPort_DiscLidOpen(void) __attribute__((weak));
extern "C" SDL_JoystickID PsyX_Pad_HostActiveInstance(void);

static int s_requestedDisc;
static bool s_discActionRefused;
static bool s_discChecking;
static bool s_quickDiscPrompt;
static int s_quickDiscAction;
static bool s_discPromptNeedsPresent;

extern "C" void PcPort_DiscPromptSetActive(int targetDisc)
{
	s_requestedDisc = targetDisc;
	s_quickDiscPrompt = false;
	s_quickDiscAction = 0;
	s_discActionRefused = false;
	s_discChecking = false;
	s_discPromptNeedsPresent = false;
}

extern "C" void PcPort_QuickDiscPrompt(int target, int state)
{
	PcPort_DiscPromptSetActive(target);
	s_quickDiscPrompt = target != 0;
	s_discActionRefused = state == 1;
	s_discChecking = state == 2;
	s_discPromptNeedsPresent = true;
}

extern "C" int PcPort_QuickDiscPromptTakeAction(void)
{
	int action = s_quickDiscAction;
	s_quickDiscAction = 0;
	return action;
}

extern "C" int PcPort_DiscPromptTakeRedraw(void)
{
	bool pending = s_discPromptNeedsPresent;
	s_discPromptNeedsPresent = false;
	return pending;
}

static bool PsyX_DiscPromptAvailable()
{
	return (s_requestedDisc == 1 || s_requestedDisc == 2) &&
	       (s_quickDiscPrompt || (!PcPort_DevMenuOpen() && !g_cfg_gameOnTextInput)) &&
	       PcPort_ToggleDiscLid && PcPort_DiscLidOpen;
}

static const char* PsyX_DiscPromptHint()
{
	if (!PsyX_DiscPromptAvailable()) return NULL;
	if (s_discChecking) return "CHECKING DISC";
	if (s_quickDiscPrompt)
		return s_requestedDisc == 2 ? "F5/Y LOAD DISC 2   F4/X CANCEL" :
		                            "F5/Y LOAD DISC 1   F4/X CANCEL";
	return PcPort_DiscLidOpen() ? "F5/Y CLOSE TRAY   F4/X REINSERT" : "F5/Y EJECT DISC";
}

static int PsyX_DiscPromptHandleEvent(const SDL_Event* event)
{
	if (!PsyX_DiscPromptAvailable()) return 0;
	bool reinsert;
	bool pressed;
	if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
		reinsert = event->key.keysym.scancode == SDL_SCANCODE_F4;
		if (!reinsert && event->key.keysym.scancode != SDL_SCANCODE_F5) return 0;
		pressed = event->type == SDL_KEYDOWN && !event->key.repeat;
	} else if (event->type == SDL_CONTROLLERBUTTONDOWN || event->type == SDL_CONTROLLERBUTTONUP) {
		reinsert = event->cbutton.button == SDL_CONTROLLER_BUTTON_X;
		if ((!reinsert && event->cbutton.button != SDL_CONTROLLER_BUTTON_Y) ||
		    event->cbutton.which < 0 ||
		    event->cbutton.which != PsyX_Pad_HostActiveInstance()) return 0;
		pressed = event->type == SDL_CONTROLLERBUTTONDOWN;
	} else {
		return 0;
	}
	/* The validator stops checking for an open lid after its Nop loop. */
	if (s_discChecking) return 1;
	if (reinsert && !s_quickDiscPrompt && !PcPort_DiscLidOpen()) return 0;
	if (!pressed) return 1;
	if (s_quickDiscPrompt) {
		if (!s_quickDiscAction) s_quickDiscAction = reinsert ? -1 : 1;
		return 1;
	}
	s_discActionRefused = !PcPort_ToggleDiscLid(s_requestedDisc, reinsert);
	s_discChecking = !s_discActionRefused && !PcPort_DiscLidOpen();
	s_discPromptNeedsPresent = true;
	return 1;
}
