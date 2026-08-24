/*	Shadow of the PSY-Q strings.h for the Win32 port.

	The vintage header (tools/psyq/include/STRINGS.H) declares memcpy/strcpy/
	strlen and friends with empty parameter lists ("to avoid conflicting"),
	which in C++ means zero-argument functions - every call site then fails
	under a modern compiler.  The game only wants the ANSI string/memory
	functions, so hand it the real ones.  The PS1 build never sees this file.
*/
#ifndef _PORT_SHADOW_STRINGS_H
#define _PORT_SHADOW_STRINGS_H

#include <string.h>

/*	In the vintage include set, PSY-Q's stdio/stdlib dragged sys/types.h in
	before the SDK headers needed u_char/u_long; MinGW's CRT doesn't.  This
	header sits at global.h:11, right before the SDK block, so it restores
	that ordering guarantee.
*/
#include <sys/types.h>

/*	The vintage stdlib declared only abs(int), so abs(someU32) implicitly
	converted; C++'s int/long/long long overload set makes that ambiguous.
	This overload reproduces the vintage conversion exactly.
*/
#ifdef __cplusplus
#include <stdlib.h>
inline int abs(unsigned long v)	{ return abs((int)v); }
inline int abs(unsigned int v)	{ return abs((int)v); }
#endif

#endif
