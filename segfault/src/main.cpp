#define CRASH_DEBUG
#include "main.h"


			/*static const char* luatypes[] = 
			{
				"nil",
				"bool",
				"lightuserdata",
				"number",
				"string",
				"table",
				"function",
				"userdata",
				"thread",
				"entity",
				"vector",
				"angle",
				"physobj",
				"save",
				"restore",
				"damageinfo",
				"effectdata",
				"movedata",
				"recipientfilter",
				"usercmd",
				"vehicle",
				"material",
				"panel",
				"particle",
				"particleemitter",
				"texture",
				"usermsg",
				"convar",
				"mesh",
				"matrix",
				"sound",
				"pixelvishandle",
				"dlight",
				"video",
				"file",

				0,
				0,
				0,
				0,
				0,
				0,
				0
				
			};
			*/
//extern "C" 
inline int lua_dostackprint(lua_State *l,bool nondestructive) {
    int i,top;
	log("\nSTACK:");
    top = lua_gettop(l);
    logf("%d",top);
	log(": ");
    for (i = 1; i <= top; i++) 
	{
		int t = lua_type(l, i);
 		if (i>1) { log(", "); };
        switch (t) {
            case LUA_TSTRING: {
				/*if (nondestructive) {
				    log("<string>");
				} else*/
					log("'");
					size_t len=0;
					const char * str = lua_tolstring(l, i,&len);
					log(str,len);
					log("'");
                break;
			}
            case LUA_TBOOLEAN:
                log("B");
				log(lua_toboolean(l, i) ? "1" : "0");
                break;
            case LUA_TNUMBER:
                log("L");
				lua_Number lnum;
				lnum = lua_tonumber(l, i);
                log("N");
				logf("%g", lnum);
                break;
			case LUA_TNONE:
				logf ("<non>");
				break;
			case LUA_TLIGHTUSERDATA:
				logf ("<ptr>");
				break;
			case LUA_TTABLE:
				logf ("<tbl>");
				break;
			case LUA_TNIL:
				logf ("<nil>");
				break;
			case LUA_TFUNCTION:
				logf ("<func>");
				break;
			case LUA_TUSERDATA:
				if (nondestructive) {
					logf ("<UserData>");
				} else {
					log("<");
					log(lua_typename(l, t));
					log(">");
				}
				break;
            default:
                log("< ");
				if (nondestructive) {
					logf("unknown=%d",t);
				} else {
					log(lua_typename(l, t));
				}
				log(">");
                break;
        }
    }
    log(".\n");
	if (nondestructive) return 0;
		
    log(".\n");
	
}


inline void lua_stacktrace(lua_State* L)
{
    lua_Debug entry;
    int depth = 0;

	log("\nLUA\n");
    while (lua_getstack(L, depth, &entry))
    {
        log("| ");
		int status = lua_getinfo(L, "Sln", &entry);
        if (status) {
			logf("%i\t",depth);
			log(entry.short_src);
			logf(":%d: ", entry.currentline);
			log(entry.name ? entry.name : "?");
			log("\n");
			depth++;
		} else {
			log("\n");
		}
		if (depth>5) {
			log("END:maxdepth\n");
			return;
		}
    }
}


static volatile bool in_fail = false;
#define BACKTRACE_DEPTH 25

int crash_sg_nr = 0;
siginfo_t* crash_info = 0;
void * crash_ucontext = 0;
ucontext_t * ctx;

static sigjmp_buf                   jmpbuf;
static volatile sig_atomic_t        shouldjump = 0;
unsigned char jumps=0;
#define checkpoint  shouldjump = 1; if (sigsetjmp(jmpbuf,1)==0) \

/*
struct file_match {
	// in
	const void *address;
	
	// out
	const char *file;
	void *base;
};

static int iterate_libraries_callback(struct dl_phdr_info *info,
		size_t size, void *data)
{
	struct file_match *match = (struct file_match*) data;
	// This code is modeled from Gfind_proc_info-lsb.c:callback() from libunwind 
	long n;
	const ElfW(Phdr) *phdr;
	ElfW(Addr) load_base = info->dlpi_addr;
	phdr = info->dlpi_phdr;
	for (n = info->dlpi_phnum; --n >= 0; phdr++) {
		if (phdr->p_type == PT_LOAD) {
			ElfW(Addr) vaddr = phdr->p_vaddr + load_base;
			if ((uintptr_t)match->address >= vaddr && (uintptr_t)match->address < vaddr + phdr->p_memsz) {
				
				match->file = info->dlpi_name;
				match->base = (void*)(uintptr_t)info->dlpi_addr;
				return 1;
			}
		}
	}
	return 0;
}
bool find_shared_object_name(const void* ip, struct file_match* match) {
	match->address = ip;
	dl_iterate_phdr(iterate_libraries_callback, match);
	return;
}

*/

#define DEMANGLE_LEN 511
char *demanglealloc;
inline char * demangle_func(const char * funcName) {
	size_t alloclen = DEMANGLE_LEN;
	int status = 0;
	
	demanglealloc[0]='\0';
	//logf("Demangle: '%s' '%s'",funcName,demanglealloc); log(".\n");
	char* demangled = abi::__cxa_demangle(funcName, demanglealloc, &alloclen, &status);
	//log("DemangleFIN ");logf(": '%s' '%s' '%s'\n",funcName,demanglealloc,demangled);

	if (status == 0) {
		if ( demangled && (demangled==demanglealloc) ) {
			return demanglealloc;
		}
		
		if ( demangled && (demangled!=demanglealloc) ) {
			log("demanglealloc = demangled (2)\n");
				demanglealloc = demangled;
			return demangled;
		}
		
	} else {
		if ( demangled && (demangled!=demanglealloc) ) {
			log("demanglealloc = demangled\n");
				demanglealloc = demangled;
		}
	}
	return NULL;
}
void lua_hookhack(lua_State* L, lua_Debug *ar) {
	lua_sethook(GLUA,NULL,0,0);
	lua_pushstring(L,"SIGUSR1 HACK <EPICFAIL HAS OCCURED>");
	lua_error(L);
}

int saved_errno=0;
static void ERROR_SIGNAL_HANDLER_FUNC(int sig_nr, siginfo_t* info, void *ucontext) {
	/*
	if (sig_nr==SIGFPE) {
		fedisableexcept(FE_INVALID);
		//signal(sig_nr,SIG_IGN);
		int fe_code = info->si_code;
		switch (fe_code)
		{
			case FPE_FLTDIV: log("FPE: FPE_FLTDIV\n"); break;
			case FPE_FLTINV: log("."); break;
			case FPE_FLTOVF: log("FPE: FPE_FLTOVF\n"); break;
			case FPE_FLTUND: log("FPE: FPE_FLTUND\n"); break;
			case FPE_FLTRES: log("FPE: FPE_FLTRES\n"); break;
			case FPE_FLTSUB: log("FPE: FPE_FLTSUB\n"); break;
			case FPE_INTDIV: log("FPE: FPE_INTDIV\n"); break;
			case FPE_INTOVF: log("FPE: FPE_INTOVF\n"); break;
			default: log("FPE: ERR?\n"); break;
		 }
		ucontext_t* uc = (ucontext_t *)ucontext;
        uc->uc_mcontext.gregs[REG_EIP] += 3;
		
		return;
	}*/
	

	// restore...
	bool restored=false;
	struct sigaction isoursih;
	void * caller_address;
	
	//void * realarray[BACKTRACE_DEPTH];
	//void ** array=realarray;
	char **            messages;
	int                size, i;
	
	if (sig_nr == SIGUSR1) 
	{
		log("SIGUSR1:lua_sethook hack\n");
		lua_sethook(GLUA,lua_hookhack,LUA_MASKCOUNT,10);
		return;
	}
	
	
	
	if (in_fail && shouldjump==1 && jumps<100) {
		jumps++;
		shouldjump=0;
		log("FAIL:debugger_crashed");
		siglongjmp(jmpbuf, 1);
		log("FAIL:siglongjmp");
	}
	
	saved_errno = errno;
	crash_sg_nr = sig_nr;
	crash_info = info;
	crash_ucontext = ucontext;
	
	ctx = (ucontext_t *)ucontext;
	
	if (in_fail) {
		for(i = 0; signal_handlers[i].type != -1; ++i)
			signal(signal_handlers[i].type,SIG_DFL);
		log("RECALLED:BAILING_OUT\n");
		return;
	};
	in_fail = true;

	if (crash_sg_nr == SIGUSR2) 
	{
		log("SIGUSR2:BEGIN\n");
	}	


	
	// DO FUCKING NOTHING
	if (crash_sg_nr != SIGUSR2) {
		for(i = 0; signal_handlers[i].type != -1; ++i)
		{
			if(signal_handlers[i].type == crash_sg_nr)
			{		
				//sigemptyset(&signal_handlers[i].sigaction.sa_mask);
				//signal_handlers[i].sigaction.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;
				
				if (sigaction(crash_sg_nr,/*&signal_handlers[i].sigaction*/NULL,&isoursih) != 0) {
					//log("Restoring our signal failed???\n");
				};//.sa_sigaction(code, crash_info, context);
				if (signal_handlers[i].sigaction.sa_sigaction!=isoursih.sa_sigaction) 
				{
					log("WARNING:REGISTERED SIGNAL NOT US\n");
				}
				
				restored = true; // always true
				break;
			}
		}
	} else restored = true;

	if (!restored) {
		log("SIGRESTOREFAIL"); //???
		return;
	}
	log("\n");

	
	checkpoint {
		caller_address = (void *) ctx->uc_mcontext.gregs [REG_EIP]; 
		log("\nCRASH:");
		log(" Signal=");
			logf("%i",crash_sg_nr);
			logf(",%i",crash_info->si_code);
			pid_t cur_thread=syscall(SYS_gettid);
			
			if ( main_thread!=cur_thread ) {
				log(" | THREAD_CRASH!");
			}
			log("\n");
		log("\nTIME:");
			logf("%lu",time(NULL));
			log("\n");
			
		log("Fault=");
		logf("%p",crash_info->si_addr);
		log(" Caller=");
		logf("%p",(void *)caller_address);
		log("\n");
	} else  { log("Failed printing signal err!\n"); }
	
	
		
	checkpoint {
		logf( "REG: gs %x fs: %x es: %x ds: %x edi: %x esi: %x ebp: %x esp: %x ebx: %x edx: %x ecx: %x\n",
				 ctx->uc_mcontext.gregs [REG_GS], ctx->uc_mcontext.gregs [REG_FS], ctx->uc_mcontext.gregs [REG_ES], ctx->uc_mcontext.gregs [REG_DS],
				 ctx->uc_mcontext.gregs [REG_EDI], ctx->uc_mcontext.gregs [REG_ESI], ctx->uc_mcontext.gregs [REG_EBP], ctx->uc_mcontext.gregs [REG_ESP],
				 ctx->uc_mcontext.gregs [REG_EBX], ctx->uc_mcontext.gregs [REG_EDX], ctx->uc_mcontext.gregs [REG_ECX], ctx->uc_mcontext.gregs [REG_EAX]);
				logf( "REG: eax: %x trap: %u err: %x eip: %x cs: %x flag: %x sp: %x ss: %x cr2: %lx\n",
				 ctx->uc_mcontext.gregs [REG_TRAPNO], ctx->uc_mcontext.gregs [REG_ERR], ctx->uc_mcontext.gregs [REG_EIP], ctx->uc_mcontext.gregs [REG_CS],
				 ctx->uc_mcontext.gregs [REG_EFL], ctx->uc_mcontext.gregs [REG_UESP], ctx->uc_mcontext.gregs [REG_SS], ctx->uc_mcontext.cr2
		);
	} else  { log("Failed printing registers!\n"); }
	
	//////////////////
	
	/*
	checkpoint {

        void * p = __builtin_return_address(0);
        logf("__builtin_return_address = %x\n", p);
       // p = __builtin_return_address(1);
       // logf("__builtin_return_address = %x\n", p);
	}
	*/
	
	checkpoint {
		size_t SET_IP=0;
		if (crash_sg_nr==SIGSEGV && caller_address<(void*)0xFF) {
			log("\nTRACE: (Info lost, called NULL function? Recovering at least return info: EIP <- ESP ");
			size_t ESP(*reinterpret_cast<size_t *> (ctx->uc_mcontext.gregs[REG_ESP]));
			logf("%p",ESP);
			if (ESP>0xFF) {
				SET_IP=ESP;//__builtin_return_address(0);
			} else 
			{
				log("<Invalid!!!>");
			}
			log("):\n");
			
		} else {
			log("\nTRACE\n");
		}
		
		
		
		
		
		unw_cursor_t    cursor;

		unw_context_t   context;
		int err=0;
		err = unw_getcontext(&context);
		bool got_frame=false;
		if (err==0) {
			err = unw_init_local(&cursor, &context);
			if (SET_IP>0)
				unw_set_reg(&cursor, UNW_REG_IP, SET_IP);
			if (err==0) {
				
				//
				int j=0;
				do {
					j++;
					if (j>50) {
						log("\nEND:big_stack\n");
						break;
					}
					
					unw_word_t  offset, pc, sp;
					char        fname[255];

					unw_get_reg(&cursor, UNW_REG_IP, &pc);
					unw_get_reg(&cursor, UNW_REG_SP, &sp);

					fname[0] = '\0';
					(void) unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);
					
					
					if (strstr(fname,"Host_RunFrame") != NULL) {
						log( "END:Host_RunFrame\n");						
						got_frame = true;
						
						break;
					}
					
					
					char * demangled = NULL;
					if (fname[0]!='\0') {
						demangled = demangle_func(fname);
					}
	
					Dl_info info;

					const char * fallback_name = NULL;
					if ( (!demangled || *demangled=='\0') && (fname[0]=='\0') ) {			
						fallback_name = info.dli_sname;
					}
					
					logf("%2i %p %p ", j, pc, sp);
					logf("%s +%p ",fallback_name?fallback_name:demangled?demangled:( (fname[0]=='\0')?"?":fname), offset);
					if (unw_is_signal_frame(&cursor)>0) log("(SF)");
					if (dladdr((void *)pc, &info)) {
						log(" \t\t@ ");
						if ( (info.dli_fname!=NULL) && (*info.dli_fname!='\0') ) {
							log(basename(info.dli_fname));
						} else if ( (info.dli_sname != NULL) && (*info.dli_sname!='\0')  ) {
							log(info.dli_sname);
						} else {
							log("<not found>");
						}
						
						uintptr_t relative = ((uintptr_t)pc)-((uintptr_t)info.dli_fbase);
						logf(" +%p",relative);
					} else {
						log(" \t\t<dladdr fail>");
					}

					

					log("\n");
					
					/*
					struct file_match match;
					find_shared_object_name((const void*) pc,&match);
					logf(" - %s\n",match.file);*/

					
					
					err = unw_step(&cursor);
					if (err<=0) {
						log("END:UNWIND: ");
						log(unw_strerror(err));
						log("\n");
						break;
					}
					
					
				} while (true);
			} else  { 	log("\nFAIL: C stack trace: "); log(unw_strerror(err));log("\n");  };
		} else  { 		log("\nFAIL: C stack trace: "); log(unw_strerror(err));log("\n");  };
		
		//if (got_frame) { // todo: stack overflow
		//	log("trying to resume\n");
		//	return;
		//}
		
	} else  { log("\nFAIL: C Stack Trace\n"); };
	

	/*
	 // LEGACY VER
	log("Stack trace:\n");
	
	checkpoint {
		array++;
		size = backtrace(array, BACKTRACE_DEPTH - 1);

		bool found = false;
		for (i = 0; i < size && i < 5; ++i) {
			if (array[i]==caller_address) {
				found = true;
				break;
			}
		}	
		
		
		//crash_infinite

		if (!found) {
			array--;
			array[0] = caller_address;
			size++;
		}
		
		
		
		 //printStackTrace(array,size);
		 
		messages = backtrace_symbols_new(array, size);

		
		
		for (i = 0; i < size && messages != NULL; ++i) {
			if (strstr(messages[i],"Host_RunFrame") != NULL) {
				log( "<Snip Host Frames>\n");
				//TEST int *p = NULL; *p = 1;
				break;
			}
			logf( "> %i\t%s\n", i, messages[i]);
		}
		
		// otherwise dont bother
		if (crash_sg_nr == SIGUSR2) {
			free(messages); 
		}
	} else {
		log("Failed printing backtrace!\n");
	}
	
	checkpoint {
		
			 
		log("Stack trace:\n");
		
		checkpoint {
			messages = backtrace_symbols(array, size);

			for (i = 0; i < size && messages != NULL; ++i) {
				if (strstr(messages[i],"RunFrame") != NULL) {
					log( "<Snip Host Frames>\n");
					//TEST int *p = NULL; *p = 1;
					break;
				}
				logf( "> %i\t%s\n", i, messages[i]);
			}
			
			// otherwise dont bother
			if (crash_sg_nr == SIGUSR2) {
				free(messages); 
			}
		}	else  { 
			log("Alternative backtrace failed too :(\n");
		}
	}
	*/
	
	///////////////////
	
	checkpoint {
		if (GLUA) {
			lua_stacktrace(GLUA);
		} else {
			log("\nGLUA is NULL! (map change?)\n");
		}
	} else  { log("\nFAIL:luatrace\n"); }
	
	
	checkpoint {
		if (GLUA) {
			lua_dostackprint(GLUA,crash_sg_nr == SIGUSR2);
		} else {
			log("\nGLUA is NULL! (map change?)\n");
		}
	} else  { log("\nFAIL:luastack\n"); }
	
	in_fail = false;
	shouldjump = 0;
	
	if (crash_sg_nr == SIGUSR2) 
	{
		log("SIGUSR2:END\n\n");
		errno = saved_errno;
		return;
	}
	
	log("EOF:U");
	for(i = 0; signal_handlers[i].type != -1; ++i)
		signal(signal_handlers[i].type,SIG_DFL);

	for(unsigned i = 0; signal_handlers[i].type != -1; ++i)
	{
		if(signal_handlers[i].type == crash_sg_nr)
		{
			if(signal_handlers[i].old_sigaction.sa_flags & SA_SIGINFO)
			{
				if ((void*)signal_handlers[i].old_sigaction.sa_sigaction!=(void*)SIG_IGN && (void*)signal_handlers[i].old_sigaction.sa_sigaction != (void*)SIG_DFL) {
					logf("ES\n");
					signal_handlers[i].old_sigaction.sa_sigaction(crash_sg_nr, crash_info, crash_ucontext);
				} else {
					//log("no other crashhandlers found\n");					
				}
				break;
			}
			else
			{
				if (signal_handlers[i].old_sigaction.sa_handler!=SIG_IGN && signal_handlers[i].old_sigaction.sa_handler != SIG_DFL) {
					logf("E\n");
					signal_handlers[i].old_sigaction.sa_handler(crash_sg_nr);
				} else {
					//log("no other crashhandlers found\n");
				}
				break; 
			}
		}
	}
	
	errno=0;
}


#ifdef CRASH_DEBUG
void write_null(int a)
{
	int *p = 0;
	*p = a;
}
extern "C" int lua_dosegfault(lua_State *L) {
	log("Doing segfault...");
	//raise(SIGSEGV);
	write_null(321);
	return 0;
};




extern "C" int lua_docrash_nullptr(lua_State *L) {
	log("Doing nullptr call...\n");
	void (*pFunc)(unsigned int) = NULL; 
	pFunc(0xDEADBEEF);
	return 0;
};

extern "C" int lua_dostack(lua_State *L) {
	log("Doing stack overflow...\n");
	cause_stackoverflow(0);
	log("NOT REACHABLE\n");
	return 0;
};


void * thread1(void * a)
{
    log("<THREAD START>\n");
	long t1 = time(NULL)+3;
	while (t1>time(NULL)) {};
	
    log("<THREAD CRASH>\n");
	write_null(123);
    log("<THREAD END>\n");
}

pthread_t tid1;
extern "C" int lua_docrash_thread(lua_State *L) {
	log("Doing thread crash...");
	

	pthread_create(&tid1,NULL,thread1,NULL);
		
	log("..thread created.\n");
	return 0;
};
#endif

void setup_outfile() {
	char fname[64];
	sprintf(fname,"logs/%lu.log",time(NULL));
	logf("gmsv_segfault: Logging to %s\n",fname);

	if (fname != NULL)
	{
		logfile = open (fname, O_TRUNC | O_WRONLY | O_CREAT, 0666);
		if (logfile == -1) {
			logfile = 0;
		} else {
			unlink(					"logs/latest.log");
			symlink(basename(fname),"logs/latest.log");
		}
	}
	
	log("START:");
	logf("%lu",time(NULL));
	log("\n");
}


void disable_ctrl_c(int sigid) {
	static volatile char count = 3;
	int oerrno=errno;
	--count;
	if (count<=0) {
		signal(SIGINT,SIG_DFL);
		log("Next CTRL+C Closes the server!\n");
	}  else {
		log("Blocked Ctrl+C\n");
	}
	errno=oerrno;
};

inline void Setup() {
	setup_outfile();
	main_thread = syscall(SYS_gettid);
	//bfd_init(); // maybe saves us from a crash

	#ifdef CRASH_DEBUG
	// init this shit, maybe it works
	void * dummy_trace_array[1];
	unsigned int dummy_trace_size;
	dummy_trace_size = backtrace(dummy_trace_array, 1);
	#endif
	
	// ALT STACK
	static char ssp[SIGSTKSZ*2];
	signal_alt_stack.ss_size = SIGSTKSZ*2;
	signal_alt_stack.ss_flags = 0;
	signal_alt_stack.ss_sp = ssp;
	signal_alt_stack.ss_size = SIGSTKSZ*2;
	if (sigaltstack (&signal_alt_stack, 0) < 0) {
		signal_alt_stack.ss_size = 0;
		logf("sigaltstack errno = %d\n", errno);
	}

	demanglealloc = (char *) malloc(DEMANGLE_LEN);
	demanglealloc[0]=0x00;

	signal(SIGINT,disable_ctrl_c);
	
	// HANDLERS
	for(unsigned i = 0; signal_handlers[i].type != -1; ++i)
	{
		signal_handlers[i].sigaction.sa_sigaction = ERROR_SIGNAL_HANDLER_FUNC;
		sigemptyset(&signal_handlers[i].sigaction.sa_mask);
		signal_handlers[i].sigaction.sa_flags = SA_NODEFER | SA_ONSTACK | SA_RESTART | SA_SIGINFO;

		if(sigaction(signal_handlers[i].type, &signal_handlers[i].sigaction, &signal_handlers[i].old_sigaction) != 0)
		{
			logf("Registering signal handler \"%s\" failed: %s\n", strsignal(signal_handlers[i].type), strerror(errno));
		}
	}
	
	//feenableexcept (FE_INVALID);
}

extern "C" __attribute__( ( visibility("default") ) ) int gmod13_open( lua_State* L )
{
	GLUA = L;

	#ifdef CRASH_DEBUG
	//lua_register(L,"dumpstack",lua_dostackprint);
	lua_register(L,"docrash",lua_dosegfault);
	lua_register(L,"docrash_stack",lua_dostack);
	
	lua_register(L,"docrash_nullptr",lua_docrash_nullptr);
	lua_register(L,"docrash_thread",lua_docrash_thread);
	#endif
	
	
	if (sig_loaded) {
		log("\n[SigSegv]\tReloaded...\n");
		return 0;
	}
	sig_loaded = true;
	
	Setup();
	return 0;
}

extern "C" __attribute__( ( visibility("default") ) ) int gmod13_close( lua_State* L )
{
	log("[SigSegv]\tLua unload.\n");
	GLUA=NULL;
	return 0;
}

