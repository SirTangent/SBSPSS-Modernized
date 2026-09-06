/*	M8 harness diagnostics - see diag.h.

	  [scene] <name> vblank=<n>                       GameState opened a scene;
	                                                  FMA scripts add FMA:<script>
	  [assert] <expr> at <file>:<line> (<scene>, vblank <n>)
	  [summary] exit=<code> vblanks=<n> scene=<name> asserts=<n>

	Exit codes: 0 clean, 10 assert, 11 fault, 12 watchdog, 13 oracle/replay.
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "system/types.h"
#include "system/asmport.h"		/* PORT_Scratchpad + guard */
#include "host/diag.h"
#include "host/pump.h"

extern "C" unsigned long GPU_PrimPoolPeak(void);	/* gpu/gp0.cpp */

namespace
{

struct SceneRec
{
	std::string					name;
	std::vector<unsigned long>	opens;		/* vblank of each open, in order */
};

std::vector<SceneRec>	g_scenes;
std::string				g_currentScene = "boot";
unsigned long			g_assertCount;
PortGameGlobals			g_globals;		/* all NULL until Port_RegisterGameGlobals */
unsigned long			g_peakRam;
int						g_peakNodes;

SceneRec *findScene(const char *name)
{
	for (SceneRec &r : g_scenes)
		if (r.name == name)
			return &r;
	return NULL;
}

int envFlag(const char *name)
{
	const char *e = getenv(name);
	return e && *e && *e != '0';
}

void noteOpen(const char *name)
{
	unsigned long vb = Port_VBlankCount();
	SceneRec *r = findScene(name);
	if (!r)
	{
		g_scenes.push_back(SceneRec());
		r = &g_scenes.back();
		r->name = name;
	}
	r->opens.push_back(vb);
	g_currentScene = name;
	fprintf(stderr, "[scene] %s vblank=%lu\n", name, vb);
	fflush(stderr);
}

}

/*****************************************************************************/
extern "C" void Port_SceneEvent(const char *name)
{
	noteOpen(name ? name : "?");
}

extern "C" void Port_FmaEvent(int script)
{
	/* index = CFmaScene::FMA_SCRIPT_NUMBER (source/fma/fma.h) */
	static const char *const names[] =
	{
		"FMA:INTRO",       "FMA:CH1FINISHED", "FMA:CH2FINISHED", "FMA:CH3FINISHED",
		"FMA:CH4FINISHED", "FMA:CH5FINISHED", "FMA:PLANKTON",    "FMA:PARTY",
	};
	char buf[32];
	if (script >= 0 && script < (int)(sizeof(names) / sizeof(names[0])))
		noteOpen(names[script]);
	else
	{
		snprintf(buf, sizeof(buf), "FMA:%d", script);
		noteOpen(buf);
	}
}

extern "C" const char *Port_CurrentScene(void)
{
	return g_currentScene.c_str();
}

/*****************************************************************************/
extern "C" void Port_RegisterGameGlobals(unsigned long *ramUsed, int *memNodeCount,
										 int *invincibleSponge,
										 unsigned char **currPrim, unsigned char **endPrim,
										 unsigned char **primListStart, unsigned char **primListEnd)
{
	g_globals.ramUsed          = ramUsed;
	g_globals.memNodeCount     = memNodeCount;
	g_globals.invincibleSponge = invincibleSponge;
	g_globals.currPrim         = currPrim;
	g_globals.endPrim          = endPrim;
	g_globals.primListStart    = primListStart;
	g_globals.primListEnd      = primListEnd;

	/*	--invincible: the DEBUG pause menu's toggle (player.cpp
		invincibleSponge, read by CPlayer::takeDamage) exists in both
		variants; only the menu is DEBUG-only.  */
	if (invincibleSponge && envFlag("SBSP_INVINCIBLE"))
	{
		*invincibleSponge = 1;
		fprintf(stderr, "[args] invincible: on\n");
	}
}

extern "C" const PortGameGlobals *Port_GameGlobals(void)
{
	return &g_globals;
}

/*****************************************************************************/
/*	Mirrors gp0.cpp's primPoolWatch: high-water logging behind an env flag,
	one-shot warnings at 87.5% of a hard cap.  MemNodeCount's cap is LListLen
	(256, mem/memory.h) - the game's own `ASSERT(MemNodeCount<LListLen)` is
	DEBUG-only and post-hoc.  */
extern "C" void Port_MemWatch(void)
{
	static int	logging = -1;
	static int	warnedNodes, warnedPad;

	if (logging < 0)
		logging = envFlag("SBSP_MEM_LOG");

	if (g_globals.ramUsed && *g_globals.ramUsed > g_peakRam)
	{
		g_peakRam = *g_globals.ramUsed;
		if (logging)
			fprintf(stderr, "[mem] RamUsed high-water %lu bytes (%s, vblank %lu)\n",
					g_peakRam, g_currentScene.c_str(), Port_VBlankCount());
	}

	if (g_globals.memNodeCount)
	{
		int n = *g_globals.memNodeCount;
		if (n > g_peakNodes)
			g_peakNodes = n;
		if (!warnedNodes && n >= 256 - 256 / 8)
		{
			warnedNodes = 1;
			fprintf(stderr, "[mem] WARNING: MemNodeCount at %d/256 (%s, vblank %lu) - "
							"raise LListLen (source/mem/memory.h) before it overruns\n",
					n, g_currentScene.c_str(), Port_VBlankCount());
		}
	}

	if (!warnedPad)
	{
		for (int i = 0; i < PORT_SCRATCHPAD_GUARD; i++)
		{
			if (PORT_Scratchpad[1024 + i] != PORT_SCRATCHPAD_GUARD_BYTE)
			{
				warnedPad = 1;
				fprintf(stderr, "[mem] LEAK scratchpad overrun: guard byte %d = 0x%02X "
								"(%s, vblank %lu)\n",
						i, PORT_Scratchpad[1024 + i], g_currentScene.c_str(),
						Port_VBlankCount());
				break;
			}
		}
	}
	fflush(stderr);
}

extern "C" int Port_SceneOpenCount(const char *name)
{
	SceneRec *r = findScene(name);
	return r ? (int)r->opens.size() : 0;
}

extern "C" int Port_SceneOpenVblank(const char *name, int nth, unsigned long *vblank)
{
	SceneRec *r = findScene(name);
	if (!r || nth < 1 || nth > (int)r->opens.size())
		return 0;
	*vblank = r->opens[nth - 1];
	return 1;
}

/*****************************************************************************/
extern "C" void Port_Assert(const char *expr, const char *file, int line)
{
	g_assertCount++;
	fprintf(stderr, "[assert] %s at %s:%d (%s, vblank %lu)\n",
			expr, file, line, g_currentScene.c_str(), Port_VBlankCount());
	fflush(stderr);
	if (!envFlag("SBSP_ASSERT_CONTINUE"))
		Port_Exit(PORT_EXIT_ASSERT);
}

/*****************************************************************************/
extern "C" void Port_Exit(int code)
{
	static volatile LONG entered;

	/*	Second caller (a fault while printing the summary, or the watchdog
		thread racing the main thread): the first owns the summary.  */
	if (InterlockedCompareExchange(&entered, 1, 0) != 0)
	{
		fflush(stderr);
		_exit(code);
	}

	fprintf(stderr, "[summary] exit=%d vblanks=%lu scene=%s asserts=%lu "
					"peak_ram=%lu peak_memnodes=%d/256 peak_prim=%lu\n",
			code, Port_VBlankCount(), g_currentScene.c_str(), g_assertCount,
			g_peakRam, g_peakNodes, GPU_PrimPoolPeak());
	fflush(stderr);
	_exit(code);
}
