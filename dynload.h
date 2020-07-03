/* dynload.h */
/* Original Copyright (c) 1999 Alexander Shendi     */
/* Modifications for NT and dl_* interface: D. Souflis */

#ifndef DYNLOAD_H
#define DYNLOAD_H

#include "scheme-private.h"

TS_EXPORT ts_ptr scm_load_ext(scheme *sc, ts_ptr arglist);

#endif
