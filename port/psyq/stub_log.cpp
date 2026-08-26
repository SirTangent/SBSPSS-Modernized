#include <stdarg.h>
#include <stdio.h>
#include "stub_log.h"

extern "C" void PSYQ_StubOnceNamed(const char *name)
{
	fprintf(stderr, "[shim] stub called: %s\n", name);
	fflush(stderr);
}

/*	Backing printer for PSYQ_LOG_ONCE_KEYED - keeps <stdio.h> and the flush
	policy out of the header and out of every caller.  */
extern "C" void PSYQ_LogLine(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fflush(stderr);
}
