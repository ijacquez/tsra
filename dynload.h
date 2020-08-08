/* dynload.h */
/* Original Copyright (c) 1999 Alexander Shendi     */
/* Modifications for NT and dl_* interface: D. Souflis */

#ifndef DYNLOAD_H
#define DYNLOAD_H

#include "scheme.h"

TS_EXPORT ts_ptr ts_load_ext(ts_interp *sc, ts_ptr arglist);

#endif
