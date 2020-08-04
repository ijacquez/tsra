#ifndef TS_DL_H
#define TS_DL_H

#ifdef _WIN32
#include "ts_dl_win.h"
#else
#include "ts_dl_unix.h"
#endif

HMODULE dl_attach(const char *module);
FARPROC dl_proc(HMODULE mo, const char *proc);
void dl_detach(HMODULE mo);

#endif
