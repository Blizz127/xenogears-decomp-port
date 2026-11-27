/* A VBlank can arrive while all 16 original samples await their reader.
 * The renderer still pumps SDL events. Keep the intervening digital levels
 * in order, then supply one level at each real game poll that has space.
 * Port 1 can contain both a keyboard and the selected SDL controller. */
struct XenoPadLevels
{
	u_short keyboard;
	u_short gamepad;
	unsigned menu;
};

/* Allocate only for genuine transitions. Consumed blocks are returned to
 * SDL immediately, including during long periods of partial draining. */
struct XenoPadLevelBlock
{
	XenoPadLevels levels[32];
	unsigned count;
	XenoPadLevelBlock* next;
};

static XenoPadLevelBlock* s_deferredHead;
static XenoPadLevelBlock* s_deferredTail;
static unsigned s_deferredRead;
static bool s_deferredLost;
static XenoPadLevels s_lastSample = {0xffff, 0xffff, 0};
static XenoPadLevels s_eventLevels = {0xffff, 0xffff, 0};
static bool s_lastSampleValid;
static bool s_deferredCapture;
static bool s_deferredFull;
static bool s_deferredFocusLost; /* headless hosts can poll without a focus event */
static SDL_JoystickID s_deferredInstance = -1;
static Uint8 s_deferredButtons[SDL_CONTROLLER_BUTTON_MAX];
static Sint16 s_deferredAxes[SDL_CONTROLLER_AXIS_MAX];
static int s_deferredStickDir;
static Uint8 s_deferredPadId;

static int XenoPad_ReadDeferredControl(int binding)
{
	if (binding & CONTROLLER_MAP_FLAG_AXIS)
	{
		int axis = binding & ~(CONTROLLER_MAP_FLAG_AXIS | CONTROLLER_MAP_FLAG_INVERSE);
		if (axis < 0 || axis >= SDL_CONTROLLER_AXIS_MAX)
			return 0;
		return PsyX_Pad_SignedAxis(s_deferredAxes[axis], binding);
	}
	return binding >= 0 && binding < SDL_CONTROLLER_BUTTON_MAX && s_deferredButtons[binding]
		? 32767 : 0;
}

static u_short XenoPad_DeferredGamepadLevel()
{
	PADRAW pad = {};
	pad.id = s_deferredPadId;
	PsyX_Pad_MapGamepad(XenoPad_ReadDeferredControl, &pad, s_deferredStickDir);
	u_short level;
	memcpy(&level, pad.buttons, sizeof(level));
	return level;
}

static unsigned XenoPad_DeferredMenuButtons(u_short gamepad)
{
	return XenoPad_MenuButtons(gamepad,
		[](SDL_GameControllerButton b) { return s_deferredButtons[b]; });
}

static void XenoPad_QueueLevels(const XenoPadLevels& levels)
{
	/* Independently held keyboard and gamepad buttons must survive overlap. */
	if (levels.keyboard == s_eventLevels.keyboard && levels.gamepad == s_eventLevels.gamepad &&
		levels.menu == s_eventLevels.menu)
		return;
	s_eventLevels = levels;
	/* SDL may drain focus gain and several button events together, with
	 * SDL_Get* already at the final state. While unfocused, track only
	 * current levels, without saving any transitions for the game. */
	if (s_deferredLost || s_deferredFocusLost)
		return;
	if (!s_deferredTail || s_deferredTail->count == SDL_arraysize(s_deferredTail->levels))
	{
		XenoPadLevelBlock* block = (XenoPadLevelBlock*)SDL_malloc(sizeof(*block));
		if (!block)
		{
			s_deferredLost = true;
			eprintwarn("pad: out of memory preserving input; returning to live input after the queued samples\n");
			return;
		}
		block->count = 0;
		block->next = NULL;
		if (s_deferredTail)
			s_deferredTail->next = block;
		else
			s_deferredHead = block;
		s_deferredTail = block;
	}
	s_deferredTail->levels[s_deferredTail->count++] = levels;
}

static void XenoPad_FreeHeadBlock()
{
	XenoPadLevelBlock* next = s_deferredHead->next;
	SDL_free(s_deferredHead);
	s_deferredHead = next;
	s_deferredRead = 0;
	if (!next)
		s_deferredTail = NULL;
}

static void XenoPad_DiscardDeferredLevels()
{
	while (s_deferredHead)
		XenoPad_FreeHeadBlock();
	s_deferredLost = false;
	s_lastSampleValid = false;
	g_latchedKeyboardInputs = 0xffff;
}

extern "C" void PsyX_Pad_ClearDeferredSamples()
{
	s_deferredFull = false;
	s_deferredCapture = false;
	XenoPad_DiscardDeferredLevels();
}

extern "C" void PsyX_Pad_SetSampleDeferred(int deferred)
{
	s_deferredFull = deferred != 0;
	if (!s_deferredFull && s_deferredFocusLost)
		s_deferredCapture = false;
	if (!s_deferredFull || s_deferredCapture)
		return;

	s_deferredCapture = true;
	s_deferredInstance = PsyX_Pad_HostActiveInstance();
	s_deferredStickDir = s_xenoStickDir;
	const PsyXController& port = g_controllers[0];
	s_deferredPadId = port.padData ? ((LPPADRAW)port.padData)->id : 0x41;
	u_short gamepad = 0xffff;
	if (s_deferredInstance >= 0 && port.gc)
	{
		for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++)
			s_deferredButtons[i] = SDL_GameControllerGetButton(port.gc, (SDL_GameControllerButton)i);
		for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++)
			s_deferredAxes[i] = SDL_GameControllerGetAxis(port.gc, (SDL_GameControllerAxis)i);
		gamepad = XenoPad_DeferredGamepadLevel();
	}
	XenoPadLevels held = {PsyX_Pad_HeldKeyboardInput(), gamepad,
		s_deferredInstance >= 0 ? XenoPad_DeferredMenuButtons(gamepad) : 0};
	/* A tap latched by the last ordinary sample may already be released. */
	s_eventLevels = s_lastSampleValid ? s_lastSample : held;
	XenoPad_QueueLevels(held);
}

extern "C" void PsyX_Pad_RecordDeferredEvent(const SDL_Event* event)
{
	if (event->type == SDL_WINDOWEVENT)
	{
		if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
		{
			s_deferredFocusLost = true;
			XenoPad_DiscardDeferredLevels();
			if (!s_deferredFull)
				s_deferredCapture = false;
		}
		else if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED && s_deferredFocusLost)
		{
			s_deferredFocusLost = false;
			/* Any intervening game polls or queue reset updated the full flag.
			 * A tap can finish before the next VBlank reports it again. */
			s_lastSampleValid = false;
			PsyX_Pad_SetSampleDeferred(s_deferredFull);
		}
		return;
	}
	if (!s_deferredCapture)
		return;

	XenoPadLevels next = s_eventLevels;
	switch (event->type)
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
	{
		if (event->key.repeat)
			return;
		u_short mask = PsyX_Pad_KeyboardButtonMask(event->key.keysym.scancode);
		if (event->type == SDL_KEYDOWN)
			next.keyboard &= ~mask;
		else
			next.keyboard |= mask;
		break;
	}
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
		if (s_deferredInstance < 0 || event->cbutton.which != s_deferredInstance ||
			event->cbutton.button >= SDL_CONTROLLER_BUTTON_MAX)
			return;
		s_deferredButtons[event->cbutton.button] = event->cbutton.state == SDL_PRESSED;
		next.gamepad = XenoPad_DeferredGamepadLevel();
		next.menu = XenoPad_DeferredMenuButtons(next.gamepad);
		break;
	case SDL_CONTROLLERAXISMOTION:
		if (s_deferredInstance < 0 || event->caxis.which != s_deferredInstance ||
			event->caxis.axis >= SDL_CONTROLLER_AXIS_MAX)
			return;
		s_deferredAxes[event->caxis.axis] = event->caxis.value;
		next.gamepad = XenoPad_DeferredGamepadLevel();
		next.menu = XenoPad_DeferredMenuButtons(next.gamepad);
		break;
	default:
		return;
	}
	XenoPad_QueueLevels(next);
}

static void PsyX_Pad_NextSample(u_short& keyboard, u_short& gamepad, unsigned& menu)
{
	if (s_deferredCapture && s_deferredInstance != PsyX_Pad_HostActiveInstance())
		PsyX_Pad_ClearDeferredSamples();
	if (s_deferredFull)
		return;

	if (s_deferredHead)
	{
		const XenoPadLevels& levels = s_deferredHead->levels[s_deferredRead++];
		keyboard = levels.keyboard;
		gamepad = levels.gamepad;
		menu = levels.menu;
		if (s_deferredRead == s_deferredHead->count)
			XenoPad_FreeHeadBlock();
	}
	else
		s_deferredLost = false; /* This poll returns the actual live levels. */
	s_deferredCapture = s_deferredHead || s_deferredLost;
	s_lastSample = {keyboard, gamepad, menu};
	s_lastSampleValid = true;
}
