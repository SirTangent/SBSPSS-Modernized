/*	SDL3 keyboard + gamepad -> PS1 controller packet (M3).

	Once per emulated vblank (Host_VBlank), Port_InputFrame rebuilds the
	34-byte pad buffer that PadInitDirect captured (pads_shim.cpp) in the
	PS1 wire format the game's ReadController parses:
	  [0]=0 connected, [1]=0x73 analog controller (type 7, 3 halfwords),
	  [2]/[3] active-low buttons, [4..7] RightX,RightY,LeftX,LeftY.

	Keyboard (RetroArch's default RetroPad layout - the de-facto standard,
	user-confirmed): arrows=D-pad, Z=Cross, X=Circle, A=Square, S=Triangle,
	Enter=Start, RShift=Select, Q=L1, W=R1, E=L2, R=R2.  Sticks centred.
	The first connected SDL gamepad ORs in on top (south/east/west/north =
	Cross/Circle/Square/Triangle, triggers = L2/R2, sticks pass through).
	Port 1 stays disconnected.

	Test tooling: SBSP_PAD_SCRIPT="vblank:HEXmask[,vblank:HEXmask...]"
	injects buttons for automated runs.  Each entry applies from its vblank
	until the next one comes due (by vblank, not by listing order); the
	mask is the active-HIGH 16-bit
	(Button1<<8)|Button2 hardware word (LIBETC.H order), e.g. START=0800,
	CROSS=0040, SELECT=0100, DOWN=4000.
*/
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned char *Port_PadBuffer[2];	/* pads_shim.cpp */
extern unsigned char *Port_PadMotor[2];		/* pads_shim.cpp - PadSetAct buffer */

static SDL_Gamepad	*g_gamepad;

/*	Rumble (M6): last values armed on the device, so a steady game state
	does not spam the driver every vblank, plus the small motor's smoothed
	level (see rumbleFrame).  */
static Uint16			g_rumbleLow, g_rumbleHigh;
static unsigned long	g_rumbleArmVblank;
static int				g_smallLevel;

/*	button masks in the (Button1<<8)|Button2 word (active-high here;
	inverted at packet-build time)  */
#define BTN_SELECT	0x0100
#define BTN_START	0x0800
#define BTN_UP		0x1000
#define BTN_RIGHT	0x2000
#define BTN_DOWN	0x4000
#define BTN_LEFT	0x8000
#define BTN_L2		0x0001
#define BTN_R2		0x0002
#define BTN_L1		0x0004
#define BTN_R1		0x0008
#define BTN_TRI		0x0010
#define BTN_CIRCLE	0x0020
#define BTN_CROSS	0x0040
#define BTN_SQUARE	0x0080

/*****************************************************************************/
/*	SBSP_PAD_SCRIPT parsing  */

struct PadScriptEntry
{
	unsigned long	vblank;
	unsigned		mask;
};
static PadScriptEntry	g_script[64];
static int				g_scriptCount = -1;		/* -1 = not parsed yet */

static void scriptParse(void)
{
	if (g_scriptCount >= 0)
		return;
	g_scriptCount = 0;
	const char *s = getenv("SBSP_PAD_SCRIPT");
	if (!s)
		return;
	while (*s && g_scriptCount < 64)
	{
		char *end;
		unsigned long vb = strtoul(s, &end, 10);
		if (end == s || *end != ':')
			break;
		unsigned mask = (unsigned)strtoul(end + 1, &end, 16);
		g_script[g_scriptCount].vblank = vb;
		g_script[g_scriptCount].mask   = mask & 0xFFFF;
		g_scriptCount++;
		if (*end != ',')
			break;
		s = end + 1;
	}
	if (g_scriptCount)
		fprintf(stderr, "[input] SBSP_PAD_SCRIPT: %d entries\n", g_scriptCount);
}

/*	The entry in force is the LATEST one that has come due - selected by
	vblank, never by position in the string.  (Keeping the last array-order
	match instead made "600:0000,300:0800" hold START forever: at vblank 700
	both are due and the later-listed, earlier-timed entry won.  Scripts are
	normally written in ascending order, but nothing enforces that and a
	silently mistimed automated run is expensive to diagnose.)  */
static unsigned scriptMask(unsigned long vblank)
{
	unsigned		mask = 0;
	int				found = 0;
	unsigned long	best = 0;

	for (int i = 0; i < g_scriptCount; i++)
	{
		if (vblank < g_script[i].vblank)
			continue;
		if (!found || g_script[i].vblank >= best)
		{
			best  = g_script[i].vblank;
			mask  = g_script[i].mask;
			found = 1;
		}
	}
	return mask;
}

/*****************************************************************************/
extern "C" void Port_InputHandleEvent(const void *evv)
{
	const SDL_Event *ev = (const SDL_Event *)evv;
	if (ev->type == SDL_EVENT_GAMEPAD_ADDED && !g_gamepad)
	{
		g_gamepad = SDL_OpenGamepad(ev->gdevice.which);
		if (g_gamepad)
			fprintf(stderr, "[input] gamepad connected: %s\n",
					SDL_GetGamepadName(g_gamepad));
		g_rumbleLow = g_rumbleHigh = 0;		/* fresh device: re-arm from scratch */
		g_smallLevel = 0;
	}
	else if (ev->type == SDL_EVENT_GAMEPAD_REMOVED && g_gamepad &&
			 ev->gdevice.which == SDL_GetGamepadID(g_gamepad))
	{
		SDL_CloseGamepad(g_gamepad);
		g_gamepad = NULL;
		g_rumbleLow = g_rumbleHigh = 0;
		g_smallLevel = 0;
		fprintf(stderr, "[input] gamepad disconnected\n");
	}
}

/*****************************************************************************/
/*	Port_InputFrame runs on every emulated vblank, including before
	Host_EnsureVideo has created the window and on hosts where it never
	will (the SBSP_PAD_SCRIPT path deliberately does not depend on SDL).
	Neither reader may be called before SDL_Init.  */
static bool sdlInputUp(void)
{
	return SDL_WasInit(SDL_INIT_VIDEO) != 0;
}

static unsigned keyboardMask(void)
{
	if (!sdlInputUp())
		return 0;

	const bool *k = SDL_GetKeyboardState(NULL);
	unsigned m = 0;
	if (!k)
		return 0;
	if (k[SDL_SCANCODE_UP])		m |= BTN_UP;
	if (k[SDL_SCANCODE_DOWN])	m |= BTN_DOWN;
	if (k[SDL_SCANCODE_LEFT])	m |= BTN_LEFT;
	if (k[SDL_SCANCODE_RIGHT])	m |= BTN_RIGHT;
	if (k[SDL_SCANCODE_Z])		m |= BTN_CROSS;
	if (k[SDL_SCANCODE_X])		m |= BTN_CIRCLE;
	if (k[SDL_SCANCODE_A])		m |= BTN_SQUARE;
	if (k[SDL_SCANCODE_S])		m |= BTN_TRI;
	if (k[SDL_SCANCODE_RETURN])	m |= BTN_START;
	if (k[SDL_SCANCODE_RSHIFT])	m |= BTN_SELECT;
	if (k[SDL_SCANCODE_Q])		m |= BTN_L1;
	if (k[SDL_SCANCODE_W])		m |= BTN_R1;
	if (k[SDL_SCANCODE_E])		m |= BTN_L2;
	if (k[SDL_SCANCODE_R])		m |= BTN_R2;
	return m;
}

static unsigned gamepadMask(void)
{
	SDL_Gamepad *p = sdlInputUp() ? g_gamepad : NULL;
	unsigned m = 0;
	if (!p)
		return 0;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_DPAD_UP))		m |= BTN_UP;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_DPAD_DOWN))		m |= BTN_DOWN;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_DPAD_LEFT))		m |= BTN_LEFT;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))		m |= BTN_RIGHT;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_SOUTH))			m |= BTN_CROSS;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_EAST))			m |= BTN_CIRCLE;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_WEST))			m |= BTN_SQUARE;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_NORTH))			m |= BTN_TRI;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_START))			m |= BTN_START;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_BACK))			m |= BTN_SELECT;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))	m |= BTN_L1;
	if (SDL_GetGamepadButton(p, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))	m |= BTN_R1;
	if (SDL_GetGamepadAxis(p, SDL_GAMEPAD_AXIS_LEFT_TRIGGER)  > 8192) m |= BTN_L2;
	if (SDL_GetGamepadAxis(p, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 8192) m |= BTN_R2;
	return m;
}

static unsigned char stickByte(SDL_Gamepad *p, SDL_GamepadAxis axis)
{
	if (!p || !sdlInputUp())
		return 0x80;			/* centred */
	int v = (SDL_GetGamepadAxis(p, axis) >> 8) + 128;	/* -32768..32767 -> 0..255 */
	return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

/*****************************************************************************/
/*	Rumble (M6): forward the actuator bytes the game writes into the
	PadSetAct buffer (pads.cpp ReadController) to the SDL gamepad.
	Layout per PadAlign {0,1,...}: byte 0 = small motor on/off, byte 1 =
	big motor 0-255.  The big motor maps to SDL's low-frequency (heavy)
	rumble, the small one to high-frequency.

	The small motor needs smoothing.  The game derives it as
	(intensity & 1) from the SAME summed envelope that drives the big motor
	(pads.cpp:271, and vibe.cpp:130 clamps only the top end), so the bit is
	effectively a coin flip that changes almost every frame throughout any
	vibration.  Driving SDL's high-frequency motor straight off it would
	slam between silence and full scale at ~30Hz - which no physical small
	motor can follow, and which would defeat the re-arm limiter below by
	making the value "change" on nearly every vblank.  A first-order decay
	toward the bit approximates the motor inertia that does the averaging on
	real hardware, and a deadband keeps the residual wobble (and the big
	motor's own per-frame envelope steps) from re-arming constantly.

	SDL_RumbleGamepad is armed with a 100ms window and re-armed every 3
	vblanks while nonzero (50-60ms, comfortably inside the window), or
	immediately on a meaningful change.  A transition to zero sends one
	explicit stop so rumble ends when the game says so, not when the window
	runs out.  */
static void rumbleFrame(unsigned long vblank)
{
	/*	The decay rate and the deadband are a pair: a bit alternating every
		frame settles into a steady oscillation of about +-1000 around the
		half-scale mean, which has to stay INSIDE the deadband or the
		re-arm limiter is defeated exactly as it was before the smoothing. */
	static const int kDeadband = 0x1000;		/* ~6% of full scale */

	if (!g_gamepad)
		return;

	const unsigned char *m = Port_PadMotor[0];
	Uint16 low = 0;
	int    bit = 0;
	if (m)
	{
		low = (Uint16)((m[1] << 8) | m[1]);		/* 0..255 -> 0..0xFFFF */
		bit = m[0] & 1;
	}

	if (!low && !bit)
		g_smallLevel = 0;						/* game says stop: no tail */
	else
		g_smallLevel += ((bit ? 0xFFFF : 0) - g_smallLevel) >> 4;
	Uint16 high = (Uint16)(g_smallLevel < 0 ? 0 : g_smallLevel);

	bool zero     = !low && !high;
	bool wasZero  = !g_rumbleLow && !g_rumbleHigh;
	int  dLow     = (int)low  - (int)g_rumbleLow;
	int  dHigh    = (int)high - (int)g_rumbleHigh;
	bool changed  = (zero != wasZero) ||
					(dLow  > kDeadband || dLow  < -kDeadband) ||
					(dHigh > kDeadband || dHigh < -kDeadband);
	bool rearm    = !zero && (vblank - g_rumbleArmVblank >= 3);
	if (!changed && !rearm)
		return;

	/*	stop = duration 0 with zero magnitudes; active = 100ms window  */
	SDL_RumbleGamepad(g_gamepad, low, high, zero ? 0 : 100);
	g_rumbleLow       = low;
	g_rumbleHigh      = high;
	g_rumbleArmVblank = vblank;
}

extern "C" void Port_InputFrame(unsigned long vblank)
{
	rumbleFrame(vblank);

	unsigned char *buf = Port_PadBuffer[0];
	if (!buf)
		return;

	scriptParse();
	unsigned mask = keyboardMask() | gamepadMask() | scriptMask(vblank);

	buf[0] = 0;								/* connected, data valid */
	buf[1] = 0x73;							/* analog controller, 3 halfwords */
	buf[2] = (unsigned char)~(mask >> 8);	/* active low */
	buf[3] = (unsigned char)~mask;
	buf[4] = stickByte(g_gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
	buf[5] = stickByte(g_gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
	buf[6] = stickByte(g_gamepad, SDL_GAMEPAD_AXIS_LEFTX);
	buf[7] = stickByte(g_gamepad, SDL_GAMEPAD_AXIS_LEFTY);
}
