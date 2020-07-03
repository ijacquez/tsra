/* scheme-private.h */

#ifndef _SCHEME_PRIVATE_H
#define _SCHEME_PRIVATE_H

#include "scheme.h"
/*------------------ Ugly internals -----------------------------------*/
/*------------------ Of interest only to FFI users --------------------*/

#define cons(sc,a,b) ts_cons(sc,a,b,0)
#define immutablets_cons(sc,a,b) ts_cons(sc,a,b,1)

#endif
