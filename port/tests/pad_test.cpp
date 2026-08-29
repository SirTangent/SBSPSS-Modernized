/*	SDL3 virtual-gamepad test for the host input path (issue #21) and the
	M6 rumble path - the gamepad-specific code in port/psyq/host/input.cpp
	that keyboard sign-off runs can never reach, verified without physical
	hardware:

	  - hotplug: SDL_EVENT_GAMEPAD_ADDED opens the pad, REMOVED closes it
	  - button-enum mapping: SOUTH/EAST/WEST/NORTH -> Cross/Circle/Square/
	    Triangle, BACK/START -> SELECT/START, shoulders -> L1/R1, D-pad
	  - trigger threshold: axis > 8192 -> L2/R2 (8192 itself is NOT pressed)
	  - stick byte conversion: (axis>>8)+128 maps -32768..32767 -> 0..255
	  - rumble: the PadSetAct motor bytes reach SDL_RumbleGamepad (recorded
	    by the virtual device's Rumble callback), big motor -> low-frequency
	    magnitude, small motor -> high-frequency, re-arm cadence bounded,
	    explicit stop on zero, nothing after disconnect

	The virtual device is typed SDL_JOYSTICK_TYPE_GAMEPAD so SDL maps it
	automatically and emits a real GAMEPAD_ADDED - the shim sees an
	ordinary hotplug.  Headless: the dummy video driver satisfies
	input.cpp's SDL_WasInit(SDL_INIT_VIDEO) gate.
*/
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>

extern unsigned char *Port_PadBuffer[2];		/* pads_shim.cpp */
extern unsigned char *Port_PadMotor[2];
extern "C" void	Port_InputHandleEvent(const void *ev);
extern "C" void	Port_InputFrame(unsigned long vblank);
extern "C" void	PadInitDirect(unsigned char *pad1, unsigned char *pad2);
extern "C" int	PadSetAct(int port, unsigned char *data, int len);

static int g_failures;

static void check(bool ok, const char *what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		g_failures++;
	}
}

/*****************************************************************************/
/*	rumble recorder - the virtual device's driver-side callback  */

struct RumbleCall { Uint16 low, high; };
static RumbleCall	g_rumbleLog[256];
static int			g_rumbleCalls;

static bool SDLCALL recordRumble(void *userdata, Uint16 low, Uint16 high)
{
	(void)userdata;
	if (g_rumbleCalls < 256)
		g_rumbleLog[g_rumbleCalls] = { low, high };
	g_rumbleCalls++;
	return true;
}

/*	forward every pending SDL event to the shim's handler, exactly as
	Host_VBlank does in window.cpp  */
static void pumpEvents(void)
{
	SDL_Event ev;
	SDL_PumpEvents();
	while (SDL_PollEvent(&ev))
		Port_InputHandleEvent(&ev);
}

/*	active-high 16-bit (Button1<<8)|Button2 word recovered from the
	active-low packet bytes  */
static unsigned packetMask(const unsigned char *buf)
{
	return ((unsigned)(unsigned char)~buf[2] << 8) | (unsigned char)~buf[3];
}

int main(void)
{
	SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD))
	{
		std::printf("FAIL: SDL_Init: %s\n", SDL_GetError());
		return 1;
	}

	static unsigned char pad0[34], pad1[34];
	PadInitDirect(pad0, pad1);

	/*	-------- hotplug: attach -> the shim opens the pad  */
	SDL_VirtualJoystickDesc desc;
	SDL_INIT_INTERFACE(&desc);
	desc.type     = SDL_JOYSTICK_TYPE_GAMEPAD;
	desc.naxes    = SDL_GAMEPAD_AXIS_COUNT;
	desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
	desc.name     = "SBSP virtual pad";
	desc.Rumble   = recordRumble;

	SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
	check(id != 0, "SDL_AttachVirtualJoystick");
	pumpEvents();

	SDL_Joystick *joy = SDL_GetJoystickFromID(id);
	check(joy != NULL, "shim opened the virtual gamepad on GAMEPAD_ADDED");
	if (!joy)
	{
		std::printf("pad_test: cannot continue without the virtual device\n");
		return 1;
	}

	unsigned long vblank = 0;

	/*	-------- button-enum mapping (issue #21 list)  */
	static const struct { SDL_GamepadButton btn; unsigned mask; const char *name; }
	buttonCases[] =
	{
		{ SDL_GAMEPAD_BUTTON_SOUTH,          0x0040, "SOUTH -> CROSS"    },
		{ SDL_GAMEPAD_BUTTON_EAST,           0x0020, "EAST -> CIRCLE"    },
		{ SDL_GAMEPAD_BUTTON_WEST,           0x0080, "WEST -> SQUARE"    },
		{ SDL_GAMEPAD_BUTTON_NORTH,          0x0010, "NORTH -> TRIANGLE" },
		{ SDL_GAMEPAD_BUTTON_START,          0x0800, "START"             },
		{ SDL_GAMEPAD_BUTTON_BACK,           0x0100, "BACK -> SELECT"    },
		{ SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,  0x0004, "L1"                },
		{ SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0x0008, "R1"                },
		{ SDL_GAMEPAD_BUTTON_DPAD_UP,        0x1000, "DPAD UP"           },
		{ SDL_GAMEPAD_BUTTON_DPAD_RIGHT,     0x2000, "DPAD RIGHT"        },
		{ SDL_GAMEPAD_BUTTON_DPAD_DOWN,      0x4000, "DPAD DOWN"         },
		{ SDL_GAMEPAD_BUTTON_DPAD_LEFT,      0x8000, "DPAD LEFT"         },
	};
	for (const auto &c : buttonCases)
	{
		SDL_SetJoystickVirtualButton(joy, (int)c.btn, true);
		pumpEvents();
		Port_InputFrame(vblank++);
		char what[64];
		std::snprintf(what, sizeof(what), "button %s pressed", c.name);
		check(packetMask(pad0) == c.mask, what);

		SDL_SetJoystickVirtualButton(joy, (int)c.btn, false);
		pumpEvents();
		Port_InputFrame(vblank++);
		std::snprintf(what, sizeof(what), "button %s released", c.name);
		check(packetMask(pad0) == 0, what);
	}
	check(pad0[0] == 0 && pad0[1] == 0x73, "packet header: connected DualShock");

	/*	-------- trigger threshold: strictly > 8192.
		SDL rescales a virtual gamepad's trigger axes from the raw joystick
		range (-32768..32767, resting at -32768) to the gamepad range
		(0..32767), so drive raw values and assert the shim's decision
		against what SDL_GetGamepadAxis actually reports either side of the
		threshold.  */
	SDL_Gamepad *gp = SDL_GetGamepadFromID(id);
	check(gp != NULL, "SDL_GetGamepadFromID");

	SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, -16400);
	pumpEvents();
	Sint16 tlow = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
	check(tlow > 0 && tlow <= 8192, "raw -16400 reads just below the threshold");
	Port_InputFrame(vblank++);
	check((packetMask(pad0) & 0x0001) == 0, "L2 not pressed at/below 8192");

	SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, -16360);
	pumpEvents();
	Sint16 thigh = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
	check(thigh > 8192 && thigh < 8300, "raw -16360 reads just above the threshold");
	Port_InputFrame(vblank++);
	check((packetMask(pad0) & 0x0001) != 0, "L2 pressed above 8192");

	SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, -32768);
	SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 30000);
	pumpEvents();
	Port_InputFrame(vblank++);
	check((packetMask(pad0) & 0x0003) == 0x0002, "R2 pressed, L2 released");
	SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, -32768);

	/*	-------- stick byte conversion: (axis>>8)+128  */
	static const struct { Sint16 axis; unsigned char expect; const char *name; }
	stickCases[] =
	{
		{      0, 128, "stick centre -> 128" },
		{ -32768,   0, "stick min -> 0"      },
		{  32767, 255, "stick max -> 255"    },
		{  16384, 192, "stick +half -> 192"  },
	};
	for (const auto &c : stickCases)
	{
		SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_LEFTX, c.axis);
		SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_RIGHTY, c.axis);
		pumpEvents();
		Port_InputFrame(vblank++);
		char what[64];
		std::snprintf(what, sizeof(what), "LEFTX %s (buf[6])", c.name);
		check(pad0[6] == c.expect, what);
		std::snprintf(what, sizeof(what), "RIGHTY %s (buf[5])", c.name);
		check(pad0[5] == c.expect, what);
	}
	SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_LEFTX, 0);
	SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_RIGHTY, 0);
	pumpEvents();

	/*	-------- rumble: PadSetAct bytes -> SDL_RumbleGamepad  */
	static unsigned char motor[2];		/* [0] small on/off, [1] big 0-255 */
	PadSetAct(0, motor, 2);
	check(Port_PadMotor[0] == motor, "PadSetAct captured the motor buffer");

	/*	big motor 200 -> low-frequency 0xC8C8, small off  */
	g_rumbleCalls = 0;
	motor[0] = 0; motor[1] = 200;
	Port_InputFrame(vblank++);
	check(g_rumbleCalls == 1, "rumble armed on value change");
	check(g_rumbleCalls > 0 && g_rumbleLog[0].low == 0xC8C8 &&
		  g_rumbleLog[0].high == 0, "big motor 200 -> low 0xC8C8, high 0");

	/*	steady value: the shim re-arms every 3 vblanks to keep refreshing the
		100ms window, but SDL dedupes identical magnitudes at the driver
		boundary (it only extends its expiration timer) - so the virtual
		device must see NO further calls.  This is the anti-spam property:
		a steady game state never floods the driver.  */
	int before = g_rumbleCalls;
	for (int i = 0; i < 6; i++)
		Port_InputFrame(vblank++);
	check(g_rumbleCalls == before, "steady rumble reaches the driver once");

	/*	small motor alone -> high-frequency full-scale  */
	motor[0] = 1; motor[1] = 0;
	before = g_rumbleCalls;
	Port_InputFrame(vblank++);
	check(g_rumbleCalls == before + 1 &&
		  g_rumbleLog[before].low == 0 && g_rumbleLog[before].high == 0xFFFF,
		  "small motor -> high-frequency 0xFFFF");

	/*	zero -> one explicit stop, then silence  */
	motor[0] = 0; motor[1] = 0;
	before = g_rumbleCalls;
	Port_InputFrame(vblank++);
	check(g_rumbleCalls == before + 1 &&
		  g_rumbleLog[before].low == 0 && g_rumbleLog[before].high == 0,
		  "transition to zero sends one explicit stop");
	before = g_rumbleCalls;
	for (int i = 0; i < 6; i++)
		Port_InputFrame(vblank++);
	check(g_rumbleCalls == before, "no rumble calls while idle at zero");

	/*	-------- hotplug: detach -> the shim closes and the pad reads idle  */
	SDL_DetachVirtualJoystick(id);
	pumpEvents();
	Port_InputFrame(vblank++);
	check(packetMask(pad0) == 0, "packet idle after disconnect");
	check(pad0[6] == 0x80, "sticks centred after disconnect");

	motor[1] = 255;
	before = g_rumbleCalls;
	Port_InputFrame(vblank++);
	check(g_rumbleCalls == before, "no rumble calls after disconnect");

	SDL_Quit();

	if (g_failures)
	{
		std::printf("pad_test: %d failure(s)\n", g_failures);
		return 1;
	}
	std::printf("pad_test: all passed\n");
	return 0;
}
