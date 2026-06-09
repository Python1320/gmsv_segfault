# gmsv_segfault

> [!WARNING]  
> HERE BE DRAGONS

*VERY EXPERIMENTAL (and old) Garry's Mod server binary module for crash signal handling and debugging.*

Catches fatal signals (SIGSEGV, SIGABRT, SIGBUS, SIGILL, SIGQUIT, SIGHUP, SIGUSR1, SIGUSR2) and dumps:
- Signal info and fault address
- CPU register state
- C++ backtrace via libunwind (with demangled symbols)
- Lua stack trace and stack contents
- Thread crash detection

**Example crash dump**: [example_crash.log](example_crash.log)


**Also includes:**
 -  physics crash mitigation (*requires additional cursed stuff*)

**Custom signals to send to SRCDS**
 - **SIGUSR1**: Lua watchdog to break infinite loops
 - **SIGUSR2**: to dump manually.

Crash log is written to `$PWD/logs/<timestamp>.log` with a symlink at `logs/latest.log`.


**Build**
1. [Install premake](https://premake.github.io/download/) 
2. Install dependencies
    ```sh
    apt-get install libunwind-dev binutils-dev liblzma-dev build-essential
    ```
2. Copy this repo inside the gbins alongside all the other binary modules (*or really just rewrite build, please*)

**Install**
 1. Put `gmsv_segfault_linux.dll` into `garrysmod/lua/bin/` (there is a precompiled version in Releases).
 2. install autorun lua to server.

### Notes

Debug builds (`#define CRASH_DEBUG`) register:

| Lua function | C function | Description |
|---|---|---|
| `docrash` | `lua_dosegfault` | Triggers a SIGSEGV |
| `docrash_stack` | `lua_dostack` | Triggers a stack overflow |
| `docrash_nullptr` | `lua_docrash_nullptr` | Calls through a null pointer |
| `docrash_thread` | `lua_docrash_thread` | Crashes in a separate thread |


Originally extracted from [gitlab.com/metastruct/internal/gbins](https://gitlab.com/metastruct/internal/gbins) (internal)

## Thanks

Garry, FreezeBug, BlueShank, MetaMan, CapsAdmin, etc, etc etc
