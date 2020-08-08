#include <dlfcn.h>
#include <stdio.h>

typedef void *HMODULE;
typedef void (*FARPROC)();

#define PREFIX "lib"
#define SUFFIX ".so"

HMODULE dl_attach(const char *module) {
  HMODULE so = dlopen(module, RTLD_LAZY);
  if (!so) {
    fprintf(stderr, "Error loading ts_interp extension \"%s\": %s\n", module,
            dlerror());
  }
  return so;
}

FARPROC dl_proc(HMODULE mo, const char *proc) {
  const char *errmsg;
  FARPROC fp = (FARPROC)dlsym(mo, proc);
  if ((errmsg = dlerror()) == 0) {
    return fp;
  }
  fprintf(stderr, "Error initializing ts_interp module \"%s\": %s\n", proc,
          errmsg);
  return 0;
}

void dl_detach(HMODULE mo) { (void)dlclose(mo); }
