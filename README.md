# gmsv_segfault

VERY EXPERIMENTAL (and old) Garry's Mod server binary module for crash signal handling and debugging.

Catches fatal signals (SIGSEGV, SIGABRT, SIGBUS, SIGILL, SIGQUIT, SIGHUP, SIGUSR1, SIGUSR2) and dumps:
- Signal info and fault address
- CPU register state
- C++ backtrace via libunwind (with demangled symbols)
- Lua stack trace and stack contents
- Thread crash detection

Also includes physics crash recovery, Lua watchdog (SIGUSR1) to break infinite loops, and Ctrl+C guard.

Output is written to `logs/<timestamp>.log` with a symlink at `logs/latest.log`.

```sh
apt-get install libunwind-dev binutils-dev liblzma-dev
```

Build with premake5.

Build scripts: [github.com/Cynosphere/gbins](https://github.com/Cynosphere/gbins)

Originally extracted from [gitlab.com/metastruct/internal/gbins](https://gitlab.com/metastruct/internal/gbins) (internal)

Example crash dump: [example_crash.log](example_crash.log)
