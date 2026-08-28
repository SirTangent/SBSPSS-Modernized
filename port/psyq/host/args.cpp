/*	Command-line arguments (M4 dev tooling).

	The game owns main() with no parameters, so the MinGW CRT globals
	__argc/__argv are read instead - from an early-priority constructor,
	because parts of the shim consume their configuration from static
	initialisers (cd.cpp's CdBoot reads SBSP_DATA_DIR while building the
	virtual disc directory, before main() runs).

	  --level C-L | N   boot straight into a level, skipping the frontend
	                    (needs the Port_BootLevel hook in system/main.cpp).
	                    C-L: chapter 1-5, level 1-5 (L=5 is that chapter's
	                    bonus level; 6-N also addresses bonus level N).
	                    N: raw LvlTable index 0..24.
	                    Env equivalent: SBSP_BOOT_LEVEL (same formats).

	The rest are aliases for the SBSP_* environment variables - the argument
	just sets the variable (overriding an inherited one), and the existing
	consumers stay env-only:

	  --data-dir <path>     SBSP_DATA_DIR
	  --pad-script <s>      SBSP_PAD_SCRIPT
	  --dump-frames <list>  SBSP_DUMP_FRAMES
	  --dump-dir <path>     SBSP_DUMP_DIR
	  --exit-after <n>      SBSP_EXIT_AFTER
	  --dump-audio <wav>    SBSP_DUMP_AUDIO (M5: deterministic mixer dump,
	                        disables the playback device)
	  --no-cd-pace          SBSP_CD_PACE=0
	  --no-audio            SBSP_NO_AUDIO=1
	  --pace-log            SBSP_PACE_LOG=1

	Both "--flag value" and "--flag=value" spellings work.  Unknown
	arguments warn and are ignored (the run continues).  --help prints
	this surface and exits.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int	g_bootLevel = -1;		/* -1 = normal boot (frontend) */

/*	"C-L" (chapter-level) or a raw LvlTable index.  Returns 0..24, or -1 on
	a malformed/out-of-range value.  Index math mirrors LvlTable's layout
	(source/level/level.cpp:139): 5 rows per chapter, row 5 = the chapter's
	bonus (kelp-world) level; rows 25/26 are FMA scenes, not bootable.  */
static int parseLevel(const char *s)
{
	char *end;
	long a = strtol(s, &end, 10);
	if (end == s)
		return -1;
	if (*end == '-')
	{
		long b = strtol(end + 1, &end, 10);
		if (*end)
			return -1;
		if (a >= 1 && a <= 5 && b >= 1 && b <= 5)
			return (int)((a - 1) * 5 + (b - 1));
		if (a == 6 && b >= 1 && b <= 5)		/* kelp world: 6-N = bonus N */
			return (int)((b - 1) * 5 + 4);
		return -1;
	}
	if (*end)
		return -1;
	return (a >= 0 && a <= 24) ? (int)a : -1;
}

static void usage(void)
{
	fprintf(stderr,
		"sbsp [options]\n"
		"  --level C-L | N       boot straight into a level (chapter 1-5,\n"
		"                        level 1-5; L=5 = bonus; or LvlTable index 0-24)\n"
		"  --data-dir <path>     CD data directory        (SBSP_DATA_DIR)\n"
		"  --pad-script <s>      scripted input           (SBSP_PAD_SCRIPT)\n"
		"  --dump-frames <list>  BMP dump vblanks         (SBSP_DUMP_FRAMES)\n"
		"  --dump-dir <path>     where dumps go           (SBSP_DUMP_DIR)\n"
		"  --exit-after <n>      clean exit at vblank n   (SBSP_EXIT_AFTER)\n"
		"  --dump-audio <wav>    mixer audio to WAV       (SBSP_DUMP_AUDIO)\n"
		"  --no-cd-pace          instant loads            (SBSP_CD_PACE=0)\n"
		"  --no-audio            no playback device       (SBSP_NO_AUDIO=1)\n"
		"  --pace-log            frame-pacing stderr log  (SBSP_PACE_LOG=1)\n"
		"See port/docs/debug-tools.md for details.\n");
}

/*	If argv[*i] names this option, set *matched and return its value:
	"--name=value", or "--name value" (advancing *i past the value).
	Returns NULL - with *matched already set, so the caller reports nothing
	further - when the option IS present but its value is missing or is
	itself an option.  Accepting the latter meant "--data-dir --level 1-2"
	silently set SBSP_DATA_DIR=--level and surfaced only much later, as an
	abort() inside CdRead.  */
static const char *argValue(const char *name, int *i, int argc, char **argv,
							int *matched)
{
	const char *a = argv[*i];
	size_t n = strlen(name);

	if (strncmp(a, name, n) != 0 || (a[n] != '\0' && a[n] != '='))
		return NULL;
	*matched = 1;
	if (a[n] == '=')
		return a + n + 1;
	if (*i + 1 < argc && strncmp(argv[*i + 1], "--", 2) != 0)
		return argv[++*i];
	fprintf(stderr, "[args] %s needs a value - ignored\n", name);
	return NULL;
}

/*	Priority 101 (0-100 are reserved): runs before every normal-priority
	static constructor in the program, so the env aliases are in place
	before any consumer - including cd.cpp's CdBoot - reads them.  */
__attribute__((constructor(101)))
static void parseArgs(void)
{
	static const struct { const char *arg; const char *env; } aliases[] =
	{
		{ "--data-dir",    "SBSP_DATA_DIR"    },
		{ "--pad-script",  "SBSP_PAD_SCRIPT"  },
		{ "--dump-frames", "SBSP_DUMP_FRAMES" },
		{ "--dump-dir",    "SBSP_DUMP_DIR"    },
		{ "--exit-after",  "SBSP_EXIT_AFTER"  },
		{ "--dump-audio",  "SBSP_DUMP_AUDIO"  },
	};

	const char *e = getenv("SBSP_BOOT_LEVEL");
	if (e && *e)
	{
		g_bootLevel = parseLevel(e);
		if (g_bootLevel < 0)
			fprintf(stderr, "[args] bad SBSP_BOOT_LEVEL '%s' - booting normally\n", e);
	}

	for (int i = 1; i < __argc; i++)
	{
		const char *v;
		if (strcmp(__argv[i], "--help") == 0 || strcmp(__argv[i], "-h") == 0)
		{
			usage();
			exit(0);
		}
		if (strcmp(__argv[i], "--no-cd-pace") == 0)
		{
			_putenv("SBSP_CD_PACE=0");
			continue;
		}
		if (strcmp(__argv[i], "--pace-log") == 0)
		{
			_putenv("SBSP_PACE_LOG=1");
			continue;
		}
		if (strcmp(__argv[i], "--no-audio") == 0)
		{
			_putenv("SBSP_NO_AUDIO=1");
			continue;
		}
		int matched = 0;
		if ((v = argValue("--level", &i, __argc, __argv, &matched)) != NULL)
		{
			g_bootLevel = parseLevel(v);
			if (g_bootLevel < 0)
				fprintf(stderr, "[args] bad --level '%s' - booting normally\n", v);
		}
		for (int a = 0; !matched && a < (int)(sizeof(aliases) / sizeof(aliases[0])); a++)
		{
			if ((v = argValue(aliases[a].arg, &i, __argc, __argv, &matched)) != NULL)
			{
				char buf[1024];
				snprintf(buf, sizeof(buf), "%s=%s", aliases[a].env, v);
				_putenv(buf);
			}
		}
		if (!matched)
			fprintf(stderr, "[args] unknown argument '%s' (see --help) - ignored\n",
					__argv[i]);
	}

	if (g_bootLevel >= 0)
		fprintf(stderr, "[args] boot level: LvlTable[%d] (chapter %d level %d)\n",
				g_bootLevel, g_bootLevel / 5 + 1, g_bootLevel % 5 + 1);
}

/*	Hook read by system/main.cpp's scene select (PC build only): the
	LvlTable index to boot into, or -1 for the normal frontend boot.  */
extern "C" int Port_BootLevel(void)
{
	return g_bootLevel;
}
