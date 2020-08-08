#ifndef TS_COMMON_H
#define TS_COMMON_H

#include "scheme.h"

ts_ptr reverse_in_place(ts_interp *sc, ts_ptr term, ts_ptr list);
void load_named_file(ts_interp *sc, FILE *fin, const char *filename);
void dump_stack_reset(ts_interp *sc);
ts_ptr get_cell(ts_interp *sc, ts_ptr a, ts_ptr b);
ts_ptr find_slot_in_env(ts_interp *sc, ts_ptr env, ts_ptr sym, int all);
void load_file(ts_interp *sc, FILE *fin);

#define car(p) ((p)->_object._cons._car)
#define cdr(p) ((p)->_object._cons._cdr)
#define caar(p) car(car(p))
#define cadr(p) car(cdr(p))
#define cdar(p) cdr(car(p))
#define cddr(p) cdr(cdr(p))
#define cadar(p) car(cdr(car(p)))
#define caddr(p) car(cdr(cdr(p)))
#define cdaar(p) cdr(car(car(p)))
#define cadaar(p) car(cdr(car(car(p))))
#define cadddr(p) car(cdr(cdr(cdr(p))))
#define cddddr(p) cdr(cdr(cdr(cdr(p))))

ts_ptr slot_value_in_env(ts_ptr slot);

/* macros for cell operations */
#define typeflag(p) ((p)->_flag)
#define type(p) (typeflag(p) & T_MASKTYPE)

/* operator code */
enum opcodes {
#define _OP_DEF(A, B, C, D, E, OP) OP,
#include "opdefines.h"
  OP_MAXDEFINED
};

void Eval_Cycle(ts_interp *sc, enum opcodes op);

enum scheme_types {
  T_STRING = 1,
  T_NUMBER = 2,
  T_SYMBOL = 3,
  T_PROC = 4,
  T_PAIR = 5,
  T_CLOSURE = 6,
  T_CONTINUATION = 7,
  T_FOREIGN = 8,
  T_CHARACTER = 9,
  T_PORT = 10,
  T_VECTOR = 11,
  T_MACRO = 12,
  T_PROMISE = 13,
  T_ENVIRONMENT = 14,
  T_USERDATA = 15,
  T_LAST_SYSTEM_TYPE = 15
};

/* ADJ is enough slack to align cells in a TYPE_BITS-bit boundary */
static const unsigned int ADJ = 32;
static const unsigned int TYPE_BITS = 5;
static const unsigned int T_MASKTYPE = 31;    /* 0000000000011111 */
static const unsigned int T_SYNTAX = 4096;    /* 0001000000000000 */
static const unsigned int T_IMMUTABLE = 8192; /* 0010000000000000 */
static const unsigned int T_ATOM = 16384;
/* 0100000000000000 */ /* only for gc */
static const unsigned int CLRATOM = 49151;
/* 1011111111111111 */                    /* only for gc */
static const unsigned int MARK = 32768;   /* 1000000000000000 */
static const unsigned int UNMARK = 32767; /* 0111111111111111 */

#endif
