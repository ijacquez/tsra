#include <windows.h>
#include <stdio.h>

#define PREFIX ""
#define SUFFIX ".dll"

static void display_w32_error_msg(const char *additional_message) {
  LPVOID msg_buf;

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                NULL, GetLastError(), 0, (LPTSTR)&msg_buf, 0, NULL);
  fprintf(stderr, "scheme load-extension: %s: %s", additional_message, msg_buf);
  LocalFree(msg_buf);
}

HMODULE dl_attach(const char *module) {
  HMODULE dll = LoadLibrary(module);
  if (!dll) display_w32_error_msg(module);
  return dll;
}

FARPROC dl_proc(HMODULE mo, const char *proc) {
  FARPROC procedure = GetProcAddress(mo, proc);
  if (!procedure) display_w32_error_msg(proc);
  return procedure;
}

void dl_detach(HMODULE mo) { (void)FreeLibrary(mo); }

