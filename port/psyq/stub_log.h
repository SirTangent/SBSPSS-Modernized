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
void PSYQ_LogLine(const char *fmt, ...);
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

/*	Log once per (call site, key) - the shape every "unimplemented opcode"
	diagnostic in the shim wants: report the first GP0 command / cop2 op of
	each kind, then stay quiet however hot the stream gets.  The seen-set is
	a 256-bit bitmap local to the expanding site, so two subsystems keyed by
	overlapping opcode numbers never mask each other.  */
#define PSYQ_LOG_ONCE_KEYED(key, ...)								\
	do {															\
		static unsigned long _psyq_seen[8];							\
		unsigned _psyq_k = (unsigned)(key) & 255u;					\
		if (!(_psyq_seen[_psyq_k >> 5] & (1ul << (_psyq_k & 31))))	\
		{															\
			_psyq_seen[_psyq_k >> 5] |= 1ul << (_psyq_k & 31);		\
			PSYQ_LogLine(__VA_ARGS__);								\
		}															\
	} while (0)

#endif
