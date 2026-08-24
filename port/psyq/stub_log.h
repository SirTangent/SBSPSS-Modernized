/*	Log-once helper for unimplemented PSY-Q shim entry points (user-chosen
	stub policy: first call per site prints a warning to stderr, then the
	stub no-ops with safe returns).
*/
#ifndef PORT_STUB_LOG_H
#define PORT_STUB_LOG_H

#ifdef __cplusplus
extern "C" {
#endif
void PSYQ_StubOnceNamed(const char *name);
#ifdef __cplusplus
}
#endif

#define PSYQ_STUB_ONCE()										\
	do {														\
		static int _psyq_stub_logged;							\
		if (!_psyq_stub_logged)									\
		{														\
			_psyq_stub_logged = 1;								\
			PSYQ_StubOnceNamed(__func__);						\
		}														\
	} while (0)

#endif
