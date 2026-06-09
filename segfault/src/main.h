// #define malloc this_isnt_how

// NOTES:
// 		http://www.ibm.com/developerworks/library/l-reent/
//		https://www.securecoding.cert.org/confluence/display/seccode/SIG30-C.+Call+only+asynchronous-safe+functions+within+signal+handlers
//
// CREDITS:
// 		Way too many to release this, I had 300 browser tabs open at one point

#include "memutils.h"
#include <bfd.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#include <sys/syscall.h>

#include <sys/stat.h>
// #include <asm/sigcontext.h>
#include <execinfo.h>
// #undef backtrace_symbols;
// #include "bridge.h"

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include <fenv.h>

/* This structure mirrors the one found in /usr/include/asm/ucontext.h */
typedef struct _sig_ucontext {
	unsigned long uc_flags;
	struct ucontext* uc_link;
	stack_t uc_stack;
	struct sigcontext uc_mcontext;
	sigset_t uc_sigmask;
} sig_ucontext_t;

// #include "register-dump.h"

extern "C" {
#include "lua.h"
}
#ifdef CRASH_DEBUG
FILE* outstream = stderr;
#endif

int logfile = 0;
static char __log_arr__[255];
int __log_len__ = 0;
int ___foo_ret___ = 0;

// not safe but what can you do...

void log(const char* str)
{
	char* s = (char*) "<NULLSTR>";
	if (str != NULL) {
		s = (char*) str;
	}
	int len = strlen(s);
	if (logfile) {
		if (write(logfile, s, len)) {
		}
	}
	if (write(2, s, strlen(s))) {
	}
};
void log(const char* str, int len)
{
	char* s = (char*) "<NULLSTR>";
	if (str != NULL) {
		s = (char*) str;
	}
	if (logfile) {
		if (write(logfile, s, len)) {
		}
	}
	if (write(2, s, len)) {
	}
};

// #define log(s) if (logfile) write (logfile, (s==NULL)?"NULL":s, strlen ((s==NULL)?"NULL":s)); write (2, (s==NULL)?"NULL":s, strlen ((s==NULL)?"NULL":s))
#define logf(a, ...)                                                              \
	{                                                                         \
		__log_len__ = snprintf(__log_arr__, 255, a, ##__VA_ARGS__);       \
		if (logfile)                                                      \
			___foo_ret___ = write(logfile, __log_arr__, __log_len__); \
		___foo_ret___ = write(2, __log_arr__, __log_len__);               \
	}

inline bool starts_with(const char* str, const char* pre)
{
	size_t lenpre = strlen(pre);
	size_t lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

struct Handler {
	int type;
	struct sigaction sigaction;
	struct sigaction old_sigaction;
};

Handler signal_handlers[] =
    {
	{SIGHUP},
	//{SIGINT},
	{SIGBUS},
	{SIGQUIT},
	{SIGILL},
	{SIGUSR1},
	{SIGUSR2},
	{SIGABRT},
	//{SIGFPE},
	{SIGSEGV},
	//{SIGPIPE},
	{-1}};

#define STACKSIZE 4096

static ucontext_t thread_context;
static ucontext_t scheduler_context;
// char thread_stack[STACKSIZE];
struct sigaction sih;
struct sigaction old_sih;

lua_State* GLUA;

pid_t main_thread;

#ifdef CRASH_DEBUG
static bfd* abfd = 0;
static asymbol** syms = 0;
static asection* text = 0;
#endif

stack_t signal_alt_stack;
static bool sig_loaded = false;

#ifdef CRASH_DEBUG
void printStackTrace(void* stk[], int depth);

bool cause_stackoverflow1(unsigned long val);
bool cause_stackoverflow(unsigned long val);

bool cause_stackoverflow1(unsigned long val)
{
	return cause_stackoverflow(val + 1);
}

bool cause_stackoverflow(unsigned long val)
{
	bool hurr = cause_stackoverflow1(val + 1);
}
#endif

// func_PhysicsGameSystem
class CPhysicsHook;
typedef CPhysicsHook* (*tPhysicsGameSystem)();
tPhysicsGameSystem func_PhysicsGameSystem = NULL;
void SetPhysPaused(bool should)
{
	int physhook_class = (int) func_PhysicsGameSystem();
	bool* m_bPaused;
	m_bPaused = (bool*) (physhook_class + 88); // from CPhysicsHook::LevelInitPostEntity()
	*m_bPaused = should;
}

bool StopPhysicsDamnit()
{
	// dlopen from a signal, are you NUTS?
	void* handle = dlopen("garrysmod/lua/bin/gmsv_physframe_linux.dll", RTLD_LAZY);
	if (!handle) {
		handle = dlopen("gmsv_physframe_linux.dll", RTLD_LAZY);
		if (!handle)
			return false;
	}

	void (*func)() = (void (*)()) dlsym(handle, "stop_physics_damnit");
	if (func) {
		func();
	}
	dlclose(handle);
	return func != NULL;
}