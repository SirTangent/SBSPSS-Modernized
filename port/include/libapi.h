/*	Shadow of the PSY-Q libapi.h for the Win32 port.

	The vintage header declares two C functions whose names collide with the
	MinGW CRT at different signatures: rename() (libapi.h:61, the memcard
	filesystem one) and _get_errno() (libapi.h:93).  The game calls neither.
	Rename them out of the way and pass through to the vintage header
	(-idirafter puts tools/psyq/include behind this file on the search path).
	The PS1 build never sees this file.
*/
#ifndef _PORT_SHADOW_LIBAPI_H
#define _PORT_SHADOW_LIBAPI_H

#define rename     psyq_sdk_rename
#define _get_errno psyq_sdk_get_errno
#include_next <libapi.h>
#undef rename
#undef _get_errno

#endif
