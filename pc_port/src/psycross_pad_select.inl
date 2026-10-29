/* psycross_pad_select.inl -- _xeno_pad_select
 *
 * Host-side game-controller selection for port 1, compiled into PsyCross's
 * src/pad/PsyX_pad.cpp (pc_port/patches/psycross_pad_select.patch).  Pure
 * host input plumbing: the game still reads only its PS1 pad buffer, and the
 * button mapping is PsyCross's gc_* table.
 *
 *  1. Hints (before the GameController subsystem starts).  A game launched
 *     from Steam inherits SDL_GAMECONTROLLER_IGNORE_DEVICES, which lists
 *     every controller Steam wants to own -- including the handheld pads
 *     (Steam Deck 28de:1205, InputPlumber's Deck-style 28de:1205/1206 and the
 *     Lenovo Legion 17ef:*).  With Steam Input disabled for the shortcut
 *     Steam then feeds nothing to its virtual pad, so SDL shows only a
 *     silent pad.  Those handheld entries are removed (every other entry is
 *     kept) and the result is applied with SDL_HINT_OVERRIDE, the only
 *     priority that beats the environment variable.
 *  2. Selection.  Every attached controller is opened; exactly one feeds
 *     port 1.  Steam's virtual pad (28de:11ff) ranks below any other pad:
 *     it is used only when it is the sole pad, or when it produces input
 *     while no other pad ever has.  A non-virtual pad takes over from
 *     another one only after the active pad has been idle
 *     XENO_PAD_IDLE_SWITCH_MS, so a mirrored pair (Steam Input on, or a raw
 *     pad next to its InputPlumber copy) never flips mid-press and never
 *     doubles input.  Hotplug: ADDED carries a device index, REMOVED an
 *     instance id; the table is keyed by instance id.
 *  3. Left stick = D-pad while the pad is digital (id 0x41, which is what
 *     Xenogears uses): 8-way sectors, radial deadzone with hysteresis.
 */
#include <stdlib.h>
#include <string.h>

#define XENO_PAD_MAX                8
#define XENO_PAD_ACTIVITY_THRESHOLD 16000   /* axis units counted as "input" */
#define XENO_PAD_IDLE_SWITCH_MS     500
#define XENO_PAD_STICK_PRESS        13000   /* ~40% of full scale */
#define XENO_PAD_STICK_RELEASE      9800    /* ~30%: hysteresis */

enum
{
	XENO_PAD_SRC_OTHER = 0,
	XENO_PAD_SRC_HIDAPI,
	XENO_PAD_SRC_EVDEV,
	XENO_PAD_SRC_STEAM_VIRTUAL,
	XENO_PAD_SRC_SDL_VIRTUAL
};

typedef struct
{
	SDL_GameController*	gc;
	SDL_JoystickID		id;
	int					source;
	int					everInput;
	Uint32				lastInputTicks;
	unsigned			serial;
} XenoPadEntry;

static XenoPadEntry		s_xenoPads[XENO_PAD_MAX];
static SDL_JoystickID	s_xenoActivePad = -1;
static unsigned			s_xenoPadSerial = 0;
static int				s_xenoPadAnnounced = 0;
static int				s_xenoHadActive = 0;
static int				s_xenoStickDir = 0;	/* last stick d-pad bits (active-high) */

extern "C" const char* PsyX_Pad_SourceName(int source)
{
	switch (source)
	{
	case XENO_PAD_SRC_HIDAPI: return "hidapi";
	case XENO_PAD_SRC_EVDEV: return "evdev";
	case XENO_PAD_SRC_STEAM_VIRTUAL: return "steam-virtual";
	case XENO_PAD_SRC_SDL_VIRTUAL: return "sdl-virtual";
	default: return "other";
	}
}

/* Handheld / InputPlumber entries that must not be ignored. */
static int XenoPad_IsHandheldEntry(unsigned vendor, unsigned product)
{
	if (vendor == 0x17ef)
		return 1;	/* Lenovo Legion Go / Go S pads */
	if (vendor == 0x28de && (product == 0x1205 || product == 0x1206))
		return 1;	/* Steam Deck, InputPlumber Deck-style targets */
	if (vendor == 0x28de && product >= 0x12f0 && product <= 0x12ff)
		return 1;	/* InputPlumber virtual range (Legion Go 2 = 28de:12fb) */
	return 0;
}

/* Copy the comma-separated VID/PID list `in` to `out` without its handheld
 * entries (see above); unparsable entries are kept verbatim.  `removed`
 * (optional, removedSize bytes) receives the dropped entries.  Returns the
 * number of entries removed, or -1 when `out` is too small. */
extern "C" int PsyX_Pad_SanitizeIgnoreDevices(const char* in, char* out, size_t outSize,
                                              char* removed, size_t removedSize)
{
	size_t outLen = 0, remLen = 0;
	int count = 0;
	const char* p = in ? in : "";

	if (!out || outSize == 0)
		return -1;
	out[0] = '\0';
	if (removed && removedSize)
		removed[0] = '\0';

	while (*p)
	{
		const char* end = strchr(p, ',');
		size_t len = end ? (size_t)(end - p) : strlen(p);
		const char* s = p;
		size_t n = len;
		char token[64];
		int drop = 0;

		while (n && (*s == ' ' || *s == '\t')) { s++; n--; }
		while (n && (s[n - 1] == ' ' || s[n - 1] == '\t')) n--;

		if (n > 0 && n < sizeof(token))
		{
			char* slash;
			memcpy(token, s, n);
			token[n] = '\0';
			slash = strchr(token, '/');
			if (slash)
			{
				char* vEnd;
				char* pEnd;
				unsigned long vendor = strtoul(token, &vEnd, 0);
				unsigned long product = strtoul(slash + 1, &pEnd, 0);
				if (vEnd == slash && *pEnd == '\0' && vEnd != token)
					drop = XenoPad_IsHandheldEntry((unsigned)vendor, (unsigned)product);
			}
		}

		if (n > 0)
		{
			if (drop)
			{
				count++;
				if (removed && removedSize && remLen + n + 2 <= removedSize)
				{
					if (remLen) removed[remLen++] = ',';
					memcpy(removed + remLen, s, n);
					remLen += n;
					removed[remLen] = '\0';
				}
			}
			else
			{
				if (outLen + n + 2 > outSize)
					return -1;
				if (outLen) out[outLen++] = ',';
				memcpy(out + outLen, s, n);
				outLen += n;
				out[outLen] = '\0';
			}
		}
		p += len;
		if (*p == ',')
			p++;
	}
	return count;
}

/* Run before SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER). */
extern "C" void PsyX_Pad_ApplyHints()
{
	SDL_version linked;
	const char* ignore = SDL_GetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES);
	const char* except = SDL_GetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT);
	const char* deck = SDL_getenv("SDL_JOYSTICK_HIDAPI_STEAMDECK");
	const char* hidapi = SDL_getenv("SDL_JOYSTICK_HIDAPI");
	const char* steamVirtual = SDL_getenv("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD");

	SDL_GetVersion(&linked);
	eprintinfo("pad: SDL %d.%d.%d; SDL_JOYSTICK_HIDAPI=%s SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD=%s\n",
		linked.major, linked.minor, linked.patch,
		hidapi ? hidapi : "(unset)", steamVirtual ? steamVirtual : "(unset)");

	if (ignore && *ignore)
	{
		size_t size = strlen(ignore) + 1;
		char* kept = (char*)malloc(size);
		char* dropped = (char*)malloc(size + 1);
		int removedCount = (kept && dropped)
			? PsyX_Pad_SanitizeIgnoreDevices(ignore, kept, size, dropped, size + 1) : -1;

		if (removedCount > 0)
		{
			SDL_SetHintWithPriority(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES, kept, SDL_HINT_OVERRIDE);
			eprintinfo("pad: SDL_GAMECONTROLLER_IGNORE_DEVICES: un-ignored %d handheld/InputPlumber "
				"entr%s (%s); kept the other entries\n",
				removedCount, removedCount == 1 ? "y" : "ies", dropped);
		}
		else if (removedCount == 0)
		{
			eprintinfo("pad: SDL_GAMECONTROLLER_IGNORE_DEVICES has no handheld entries; unchanged\n");
		}
		else
		{
			eprintwarn("pad: could not sanitise SDL_GAMECONTROLLER_IGNORE_DEVICES; unchanged\n");
		}
		free(kept);
		free(dropped);
	}

	if (except && *except)
		eprintwarn("pad: SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT is set (%s); only those "
			"controllers are visible\n", except);

	/* Steam Deck-type HID pads (real Deck, InputPlumber deck-uhid).  SDL
	 * enables the driver with HIDAPI by default; an explicit environment
	 * value from the user still wins. */
	if (deck == NULL)
		SDL_SetHint("SDL_JOYSTICK_HIDAPI_STEAMDECK", "1");
	else
		eprintinfo("pad: SDL_JOYSTICK_HIDAPI_STEAMDECK=%s (from environment)\n", deck);
}

static int XenoPad_Classify(SDL_GameController* gc)
{
	SDL_Joystick* js = SDL_GameControllerGetJoystick(gc);
	SDL_JoystickGUID guid = SDL_JoystickGetGUID(js);
	const char* path = SDL_JoystickPath(js);

	if (SDL_JoystickGetVendor(js) == 0x28de && SDL_JoystickGetProduct(js) == 0x11ff)
		return XENO_PAD_SRC_STEAM_VIRTUAL;
	/* SDL >= 2.26 marks the backend in GUID byte 14. */
	if (guid.data[14] == 'h')
		return XENO_PAD_SRC_HIDAPI;
	if (guid.data[14] == 'v')
		return XENO_PAD_SRC_SDL_VIRTUAL;
	if (path && strncmp(path, "/dev/input/", 11) == 0)
		return XENO_PAD_SRC_EVDEV;
	return XENO_PAD_SRC_OTHER;
}

static void XenoPad_Describe(const XenoPadEntry* e, char* buf, size_t size)
{
	SDL_Joystick* js = SDL_GameControllerGetJoystick(e->gc);
	char guid[33];
	const char* name = SDL_GameControllerName(e->gc);

	SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(js), guid, sizeof(guid));
	SDL_snprintf(buf, size, "'%s' guid=%s vid:pid=%04x:%04x source=%s instance=%d",
		name ? name : "?", guid, SDL_JoystickGetVendor(js), SDL_JoystickGetProduct(js),
		PsyX_Pad_SourceName(e->source), (int)e->id);
}

static XenoPadEntry* XenoPad_Find(SDL_JoystickID id)
{
	for (int i = 0; i < XENO_PAD_MAX; i++)
		if (s_xenoPads[i].gc && s_xenoPads[i].id == id)
			return &s_xenoPads[i];
	return NULL;
}

/* SDL_CONTROLLERDEVICEADDED: `deviceIndex` is a device INDEX. */
extern "C" void PsyX_Pad_HostDeviceAdded(int deviceIndex)
{
	SDL_GameController* gc = SDL_GameControllerOpen(deviceIndex);
	SDL_JoystickID id;
	XenoPadEntry* e;
	char desc[256];

	if (!gc)
	{
		eprintwarn("pad: could not open controller at index %d: %s\n", deviceIndex, SDL_GetError());
		return;
	}
	id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc));
	if (XenoPad_Find(id))
	{
		/* Already tracked (SDL re-announces devices at subsystem init). */
		SDL_GameControllerClose(gc);
		return;
	}
	e = NULL;
	for (int i = 0; i < XENO_PAD_MAX; i++)
		if (!s_xenoPads[i].gc) { e = &s_xenoPads[i]; break; }
	if (!e)
	{
		eprintwarn("pad: more than %d controllers; ignoring index %d\n", XENO_PAD_MAX, deviceIndex);
		SDL_GameControllerClose(gc);
		return;
	}
	e->gc = gc;
	e->id = id;
	e->source = XenoPad_Classify(gc);
	e->everInput = 0;
	e->lastInputTicks = 0;
	e->serial = ++s_xenoPadSerial;
	XenoPad_Describe(e, desc, sizeof(desc));
	eprintinfo("pad: connected %s\n", desc);
}

static void XenoPad_Remove(XenoPadEntry* e, const char* why)
{
	char desc[256];
	XenoPad_Describe(e, desc, sizeof(desc));
	eprintinfo("pad: %s %s\n", why, desc);
	if (s_xenoActivePad == e->id)
	{
		s_xenoActivePad = -1;
		s_xenoStickDir = 0;
	}
	SDL_GameControllerClose(e->gc);
	memset(e, 0, sizeof(*e));
}

/* SDL_CONTROLLERDEVICEREMOVED: `instanceId` is an INSTANCE id. */
extern "C" void PsyX_Pad_HostDeviceRemoved(SDL_JoystickID instanceId)
{
	XenoPadEntry* e = XenoPad_Find(instanceId);
	if (e)
		XenoPad_Remove(e, "disconnected");
}

static int XenoPad_HasInput(SDL_GameController* gc)
{
	for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++)
		if (SDL_GameControllerGetButton(gc, (SDL_GameControllerButton)b))
			return 1;
	for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX; a++)
	{
		int v = SDL_GameControllerGetAxis(gc, (SDL_GameControllerAxis)a);
		if (v > XENO_PAD_ACTIVITY_THRESHOLD || v < -XENO_PAD_ACTIVITY_THRESHOLD)
			return 1;
	}
	return 0;
}

static int XenoPad_Better(const XenoPadEntry* a, const XenoPadEntry* b)
{
	int av = a->source == XENO_PAD_SRC_STEAM_VIRTUAL;
	int bv = b->source == XENO_PAD_SRC_STEAM_VIRTUAL;
	if (a->everInput != b->everInput)
		return a->everInput > b->everInput;
	if (av != bv)
		return av < bv;
	return a->serial < b->serial;
}

/* Pick port 1's pad.  Call after SDL has pumped joystick state. */
extern "C" SDL_GameController* PsyX_Pad_HostSelect()
{
	Uint32 now = SDL_GetTicks();
	int inputNow[XENO_PAD_MAX] = { 0 };
	XenoPadEntry* active = NULL;
	XenoPadEntry* next = NULL;
	const char* reason = NULL;

	for (int i = 0; i < XENO_PAD_MAX; i++)
	{
		XenoPadEntry* e = &s_xenoPads[i];
		if (!e->gc)
			continue;
		if (!SDL_GameControllerGetAttached(e->gc))
		{
			XenoPad_Remove(e, "lost");
			continue;
		}
		inputNow[i] = XenoPad_HasInput(e->gc);
		if (e->id == s_xenoActivePad)
			active = e;
	}

	if (!active)
	{
		for (int i = 0; i < XENO_PAD_MAX; i++)
			if (s_xenoPads[i].gc && (!next || XenoPad_Better(&s_xenoPads[i], next)))
				next = &s_xenoPads[i];
		reason = s_xenoHadActive ? "fallback: the previous pad is gone" : "best available pad";
	}
	else
	{
		int activeVirtual = active->source == XENO_PAD_SRC_STEAM_VIRTUAL;
		int activeIdle = !inputNow[active - s_xenoPads] &&
			(!active->everInput || now - active->lastInputTicks >= XENO_PAD_IDLE_SWITCH_MS);

		for (int i = 0; i < XENO_PAD_MAX && !next; i++)
		{
			XenoPadEntry* e = &s_xenoPads[i];
			int virt = e->source == XENO_PAD_SRC_STEAM_VIRTUAL;
			if (!e->gc || e == active || !inputNow[i])
				continue;
			if (activeVirtual && !virt)
			{
				next = e;
				reason = "physical pad input (preferred over Steam virtual pad)";
			}
			else if (!activeVirtual && !virt && activeIdle)
			{
				next = e;
				reason = "input on another pad while the active pad is idle";
			}
			else if (!activeVirtual && virt && !active->everInput)
			{
				next = e;
				reason = "only the Steam virtual pad produces input";
			}
		}
		/* An active pad that never produced input yields to a better one
		 * (a physical pad connected after Steam's virtual pad). */
		if (!next && !active->everInput)
		{
			XenoPadEntry* best = active;
			for (int i = 0; i < XENO_PAD_MAX; i++)
				if (s_xenoPads[i].gc && XenoPad_Better(&s_xenoPads[i], best))
					best = &s_xenoPads[i];
			if (best != active)
			{
				next = best;
				reason = "a preferred pad is available";
			}
		}
	}

	for (int i = 0; i < XENO_PAD_MAX; i++)
	{
		if (s_xenoPads[i].gc && inputNow[i])
		{
			s_xenoPads[i].everInput = 1;
			s_xenoPads[i].lastInputTicks = now;
		}
	}

	if (next)
	{
		char desc[256];
		XenoPad_Describe(next, desc, sizeof(desc));
		eprintinfo("pad: port 1 uses %s (%s)\n", desc, reason);
		s_xenoActivePad = next->id;
		s_xenoStickDir = 0;
		s_xenoPadAnnounced = 1;
		s_xenoHadActive = 1;
		return next->gc;
	}
	if (active)
		return active->gc;

	if (!s_xenoPadAnnounced || s_xenoActivePad != -2)
	{
		eprintinfo("pad: no game controller; port 1 is keyboard only\n");
		s_xenoPadAnnounced = 1;
		s_xenoHadActive = 0;
		s_xenoActivePad = -2;
	}
	return NULL;
}

extern "C" int PsyX_Pad_HostActiveSource()
{
	XenoPadEntry* e = XenoPad_Find(s_xenoActivePad);
	return e ? e->source : -1;
}

extern "C" SDL_JoystickID PsyX_Pad_HostActiveInstance()
{
	return XenoPad_Find(s_xenoActivePad) ? s_xenoActivePad : -1;
}

/* Left stick -> active-low D-pad bits for a digital pad.  Radial deadzone
 * with hysteresis, 8 sectors of 45 degrees (a direction is pressed when its
 * axis exceeds tan(22.5 deg) of the other axis). */
static u_short XenoPad_StickToDpad(int x, int y)
{
	long long mag2 = (long long)x * x + (long long)y * y;
	long long press = (long long)XENO_PAD_STICK_PRESS * XENO_PAD_STICK_PRESS;
	long long release = (long long)XENO_PAD_STICK_RELEASE * XENO_PAD_STICK_RELEASE;
	int ax = x < 0 ? -x : x;
	int ay = y < 0 ? -y : y;
	int dir = 0;
	u_short ret = 0xFFFF;

	if (mag2 >= press || (s_xenoStickDir && mag2 >= release))
	{
		/* 5/12 ~ tan(22.5 deg) = 0.414 */
		if (ax * 12 >= ay * 5)
			dir |= x < 0 ? 0x80 : 0x20;	/* LEFT : RIGHT */
		if (ay * 12 >= ax * 5)
			dir |= y < 0 ? 0x10 : 0x40;	/* UP : DOWN (SDL +y is down) */
	}
	s_xenoStickDir = dir;
	ret &= ~dir;
	return ret;
}

/* Pad fast-forward (opt-in: input.pad_fast_forward = on; default OFF).
 * L3 toggles fast-forward, R3 holds it (like F6 / Backspace); both are
 * suppressed while the dev menu owns the pad.  They count only when the
 * stick click is pressed ALONE (no face button or D-pad held), sampled on
 * XENO_PAD_FF_STABLE consecutive frames: a mis-mapped device (the Legion Go's
 * "SteamOS Handheld Controller" 28de:12fb reported RIGHTSTICK with every A
 * press) asserts the click together with a face button, and must not change
 * the game speed.  PcPort_PadFastForwardEnabled comes from port_main.c. */
extern "C" int PcPort_PadFastForwardEnabled(void) __attribute__((weak));
extern "C" void PsyX_HostFastForwardToggleFrom(const char* source) __attribute__((weak));
extern "C" void PsyX_HostFastForwardPadHoldFrom(int held, const char* source) __attribute__((weak));
#define XENO_PAD_FF_STABLE 2
static int s_xenoL3Stable = 0;
static int s_xenoR3Stable = 0;
static int s_xenoR3Hold = 0;
static int s_xenoFfWarned = 0;
static void XenoPad_FastForwardKeys(SDL_GameController* gc, int suppressed)
{
	int enabled = PcPort_PadFastForwardEnabled && PcPort_PadFastForwardEnabled();
	int l3 = 0, r3 = 0, other = 0, r3Alone, l3Alone;
	if (gc && enabled && !suppressed)
	{
		static const SDL_GameControllerButton guard[] = {
			SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X,
			SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
			SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT };
		l3 = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_LEFTSTICK);
		r3 = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
		for (size_t i = 0; i < sizeof guard / sizeof guard[0]; i++)
			if (SDL_GameControllerGetButton(gc, guard[i]))
				other = 1;
	}
	l3Alone = l3 && !r3 && !other;
	r3Alone = r3 && !l3 && !other;
	if ((l3 || r3) && (other || (l3 && r3)) && !s_xenoFfWarned)
	{
		s_xenoFfWarned = 1;
		eprintwarn("pad: L3/R3 pressed together with a face button, D-pad or each other: "
			"not a fast-forward request (mis-mapped device?)\n");
	}
	s_xenoL3Stable = l3Alone ? s_xenoL3Stable + 1 : 0;
	s_xenoR3Stable = r3Alone ? s_xenoR3Stable + 1 : 0;
	if (s_xenoL3Stable == XENO_PAD_FF_STABLE && PsyX_HostFastForwardToggleFrom)
		PsyX_HostFastForwardToggleFrom("L3");
	r3 = s_xenoR3Stable >= XENO_PAD_FF_STABLE;
	if (r3 != s_xenoR3Hold && PsyX_HostFastForwardPadHoldFrom)
		PsyX_HostFastForwardPadHoldFrom(r3, "R3");
	s_xenoR3Hold = r3;
}
