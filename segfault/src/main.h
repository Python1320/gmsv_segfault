//#define malloc this_isnt_how

// NOTES:
// 		http://www.ibm.com/developerworks/library/l-reent/
//		https://www.securecoding.cert.org/confluence/display/seccode/SIG30-C.+Call+only+asynchronous-safe+functions+within+signal+handlers
//		
// CREDITS:
// 		Way too many to release this, I had 300 browser tabs open at one point


#include <pthread.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>  
#include <string.h>
#include <signal.h>
#include <ucontext.h>
#include <errno.h>
#include <bfd.h>
#include <dlfcn.h>
#include <link.h>
#include <fcntl.h>
#include <setjmp.h>
#include <cxxabi.h>
#include <sys/types.h>
 
#include <sys/syscall.h>

#include <sys/stat.h>
#include <asm/sigcontext.h>
#include <execinfo.h>
	//#undef backtrace_symbols;
//#include "bridge.h"

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#define _GNU_SOURCE
#include <fenv.h>

//////////////////77
static const char *

unw_strerror(int err)
{
   if (err < 0) {
      err = -err;
   }
   switch (err) {
   case UNW_ESUCCESS:	    return "No error";
   case UNW_EUNSPEC:	    return "Unspecified (general) error";
   case UNW_ENOMEM:	    return "Out of memory";
   case UNW_EBADREG:	    return "Bad register number";
   case UNW_EREADONLYREG:   return "Attempt to write read-only register";
   case UNW_ESTOPUNWIND:    return "Stop unwinding";
   case UNW_EINVALIDIP:	    return "Invalid IP";
   case UNW_EBADFRAME:	    return "Bad frame";
   case UNW_EINVAL:	    return "Unsupported operation or bad value";
   case UNW_EBADVERSION:    return "Unwind info has unsupported version";
   case UNW_ENOINFO:        return "No unwind info found";
   default:		    return "Unknown error";
   }
}
////////////////////////



/* This structure mirrors the one found in /usr/include/asm/ucontext.h */
typedef struct _sig_ucontext {
 unsigned long     uc_flags;
 struct ucontext   *uc_link;
 stack_t           uc_stack;
 struct sigcontext uc_mcontext;
 sigset_t          uc_sigmask;
} sig_ucontext_t;

//#include "register-dump.h"

extern "C" {
	#include "lua.h"
}
#ifdef CRASH_DEBUG
FILE * outstream = stderr;
#endif

int logfile = 0;
static char __log_arr__[255]; 
int __log_len__ = 0;

// not safe but what can you do...

void log(char * s) {
	static const char nullstr = "<NULLSTR>";
	if (s==NULL) {
		s=nullstr;
	}
	int len = strlen(s);
	if (logfile) {
		write (logfile, s, len); 
	}
	write (2, s, strlen(s));
};

//#define log(s) if (logfile) write (logfile, (s==NULL)?"NULL":s, strlen ((s==NULL)?"NULL":s)); write (2, (s==NULL)?"NULL":s, strlen ((s==NULL)?"NULL":s))
#define logf(a,...) __log_len__ = snprintf (__log_arr__, 255,a, ##__VA_ARGS__);\
					if (logfile) write (logfile,	__log_arr__, __log_len__ ); \
					write (2, 		__log_arr__, __log_len__ )

struct Handler
{	int type;
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
	{SIGUSR2},
	{SIGABRT},
	//{SIGFPE},
	{SIGSEGV},
	//{SIGPIPE},
	{-1}
};

#define STACKSIZE 4096

static ucontext_t thread_context;
static ucontext_t scheduler_context;
char thread_stack[STACKSIZE];
struct sigaction sih;
struct sigaction old_sih;
	
lua_State* GLUA;

pid_t main_thread;

#ifdef CRASH_DEBUG
static bfd* abfd = 0;
static asymbol **syms = 0;
static asection *text = 0;
#endif

stack_t signal_alt_stack;	
static bool sig_loaded = false;

#ifdef CRASH_DEBUG
void printStackTrace(void* stk[],int depth);



bool cause_stackoverflow1(unsigned long val);
bool cause_stackoverflow(unsigned long val);

bool cause_stackoverflow1(unsigned long val) {
	return cause_stackoverflow(val+1);
}

bool cause_stackoverflow(unsigned long val) {
	bool hurr = cause_stackoverflow1(val+1);
	
}
#endif