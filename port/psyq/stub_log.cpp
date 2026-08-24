#include <stdio.h>
#include "stub_log.h"

extern "C" void PSYQ_StubOnceNamed(const char *name)
{
	fprintf(stderr, "[shim] stub called: %s\n", name);
	fflush(stderr);
}
