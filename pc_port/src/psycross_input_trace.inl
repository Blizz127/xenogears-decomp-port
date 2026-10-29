/* psycross_input_trace.inl -- _xeno_input_trace
 *
 * Always-on, rate-limited host input trace, compiled into PsyCross's
 * PsyX_main.cpp (pc_port/patches/psycross_host_hotkeys.patch).  It records
 * what the host actually delivers (keyboard scancodes, mouse buttons and
 * wheel, game-controller buttons/axes, raw joystick buttons/hats/axes) with
 * the device name and GUID, so a handheld's buttons can be identified from
 * a player's terminal log.  Observation only: every event continues to its
 * normal handler.
 *
 * Button events are NEVER throttled: every key, mouse, game-controller and
 * joystick button / hat down and up is logged in full (a rate limit once hid
 * the very lines that explained a pad changing the game speed).  Only axis
 * motion and the mouse wheel share a budget: the first XENO_TRACE_BURST, then
 * at most one per second (the suppressed count is reported).  Window focus
 * changes and joystick attach/detach are always logged too. */

#define XENO_TRACE_BURST       200
#define XENO_TRACE_AXIS_ON     16000
#define XENO_TRACE_FIRST_MAX   256

static int		s_xenoTraceCount = 0;
static Uint32	s_xenoTraceLastTicks = 0;
static unsigned	s_xenoTraceSuppressed = 0;
static struct { SDL_JoystickID id; int kind; int button; } s_xenoTraceFirst[XENO_TRACE_FIRST_MAX];
static int		s_xenoTraceFirstCount = 0;
/* Axis bucket per (instance, axis): -1 / 0 / +1, so axis motion is logged
 * only when it crosses +-XENO_TRACE_AXIS_ON. */
static struct { SDL_JoystickID id; int kind; int axis; int bucket; } s_xenoTraceAxis[128];
static int		s_xenoTraceAxisCount = 0;

static int XenoTrace_Budget(void)
{
	Uint32 now = SDL_GetTicks();
	if (s_xenoTraceCount < XENO_TRACE_BURST)
	{
		s_xenoTraceCount++;
		return 1;
	}
	if (now - s_xenoTraceLastTicks >= 1000)
	{
		s_xenoTraceLastTicks = now;
		if (s_xenoTraceSuppressed)
			eprintinfo("input: (%u events not logged in the last interval)\n", s_xenoTraceSuppressed);
		s_xenoTraceSuppressed = 0;
		return 1;
	}
	s_xenoTraceSuppressed++;
	return 0;
}

static void XenoTrace_Device(SDL_JoystickID id, char* buf, size_t size)
{
	SDL_Joystick* js = SDL_JoystickFromInstanceID(id);
	char guid[33] = "?";
	const char* name = NULL;
	if (js)
	{
		SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(js), guid, sizeof(guid));
		name = SDL_JoystickName(js);
		SDL_snprintf(buf, size, "'%s' guid=%s vid:pid=%04x:%04x inst=%d",
			name ? name : "?", guid, SDL_JoystickGetVendor(js), SDL_JoystickGetProduct(js), (int)id);
	}
	else
		SDL_snprintf(buf, size, "inst=%d", (int)id);
}

/* 1 the first time (instance, kind, button) is pressed. */
static int XenoTrace_FirstPress(SDL_JoystickID id, int kind, int button)
{
	for (int i = 0; i < s_xenoTraceFirstCount; i++)
		if (s_xenoTraceFirst[i].id == id && s_xenoTraceFirst[i].kind == kind && s_xenoTraceFirst[i].button == button)
			return 0;
	if (s_xenoTraceFirstCount < XENO_TRACE_FIRST_MAX)
	{
		s_xenoTraceFirst[s_xenoTraceFirstCount].id = id;
		s_xenoTraceFirst[s_xenoTraceFirstCount].kind = kind;
		s_xenoTraceFirst[s_xenoTraceFirstCount].button = button;
		s_xenoTraceFirstCount++;
	}
	return 1;
}

/* 1 when the axis changed bucket (and the event should be considered). */
static int XenoTrace_AxisCrossed(SDL_JoystickID id, int kind, int axis, int value)
{
	int bucket = value > XENO_TRACE_AXIS_ON ? 1 : value < -XENO_TRACE_AXIS_ON ? -1 : 0;
	for (int i = 0; i < s_xenoTraceAxisCount; i++)
	{
		if (s_xenoTraceAxis[i].id == id && s_xenoTraceAxis[i].kind == kind && s_xenoTraceAxis[i].axis == axis)
		{
			if (s_xenoTraceAxis[i].bucket == bucket)
				return 0;
			s_xenoTraceAxis[i].bucket = bucket;
			return 1;
		}
	}
	if (bucket == 0)
		return 0;
	if (s_xenoTraceAxisCount < (int)(sizeof(s_xenoTraceAxis) / sizeof(s_xenoTraceAxis[0])))
	{
		s_xenoTraceAxis[s_xenoTraceAxisCount].id = id;
		s_xenoTraceAxis[s_xenoTraceAxisCount].kind = kind;
		s_xenoTraceAxis[s_xenoTraceAxisCount].axis = axis;
		s_xenoTraceAxis[s_xenoTraceAxisCount].bucket = bucket;
		s_xenoTraceAxisCount++;
	}
	return 1;
}

static void PsyX_InputTrace(const SDL_Event* e)
{
	char dev[256];

	switch (e->type)
	{
	case SDL_WINDOWEVENT:
		if (e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
			eprintinfo("input: window focus gained\n");
		else if (e->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			eprintinfo("input: window focus lost\n");
		break;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		if (e->key.repeat == 0)
			eprintinfo("input: key %s scancode=%d '%s' mod=0x%04x\n",
				e->type == SDL_KEYDOWN ? "down" : "up", (int)e->key.keysym.scancode,
				SDL_GetScancodeName(e->key.keysym.scancode), (unsigned)e->key.keysym.mod);
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		eprintinfo("input: mouse button %d %s at %d,%d mouse=%u%s\n", (int)e->button.button,
				e->type == SDL_MOUSEBUTTONDOWN ? "down" : "up", e->button.x, e->button.y,
				(unsigned)e->button.which, e->button.which == SDL_TOUCH_MOUSEID ? " (touch)" : "");
		break;
	case SDL_MOUSEWHEEL:
		if (XenoTrace_Budget())
			eprintinfo("input: mouse wheel x=%d y=%d mouse=%u\n", e->wheel.x, e->wheel.y, (unsigned)e->wheel.which);
		break;
	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
	{
		int down = e->type == SDL_CONTROLLERBUTTONDOWN;
		int first = down && XenoTrace_FirstPress(e->cbutton.which, 0, e->cbutton.button);
		{
			XenoTrace_Device(e->cbutton.which, dev, sizeof(dev));
			eprintinfo("input: %spad button %s (%d) %s %s\n", first ? "first " : "",
				SDL_GameControllerGetStringForButton((SDL_GameControllerButton)e->cbutton.button),
				(int)e->cbutton.button, down ? "down" : "up", dev);
		}
		break;
	}
	case SDL_CONTROLLERAXISMOTION:
		if (XenoTrace_AxisCrossed(e->caxis.which, 0, e->caxis.axis, e->caxis.value) && XenoTrace_Budget())
		{
			XenoTrace_Device(e->caxis.which, dev, sizeof(dev));
			eprintinfo("input: pad axis %s = %d %s\n",
				SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)e->caxis.axis),
				(int)e->caxis.value, dev);
		}
		break;
	case SDL_JOYDEVICEADDED:
	{
		/* Open every joystick so raw (non-GameController) devices report too. */
		SDL_Joystick* js = SDL_JoystickOpen(e->jdevice.which);
		if (js)
		{
			XenoTrace_Device(SDL_JoystickInstanceID(js), dev, sizeof(dev));
			eprintinfo("input: joystick attached %s gamecontroller=%s path=%s\n", dev,
				SDL_IsGameController(e->jdevice.which) ? "yes" : "no",
				SDL_JoystickPathForIndex(e->jdevice.which) ? SDL_JoystickPathForIndex(e->jdevice.which) : "?");
		}
		break;
	}
	case SDL_JOYDEVICEREMOVED:
		eprintinfo("input: joystick detached inst=%d\n", (int)e->jdevice.which);
		{
			SDL_Joystick* js = SDL_JoystickFromInstanceID(e->jdevice.which);
			if (js)
				SDL_JoystickClose(js);
		}
		break;
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
	{
		int down = e->type == SDL_JOYBUTTONDOWN;
		int first = down && XenoTrace_FirstPress(e->jbutton.which, 1, e->jbutton.button);
		{
			XenoTrace_Device(e->jbutton.which, dev, sizeof(dev));
			eprintinfo("input: %sjoystick button %d %s %s\n", first ? "first " : "",
				(int)e->jbutton.button, down ? "down" : "up", dev);
		}
		break;
	}
	case SDL_JOYHATMOTION:
		{
			XenoTrace_Device(e->jhat.which, dev, sizeof(dev));
			eprintinfo("input: joystick hat %d = 0x%x %s\n", (int)e->jhat.hat, (unsigned)e->jhat.value, dev);
		}
		break;
	case SDL_JOYAXISMOTION:
		if (XenoTrace_AxisCrossed(e->jaxis.which, 1, e->jaxis.axis, e->jaxis.value) && XenoTrace_Budget())
		{
			XenoTrace_Device(e->jaxis.which, dev, sizeof(dev));
			eprintinfo("input: joystick axis %d = %d %s\n", (int)e->jaxis.axis, (int)e->jaxis.value, dev);
		}
		break;
	}
}
