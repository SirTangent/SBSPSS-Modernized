/*	Shadow of the PSY-Q sys/types.h for the Win32 port.

	The SDK headers (libcd.h, libgte.h, kernel.h, ...) lean on the BSD-style
	u_char/u_short/u_int/u_long typedefs from the vintage sys/types.h.  With
	tools/psyq/include demoted to -idirafter, a plain #include <sys/types.h>
	would find MinGW's header (which lacks them), so this shadow provides the
	PSY-Q-compatible subset the game and SDK actually use.  The conflicting
	vintage typedefs (time_t as long, dev_t as short, off_t, size_t) are
	deliberately NOT replicated - MinGW's CRT owns those on this platform.
	The PS1 build never sees this file.
*/
#ifndef _PORT_SHADOW_SYS_TYPES_H
#define _PORT_SHADOW_SYS_TYPES_H

#include <stddef.h>		/* size_t */

#ifndef _UCHAR_T
#define _UCHAR_T
typedef	unsigned char	u_char;
#endif
#ifndef _USHORT_T
#define _USHORT_T
typedef	unsigned short	u_short;
#endif
#ifndef _UINT_T
#define _UINT_T
typedef	unsigned int	u_int;
#endif
#ifndef _ULONG_T
#define _ULONG_T
typedef	unsigned long	u_long;
#endif
#ifndef _SYSIII_USHORT
#define _SYSIII_USHORT
typedef	unsigned short	ushort;
#endif

typedef	char *	caddr_t;

#endif
