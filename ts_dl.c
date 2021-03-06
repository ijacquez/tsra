/* dynload.c Dynamic Loader for TinyScheme */
/* Original Copyright (c) 1999 Alexander Shendi     */
/* Modifications for NT and dl_* interface, ts_load_ext: D. Souflis */
/* Refurbished by Stephen Gildea */

#define _SCHEME_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynload.h"
#include "ts_dl.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

static void make_filename(const char *name, char *filename);
static void make_init_fn(const char *name, char *init_fn);

ts_ptr ts_load_ext(ts_interp *sc, ts_ptr args) {
  ts_ptr first_arg;
  ts_ptr retval;
  char filename[MAXPATHLEN], init_fn[MAXPATHLEN + 6];
  char *name;
  HMODULE dll_handle;
  void (*module_init)(ts_interp * sc);

  if ((args != sc->nil) && ts_is_str((first_arg = ts_pair_car(args)))) {
    name = ts_str_val(first_arg);
    make_filename(name, filename);
    make_init_fn(name, init_fn);
    dll_handle = dl_attach(filename);
    if (dll_handle == 0) {
      retval = sc->f;
    } else {
      module_init = (void (*)(ts_interp *))dl_proc(dll_handle, init_fn);
      if (module_init != 0) {
        (*module_init)(sc);
        retval = sc->t;
      } else {
        retval = sc->f;
      }
    }
  } else {
    retval = sc->f;
  }

  return (retval);
}

static void make_filename(const char *name, char *filename) {
  strcpy(filename, name);
  strcat(filename, SUFFIX);
}

static void make_init_fn(const char *name, char *init_fn) {
  const char *p = strrchr(name, '/');
  if (p == 0) {
    p = name;
  } else {
    p++;
  }
  strcpy(init_fn, "init_");
  strcat(init_fn, p);
}
