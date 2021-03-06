/* T I N Y S C H E M E    1 . 4 2
 *   Dimitrios Souflis (dsouflis@acm.org)
 *   Based on MiniScheme (original credits follow)
 * (MINISCM)               coded by Atsushi Moriwaki (11/5/1989)
 * (MINISCM)           E-MAIL :  moriwaki@kurims.kurims.kyoto-u.ac.jp
 * (MINISCM) This version has been modified by R.C. Secrist.
 * (MINISCM)
 * (MINISCM) Mini-Scheme is now maintained by Akira KIDA.
 * (MINISCM)
 * (MINISCM) This is a revised and modified version by Akira KIDA.
 * (MINISCM)    current version is 0.85k4 (15 May 1994)
 *
 */

#define _SCHEME_SOURCE

#include "ts_std.h"
#include "common.h"

#if USE_DL
#include "dynload.h"
#endif
#if USE_MATH
#include <math.h>
#endif

#include <ctype.h>
#include <float.h>
#include <limits.h>

#include <stdlib.h>
#include <string.h>

/* Used for documentation purposes, to signal functions in 'interface' */
#define INTERFACE

enum {
  TOK_EOF = -1,
  TOK_LPAREN = 0,
  TOK_RPAREN = 1,
  TOK_DOT = 2,
  TOK_ATOM = 3,
  TOK_QUOTE = 4,
  TOK_COMMENT = 5,
  TOK_DQUOTE = 6,
  TOK_BQUOTE = 7,
  TOK_COMMA = 8,
  TOK_ATMARK = 9,
  TOK_SHARP = 10,
  TOK_SHARP_CONST = 11,
  TOK_VEC = 12,
};

#define BACKQUOTE '`'

static const char *const DELIMITERS = "()\";\f\t\v\n\r ";

/*
 *  Basic memory allocation units
 */

static const char *const banner = "TinyScheme 1.42";
static const char *prompt = "ts> ";

const unsigned int FIRST_CELLSEGS = 3;

static ts_num num_add(ts_num a, ts_num b);
static ts_num num_mul(ts_num a, ts_num b);
static ts_num num_div(ts_num a, ts_num b);
static ts_num num_intdiv(ts_num a, ts_num b);
static ts_num num_sub(ts_num a, ts_num b);
static ts_num num_rem(ts_num a, ts_num b);
static ts_num num_mod(ts_num a, ts_num b);
static int num_eq(ts_num a, ts_num b);
static int num_gt(ts_num a, ts_num b);
static int num_ge(ts_num a, ts_num b);
static int num_lt(ts_num a, ts_num b);
static int num_le(ts_num a, ts_num b);

#if USE_MATH
static double round_per_R5RS(double x);
#endif
static bool is_zero_double(double x);
inline static int num_is_integer(ts_ptr p) {
  return ((p)->_object._number.is_fixnum);
}

static ts_num num_zero;
static ts_num num_one;

inline INTERFACE bool ts_is_str(ts_ptr p) { return (type(p) == T_STRING); }
#define strvalue(p) ((p)->_object._string._svalue)
#define strlength(p) ((p)->_object._string._length)

INTERFACE bool ts_is_list(ts_interp *sc, ts_ptr p);
inline INTERFACE bool ts_is_vec(ts_ptr p) { return (type(p) == T_VECTOR); }
INTERFACE void ts_fill_vec(ts_ptr vec, ts_ptr obj);
INTERFACE ts_ptr ts_vec_elem(ts_ptr vec, int ielem);
INTERFACE ts_ptr ts_set_vec_elem(ts_ptr vec, int ielem, ts_ptr a);
inline INTERFACE bool ts_is_num(ts_ptr p) { return (type(p) == T_NUMBER); }
INTERFACE bool ts_is_int(ts_ptr p) {
  if (!ts_is_num(p)) return 0;
  if (num_is_integer(p) || (double)ts_int_val(p) == ts_real_val(p)) return 1;
  return 0;
}

inline INTERFACE bool ts_is_real(ts_ptr p) {
  return ts_is_num(p) && (!(p)->_object._number.is_fixnum);
}

inline INTERFACE bool ts_is_char(ts_ptr p) { return (type(p) == T_CHARACTER); }
INTERFACE char *ts_str_val(ts_ptr p) { return strvalue(p); }
ts_num ts_num_val(ts_ptr p) { return ((p)->_object._number); }
INTERFACE int ts_int_val(ts_ptr p) {
  return (num_is_integer(p) ? (p)->_object._number.value.ivalue
                            : (int)(p)->_object._number.value.rvalue);
}
INTERFACE double ts_real_val(ts_ptr p) {
  return (!num_is_integer(p) ? (p)->_object._number.value.rvalue
                             : (double)(p)->_object._number.value.ivalue);
}
#define ivalue_unchecked(p) ((p)->_object._number.value.ivalue)
#define rvalue_unchecked(p) ((p)->_object._number.value.rvalue)
#define set_num_integer(p) (p)->_object._number.is_fixnum = 1;
#define set_num_real(p) (p)->_object._number.is_fixnum = 0;
INTERFACE int ts_char_val(ts_ptr p) { return ivalue_unchecked(p); }

inline INTERFACE bool ts_is_port(ts_ptr p) { return (type(p) == T_PORT); }
INTERFACE bool is_inport(ts_ptr p) {
  return ts_is_port(p) && p->_object._port->kind & TS_PORT_INPUT;
}
INTERFACE bool is_outport(ts_ptr p) {
  return ts_is_port(p) && p->_object._port->kind & TS_PORT_OUTPUT;
}

inline INTERFACE bool ts_is_pair(ts_ptr p) { return (type(p) == T_PAIR); }
INTERFACE ts_ptr ts_pair_car(ts_ptr p) { return car(p); }
INTERFACE ts_ptr ts_pair_cdr(ts_ptr p) { return cdr(p); }
INTERFACE ts_ptr ts_set_car(ts_ptr p, ts_ptr q) { return car(p) = q; }
INTERFACE ts_ptr ts_set_cdr(ts_ptr p, ts_ptr q) { return cdr(p) = q; }

inline INTERFACE bool ts_is_sym(ts_ptr p) { return (type(p) == T_SYMBOL); }
INTERFACE char *ts_sym_name(ts_ptr p) { return strvalue(car(p)); }
#if USE_PLIST
inline TS_EXPORT bool ts_has_prop(ts_ptr p) { return (typeflag(p) & T_SYMBOL); }
#define symprop(p) cdr(p)
#endif

inline INTERFACE bool ts_is_syntax(ts_ptr p) {
  return (typeflag(p) & T_SYNTAX);
}
INTERFACE bool ts_is_proc(ts_ptr p) { return (type(p) == T_PROC); }
INTERFACE bool ts_is_foreign(ts_ptr p) { return (type(p) == T_FOREIGN); }
INTERFACE char *ts_syntax_name(ts_ptr p) { return strvalue(car(p)); }
#define procnum(p) ts_int_val(p)
static const char *procname(ts_ptr x);

inline INTERFACE bool ts_is_closure(ts_ptr p) { return (type(p) == T_CLOSURE); }
INTERFACE bool ts_is_macro(ts_ptr p) { return (type(p) == T_MACRO); }
INTERFACE ts_ptr ts_closure_code(ts_ptr p) { return car(p); }
INTERFACE ts_ptr ts_closure_env(ts_ptr p) { return cdr(p); }

inline INTERFACE bool ts_is_continuation(ts_ptr p) {
  return (type(p) == T_CONTINUATION);
}
#define cont_dump(p) cdr(p)

/* To do: promise should be forced ONCE only */
inline INTERFACE bool ts_is_promise(ts_ptr p) { return (type(p) == T_PROMISE); }

inline INTERFACE bool ts_is_env(ts_ptr p) { return (type(p) == T_ENVIRONMENT); }
#define setenvironment(p) typeflag(p) = T_ENVIRONMENT

#define is_atom(p) (typeflag(p) & T_ATOM)
#define setatom(p) typeflag(p) |= T_ATOM
#define clratom(p) typeflag(p) &= CLRATOM

#define is_mark(p) (typeflag(p) & MARK)
#define setmark(p) typeflag(p) |= MARK
#define clrmark(p) typeflag(p) &= UNMARK

inline INTERFACE bool ts_is_immutable(ts_ptr p) {
  return (typeflag(p) & T_IMMUTABLE);
}
/*#define ts_set_immutable(p)  typeflag(p) |= T_IMMUTABLE*/
inline INTERFACE void ts_set_immutable(ts_ptr p) { typeflag(p) |= T_IMMUTABLE; }

#if USE_CHAR_CLASSIFIERS
inline static int Cisalpha(char c) { return isascii(c) && isalpha(c); }
static int Cisdigit(char c) { return isascii(c) && isdigit(c); }
static int Cisspace(char c) { return isascii(c) && isspace(c); }
static int Cisupper(char c) { return isascii(c) && isupper(c); }
static int Cislower(char c) { return isascii(c) && islower(c); }
#endif

#if USE_ASCII_NAMES
static const char *charnames[32] = {
    "nul", "soh", "stx", "etx", "eot", "enq", "ack", "bel", "bs",  "ht",  "lf",
    "vt",  "ff",  "cr",  "so",  "si",  "dle", "dc1", "dc2", "dc3", "dc4", "nak",
    "syn", "etb", "can", "em",  "sub", "esc", "fs",  "gs",  "rs",  "us"};

static bool is_ascii_name(const char *name, char *pc) {
  int i;
  for (i = 0; i < 32; i++) {
    if (stricmp(name, charnames[i]) == 0) {
      *pc = i;
      return 1;
    }
  }
  if (stricmp(name, "del") == 0) {
    *pc = 127;
    return 1;
  }
  return 0;
}

#endif

static int file_push(ts_interp *sc, const char *fname);
static void file_pop(ts_interp *sc);
static bool file_interactive(ts_interp *sc);
inline static bool is_one_of(const char *s, char c);
static int alloc_cellseg(ts_interp *sc, int n);
static int binary_decode(const char *s);
static ts_ptr _get_cell(ts_interp *sc, ts_ptr a, ts_ptr b);
ts_ptr ts_reserve_cells(ts_interp *sc, int n);
static ts_ptr get_consecutive_cells(ts_interp *sc, int n);
static ts_ptr find_consecutive_cells(ts_interp *sc, int n);
static void finalize_cell(ts_interp *sc, ts_ptr a);
static int count_consecutive_cells(ts_ptr x, int needed);
static ts_ptr mk_number(ts_interp *sc, ts_num n);
static char *store_string(ts_interp *sc, int len, const char *str, char fill);
ts_ptr ts_mk_vec(ts_interp *sc, int len);
static ts_ptr mk_atom(ts_interp *sc, char *q);
static ts_ptr mk_sharp_const(ts_interp *sc, char *name);
static ts_ptr mk_port(ts_interp *sc, ts_port *p);
static ts_ptr port_from_filename(ts_interp *sc, const char *fn, int prop);
static ts_ptr port_from_file(ts_interp *sc, FILE *, int prop);
static ts_ptr port_from_string(ts_interp *sc, char *start, char *past_the_end,
                               int prop);
static ts_port *port_rep_from_filename(ts_interp *sc, const char *fn, int prop);
static ts_port *port_rep_from_file(ts_interp *sc, FILE *, int prop);
static ts_port *port_rep_from_string(ts_interp *sc, char *start,
                                     char *past_the_end, int prop);
static void port_close(ts_interp *sc, ts_ptr p, int flag);
static void mark(ts_ptr a);
static void gc(ts_interp *sc, ts_ptr a, ts_ptr b);
static int basic_inchar(ts_port *pt);
static int inchar(ts_interp *sc);
static void backchar(ts_interp *sc, char c);
static char *readstr_upto(ts_interp *sc, const char *delim);
static ts_ptr readstrexp(ts_interp *sc);
inline static int skipspace(ts_interp *sc);
static int token(ts_interp *sc);
static void printslashstring(ts_interp *sc, char *s, int len);
static void atom2str(ts_interp *sc, ts_ptr l, int f, char **pp, int *plen);
static void printatom(ts_interp *sc, ts_ptr l, int f);
static ts_ptr mk_proc(ts_interp *sc, enum opcodes op);
static ts_ptr mk_closure(ts_interp *sc, ts_ptr c, ts_ptr e);
static ts_ptr mk_continuation(ts_interp *sc, ts_ptr d);
static ts_ptr reverse(ts_interp *sc, ts_ptr a);
static ts_ptr revappend(ts_interp *sc, ts_ptr a, ts_ptr b);
static void dump_stack_mark(ts_interp *);
static ts_ptr opexe_0(ts_interp *sc, enum opcodes op);
static ts_ptr opexe_1(ts_interp *sc, enum opcodes op);
static ts_ptr opexe_2(ts_interp *sc, enum opcodes op);
static ts_ptr opexe_3(ts_interp *sc, enum opcodes op);
static ts_ptr opexe_4(ts_interp *sc, enum opcodes op);
static ts_ptr opexe_5(ts_interp *sc, enum opcodes op);
static ts_ptr opexe_6(ts_interp *sc, enum opcodes op);
static void assign_syntax(ts_interp *sc, char *name);
static int syntaxnum(ts_ptr p);
static void assign_proc(ts_interp *sc, enum opcodes, char *name);
void load_named_file(ts_interp *sc, FILE *fin, const char *filename);

#define num_ivalue(n) (n.is_fixnum ? (n).value.ivalue : (int)(n).value.rvalue)
#define num_rvalue(n) \
  (!n.is_fixnum ? (n).value.rvalue : (double)(n).value.ivalue)

static ts_num num_add(ts_num a, ts_num b) {
  ts_num ret;
  ret.is_fixnum = a.is_fixnum && b.is_fixnum;
  if (ret.is_fixnum) {
    ret.value.ivalue = a.value.ivalue + b.value.ivalue;
  } else {
    ret.value.rvalue = num_rvalue(a) + num_rvalue(b);
  }
  return ret;
}

static ts_num num_mul(ts_num a, ts_num b) {
  ts_num ret;
  ret.is_fixnum = a.is_fixnum && b.is_fixnum;
  if (ret.is_fixnum) {
    ret.value.ivalue = a.value.ivalue * b.value.ivalue;
  } else {
    ret.value.rvalue = num_rvalue(a) * num_rvalue(b);
  }
  return ret;
}

static ts_num num_div(ts_num a, ts_num b) {
  ts_num ret;
  ret.is_fixnum =
      a.is_fixnum && b.is_fixnum && a.value.ivalue % b.value.ivalue == 0;
  if (ret.is_fixnum) {
    ret.value.ivalue = a.value.ivalue / b.value.ivalue;
  } else {
    ret.value.rvalue = num_rvalue(a) / num_rvalue(b);
  }
  return ret;
}

static ts_num num_intdiv(ts_num a, ts_num b) {
  ts_num ret;
  ret.is_fixnum = a.is_fixnum && b.is_fixnum;
  if (ret.is_fixnum) {
    ret.value.ivalue = a.value.ivalue / b.value.ivalue;
  } else {
    ret.value.rvalue = num_rvalue(a) / num_rvalue(b);
  }
  return ret;
}

static ts_num num_sub(ts_num a, ts_num b) {
  ts_num ret;
  ret.is_fixnum = a.is_fixnum && b.is_fixnum;
  if (ret.is_fixnum) {
    ret.value.ivalue = a.value.ivalue - b.value.ivalue;
  } else {
    ret.value.rvalue = num_rvalue(a) - num_rvalue(b);
  }
  return ret;
}

static ts_num num_rem(ts_num a, ts_num b) {
  ts_num ret;
  int e1, e2, res;
  ret.is_fixnum = a.is_fixnum && b.is_fixnum;
  e1 = num_ivalue(a);
  e2 = num_ivalue(b);
  res = e1 % e2;
  /* remainder should have same sign as second operand */
  if (res > 0) {
    if (e1 < 0) {
      res -= labs(e2);
    }
  } else if (res < 0) {
    if (e1 > 0) {
      res += labs(e2);
    }
  }
  if (ret.is_fixnum) {
    ret.value.ivalue = res;
  } else {
    ret.value.rvalue = res;
  }
  return ret;
}

static ts_num num_mod(ts_num a, ts_num b) {
  ts_num ret;
  int e1, e2, res;
  ret.is_fixnum = a.is_fixnum && b.is_fixnum;
  e1 = num_ivalue(a);
  e2 = num_ivalue(b);
  res = e1 % e2;
  /* modulo should have same sign as second operand */
  if (res * e2 < 0) {
    res += e2;
  }
  if (ret.is_fixnum) {
    ret.value.ivalue = res;
  } else {
    ret.value.rvalue = res;
  }
  return ret;
}

static int num_eq(ts_num a, ts_num b) {
  int ret;
  bool is_fixnum = a.is_fixnum && b.is_fixnum;
  if (is_fixnum) {
    ret = a.value.ivalue == b.value.ivalue;
  } else {
    ret = num_rvalue(a) == num_rvalue(b);
  }
  return ret;
}

static int num_gt(ts_num a, ts_num b) {
  int ret;
  bool is_fixnum = a.is_fixnum && b.is_fixnum;
  if (is_fixnum) {
    ret = a.value.ivalue > b.value.ivalue;
  } else {
    ret = num_rvalue(a) > num_rvalue(b);
  }
  return ret;
}

static int num_ge(ts_num a, ts_num b) { return !num_lt(a, b); }

static int num_lt(ts_num a, ts_num b) {
  int ret;
  bool is_fixnum = a.is_fixnum && b.is_fixnum;
  if (is_fixnum) {
    ret = a.value.ivalue < b.value.ivalue;
  } else {
    ret = num_rvalue(a) < num_rvalue(b);
  }
  return ret;
}

static int num_le(ts_num a, ts_num b) { return !num_gt(a, b); }

#if USE_MATH
/* Round to nearest. Round to even if midway */
static double round_per_R5RS(double x) {
  double fl = floor(x);
  double ce = ceil(x);
  double dfl = x - fl;
  double dce = ce - x;
  if (dfl > dce) {
    return ce;
  } else if (dfl < dce) {
    return fl;
  } else {
    if (fmod(fl, 2.0) == 0.0) { /* I imagine this holds */
      return fl;
    } else {
      return ce;
    }
  }
}
#endif

static bool is_zero_double(double x) { return x < DBL_MIN && x > -DBL_MIN; }

static int binary_decode(const char *s) {
  int x = 0;

  while (*s != 0 && (*s == '1' || *s == '0')) {
    x <<= 1;
    x += *s - '0';
    s++;
  }

  return x;
}

/* allocate new cell segment */
static int alloc_cellseg(ts_interp *sc, int n) {
  ts_ptr newp;
  ts_ptr last;
  ts_ptr p;
  char *cp;
  int i;
  int k;
  int adj = ADJ;

  if (adj < sizeof(struct ts_cell)) {
    adj = sizeof(struct ts_cell);
  }

  for (k = 0; k < n; k++) {
    if (sc->last_cell_seg >= TS_CELL_NSEGMENT - 1) return k;
    cp = (char *)sc->malloc(TS_CELL_SEGSIZE * sizeof(struct ts_cell) + adj);
    if (cp == 0) return k;
    i = ++sc->last_cell_seg;
    sc->alloc_seg[i] = cp;
    /* adjust in TYPE_BITS-bit boundary */
    if (((unsigned long)cp) % adj != 0) {
      cp = (char *)(adj * ((unsigned long)cp / adj + 1));
    }
    /* insert new segment in address order */
    newp = (ts_ptr)cp;
    sc->cell_seg[i] = newp;
    while (i > 0 && sc->cell_seg[i - 1] > sc->cell_seg[i]) {
      p = sc->cell_seg[i];
      sc->cell_seg[i] = sc->cell_seg[i - 1];
      sc->cell_seg[--i] = p;
    }
    sc->fcells += TS_CELL_SEGSIZE;
    last = newp + TS_CELL_SEGSIZE - 1;
    for (p = newp; p <= last; p++) {
      typeflag(p) = 0;
      cdr(p) = p + 1;
      car(p) = sc->nil;
    }
    /* insert new cells in address order on free list */
    if (sc->free_cell == sc->nil || p < sc->free_cell) {
      cdr(last) = sc->free_cell;
      sc->free_cell = newp;
    } else {
      p = sc->free_cell;
      while (cdr(p) != sc->nil && newp > cdr(p)) p = cdr(p);
      cdr(last) = cdr(p);
      cdr(p) = newp;
    }
  }
  return n;
}

static inline ts_ptr get_cell_x(ts_interp *sc, ts_ptr a, ts_ptr b) {
  if (sc->free_cell != sc->nil) {
    ts_ptr x = sc->free_cell;
    sc->free_cell = cdr(x);
    --sc->fcells;
    return (x);
  }
  return _get_cell(sc, a, b);
}

/* get new cell.  parameter a, b is marked by gc. */
static ts_ptr _get_cell(ts_interp *sc, ts_ptr a, ts_ptr b) {
  ts_ptr x;

  if (sc->no_memory) {
    return sc->sink;
  }

  if (sc->free_cell == sc->nil) {
    const int min_to_be_recovered = sc->last_cell_seg * 8;
    gc(sc, a, b);
    if (sc->fcells < min_to_be_recovered || sc->free_cell == sc->nil) {
      /* if only a few recovered, get more to avoid fruitless gc's */
      if (!alloc_cellseg(sc, 1) && sc->free_cell == sc->nil) {
        sc->no_memory = 1;
        return sc->sink;
      }
    }
  }
  x = sc->free_cell;
  sc->free_cell = cdr(x);
  --sc->fcells;
  return (x);
}

/* make sure that there is a given number of cells free */
ts_ptr ts_reserve_cells(ts_interp *sc, int n) {
  if (sc->no_memory) {
    return sc->nil;
  }

  /* Are there enough cells available? */
  if (sc->fcells < n) {
    /* If not, try gc'ing some */
    gc(sc, sc->nil, sc->nil);
    if (sc->fcells < n) {
      /* If there still aren't, try getting more heap */
      if (!alloc_cellseg(sc, 1)) {
        sc->no_memory = 1;
        return sc->nil;
      }
    }
    if (sc->fcells < n) {
      /* If all fail, report failure */
      sc->no_memory = 1;
      return sc->nil;
    }
  }
  return (sc->t);
}

static ts_ptr get_consecutive_cells(ts_interp *sc, int n) {
  ts_ptr x;

  if (sc->no_memory) {
    return sc->sink;
  }

  /* Are there any cells available? */
  x = find_consecutive_cells(sc, n);
  if (x != sc->nil) {
    return x;
  }

  /* If not, try gc'ing some */
  gc(sc, sc->nil, sc->nil);
  x = find_consecutive_cells(sc, n);
  if (x != sc->nil) {
    return x;
  }

  /* If there still aren't, try getting more heap */
  if (!alloc_cellseg(sc, 1)) {
    sc->no_memory = 1;
    return sc->sink;
  }

  x = find_consecutive_cells(sc, n);
  if (x != sc->nil) {
    return x;
  }

  /* If all fail, report failure */
  sc->no_memory = 1;
  return sc->sink;
}

static int count_consecutive_cells(ts_ptr x, int needed) {
  int n = 1;
  while (cdr(x) == x + 1) {
    x = cdr(x);
    n++;
    if (n > needed) return n;
  }
  return n;
}

static ts_ptr find_consecutive_cells(ts_interp *sc, int n) {
  ts_ptr *pp;
  int cnt;

  pp = &sc->free_cell;
  while (*pp != sc->nil) {
    cnt = count_consecutive_cells(*pp, n);
    if (cnt >= n) {
      ts_ptr x = *pp;
      *pp = cdr(*pp + n - 1);
      sc->fcells -= n;
      return x;
    }
    pp = &cdr(*pp + cnt - 1);
  }
  return sc->nil;
}

/* To retain recent allocs before interpreter knows about them -
   Tehom */

static void push_recent_alloc(ts_interp *sc, ts_ptr recent, ts_ptr extra) {
  ts_ptr holder = get_cell_x(sc, recent, extra);
  typeflag(holder) = T_PAIR | T_IMMUTABLE;
  car(holder) = recent;
  cdr(holder) = car(sc->sink);
  car(sc->sink) = holder;
}

ts_ptr get_cell(ts_interp *sc, ts_ptr a, ts_ptr b) {
  ts_ptr cell = get_cell_x(sc, a, b);
  /* For right now, include "a" and "b" in "cell" so that gc doesn't
     think they are garbage. */
  /* Tentatively record it as a pair so gc understands it. */
  typeflag(cell) = T_PAIR;
  car(cell) = a;
  cdr(cell) = b;
  push_recent_alloc(sc, cell, sc->nil);
  return cell;
}

static ts_ptr get_vector_object(ts_interp *sc, int len, ts_ptr init) {
  ts_ptr cells = get_consecutive_cells(sc, len / 2 + len % 2 + 1);
  if (sc->no_memory) {
    return sc->sink;
  }
  /* Record it as a vector so that gc understands it. */
  typeflag(cells) = (T_VECTOR | T_ATOM);
  ivalue_unchecked(cells) = len;
  set_num_integer(cells);
  ts_fill_vec(cells, init);
  push_recent_alloc(sc, cells, sc->nil);
  return cells;
}

inline static void ok_to_freely_gc(ts_interp *sc) { car(sc->sink) = sc->nil; }

#if defined TSGRIND
static void check_cell_alloced(ts_ptr p, int expect_alloced) {
  /* Can't use ts_put_str(sc,str) because callers have no access to
     sc.  */
  if (typeflag(p) & !expect_alloced) {
    fprintf(stderr, "Cell is already allocated!\n");
  }
  if (!(typeflag(p)) & expect_alloced) {
    fprintf(stderr, "Cell is not allocated!\n");
  }
}
static void check_range_alloced(ts_ptr p, int n, int expect_alloced) {
  int i;
  for (i = 0; i < n; i++) {
    (void)check_cell_alloced(p + i, expect_alloced);
  }
}

#endif

/* Medium level cell allocation */

/* get new cons cell */
ts_ptr _cons(ts_interp *sc, ts_ptr a, ts_ptr b, bool immutable) {
  ts_ptr x = get_cell(sc, a, b);

  typeflag(x) = T_PAIR;
  if (immutable) {
    ts_set_immutable(x);
  }
  car(x) = a;
  cdr(x) = b;
  return (x);
}

INTERFACE ts_ptr ts_cons(ts_interp *sc, ts_ptr a, ts_ptr b) {
  return _cons(sc, a, b, 0);
}

INTERFACE ts_ptr ts_immutable_cons(ts_interp *sc, ts_ptr a, ts_ptr b) {
  return _cons(sc, a, b, 1);
}

/* ========== oblist implementation  ========== */

#ifndef USE_OBJECT_LIST

static int hash_fn(const char *key, int table_size);

static ts_ptr oblist_initial_value(ts_interp *sc) {
  return ts_mk_vec(sc, 461); /* probably should be bigger */
}

/* returns the new symbol */
static ts_ptr oblist_add_by_name(ts_interp *sc, const char *name) {
  ts_ptr x;
  int location;

  x = ts_immutable_cons(sc, ts_mk_str(sc, name), sc->nil);
  typeflag(x) = T_SYMBOL;
  ts_set_immutable(car(x));

  location = hash_fn(name, ivalue_unchecked(sc->oblist));
  ts_set_vec_elem(sc->oblist, location,
                  ts_immutable_cons(sc, x, ts_vec_elem(sc->oblist, location)));
  return x;
}

inline static ts_ptr oblist_find_by_name(ts_interp *sc, const char *name) {
  int location;
  ts_ptr x;
  char *s;

  location = hash_fn(name, ivalue_unchecked(sc->oblist));
  for (x = ts_vec_elem(sc->oblist, location); x != sc->nil; x = cdr(x)) {
    s = ts_sym_name(car(x));
    /* case-insensitive, per R5RS section 2. */
    if (stricmp(name, s) == 0) {
      return car(x);
    }
  }
  return sc->nil;
}

static ts_ptr oblist_all_symbols(ts_interp *sc) {
  int i;
  ts_ptr x;
  ts_ptr ob_list = sc->nil;

  for (i = 0; i < ivalue_unchecked(sc->oblist); i++) {
    for (x = ts_vec_elem(sc->oblist, i); x != sc->nil; x = cdr(x)) {
      ob_list = ts_cons(sc, x, ob_list);
    }
  }
  return ob_list;
}

#else

static ts_ptr oblist_initial_value(ts_interp *sc) { return sc->nil; }

inline static ts_ptr oblist_find_by_name(ts_interp *sc, const char *name) {
  ts_ptr x;
  char *s;

  for (x = sc->oblist; x != sc->nil; x = cdr(x)) {
    s = ts_sym_name(car(x));
    /* case-insensitive, per R5RS section 2. */
    if (stricmp(name, s) == 0) {
      return car(x);
    }
  }
  return sc->nil;
}

/* returns the new symbol */
static ts_ptr oblist_add_by_name(ts_interp *sc, const char *name) {
  ts_ptr x;

  x = ts_immutable_cons(sc, ts_mk_str(sc, name), sc->nil);
  typeflag(x) = T_SYMBOL;
  ts_set_immutable(car(x));
  sc->oblist = ts_immutable_cons(sc, x, sc->oblist);
  return x;
}
static ts_ptr oblist_all_symbols(ts_interp *sc) { return sc->oblist; }

#endif

static ts_ptr mk_port(ts_interp *sc, ts_port *p) {
  ts_ptr x = get_cell(sc, sc->nil, sc->nil);

  typeflag(x) = T_PORT | T_ATOM;
  x->_object._port = p;
  return (x);
}

ts_ptr ts_mk_foreign_func(ts_interp *sc, ts_foreign_func f) {
  ts_ptr x = get_cell(sc, sc->nil, sc->nil);

  typeflag(x) = (T_FOREIGN | T_ATOM);
  x->_object._ff = f;
  return (x);
}

INTERFACE ts_ptr ts_mk_char(ts_interp *sc, char c) {
  ts_ptr x = get_cell(sc, sc->nil, sc->nil);

  typeflag(x) = (T_CHARACTER | T_ATOM);
  ivalue_unchecked(x) = c;
  set_num_integer(x);
  return (x);
}

/* get number atom (integer) */
INTERFACE ts_ptr ts_mk_int(ts_interp *sc, int n) {
  ts_ptr x = get_cell(sc, sc->nil, sc->nil);

  typeflag(x) = (T_NUMBER | T_ATOM);
  ivalue_unchecked(x) = n;
  set_num_integer(x);
  return (x);
}

INTERFACE ts_ptr ts_mk_real(ts_interp *sc, double n) {
  ts_ptr x = get_cell(sc, sc->nil, sc->nil);

  typeflag(x) = (T_NUMBER | T_ATOM);
  rvalue_unchecked(x) = n;
  set_num_real(x);
  return (x);
}

static ts_ptr mk_number(ts_interp *sc, ts_num n) {
  if (n.is_fixnum) {
    return ts_mk_int(sc, n.value.ivalue);
  } else {
    return ts_mk_real(sc, n.value.rvalue);
  }
}

/* allocate name to string area */
static char *store_string(ts_interp *sc, int len_str, const char *str, char fill) {
  char *q;

  q = (char *)sc->malloc(len_str + 1);
  if (q == 0) {
    sc->no_memory = 1;
    return sc->strbuff;
  }
  if (str != 0) {
    snprintf(q, len_str + 1, "%s", str);
  } else {
    memset(q, fill, len_str);
    q[len_str] = 0;
  }
  return (q);
}

/* get new string */
INTERFACE ts_ptr ts_mk_str(ts_interp *sc, const char *str) {
  return ts_mk_counted_str(sc, str, strlen(str));
}

INTERFACE ts_ptr ts_mk_counted_str(ts_interp *sc, const char *str, int len) {
  ts_ptr x = get_cell(sc, sc->nil, sc->nil);
  typeflag(x) = (T_STRING | T_ATOM);
  strvalue(x) = store_string(sc, len, str, 0);
  strlength(x) = len;
  return (x);
}

INTERFACE ts_ptr ts_mk_empty_str(ts_interp *sc, int len, char fill) {
  ts_ptr x = get_cell(sc, sc->nil, sc->nil);
  typeflag(x) = (T_STRING | T_ATOM);
  strvalue(x) = store_string(sc, len, 0, fill);
  strlength(x) = len;
  return (x);
}

INTERFACE ts_ptr ts_mk_vec(ts_interp *sc, int len) {
  return get_vector_object(sc, len, sc->nil);
}

INTERFACE void ts_fill_vec(ts_ptr vec, ts_ptr obj) {
  int i;
  int num = ts_int_val(vec) / 2 + ts_int_val(vec) % 2;
  for (i = 0; i < num; i++) {
    typeflag(vec + 1 + i) = T_PAIR;
    ts_set_immutable(vec + 1 + i);
    car(vec + 1 + i) = obj;
    cdr(vec + 1 + i) = obj;
  }
}

INTERFACE ts_ptr ts_vec_elem(ts_ptr vec, int ielem) {
  int n = ielem / 2;
  if (ielem % 2 == 0) {
    return car(vec + 1 + n);
  } else {
    return cdr(vec + 1 + n);
  }
}

INTERFACE ts_ptr ts_set_vec_elem(ts_ptr vec, int ielem, ts_ptr a) {
  int n = ielem / 2;
  if (ielem % 2 == 0) {
    return car(vec + 1 + n) = a;
  } else {
    return cdr(vec + 1 + n) = a;
  }
}

/* get new symbol */
INTERFACE ts_ptr ts_mk_sym(ts_interp *sc, const char *name) {
  ts_ptr x;

  /* first check oblist */
  x = oblist_find_by_name(sc, name);
  if (x != sc->nil) {
    return (x);
  } else {
    x = oblist_add_by_name(sc, name);
    return (x);
  }
}

INTERFACE ts_ptr ts_gen_sym(ts_interp *sc) {
  ts_ptr x;
  char name[40];

  for (; sc->gensym_cnt < INT_MAX; sc->gensym_cnt++) {
    snprintf(name, 40, "gensym-%d", sc->gensym_cnt);

    /* first check oblist */
    x = oblist_find_by_name(sc, name);

    if (x != sc->nil) {
      continue;
    } else {
      x = oblist_add_by_name(sc, name);
      return (x);
    }
  }

  return sc->nil;
}

/* make symbol or number atom from string */
static ts_ptr mk_atom(ts_interp *sc, char *q) {
  char c, *p;
  int has_dec_point = 0;
  int has_fp_exp = 0;

#if USE_COLON_HOOK
  if ((p = strstr(q, "::")) != 0) {
    *p = 0;
    return ts_cons(sc, sc->colon_hook,
                   ts_cons(sc,
                           ts_cons(sc, sc->quote,
                                   ts_cons(sc, mk_atom(sc, p + 2), sc->nil)),
                           ts_cons(sc, ts_mk_sym(sc, strlwr(q)), sc->nil)));
  }
#endif

  p = q;
  c = *p++;
  if ((c == '+') || (c == '-')) {
    c = *p++;
    if (c == '.') {
      has_dec_point = 1;
      c = *p++;
    }
    if (!isdigit(c)) {
      return (ts_mk_sym(sc, strlwr(q)));
    }
  } else if (c == '.') {
    has_dec_point = 1;
    c = *p++;
    if (!isdigit(c)) {
      return (ts_mk_sym(sc, strlwr(q)));
    }
  } else if (!isdigit(c)) {
    return (ts_mk_sym(sc, strlwr(q)));
  }

  for (; (c = *p) != 0; ++p) {
    if (!isdigit(c)) {
      if (c == '.') {
        if (!has_dec_point) {
          has_dec_point = 1;
          continue;
        }
      } else if ((c == 'e') || (c == 'E')) {
        if (!has_fp_exp) {
          has_dec_point = 1; /* decimal point illegal
                                from now on */
          p++;
          if ((*p == '-') || (*p == '+') || isdigit(*p)) {
            continue;
          }
        }
      }
      return (ts_mk_sym(sc, strlwr(q)));
    }
  }
  if (has_dec_point) {
    return ts_mk_real(sc, atof(q));
  }
  return (ts_mk_int(sc, atol(q)));
}

/* make constant */
static ts_ptr mk_sharp_const(ts_interp *sc, char *name) {
  int x;
  char tmp[TS_STRBUFFSIZE];

  if (!strcmp(name, "t"))
    return (sc->t);
  else if (!strcmp(name, "f"))
    return (sc->f);
  else if (*name == 'o') { /* #o (octal) */  
    x = strtol(name + 1, NULL, 8);
    return (ts_mk_int(sc, x));
  } else if (*name == 'd') { /* #d (decimal) */
    x = strtol(name + 1, NULL, 10);
    return (ts_mk_int(sc, x));
  } else if (*name == 'x') { /* #x (hex) */
    x = strtol(name + 1, NULL, 16);
    return (ts_mk_int(sc, x));
  } else if (*name == 'b') { /* #b (binary) */
    x = binary_decode(name + 1);
    return (ts_mk_int(sc, x));
  } else if (*name == '\\') { /* #\w (character) */
    char c = 0;
    if (stricmp(name + 1, "space") == 0) {
      c = ' ';
    } else if (stricmp(name + 1, "newline") == 0) {
      c = '\n';
    } else if (stricmp(name + 1, "return") == 0) {
      c = '\r';
    } else if (stricmp(name + 1, "tab") == 0) {
      c = '\t';
    } else if (name[1] == 'x' && name[2] != 0) {
      char *c1_endptr = NULL;
      int c1 = strtol(name + 2, &c1_endptr, 16);
      if ((*c1_endptr == '\0') && (c1 < UCHAR_MAX)) {
        c = c1;
      } else {
        return sc->nil;
      }
#if USE_ASCII_NAMES
    } else if (is_ascii_name(name + 1, &c)) {
      /* nothing */
#endif
    } else if (name[2] == 0) {
      c = name[1];
    } else {
      return sc->nil;
    }
    return ts_mk_char(sc, c);
  } else
    return sc->nil;
}

/* ========== garbage collector ========== */

/*--
 *  We use algorithm E (Knuth, The Art of Computer Programming Vol.1,
 *  sec. 2.3.5), the Schorr-Deutsch-Waite link-inversion algorithm,
 *  for marking.
 */
static void mark(ts_ptr a) {
  ts_ptr t, q, p;

  t = (ts_ptr)0;
  p = a;
E2:
  setmark(p);
  if (ts_is_vec(p)) {
    int i;
    int num = ivalue_unchecked(p) / 2 + ivalue_unchecked(p) % 2;
    for (i = 0; i < num; i++) {
      /* Vector cells will be treated like ordinary cells */
      mark(p + 1 + i);
    }
  }
  if (is_atom(p)) goto E6;
  /* E4: down car */
  q = car(p);
  if (q && !is_mark(q)) {
    setatom(p); /* a note that we have moved car */
    car(p) = t;
    t = p;
    p = q;
    goto E2;
  }
E5:
  q = cdr(p); /* down cdr */
  if (q && !is_mark(q)) {
    cdr(p) = t;
    t = p;
    p = q;
    goto E2;
  }
E6: /* up.  Undo the link switching from steps E4 and E5. */
  if (!t) return;
  q = t;
  if (is_atom(q)) {
    clratom(q);
    t = car(q);
    car(q) = p;
    p = q;
    goto E5;
  } else {
    t = cdr(q);
    cdr(q) = p;
    p = q;
    goto E6;
  }
}

/* garbage collection. parameter a, b is marked. */
static void gc(ts_interp *sc, ts_ptr a, ts_ptr b) {
  ts_ptr p;
  int i;

  if (sc->gc_verbose) {
    ts_put_str(sc, "gc...");
  }

  /* mark system globals */
  mark(sc->oblist);
  mark(sc->global_env);

  /* mark current registers */
  mark(sc->args);
  mark(sc->envir);
  mark(sc->code);
  dump_stack_mark(sc);
  mark(sc->value);
  mark(sc->inport);
  mark(sc->save_inport);
  mark(sc->outport);
  mark(sc->loadport);

  /* Mark recent objects the interpreter doesn't know about yet. */
  mark(car(sc->sink));
  /* Mark any older stuff above nested C calls */
  mark(sc->c_nest);

  /* mark variables a, b */
  mark(a);
  mark(b);

  /* garbage collect */
  clrmark(sc->nil);
  sc->fcells = 0;
  sc->free_cell = sc->nil;
  /* free-list is kept sorted by address so as to maintain consecutive
     ranges, if possible, for use with vectors. Here we scan the cells
     (which are also kept sorted by address) downwards to build the
     free-list in sorted order.
  */
  for (i = sc->last_cell_seg; i >= 0; i--) {
    p = sc->cell_seg[i] + TS_CELL_SEGSIZE;
    while (--p >= sc->cell_seg[i]) {
      if (is_mark(p)) {
        clrmark(p);
      } else {
        /* reclaim cell */
        if (typeflag(p) != 0) {
          finalize_cell(sc, p);
          typeflag(p) = 0;
          car(p) = sc->nil;
        }
        ++sc->fcells;
        cdr(p) = sc->free_cell;
        sc->free_cell = p;
      }
    }
  }

  if (sc->gc_verbose) {
    char msg[80];
    snprintf(msg, 80, "done: %d cells were recovered.\n", sc->fcells);
    ts_put_str(sc, msg);
  }
}

static void finalize_cell(ts_interp *sc, ts_ptr a) {
  if (ts_is_str(a)) {
    sc->free(strvalue(a));
  } else if (ts_is_port(a)) {
    if (a->_object._port->kind & TS_PORT_FILE &&
        a->_object._port->rep.stdio.closeit) {
      port_close(sc, a, TS_PORT_INPUT | TS_PORT_OUTPUT);
    }
    sc->free(a->_object._port);
  }
#if !STANDALONE
  else if (ts_is_userdata(a)) {
    a->userdata.finalizer(a->userdata.ptr);
  }
#endif
}

/* ========== Routines for Reading ========== */

static int file_push(ts_interp *sc, const char *fname) {
  FILE *fin = NULL;

  if (sc->file_i == TS_MAXFIL - 1) return 0;
  fin = fopen(fname, "r");
  if (fin != 0) {
    sc->file_i++;
    sc->load_stack[sc->file_i].kind = TS_PORT_FILE | TS_PORT_INPUT;
    sc->load_stack[sc->file_i].rep.stdio.file = fin;
    sc->load_stack[sc->file_i].rep.stdio.closeit = 1;
    sc->nesting_stack[sc->file_i] = 0;
    sc->loadport->_object._port = sc->load_stack + sc->file_i;

#if SHOW_ERROR_LINE
    sc->load_stack[sc->file_i].rep.stdio.curr_line = 0;
    if (fname)
      sc->load_stack[sc->file_i].rep.stdio.filename =
          store_string(sc, strlen(fname), fname, 0);
#endif
  }
  return fin != 0;
}

static void file_pop(ts_interp *sc) {
  if (sc->file_i != 0) {
    sc->nesting = sc->nesting_stack[sc->file_i];
    port_close(sc, sc->loadport, TS_PORT_INPUT);
    sc->file_i--;
    sc->loadport->_object._port = sc->load_stack + sc->file_i;
  }
}

static bool file_interactive(ts_interp *sc) {
  return (sc->file_i == 0) &&
         (sc->load_stack[0].rep.stdio.file == stdin) &&
         ((sc->inport->_object._port->kind & TS_PORT_FILE) != 0);
}

static ts_port *port_rep_from_filename(ts_interp *sc, const char *fn, int prop) {
  FILE *f;
  char *rw;
  ts_port *pt;
  if (prop == (TS_PORT_INPUT | TS_PORT_OUTPUT)) {
    rw = "a+";
  } else if (prop == TS_PORT_OUTPUT) {
    rw = "w";
  } else {
    rw = "r";
  }
  f = fopen(fn, rw);
  if (f == 0) {
    return 0;
  }
  pt = port_rep_from_file(sc, f, prop);
  pt->rep.stdio.closeit = 1;

#if SHOW_ERROR_LINE
  if (fn) pt->rep.stdio.filename = store_string(sc, strlen(fn), fn, 0);

  pt->rep.stdio.curr_line = 0;
#endif
  return pt;
}

static ts_ptr port_from_filename(ts_interp *sc, const char *fn, int prop) {
  ts_port *pt;
  pt = port_rep_from_filename(sc, fn, prop);
  if (pt == 0) {
    return sc->nil;
  }
  return mk_port(sc, pt);
}

static ts_port *port_rep_from_file(ts_interp *sc, FILE *f, int prop) {
  ts_port *pt;

  pt = (ts_port *)sc->malloc(sizeof *pt);
  if (pt == NULL) {
    return NULL;
  }
  pt->kind = TS_PORT_FILE | prop;
  pt->rep.stdio.file = f;
  pt->rep.stdio.closeit = 0;
  return pt;
}

static ts_ptr port_from_file(ts_interp *sc, FILE *f, int prop) {
  ts_port *pt;
  pt = port_rep_from_file(sc, f, prop);
  if (pt == 0) {
    return sc->nil;
  }
  return mk_port(sc, pt);
}

static ts_port *port_rep_from_string(ts_interp *sc, char *start,
                                     char *past_the_end, int prop) {
  ts_port *pt;
  pt = (ts_port *)sc->malloc(sizeof(ts_port));
  if (pt == 0) {
    return 0;
  }
  pt->kind = TS_PORT_STRING | prop;
  pt->rep.string.start = start;
  pt->rep.string.curr = start;
  pt->rep.string.past_the_end = past_the_end;
  return pt;
}

static ts_ptr port_from_string(ts_interp *sc, char *start, char *past_the_end,
                               int prop) {
  ts_port *pt;
  pt = port_rep_from_string(sc, start, past_the_end, prop);
  if (pt == 0) {
    return sc->nil;
  }
  return mk_port(sc, pt);
}

#define BLOCK_SIZE 256

static ts_port *port_rep_from_scratch(ts_interp *sc) {
  ts_port *pt;
  char *start;
  pt = (ts_port *)sc->malloc(sizeof(ts_port));
  if (pt == 0) {
    return 0;
  }
  start = sc->malloc(BLOCK_SIZE);
  if (start == 0) {
    return 0;
  }
  memset(start, ' ', BLOCK_SIZE - 1);
  start[BLOCK_SIZE - 1] = '\0';
  pt->kind = TS_PORT_STRING | TS_PORT_OUTPUT | TS_PORT_SRFI6;
  pt->rep.string.start = start;
  pt->rep.string.curr = start;
  pt->rep.string.past_the_end = start + BLOCK_SIZE - 1;
  return pt;
}

static ts_ptr port_from_scratch(ts_interp *sc) {
  ts_port *pt;
  pt = port_rep_from_scratch(sc);
  if (pt == 0) {
    return sc->nil;
  }
  return mk_port(sc, pt);
}

static void port_close(ts_interp *sc, ts_ptr p, int flag) {
  ts_port *pt = p->_object._port;
  pt->kind &= ~flag;
  if ((pt->kind & (TS_PORT_INPUT | TS_PORT_OUTPUT)) == 0) {
    if (pt->kind & TS_PORT_FILE) {
#if SHOW_ERROR_LINE
      /* Cleanup is here so (close-*-port) functions could work too */
      pt->rep.stdio.curr_line = 0;

      if (pt->rep.stdio.filename) sc->free(pt->rep.stdio.filename);
#endif

      fclose(pt->rep.stdio.file);
    }
    pt->kind = TS_PORT_FREE;
  }
}

/* get new character from input file */
static int inchar(ts_interp *sc) {
  char c;
  ts_port *pt;

  pt = sc->inport->_object._port;
  if (pt->kind & TS_PORT_SAW_EOF) {
    return EOF;
  }
  c = basic_inchar(pt);
  if (c == EOF && sc->inport == sc->loadport) {
    /* Instead, set TS_PORT_SAW_EOF */
    pt->kind |= TS_PORT_SAW_EOF;

    /* file_pop(sc); */
    return EOF;
    /* NOTREACHED */
  }
  return c;
}

static int basic_inchar(ts_port *pt) {
  if (pt->kind & TS_PORT_FILE) {
    return fgetc(pt->rep.stdio.file);
  } else {
    if (*pt->rep.string.curr == 0 ||
        pt->rep.string.curr == pt->rep.string.past_the_end) {
      return EOF;
    } else {
      return *pt->rep.string.curr++;
    }
  }
}

/* back character to input buffer */
static void backchar(ts_interp *sc, char c) {
  ts_port *pt;
  if (c == EOF) return;
  pt = sc->inport->_object._port;
  if (pt->kind & TS_PORT_FILE) {
    ungetc(c, pt->rep.stdio.file);
  } else {
    if (pt->rep.string.curr != pt->rep.string.start) {
      --pt->rep.string.curr;
    }
  }
}

static int realloc_port_string(ts_interp *sc, ts_port *p) {
  char *start = p->rep.string.start;
  size_t new_size = p->rep.string.past_the_end - start + 1 + BLOCK_SIZE;
  char *str = sc->malloc(new_size);
  if (str) {
    memset(str, ' ', new_size - 1);
    str[new_size - 1] = '\0';
    strcpy(str, start);
    p->rep.string.start = str;
    p->rep.string.past_the_end = str + new_size - 1;
    p->rep.string.curr -= start - str;
    sc->free(start);
    return 1;
  } else {
    return 0;
  }
}

INTERFACE void ts_put_str(ts_interp *sc, const char *s) {
  ts_port *pt = sc->outport->_object._port;
  if (pt->kind & TS_PORT_FILE) {
    fputs(s, pt->rep.stdio.file);
  } else {
    for (; *s; s++) {
      if (pt->rep.string.curr != pt->rep.string.past_the_end) {
        *pt->rep.string.curr++ = *s;
      } else if (pt->kind & TS_PORT_SRFI6 && realloc_port_string(sc, pt)) {
        *pt->rep.string.curr++ = *s;
      }
    }
  }
}

static void putchars(ts_interp *sc, const char *s, int len) {
  ts_port *pt = sc->outport->_object._port;
  if (pt->kind & TS_PORT_FILE) {
    fwrite(s, 1, len, pt->rep.stdio.file);
  } else {
    for (; len; len--) {
      if (pt->rep.string.curr != pt->rep.string.past_the_end) {
        *pt->rep.string.curr++ = *s++;
      } else if (pt->kind & TS_PORT_SRFI6 && realloc_port_string(sc, pt)) {
        *pt->rep.string.curr++ = *s++;
      }
    }
  }
}

INTERFACE void ts_put_char(ts_interp *sc, char c) {
  ts_port *pt = sc->outport->_object._port;
  if (pt->kind & TS_PORT_FILE) {
    fputc(c, pt->rep.stdio.file);
  } else {
    if (pt->rep.string.curr != pt->rep.string.past_the_end) {
      *pt->rep.string.curr++ = c;
    } else if (pt->kind & TS_PORT_SRFI6 && realloc_port_string(sc, pt)) {
      *pt->rep.string.curr++ = c;
    }
  }
}

/* read characters up to delimiter, but cater to character constants */
static char *readstr_upto(ts_interp *sc, const char *delim) {
  char *p = sc->strbuff;

  while ((p - sc->strbuff < sizeof(sc->strbuff)) &&
         !is_one_of(delim, (*p++ = inchar(sc))))
    ;

  if (p == sc->strbuff + 2 && p[-2] == '\\') {
    *p = 0;
  } else {
    backchar(sc, p[-1]);
    *--p = '\0';
  }
  return sc->strbuff;
}

/* read string expression "xxx...xxx" */
static ts_ptr readstrexp(ts_interp *sc) {
  char *p = sc->strbuff;
  char c;
  int c1 = 0;
  enum { st_ok, st_bsl, st_x1, st_x2, st_oct1, st_oct2 } state = st_ok;

  for (;;) {
    c = inchar(sc);
    if (c == EOF || p - sc->strbuff > sizeof(sc->strbuff) - 1) {
      return sc->f;
    }
    switch (state) {
      case st_ok:
        switch (c) {
          case '\\':
            state = st_bsl;
            break;
          case '"':
            *p = 0;
            return ts_mk_counted_str(sc, sc->strbuff, p - sc->strbuff);
          default:
            *p++ = c;
            break;
        }
        break;
      case st_bsl:
        switch (c) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
            state = st_oct1;
            c1 = c - '0';
            break;
          case 'x':
          case 'X':
            state = st_x1;
            c1 = 0;
            break;
          case 'n':
            *p++ = '\n';
            state = st_ok;
            break;
          case 't':
            *p++ = '\t';
            state = st_ok;
            break;
          case 'r':
            *p++ = '\r';
            state = st_ok;
            break;
          case '"':
            *p++ = '"';
            state = st_ok;
            break;
          default:
            *p++ = c;
            state = st_ok;
            break;
        }
        break;
      case st_x1:
      case st_x2:
        c = toupper(c);
        if (c >= '0' && c <= 'F') {
          if (c <= '9') {
            c1 = (c1 << 4) + c - '0';
          } else {
            c1 = (c1 << 4) + c - 'A' + 10;
          }
          if (state == st_x1) {
            state = st_x2;
          } else {
            *p++ = c1;
            state = st_ok;
          }
        } else {
          return sc->f;
        }
        break;
      case st_oct1:
      case st_oct2:
        if (c < '0' || c > '7') {
          *p++ = c1;
          backchar(sc, c);
          state = st_ok;
        } else {
          if (state == st_oct2 && c1 >= 32) return sc->f;

          c1 = (c1 << 3) + (c - '0');

          if (state == st_oct1)
            state = st_oct2;
          else {
            *p++ = c1;
            state = st_ok;
          }
        }
        break;
    }
  }
}

/* check c is in chars */
inline static bool is_one_of(const char *s, char c) {
  if (c == EOF) return 1;
  while (*s)
    if (*s++ == c) return (1);
  return (0);
}

/* skip white characters */
inline static int skipspace(ts_interp *sc) {
  char c = 0, curr_line = 0;

  do {
    c = inchar(sc);
#if SHOW_ERROR_LINE
    if (c == '\n') curr_line++;
#endif
  } while (isspace(c));

/* record it */
#if SHOW_ERROR_LINE
  if (sc->load_stack[sc->file_i].kind & TS_PORT_FILE)
    sc->load_stack[sc->file_i].rep.stdio.curr_line += curr_line;
#endif

  if (c != EOF) {
    backchar(sc, c);
    return 1;
  } else {
    return EOF;
  }
}

/* get token */
static int token(ts_interp *sc) {
  char c;
  c = skipspace(sc);
  if (c == EOF) {
    return (TOK_EOF);
  }
  switch (c = inchar(sc)) {
    case EOF:
      return (TOK_EOF);
    case '(':
      return (TOK_LPAREN);
    case ')':
      return (TOK_RPAREN);
    case '.':
      c = inchar(sc);
      if (is_one_of(" \n\t", c)) {
        return (TOK_DOT);
      } else {
        backchar(sc, c);
        backchar(sc, '.');
        return TOK_ATOM;
      }
    case '\'':
      return (TOK_QUOTE);
    case ';':
      while ((c = inchar(sc)) != '\n' && c != EOF)
        ;

#if SHOW_ERROR_LINE
      if (c == '\n' && sc->load_stack[sc->file_i].kind & TS_PORT_FILE)
        sc->load_stack[sc->file_i].rep.stdio.curr_line++;
#endif

      if (c == EOF) {
        return (TOK_EOF);
      } else {
        return (token(sc));
      }
    case '"':
      return (TOK_DQUOTE);
    case BACKQUOTE:
      return (TOK_BQUOTE);
    case ',':
      if ((c = inchar(sc)) == '@') {
        return (TOK_ATMARK);
      } else {
        backchar(sc, c);
        return (TOK_COMMA);
      }
    case '#':
      c = inchar(sc);
      if (c == '(') {
        return (TOK_VEC);
      } else if (c == '!') {
        while ((c = inchar(sc)) != '\n' && c != EOF)
          ;

#if SHOW_ERROR_LINE
        if (c == '\n' && sc->load_stack[sc->file_i].kind & TS_PORT_FILE)
          sc->load_stack[sc->file_i].rep.stdio.curr_line++;
#endif

        if (c == EOF) {
          return (TOK_EOF);
        } else {
          return (token(sc));
        }
      } else {
        backchar(sc, c);
        if (is_one_of(" tfodxb\\", c)) {
          return TOK_SHARP_CONST;
        } else {
          return (TOK_SHARP);
        }
      }
    default:
      backchar(sc, c);
      return (TOK_ATOM);
  }
}

/* ========== Routines for Printing ========== */
#define ok_abbrev(x) (ts_is_pair(x) && cdr(x) == sc->nil)

static void printslashstring(ts_interp *sc, char *p, int len) {
  int i;
  unsigned char *s = (unsigned char *)p;
  ts_put_char(sc, '"');
  for (i = 0; i < len; i++) {
    if (*s == 0xff || *s == '"' || *s < ' ' || *s == '\\') {
      ts_put_char(sc, '\\');
      switch (*s) {
        case '"':
          ts_put_char(sc, '"');
          break;
        case '\n':
          ts_put_char(sc, 'n');
          break;
        case '\t':
          ts_put_char(sc, 't');
          break;
        case '\r':
          ts_put_char(sc, 'r');
          break;
        case '\\':
          ts_put_char(sc, '\\');
          break;
        default: {
          int d = *s / 16;
          ts_put_char(sc, 'x');
          if (d < 10) {
            ts_put_char(sc, d + '0');
          } else {
            ts_put_char(sc, d - 10 + 'A');
          }
          d = *s % 16;
          if (d < 10) {
            ts_put_char(sc, d + '0');
          } else {
            ts_put_char(sc, d - 10 + 'A');
          }
        }
      }
    } else {
      ts_put_char(sc, *s);
    }
    s++;
  }
  ts_put_char(sc, '"');
}

/* print atoms */
static void printatom(ts_interp *sc, ts_ptr l, int f) {
  char *p;
  int len;
  atom2str(sc, l, f, &p, &len);
  putchars(sc, p, len);
}

/* Uses internal buffer unless string pointer is already available */
static void atom2str(ts_interp *sc, ts_ptr l, int f, char **pp, int *plen) {
  char *p;

  if (l == sc->nil) {
    p = "()";
  } else if (l == sc->t) {
    p = "#t";
  } else if (l == sc->f) {
    p = "#f";
  } else if (l == sc->eof_obj) {
    p = "#<EOF>";
  } else if (ts_is_port(l)) {
    p = "#<PORT>";
  } else if (ts_is_num(l)) {
    p = sc->strbuff;
    if (f <= 1 || f == 10) /* f is the base for numbers if > 1 */ {
      if (num_is_integer(l)) {
        snprintf(p, TS_STRBUFFSIZE, "%d", ivalue_unchecked(l));
      } else {
        snprintf(p, TS_STRBUFFSIZE, "%.10g", rvalue_unchecked(l));
        /* r5rs says there must be a '.' (unless 'e'?) */
        f = strcspn(p, ".e");
        if (p[f] == 0) {
          p[f] = '.'; /* not found, so add '.0' at the end */
          p[f + 1] = '0';
          p[f + 2] = 0;
        }
      }
    } else {
      int v = ts_int_val(l);
      if (f == 16) {
        if (v >= 0)
          snprintf(p, TS_STRBUFFSIZE, "%x", v);
        else
          snprintf(p, TS_STRBUFFSIZE, "-%x", -v);
      } else if (f == 8) {
        if (v >= 0)
          snprintf(p, TS_STRBUFFSIZE, "%o", v);
        else
          snprintf(p, TS_STRBUFFSIZE, "-%o", -v);
      } else if (f == 2) {
        unsigned int b = (v < 0) ? -v : v;
        p = &p[TS_STRBUFFSIZE - 1];
        *p = 0;
        do {
          *--p = (b & 1) ? '1' : '0';
          b >>= 1;
        } while (b != 0);
        if (v < 0) *--p = '-';
      }
    }
  } else if (ts_is_str(l)) {
    if (!f) {
      p = strvalue(l);
    } else { /* Hack, uses the fact that printing is needed */
      *pp = sc->strbuff;
      *plen = 0;
      printslashstring(sc, strvalue(l), strlength(l));
      return;
    }
  } else if (ts_is_char(l)) {
    char c = ts_char_val(l);
    p = sc->strbuff;
    if (!f) {
      p[0] = c;
      p[1] = 0;
    } else {
      switch (c) {
        case ' ':
          p = "#\\space";
          break;
        case '\n':
          p = "#\\newline";
          break;
        case '\r':
          p = "#\\return";
          break;
        case '\t':
          p = "#\\tab";
          break;
        default:
#if USE_ASCII_NAMES
          if (c == 127) {
            p = "#\\del";
            break;
          } else if (c < 32) {
            snprintf(p, TS_STRBUFFSIZE, "#\\%s", charnames[c]);
            break;
          }
#else
          if (c < 32) {
            snprintf(p, TS_STRBUFFSIZE, "#\\x%x", c);
            break;
          }
#endif
          snprintf(p, TS_STRBUFFSIZE, "#\\%c", c);
          break;
      }
    }
  } else if (ts_is_sym(l)) {
    p = ts_sym_name(l);
  } else if (ts_is_proc(l)) {
    p = sc->strbuff;
    snprintf(p, TS_STRBUFFSIZE, "#<%s PROCEDURE %d>", procname(l), procnum(l));
  } else if (ts_is_macro(l)) {
    p = "#<MACRO>";
  } else if (ts_is_closure(l)) {
    p = "#<CLOSURE>";
  } else if (ts_is_promise(l)) {
    p = "#<PROMISE>";
  } else if (ts_is_foreign(l)) {
    p = sc->strbuff;
    snprintf(p, TS_STRBUFFSIZE, "#<FOREIGN PROCEDURE %d>", procnum(l));
  } else if (ts_is_continuation(l)) {
    p = "#<CONTINUATION>";
  }
#if !STANDALONE
  else if (ts_is_userdata(l)) {
    p = "#<USERDATA>";
  }
#endif
  else {
    p = "#<ERROR>";
  }
  *pp = p;
  *plen = strlen(p);
}
/* ========== Routines for Evaluation Cycle ========== */

/* make closure. c is code. e is environment */
static ts_ptr mk_closure(ts_interp *sc, ts_ptr c, ts_ptr e) {
  ts_ptr x = get_cell(sc, c, e);

  typeflag(x) = T_CLOSURE;
  car(x) = c;
  cdr(x) = e;
  return (x);
}

/* make continuation. */
static ts_ptr mk_continuation(ts_interp *sc, ts_ptr d) {
  ts_ptr x = get_cell(sc, sc->nil, d);

  typeflag(x) = T_CONTINUATION;
  cont_dump(x) = d;
  return (x);
}

static ts_ptr list_star(ts_interp *sc, ts_ptr d) {
  ts_ptr p, q;
  if (cdr(d) == sc->nil) {
    return car(d);
  }
  p = ts_cons(sc, car(d), cdr(d));
  q = p;
  while (cdr(cdr(p)) != sc->nil) {
    d = ts_cons(sc, car(p), cdr(p));
    if (cdr(cdr(p)) != sc->nil) {
      p = cdr(d);
    }
  }
  cdr(p) = car(cdr(p));
  return q;
}

/* reverse list -- produce new list */
static ts_ptr reverse(ts_interp *sc, ts_ptr a) {
  /* a must be checked by gc */
  ts_ptr p = sc->nil;

  for (; ts_is_pair(a); a = cdr(a)) {
    p = ts_cons(sc, car(a), p);
  }
  return (p);
}

/* reverse list --- in-place */
ts_ptr reverse_in_place(ts_interp *sc, ts_ptr term, ts_ptr list) {
  ts_ptr p = list, result = term, q;

  while (p != sc->nil) {
    q = cdr(p);
    cdr(p) = result;
    result = p;
    p = q;
  }
  return (result);
}

/* append list -- produce new list (in reverse order) */
static ts_ptr revappend(ts_interp *sc, ts_ptr a, ts_ptr b) {
  ts_ptr result = a;
  ts_ptr p = b;

  while (ts_is_pair(p)) {
    result = ts_cons(sc, car(p), result);
    p = cdr(p);
  }

  if (p == sc->nil) {
    return result;
  }

  return sc->f; /* signal an error */
}

/* equivalence of atoms */
int ts_eqv(ts_ptr a, ts_ptr b) {
  if (ts_is_str(a)) {
    if (ts_is_str(b))
      return (strvalue(a) == strvalue(b));
    else
      return (0);
  } else if (ts_is_num(a)) {
    if (ts_is_num(b)) {
      if (num_is_integer(a) == num_is_integer(b))
        return num_eq(ts_num_val(a), ts_num_val(b));
    }
    return (0);
  } else if (ts_is_char(a)) {
    if (ts_is_char(b))
      return ts_char_val(a) == ts_char_val(b);
    else
      return (0);
  } else if (ts_is_port(a)) {
    if (ts_is_port(b))
      return a == b;
    else
      return (0);
  } else if (ts_is_proc(a)) {
    if (ts_is_proc(b))
      return procnum(a) == procnum(b);
    else
      return (0);
  } else {
    return (a == b);
  }
}

/* true or false value macro */
/* () is #t in R5RS */
#define is_true(p) ((p) != sc->f)
#define is_false(p) ((p) == sc->f)

/* ========== Environment implementation  ========== */

#if !defined(USE_ALIST_ENV) || !defined(USE_OBJECT_LIST)

static int hash_fn(const char *key, int table_size) {
  unsigned int hashed = 0;
  const char *c;
  int bits_per_int = sizeof(unsigned int) * 8;

  for (c = key; *c; c++) {
    /* letters have about 5 bits in them */
    hashed = (hashed << 5) | (hashed >> (bits_per_int - 5));
    hashed ^= *c;
  }
  return hashed % table_size;
}
#endif

#ifndef USE_ALIST_ENV

/*
 * In this implementation, each frame of the environment may be
 * a hash table: a vector of alists hashed by variable name.
 * In practice, we use a vector only for the initial frame;
 * subsequent frames are too small and transient for the lookup
 * speed to out-weigh the cost of making a new vector.
 */

static void new_frame_in_env(ts_interp *sc, ts_ptr old_env) {
  ts_ptr new_frame;

  /* The interaction-environment has about 300 variables in it. */
  if (old_env == sc->nil) {
    new_frame = ts_mk_vec(sc, 461);
  } else {
    new_frame = sc->nil;
  }

  sc->envir = ts_immutable_cons(sc, new_frame, old_env);
  setenvironment(sc->envir);
}

inline static void new_slot_spec_in_env(ts_interp *sc, ts_ptr env, ts_ptr variable,
                                        ts_ptr value) {
  ts_ptr slot = ts_immutable_cons(sc, variable, value);

  if (ts_is_vec(car(env))) {
    int location = hash_fn(ts_sym_name(variable), ivalue_unchecked(car(env)));

    ts_set_vec_elem(
        car(env), location,
        ts_immutable_cons(sc, slot, ts_vec_elem(car(env), location)));
  } else {
    car(env) = ts_immutable_cons(sc, slot, car(env));
  }
}

ts_ptr find_slot_in_env(ts_interp *sc, ts_ptr env, ts_ptr hdl, int all) {
  ts_ptr x, y;
  int location;

  for (x = env; x != sc->nil; x = cdr(x)) {
    if (ts_is_vec(car(x))) {
      location = hash_fn(ts_sym_name(hdl), ivalue_unchecked(car(x)));
      y = ts_vec_elem(car(x), location);
    } else {
      y = car(x);
    }
    for (; y != sc->nil; y = cdr(y)) {
      if (caar(y) == hdl) {
        break;
      }
    }
    if (y != sc->nil) {
      break;
    }
    if (!all) {
      return sc->nil;
    }
  }
  if (x != sc->nil) {
    return car(y);
  }
  return sc->nil;
}

#else /* USE_ALIST_ENV */

inline static void new_frame_in_env(ts_interp *sc, ts_ptr old_env) {
  sc->envir = ts_immutable_cons(sc, sc->nil, old_env);
  setenvironment(sc->envir);
}

inline static void new_slot_spec_in_env(ts_interp *sc, ts_ptr env, ts_ptr variable,
                                        ts_ptr value) {
  car(env) =
      ts_immutable_cons(sc, ts_immutable_cons(sc, variable, value), car(env));
}

static ts_ptr find_slot_in_env(ts_interp *sc, ts_ptr env, ts_ptr hdl, int all) {
  ts_ptr x, y;
  for (x = env; x != sc->nil; x = cdr(x)) {
    for (y = car(x); y != sc->nil; y = cdr(y)) {
      if (caar(y) == hdl) {
        break;
      }
    }
    if (y != sc->nil) {
      break;
    }
    if (!all) {
      return sc->nil;
    }
  }
  if (x != sc->nil) {
    return car(y);
  }
  return sc->nil;
}

#endif /* USE_ALIST_ENV else */

inline static void new_slot_in_env(ts_interp *sc, ts_ptr variable, ts_ptr value) {
  new_slot_spec_in_env(sc, sc->envir, variable, value);
}

inline static void set_slot_in_env(ts_interp *sc, ts_ptr slot, ts_ptr value) {
  cdr(slot) = value;
}

inline ts_ptr slot_value_in_env(ts_ptr slot) { return cdr(slot); }

/* ========== Evaluation Cycle ========== */

static ts_ptr _Error_1(ts_interp *sc, const char *s, ts_ptr a) {
  const char *str = s;
#if USE_ERROR_HOOK
  ts_ptr x;
  ts_ptr hdl = sc->error_hook;
#endif

#if SHOW_ERROR_LINE
  char sbuf[TS_STRBUFFSIZE];

  /* make sure error is not in REPL */
  if (sc->load_stack[sc->file_i].kind & TS_PORT_FILE &&
      sc->load_stack[sc->file_i].rep.stdio.file != stdin) {
    int ln = sc->load_stack[sc->file_i].rep.stdio.curr_line;
    const char *fname = sc->load_stack[sc->file_i].rep.stdio.filename;

    /* should never happen */
    if (!fname) fname = "<unknown>";

    /* we started from 0 */
    ln++;
    snprintf(sbuf, TS_STRBUFFSIZE, "(%s : %i) %s", fname, ln, s);

    str = (const char *)sbuf;
  }
#endif

#if USE_ERROR_HOOK
  x = find_slot_in_env(sc, sc->envir, hdl, 1);
  if (x != sc->nil) {
    if (a != 0) {
      sc->code = ts_cons(sc, ts_cons(sc, sc->quote, ts_cons(sc, (a), sc->nil)),
                         sc->nil);
    } else {
      sc->code = sc->nil;
    }
    sc->code = ts_cons(sc, ts_mk_str(sc, str), sc->code);
    ts_set_immutable(car(sc->code));
    sc->code = ts_cons(sc, slot_value_in_env(x), sc->code);
    sc->op = (int)OP_EVAL;
    return sc->t;
  }
#endif

  if (a != 0) {
    sc->args = ts_cons(sc, (a), sc->nil);
  } else {
    sc->args = sc->nil;
  }
  sc->args = ts_cons(sc, ts_mk_str(sc, str), sc->args);
  ts_set_immutable(car(sc->args));
  sc->op = (int)OP_ERR0;
  return sc->t;
}
#define Error_1(sc, s, a) return _Error_1(sc, s, a)
#define Error_0(sc, s) return _Error_1(sc, s, 0)

/* Too small to turn into function */
#define BEGIN do {
#define END \
  }         \
  while (0)
#define s_goto(sc, a) \
  BEGIN               \
  sc->op = (int)(a);  \
  return sc->t;       \
  END

#define s_return(sc, a) return _s_return(sc, a)

#if !USE_STACK

/* this structure holds all the interpreter's registers */
struct dump_stack_frame {
  enum opcodes op;
  ts_ptr args;
  ts_ptr envir;
  ts_ptr code;
};

#define STACK_GROWTH 3

static void s_save(ts_interp *sc, enum opcodes op, ts_ptr args, ts_ptr code) {
  int nframes = (int)sc->dump;
  struct dump_stack_frame *next_frame;

  /* enough room for the next frame? */
  if (nframes >= sc->dump_size) {
    sc->dump_size += STACK_GROWTH;
    /* alas there is no sc->realloc */
    sc->dump_base =
        realloc(sc->dump_base, sizeof(struct dump_stack_frame) * sc->dump_size);
  }
  next_frame = (struct dump_stack_frame *)sc->dump_base + nframes;
  next_frame->op = op;
  next_frame->args = args;
  next_frame->envir = sc->envir;
  next_frame->code = code;
  sc->dump = (ts_ptr)(nframes + 1);
}

static ts_ptr _s_return(ts_interp *sc, ts_ptr a) {
  int nframes = (int)sc->dump;
  struct dump_stack_frame *frame;

  sc->value = (a);
  if (nframes <= 0) {
    return sc->nil;
  }
  nframes--;
  frame = (struct dump_stack_frame *)sc->dump_base + nframes;
  sc->op = frame->op;
  sc->args = frame->args;
  sc->envir = frame->envir;
  sc->code = frame->code;
  sc->dump = (ts_ptr)nframes;
  return sc->t;
}

inline void dump_stack_reset(ts_interp *sc) {
  /* in this implementation, sc->dump is the number of frames on the stack */
  sc->dump = (ts_ptr)0;
}

inline static void dump_stack_initialize(ts_interp *sc) {
  sc->dump_size = 0;
  sc->dump_base = NULL;
  dump_stack_reset(sc);
}

static void dump_stack_free(ts_interp *sc) {
  free(sc->dump_base);
  sc->dump_base = NULL;
  sc->dump = (ts_ptr)0;
  sc->dump_size = 0;
}

inline static void dump_stack_mark(ts_interp *sc) {
  int nframes = (int)sc->dump;
  int i;
  for (i = 0; i < nframes; i++) {
    struct dump_stack_frame *frame;
    frame = (struct dump_stack_frame *)sc->dump_base + i;
    mark(frame->args);
    mark(frame->envir);
    mark(frame->code);
  }
}

#else

inline void dump_stack_reset(ts_interp *sc) { sc->dump = sc->nil; }

inline static void dump_stack_initialize(ts_interp *sc) { dump_stack_reset(sc); }

static void dump_stack_free(ts_interp *sc) { sc->dump = sc->nil; }

static ts_ptr _s_return(ts_interp *sc, ts_ptr a) {
  sc->value = (a);
  if (sc->dump == sc->nil) return sc->nil;
  sc->op = ts_int_val(car(sc->dump));
  sc->args = cadr(sc->dump);
  sc->envir = caddr(sc->dump);
  sc->code = cadddr(sc->dump);
  sc->dump = cddddr(sc->dump);
  return sc->t;
}

static void s_save(ts_interp *sc, enum opcodes op, ts_ptr args, ts_ptr code) {
  sc->dump = ts_cons(sc, sc->envir, ts_cons(sc, (code), sc->dump));
  sc->dump = ts_cons(sc, (args), sc->dump);
  sc->dump = ts_cons(sc, ts_mk_int(sc, (int)(op)), sc->dump);
}

inline static void dump_stack_mark(ts_interp *sc) { mark(sc->dump); }
#endif

#define s_retbool(tf) s_return(sc, (tf) ? sc->t : sc->f)

static ts_ptr opexe_0(ts_interp *sc, enum opcodes op) {
  ts_ptr x, y;

  switch (op) {
    case OP_LOAD: /* load */
      if (file_interactive(sc)) {
        fprintf(sc->outport->_object._port->rep.stdio.file, "Loading %s\n",
                strvalue(car(sc->args)));
      }
      if (!file_push(sc, strvalue(car(sc->args)))) {
        Error_1(sc, "unable to open", car(sc->args));
      } else {
        sc->args = ts_mk_int(sc, sc->file_i);
        s_goto(sc, OP_T0LVL);
      }

    case OP_T0LVL: /* top level */
      /* If we reached the end of file, this loop is done. */
      if (sc->loadport->_object._port->kind & TS_PORT_SAW_EOF) {
        if (sc->file_i == 0) {
          sc->args = sc->nil;
          s_goto(sc, OP_QUIT);
        } else {
          file_pop(sc);
          s_return(sc, sc->value);
        }
        /* NOTREACHED */
      }

      /* If interactive, be nice to user. */
      if (file_interactive(sc)) {
        sc->envir = sc->global_env;
        dump_stack_reset(sc);
        ts_put_str(sc, "\n");
        ts_put_str(sc, prompt);
      }

      /* Set up another iteration of REPL */
      sc->nesting = 0;
      sc->save_inport = sc->inport;
      sc->inport = sc->loadport;
      s_save(sc, OP_T0LVL, sc->nil, sc->nil);
      s_save(sc, OP_VALUEPRINT, sc->nil, sc->nil);
      s_save(sc, OP_T1LVL, sc->nil, sc->nil);
      s_goto(sc, OP_READ_INTERNAL);

    case OP_T1LVL: /* top level */
      sc->code = sc->value;
      sc->inport = sc->save_inport;
      s_goto(sc, OP_EVAL);

    case OP_READ_INTERNAL: /* internal read */
      sc->tok = token(sc);
      if (sc->tok == TOK_EOF) {
        s_return(sc, sc->eof_obj);
      }
      s_goto(sc, OP_RDSEXPR);

    case OP_GENSYM:
      s_return(sc, ts_gen_sym(sc));

    case OP_VALUEPRINT: /* print evaluation result */
      /* OP_VALUEPRINT is always pushed, because when changing from
         non-interactive to interactive mode, it needs to be
         already on the stack */
      if (sc->tracing) {
        ts_put_str(sc, "\nGives: ");
      }
      if (file_interactive(sc)) {
        sc->print_flag = 1;
        sc->args = sc->value;
        s_goto(sc, OP_P0LIST);
      } else {
        s_return(sc, sc->value);
      }

    case OP_EVAL: /* main part of evaluation */
#if USE_TRACING
      if (sc->tracing) {
        /*s_save(sc,OP_VALUEPRINT,sc->nil,sc->nil);*/
        s_save(sc, OP_REAL_EVAL, sc->args, sc->code);
        sc->args = sc->code;
        ts_put_str(sc, "\nEval: ");
        s_goto(sc, OP_P0LIST);
      }
      /* fall through */
    case OP_REAL_EVAL:
#endif
      if (ts_is_sym(sc->code)) { /* symbol */
        x = find_slot_in_env(sc, sc->envir, sc->code, 1);
        if (x != sc->nil) {
          s_return(sc, slot_value_in_env(x));
        } else {
          Error_1(sc, "eval: unbound variable:", sc->code);
        }
      } else if (ts_is_pair(sc->code)) {
        if (ts_is_syntax(x = car(sc->code))) { /* SYNTAX */
          sc->code = cdr(sc->code);
          s_goto(sc, syntaxnum(x));
        } else { /* first, eval top element and eval arguments */
          s_save(sc, OP_E0ARGS, sc->nil, sc->code);
          /* If no macros => s_save(sc,OP_E1ARGS, sc->nil, cdr(sc->code));*/
          sc->code = car(sc->code);
          s_goto(sc, OP_EVAL);
        }
      } else {
        s_return(sc, sc->code);
      }

    case OP_E0ARGS:                 /* eval arguments */
      if (ts_is_macro(sc->value)) { /* macro expansion */
        s_save(sc, OP_DOMACRO, sc->nil, sc->nil);
        sc->args = ts_cons(sc, sc->code, sc->nil);
        sc->code = sc->value;
        s_goto(sc, OP_APPLY);
      } else {
        sc->code = cdr(sc->code);
        s_goto(sc, OP_E1ARGS);
      }

    case OP_E1ARGS: /* eval arguments */
      sc->args = ts_cons(sc, sc->value, sc->args);
      if (ts_is_pair(sc->code)) { /* continue */
        s_save(sc, OP_E1ARGS, sc->args, cdr(sc->code));
        sc->code = car(sc->code);
        sc->args = sc->nil;
        s_goto(sc, OP_EVAL);
      } else { /* end */
        sc->args = reverse_in_place(sc, sc->nil, sc->args);
        sc->code = car(sc->args);
        sc->args = cdr(sc->args);
        s_goto(sc, OP_APPLY);
      }

#if USE_TRACING
    case OP_TRACING: {
      int tr = sc->tracing;
      sc->tracing = ts_int_val(car(sc->args));
      s_return(sc, ts_mk_int(sc, tr));
    }
#endif

    case OP_APPLY: /* apply 'code' to 'args' */
#if USE_TRACING
      if (sc->tracing) {
        s_save(sc, OP_REAL_APPLY, sc->args, sc->code);
        sc->print_flag = 1;
        /*  sc->args=ts_cons(sc,sc->code,sc->args);*/
        ts_put_str(sc, "\nApply to: ");
        s_goto(sc, OP_P0LIST);
      }
      /* fall through */
    case OP_REAL_APPLY:
#endif
      if (ts_is_proc(sc->code)) {
        s_goto(sc, procnum(sc->code)); /* PROCEDURE */
      } else if (ts_is_foreign(sc->code)) {
        /* Keep nested calls from GC'ing the arglist */
        push_recent_alloc(sc, sc->args, sc->nil);
        x = sc->code->_object._ff(sc, sc->args);
        s_return(sc, x);
      } else if (ts_is_closure(sc->code) || ts_is_macro(sc->code) ||
                 ts_is_promise(sc->code)) { /* CLOSURE */
                                            /* Should not accept promise */
        /* make environment */
        new_frame_in_env(sc, ts_closure_env(sc->code));
        for (x = car(ts_closure_code(sc->code)), y = sc->args; ts_is_pair(x);
             x = cdr(x), y = cdr(y)) {
          if (y == sc->nil) {
            Error_0(sc, "not enough arguments");
          } else {
            new_slot_in_env(sc, car(x), car(y));
          }
        }
        if (x == sc->nil) {
          /*--
           * if (y != sc->nil) {
           *   Error_0(sc,"too many arguments");
           * }
           */
        } else if (ts_is_sym(x))
          new_slot_in_env(sc, x, y);
        else {
          Error_1(sc, "syntax error in closure: not a symbol:", x);
        }
        sc->code = cdr(ts_closure_code(sc->code));
        sc->args = sc->nil;
        s_goto(sc, OP_BEGIN);
      } else if (ts_is_continuation(sc->code)) { /* CONTINUATION */
        sc->dump = cont_dump(sc->code);
        s_return(sc, sc->args != sc->nil ? car(sc->args) : sc->nil);
      } else {
        Error_0(sc, "illegal function");
      }

    case OP_DOMACRO: /* do macro */
      sc->code = sc->value;
      s_goto(sc, OP_EVAL);

#if 1
    case OP_LAMBDA: /* lambda */
      /* If the hook is defined, apply it to sc->code, otherwise
         set sc->value fall thru */
      {
        ts_ptr f = find_slot_in_env(sc, sc->envir, sc->compile_hook, 1);
        if (f == sc->nil) {
          sc->value = sc->code;
          /* Fallthru */
        } else {
          s_save(sc, OP_LAMBDA1, sc->args, sc->code);
          sc->args = ts_cons(sc, sc->code, sc->nil);
          sc->code = slot_value_in_env(f);
          s_goto(sc, OP_APPLY);
        }
      }

    case OP_LAMBDA1:
      s_return(sc, mk_closure(sc, sc->value, sc->envir));

#else
    case OP_LAMBDA: /* lambda */
      s_return(sc, mk_closure(sc, sc->code, sc->envir));

#endif

    case OP_MKCLOSURE: /* make-closure */
      x = car(sc->args);
      if (car(x) == sc->lambda) {
        x = cdr(x);
      }
      if (cdr(sc->args) == sc->nil) {
        y = sc->envir;
      } else {
        y = cadr(sc->args);
      }
      s_return(sc, mk_closure(sc, x, y));

    case OP_QUOTE: /* quote */
      s_return(sc, car(sc->code));

    case OP_DEF0: /* define */
      if (ts_is_immutable(car(sc->code)))
        Error_1(sc, "define: unable to alter immutable", car(sc->code));

      if (ts_is_pair(car(sc->code))) {
        x = caar(sc->code);
        sc->code =
            ts_cons(sc, sc->lambda, ts_cons(sc, cdar(sc->code), cdr(sc->code)));
      } else {
        x = car(sc->code);
        sc->code = cadr(sc->code);
      }
      if (!ts_is_sym(x)) {
        Error_0(sc, "variable is not a symbol");
      }
      s_save(sc, OP_DEF1, sc->nil, x);
      s_goto(sc, OP_EVAL);

    case OP_DEF1: /* define */
      x = find_slot_in_env(sc, sc->envir, sc->code, 0);
      if (x != sc->nil) {
        set_slot_in_env(sc, x, sc->value);
      } else {
        new_slot_in_env(sc, sc->code, sc->value);
      }
      s_return(sc, sc->code);

    case OP_DEFP: /* defined? */
      x = sc->envir;
      if (cdr(sc->args) != sc->nil) {
        x = cadr(sc->args);
      }
      s_retbool(find_slot_in_env(sc, x, car(sc->args), 1) != sc->nil);

    case OP_SET0: /* set! */
      if (ts_is_immutable(car(sc->code)))
        Error_1(sc, "set!: unable to alter immutable variable", car(sc->code));
      s_save(sc, OP_SET1, sc->nil, car(sc->code));
      sc->code = cadr(sc->code);
      s_goto(sc, OP_EVAL);

    case OP_SET1: /* set! */
      y = find_slot_in_env(sc, sc->envir, sc->code, 1);
      if (y != sc->nil) {
        set_slot_in_env(sc, y, sc->value);
        s_return(sc, sc->value);
      } else {
        Error_1(sc, "set!: unbound variable:", sc->code);
      }

    case OP_BEGIN: /* begin */
      if (!ts_is_pair(sc->code)) {
        s_return(sc, sc->code);
      }
      if (cdr(sc->code) != sc->nil) {
        s_save(sc, OP_BEGIN, sc->nil, cdr(sc->code));
      }
      sc->code = car(sc->code);
      s_goto(sc, OP_EVAL);

    case OP_IF0: /* if */
      s_save(sc, OP_IF1, sc->nil, cdr(sc->code));
      sc->code = car(sc->code);
      s_goto(sc, OP_EVAL);

    case OP_IF1: /* if */
      if (is_true(sc->value))
        sc->code = car(sc->code);
      else
        sc->code = cadr(sc->code); /* (if #f 1) ==> () because
                                    * car(sc->nil) = sc->nil */
      s_goto(sc, OP_EVAL);

    case OP_LET0: /* let */
      sc->args = sc->nil;
      sc->value = sc->code;
      sc->code = ts_is_sym(car(sc->code)) ? cadr(sc->code) : car(sc->code);
      s_goto(sc, OP_LET1);

    case OP_LET1: /* let (calculate parameters) */
      sc->args = ts_cons(sc, sc->value, sc->args);
      if (ts_is_pair(sc->code)) { /* continue */
        if (!ts_is_pair(car(sc->code)) || !ts_is_pair(cdar(sc->code))) {
          Error_1(sc, "Bad syntax of binding spec in let :", car(sc->code));
        }
        s_save(sc, OP_LET1, sc->args, cdr(sc->code));
        sc->code = cadar(sc->code);
        sc->args = sc->nil;
        s_goto(sc, OP_EVAL);
      } else { /* end */
        sc->args = reverse_in_place(sc, sc->nil, sc->args);
        sc->code = car(sc->args);
        sc->args = cdr(sc->args);
        s_goto(sc, OP_LET2);
      }

    case OP_LET2: /* let */
      new_frame_in_env(sc, sc->envir);
      for (x = ts_is_sym(car(sc->code)) ? cadr(sc->code) : car(sc->code),
          y = sc->args;
           y != sc->nil; x = cdr(x), y = cdr(y)) {
        new_slot_in_env(sc, caar(x), car(y));
      }
      if (ts_is_sym(car(sc->code))) { /* named let */
        for (x = cadr(sc->code), sc->args = sc->nil; x != sc->nil; x = cdr(x)) {
          if (!ts_is_pair(x)) Error_1(sc, "Bad syntax of binding in let :", x);
          if (!ts_is_list(sc, car(x)))
            Error_1(sc, "Bad syntax of binding in let :", car(x));
          sc->args = ts_cons(sc, caar(x), sc->args);
        }
        x = mk_closure(sc,
                       ts_cons(sc, reverse_in_place(sc, sc->nil, sc->args),
                               cddr(sc->code)),
                       sc->envir);
        new_slot_in_env(sc, car(sc->code), x);
        sc->code = cddr(sc->code);
        sc->args = sc->nil;
      } else {
        sc->code = cdr(sc->code);
        sc->args = sc->nil;
      }
      s_goto(sc, OP_BEGIN);

    case OP_LET0AST: /* let* */
      if (car(sc->code) == sc->nil) {
        new_frame_in_env(sc, sc->envir);
        sc->code = cdr(sc->code);
        s_goto(sc, OP_BEGIN);
      }
      if (!ts_is_pair(car(sc->code)) || !ts_is_pair(caar(sc->code)) ||
          !ts_is_pair(cdaar(sc->code))) {
        Error_1(sc, "Bad syntax of binding spec in let* :", car(sc->code));
      }
      s_save(sc, OP_LET1AST, cdr(sc->code), car(sc->code));
      sc->code = cadaar(sc->code);
      s_goto(sc, OP_EVAL);

    case OP_LET1AST: /* let* (make new frame) */
      new_frame_in_env(sc, sc->envir);
      s_goto(sc, OP_LET2AST);

    case OP_LET2AST: /* let* (calculate parameters) */
      new_slot_in_env(sc, caar(sc->code), sc->value);
      sc->code = cdr(sc->code);
      if (ts_is_pair(sc->code)) { /* continue */
        s_save(sc, OP_LET2AST, sc->args, sc->code);
        sc->code = cadar(sc->code);
        sc->args = sc->nil;
        s_goto(sc, OP_EVAL);
      } else { /* end */
        sc->code = sc->args;
        sc->args = sc->nil;
        s_goto(sc, OP_BEGIN);
      }
    default:
      snprintf(sc->strbuff, TS_STRBUFFSIZE, "%d: illegal operator", sc->op);
      Error_0(sc, sc->strbuff);
  }
  return sc->t;
}

static ts_ptr opexe_1(ts_interp *sc, enum opcodes op) {
  ts_ptr x, y;

  switch (op) {
    case OP_LET0REC: /* letrec */
      new_frame_in_env(sc, sc->envir);
      sc->args = sc->nil;
      sc->value = sc->code;
      sc->code = car(sc->code);
      s_goto(sc, OP_LET1REC);

    case OP_LET1REC: /* letrec (calculate parameters) */
      sc->args = ts_cons(sc, sc->value, sc->args);
      if (ts_is_pair(sc->code)) { /* continue */
        if (!ts_is_pair(car(sc->code)) || !ts_is_pair(cdar(sc->code))) {
          Error_1(sc, "Bad syntax of binding spec in letrec :", car(sc->code));
        }
        s_save(sc, OP_LET1REC, sc->args, cdr(sc->code));
        sc->code = cadar(sc->code);
        sc->args = sc->nil;
        s_goto(sc, OP_EVAL);
      } else { /* end */
        sc->args = reverse_in_place(sc, sc->nil, sc->args);
        sc->code = car(sc->args);
        sc->args = cdr(sc->args);
        s_goto(sc, OP_LET2REC);
      }

    case OP_LET2REC: /* letrec */
      for (x = car(sc->code), y = sc->args; y != sc->nil;
           x = cdr(x), y = cdr(y)) {
        new_slot_in_env(sc, caar(x), car(y));
      }
      sc->code = cdr(sc->code);
      sc->args = sc->nil;
      s_goto(sc, OP_BEGIN);

    case OP_COND0: /* cond */
      if (!ts_is_pair(sc->code)) {
        Error_0(sc, "syntax error in cond");
      }
      s_save(sc, OP_COND1, sc->nil, sc->code);
      sc->code = caar(sc->code);
      s_goto(sc, OP_EVAL);

    case OP_COND1: /* cond */
      if (is_true(sc->value)) {
        if ((sc->code = cdar(sc->code)) == sc->nil) {
          s_return(sc, sc->value);
        }
        if (!sc->code || car(sc->code) == sc->feed_to) {
          if (!ts_is_pair(cdr(sc->code))) {
            Error_0(sc, "syntax error in cond");
          }
          x = ts_cons(sc, sc->quote, ts_cons(sc, sc->value, sc->nil));
          sc->code = ts_cons(sc, cadr(sc->code), ts_cons(sc, x, sc->nil));
          s_goto(sc, OP_EVAL);
        }
        s_goto(sc, OP_BEGIN);
      } else {
        if ((sc->code = cdr(sc->code)) == sc->nil) {
          s_return(sc, sc->nil);
        } else {
          s_save(sc, OP_COND1, sc->nil, sc->code);
          sc->code = caar(sc->code);
          s_goto(sc, OP_EVAL);
        }
      }

    case OP_DELAY: /* delay */
      x = mk_closure(sc, ts_cons(sc, sc->nil, sc->code), sc->envir);
      typeflag(x) = T_PROMISE;
      s_return(sc, x);

    case OP_AND0: /* and */
      if (sc->code == sc->nil) {
        s_return(sc, sc->t);
      }
      s_save(sc, OP_AND1, sc->nil, cdr(sc->code));
      sc->code = car(sc->code);
      s_goto(sc, OP_EVAL);

    case OP_AND1: /* and */
      if (is_false(sc->value)) {
        s_return(sc, sc->value);
      } else if (sc->code == sc->nil) {
        s_return(sc, sc->value);
      } else {
        s_save(sc, OP_AND1, sc->nil, cdr(sc->code));
        sc->code = car(sc->code);
        s_goto(sc, OP_EVAL);
      }

    case OP_OR0: /* or */
      if (sc->code == sc->nil) {
        s_return(sc, sc->f);
      }
      s_save(sc, OP_OR1, sc->nil, cdr(sc->code));
      sc->code = car(sc->code);
      s_goto(sc, OP_EVAL);

    case OP_OR1: /* or */
      if (is_true(sc->value)) {
        s_return(sc, sc->value);
      } else if (sc->code == sc->nil) {
        s_return(sc, sc->value);
      } else {
        s_save(sc, OP_OR1, sc->nil, cdr(sc->code));
        sc->code = car(sc->code);
        s_goto(sc, OP_EVAL);
      }

    case OP_C0STREAM: /* cons-stream */
      s_save(sc, OP_C1STREAM, sc->nil, cdr(sc->code));
      sc->code = car(sc->code);
      s_goto(sc, OP_EVAL);

    case OP_C1STREAM:       /* cons-stream */
      sc->args = sc->value; /* save sc->value to register sc->args for gc */
      x = mk_closure(sc, ts_cons(sc, sc->nil, sc->code), sc->envir);
      typeflag(x) = T_PROMISE;
      s_return(sc, ts_cons(sc, sc->args, x));

    case OP_MACRO0: /* macro */
      if (ts_is_pair(car(sc->code))) {
        x = caar(sc->code);
        sc->code =
            ts_cons(sc, sc->lambda, ts_cons(sc, cdar(sc->code), cdr(sc->code)));
      } else {
        x = car(sc->code);
        sc->code = cadr(sc->code);
      }
      if (!ts_is_sym(x)) {
        Error_0(sc, "variable is not a symbol");
      }
      s_save(sc, OP_MACRO1, sc->nil, x);
      s_goto(sc, OP_EVAL);

    case OP_MACRO1: /* macro */
      typeflag(sc->value) = T_MACRO;
      x = find_slot_in_env(sc, sc->envir, sc->code, 0);
      if (x != sc->nil) {
        set_slot_in_env(sc, x, sc->value);
      } else {
        new_slot_in_env(sc, sc->code, sc->value);
      }
      s_return(sc, sc->code);

    case OP_CASE0: /* case */
      s_save(sc, OP_CASE1, sc->nil, cdr(sc->code));
      sc->code = car(sc->code);
      s_goto(sc, OP_EVAL);

    case OP_CASE1: /* case */
      for (x = sc->code; x != sc->nil; x = cdr(x)) {
        if (!ts_is_pair(y = caar(x))) {
          break;
        }
        for (; y != sc->nil; y = cdr(y)) {
          if (ts_eqv(car(y), sc->value)) {
            break;
          }
        }
        if (y != sc->nil) {
          break;
        }
      }
      if (x != sc->nil) {
        if (ts_is_pair(caar(x))) {
          sc->code = cdar(x);
          s_goto(sc, OP_BEGIN);
        } else { /* else */
          s_save(sc, OP_CASE2, sc->nil, cdar(x));
          sc->code = caar(x);
          s_goto(sc, OP_EVAL);
        }
      } else {
        s_return(sc, sc->nil);
      }

    case OP_CASE2: /* case */
      if (is_true(sc->value)) {
        s_goto(sc, OP_BEGIN);
      } else {
        s_return(sc, sc->nil);
      }

    case OP_PAPPLY: /* apply */
      sc->code = car(sc->args);
      sc->args = list_star(sc, cdr(sc->args));
      /*sc->args = cadr(sc->args);*/
      s_goto(sc, OP_APPLY);

    case OP_PEVAL: /* eval */
      if (cdr(sc->args) != sc->nil) {
        sc->envir = cadr(sc->args);
      }
      sc->code = car(sc->args);
      s_goto(sc, OP_EVAL);

    case OP_CONTINUATION: /* call-with-current-continuation */
      sc->code = car(sc->args);
      sc->args = ts_cons(sc, mk_continuation(sc, sc->dump), sc->nil);
      s_goto(sc, OP_APPLY);

    default:
      snprintf(sc->strbuff, TS_STRBUFFSIZE, "%d: illegal operator", sc->op);
      Error_0(sc, sc->strbuff);
  }
  return sc->t;
}

static ts_ptr opexe_2(ts_interp *sc, enum opcodes op) {
  ts_ptr x;
  ts_num v;
#if USE_MATH
  double dd;
#endif

  switch (op) {
#if USE_MATH
    case OP_INEX2EX: /* inexact->exact */
      x = car(sc->args);
      if (num_is_integer(x)) {
        s_return(sc, x);
      } else if (modf(rvalue_unchecked(x), &dd) == 0.0) {
        s_return(sc, ts_mk_int(sc, ts_int_val(x)));
      } else {
        Error_1(sc, "inexact->exact: not integral:", x);
      }

    case OP_EXP:
      x = car(sc->args);
      s_return(sc, ts_mk_real(sc, exp(ts_real_val(x))));

    case OP_LOG:
      x = car(sc->args);
      s_return(sc, ts_mk_real(sc, log(ts_real_val(x))));

    case OP_SIN:
      x = car(sc->args);
      s_return(sc, ts_mk_real(sc, sin(ts_real_val(x))));

    case OP_COS:
      x = car(sc->args);
      s_return(sc, ts_mk_real(sc, cos(ts_real_val(x))));

    case OP_TAN:
      x = car(sc->args);
      s_return(sc, ts_mk_real(sc, tan(ts_real_val(x))));

    case OP_ASIN:
      x = car(sc->args);
      s_return(sc, ts_mk_real(sc, asin(ts_real_val(x))));

    case OP_ACOS:
      x = car(sc->args);
      s_return(sc, ts_mk_real(sc, acos(ts_real_val(x))));

    case OP_ATAN:
      x = car(sc->args);
      if (cdr(sc->args) == sc->nil) {
        s_return(sc, ts_mk_real(sc, atan(ts_real_val(x))));
      } else {
        ts_ptr y = cadr(sc->args);
        s_return(sc, ts_mk_real(sc, atan2(ts_real_val(x), ts_real_val(y))));
      }

    case OP_SQRT:
      x = car(sc->args);
      s_return(sc, ts_mk_real(sc, sqrt(ts_real_val(x))));

    case OP_EXPT: {
      double result;
      int real_result = 1;
      ts_ptr y = cadr(sc->args);
      x = car(sc->args);
      if (num_is_integer(x) && num_is_integer(y)) real_result = 0;
      /* This 'if' is an R5RS compatibility fix. */
      /* NOTE: Remove this 'if' fix for R6RS.    */
      if (ts_real_val(x) == 0 && ts_real_val(y) < 0) {
        result = 0.0;
      } else {
        result = pow(ts_real_val(x), ts_real_val(y));
      }
      /* Before returning integer result make sure we can. */
      /* If the test fails, result is too big for integer. */
      if (!real_result) {
        int result_as_long = (int)result;
        if (result != (double)result_as_long) real_result = 1;
      }
      if (real_result) {
        s_return(sc, ts_mk_real(sc, result));
      } else {
        s_return(sc, ts_mk_int(sc, result));
      }
    }

    case OP_FLOOR:
      x = car(sc->args);
      s_return(sc, ts_mk_real(sc, floor(ts_real_val(x))));

    case OP_CEILING:
      x = car(sc->args);
      s_return(sc, ts_mk_real(sc, ceil(ts_real_val(x))));

    case OP_TRUNCATE: {
      double rvalue_of_x;
      x = car(sc->args);
      rvalue_of_x = ts_real_val(x);
      if (rvalue_of_x > 0) {
        s_return(sc, ts_mk_real(sc, floor(rvalue_of_x)));
      } else {
        s_return(sc, ts_mk_real(sc, ceil(rvalue_of_x)));
      }
    }

    case OP_ROUND:
      x = car(sc->args);
      if (num_is_integer(x)) s_return(sc, x);
      s_return(sc, ts_mk_real(sc, round_per_R5RS(ts_real_val(x))));
#endif

    case OP_ADD: /* + */
      v = num_zero;
      for (x = sc->args; x != sc->nil; x = cdr(x)) {
        v = num_add(v, ts_num_val(car(x)));
      }
      s_return(sc, mk_number(sc, v));

    case OP_MUL: /* * */
      v = num_one;
      for (x = sc->args; x != sc->nil; x = cdr(x)) {
        v = num_mul(v, ts_num_val(car(x)));
      }
      s_return(sc, mk_number(sc, v));

    case OP_SUB: /* - */
      if (cdr(sc->args) == sc->nil) {
        x = sc->args;
        v = num_zero;
      } else {
        x = cdr(sc->args);
        v = ts_num_val(car(sc->args));
      }
      for (; x != sc->nil; x = cdr(x)) {
        v = num_sub(v, ts_num_val(car(x)));
      }
      s_return(sc, mk_number(sc, v));

    case OP_DIV: /* / */
      if (cdr(sc->args) == sc->nil) {
        x = sc->args;
        v = num_one;
      } else {
        x = cdr(sc->args);
        v = ts_num_val(car(sc->args));
      }
      for (; x != sc->nil; x = cdr(x)) {
        if (!is_zero_double(ts_real_val(car(x))))
          v = num_div(v, ts_num_val(car(x)));
        else {
          Error_0(sc, "/: division by zero");
        }
      }
      s_return(sc, mk_number(sc, v));

    case OP_INTDIV: /* quotient */
      v = ts_num_val(car(sc->args));
      x = cadr(sc->args);
      if (ts_int_val(x) != 0)
        v = num_intdiv(v, ts_num_val(x));
      else {
        Error_0(sc, "quotient: division by zero");
      }
      s_return(sc, mk_number(sc, v));

    case OP_REM: /* remainder */
      v = ts_num_val(car(sc->args));
      x = cadr(sc->args);
      if (ts_int_val(x) != 0)
        v = num_rem(v, ts_num_val(x));
      else {
        Error_0(sc, "remainder: division by zero");
      }
      s_return(sc, mk_number(sc, v));

    case OP_MOD: /* modulo */
      v = ts_num_val(car(sc->args));
      x = cadr(sc->args);
      if (ts_int_val(x) != 0)
        v = num_mod(v, ts_num_val(x));
      else {
        Error_0(sc, "modulo: division by zero");
      }
      s_return(sc, mk_number(sc, v));

    case OP_CAR: /* car */
      s_return(sc, caar(sc->args));

    case OP_CDR: /* cdr */
      s_return(sc, cdar(sc->args));

    case OP_CONS: /* cons */
      cdr(sc->args) = cadr(sc->args);
      s_return(sc, sc->args);

    case OP_SETCAR: /* set-car! */
      if (!ts_is_immutable(car(sc->args))) {
        caar(sc->args) = cadr(sc->args);
        s_return(sc, car(sc->args));
      } else {
        Error_0(sc, "set-car!: unable to alter immutable pair");
      }

    case OP_SETCDR: /* set-cdr! */
      if (!ts_is_immutable(car(sc->args))) {
        cdar(sc->args) = cadr(sc->args);
        s_return(sc, car(sc->args));
      } else {
        Error_0(sc, "set-cdr!: unable to alter immutable pair");
      }

    case OP_CHAR2INT: { /* char->integer */
      char c;
      c = (char)ts_int_val(car(sc->args));
      s_return(sc, ts_mk_int(sc, (unsigned char)c));
    }

    case OP_INT2CHAR: { /* integer->char */
      unsigned char c;
      c = (unsigned char)ts_int_val(car(sc->args));
      s_return(sc, ts_mk_char(sc, (char)c));
    }

    case OP_CHARUPCASE: {
      unsigned char c;
      c = (unsigned char)ts_int_val(car(sc->args));
      c = toupper(c);
      s_return(sc, ts_mk_char(sc, (char)c));
    }

    case OP_CHARDNCASE: {
      unsigned char c;
      c = (unsigned char)ts_int_val(car(sc->args));
      c = tolower(c);
      s_return(sc, ts_mk_char(sc, (char)c));
    }

    case OP_STR2SYM: /* string->symbol */
      s_return(sc, ts_mk_sym(sc, strvalue(car(sc->args))));

    case OP_STR2ATOM: /* string->atom */ {
      char *s = strvalue(car(sc->args));
      int pf = 0;
      if (cdr(sc->args) != sc->nil) {
        /* we know cadr(sc->args) is a natural number */
        /* see if it is 2, 8, 10, or 16, or error */
        pf = ivalue_unchecked(cadr(sc->args));
        if (pf == 16 || pf == 10 || pf == 8 || pf == 2) {
          /* base is OK */
        } else {
          pf = -1;
        }
      }
      if (pf < 0) {
        Error_1(sc, "string->atom: bad base:", cadr(sc->args));
      } else if (*s == '#') /* no use of base! */ {
        s_return(sc, mk_sharp_const(sc, s + 1));
      } else {
        if (pf == 0 || pf == 10) {
          s_return(sc, mk_atom(sc, s));
        } else {
          char *ep;
          int iv = strtol(s, &ep, (int)pf);
          if (*ep == 0) {
            s_return(sc, ts_mk_int(sc, iv));
          } else {
            s_return(sc, sc->f);
          }
        }
      }
    }

    case OP_SYM2STR: /* symbol->string */
      x = ts_mk_str(sc, ts_sym_name(car(sc->args)));
      ts_set_immutable(x);
      s_return(sc, x);

    case OP_ATOM2STR: /* atom->string */ {
      int pf = 0;
      x = car(sc->args);
      if (cdr(sc->args) != sc->nil) {
        /* we know cadr(sc->args) is a natural number */
        /* see if it is 2, 8, 10, or 16, or error */
        pf = ivalue_unchecked(cadr(sc->args));
        if (ts_is_num(x) && (pf == 16 || pf == 10 || pf == 8 || pf == 2)) {
          /* base is OK */
        } else {
          pf = -1;
        }
      }
      if (pf < 0) {
        Error_1(sc, "atom->string: bad base:", cadr(sc->args));
      } else if (ts_is_num(x) || ts_is_char(x) || ts_is_str(x) ||
                 ts_is_sym(x)) {
        char *p;
        int len;
        atom2str(sc, x, (int)pf, &p, &len);
        s_return(sc, ts_mk_counted_str(sc, p, len));
      } else {
        Error_1(sc, "atom->string: not an atom:", x);
      }
    }

    case OP_MKSTRING: { /* make-string */
      int fill = ' ';
      int len;

      len = ts_int_val(car(sc->args));

      if (cdr(sc->args) != sc->nil) {
        fill = ts_char_val(cadr(sc->args));
      }
      s_return(sc, ts_mk_empty_str(sc, len, (char)fill));
    }

    case OP_STRLEN: /* string-length */
      s_return(sc, ts_mk_int(sc, strlength(car(sc->args))));

    case OP_STRREF: { /* string-ref */
      char *str;
      ts_ptr x;
      int index;

      str = strvalue(car(sc->args));

      x = cadr(sc->args);
      if (ts_is_int(x)) {
        Error_1(sc, "string-ref: index must be exact:", x);
      }

      index = ts_int_val(x);
      if (index >= strlength(car(sc->args))) {
        Error_1(sc, "string-ref: out of bounds:", x);
      }

      s_return(sc, ts_mk_char(sc, ((unsigned char *)str)[index]));
    }

    case OP_STRSET: { /* string-set! */
      char *str;
      ts_ptr x;
      int index;
      char c;

      if (ts_is_immutable(car(sc->args))) {
        Error_1(sc, "string-set!: unable to alter immutable string:",
                car(sc->args));
      }
      str = strvalue(car(sc->args));

      x = cadr(sc->args);
      if (ts_is_int(x)) {
        Error_1(sc, "string-set!: index must be exact:", x);
      }

      index = ts_int_val(x);
      if (index >= strlength(car(sc->args))) {
        Error_1(sc, "string-set!: out of bounds:", x);
      }

      c = ts_char_val(caddr(sc->args));

      str[index] = (char)c;
      s_return(sc, car(sc->args));
    }

    case OP_STRAPPEND: { /* string-append */
      /* in 1.29 string-append was in Scheme in init.scm but was too slow */
      int len = 0;
      ts_ptr newstr;
      char *pos;

      /* compute needed length for new string */
      for (x = sc->args; x != sc->nil; x = cdr(x)) {
        len += strlength(car(x));
      }
      newstr = ts_mk_empty_str(sc, len, ' ');
      /* store the contents of the argument strings into the new string */
      for (pos = strvalue(newstr), x = sc->args; x != sc->nil;
           pos += strlength(car(x)), x = cdr(x)) {
        memcpy(pos, strvalue(car(x)), strlength(car(x)));
      }
      s_return(sc, newstr);
    }

    case OP_SUBSTR: { /* substring */
      char *str;
      int index0;
      int index1;
      int len;

      str = strvalue(car(sc->args));

      index0 = ts_int_val(cadr(sc->args));

      if (index0 > strlength(car(sc->args))) {
        Error_1(sc, "substring: start out of bounds:", cadr(sc->args));
      }

      if (cddr(sc->args) != sc->nil) {
        index1 = ts_int_val(caddr(sc->args));
        if (index1 > strlength(car(sc->args)) || index1 < index0) {
          Error_1(sc, "substring: end out of bounds:", caddr(sc->args));
        }
      } else {
        index1 = strlength(car(sc->args));
      }

      len = index1 - index0;
      x = ts_mk_empty_str(sc, len, ' ');
      memcpy(strvalue(x), str + index0, len);
      strvalue(x)[len] = 0;

      s_return(sc, x);
    }

    case OP_VECTOR: { /* vector */
      int i;
      ts_ptr vec;
      int len = ts_list_len(sc, sc->args);
      if (len < 0) {
        Error_1(sc, "vector: not a proper list:", sc->args);
      }
      vec = ts_mk_vec(sc, len);
      if (sc->no_memory) {
        s_return(sc, sc->sink);
      }
      for (x = sc->args, i = 0; ts_is_pair(x); x = cdr(x), i++) {
        ts_set_vec_elem(vec, i, car(x));
      }
      s_return(sc, vec);
    }

    case OP_MKVECTOR: { /* make-vector */
      ts_ptr fill = sc->nil;
      int len;
      ts_ptr vec;

      len = ts_int_val(car(sc->args));

      if (cdr(sc->args) != sc->nil) {
        fill = cadr(sc->args);
      }
      vec = ts_mk_vec(sc, len);
      if (sc->no_memory) {
        s_return(sc, sc->sink);
      }
      if (fill != sc->nil) {
        ts_fill_vec(vec, fill);
      }
      s_return(sc, vec);
    }

    case OP_VECLEN: /* vector-length */
      s_return(sc, ts_mk_int(sc, ts_int_val(car(sc->args))));

    case OP_VECREF: { /* vector-ref */
      ts_ptr x;
      int index;

      x = cadr(sc->args);
      if (ts_is_int(x)) {
        Error_1(sc, "vector-ref: index must be exact:", x);
      }
      index = ts_int_val(x);

      if (index >= ts_int_val(car(sc->args))) {
        Error_1(sc, "vector-ref: out of bounds:", x);
      }

      s_return(sc, ts_vec_elem(car(sc->args), index));
    }

    case OP_VECSET: { /* vector-set! */
      ts_ptr x;
      int index;

      if (ts_is_immutable(car(sc->args))) {
        Error_1(sc, "vector-set!: unable to alter immutable vector:",
                car(sc->args));
      }

      x = cadr(sc->args);
      if (ts_is_int(x)) {
        Error_1(sc, "vector-set!: index must be exact:", x);
      }

      index = ts_int_val(x);
      if (index >= ts_int_val(car(sc->args))) {
        Error_1(sc, "vector-set!: out of bounds:", x);
      }

      ts_set_vec_elem(car(sc->args), index, caddr(sc->args));
      s_return(sc, car(sc->args));
    }

    default:
      snprintf(sc->strbuff, TS_STRBUFFSIZE, "%d: illegal operator", sc->op);
      Error_0(sc, sc->strbuff);
  }
  return sc->t;
}

bool ts_is_list(ts_interp *sc, ts_ptr a) { return ts_list_len(sc, a) >= 0; }

/* Result is:
   proper list: length
   circular list: -1
   not even a pair: -2
   dotted list: -2 minus length before dot
*/
int ts_list_len(ts_interp *sc, ts_ptr a) {
  int i = 0;
  ts_ptr slow, fast;

  slow = fast = a;
  while (1) {
    if (fast == sc->nil) return i;
    if (!ts_is_pair(fast)) return -2 - i;
    fast = cdr(fast);
    ++i;
    if (fast == sc->nil) return i;
    if (!ts_is_pair(fast)) return -2 - i;
    ++i;
    fast = cdr(fast);

    /* Safe because we would have already returned if `fast'
       encountered a non-pair. */
    slow = cdr(slow);
    if (fast == slow) {
      /* the fast ts_ptr has looped back around and caught up
         with the slow ts_ptr, hence the structure is circular,
         not of finite length, and therefore not a list */
      return -1;
    }
  }
}

static ts_ptr opexe_3(ts_interp *sc, enum opcodes op) {
  ts_ptr x;
  ts_num v;
  int (*comp_func)(ts_num, ts_num) = 0;

  switch (op) {
    case OP_NOT: /* not */
      s_retbool(is_false(car(sc->args)));
    case OP_BOOLP: /* boolean? */
      s_retbool(car(sc->args) == sc->f || car(sc->args) == sc->t);
    case OP_EOFOBJP: /* boolean? */
      s_retbool(car(sc->args) == sc->eof_obj);
    case OP_NULLP: /* null? */
      s_retbool(car(sc->args) == sc->nil);
    case OP_NUMEQ: /* = */
    case OP_LESS:  /* < */
    case OP_GRE:   /* > */
    case OP_LEQ:   /* <= */
    case OP_GEQ:   /* >= */
      switch (op) {
        case OP_NUMEQ:
          comp_func = num_eq;
          break;
        case OP_LESS:
          comp_func = num_lt;
          break;
        case OP_GRE:
          comp_func = num_gt;
          break;
        case OP_LEQ:
          comp_func = num_le;
          break;
        case OP_GEQ:
          comp_func = num_ge;
          break;
        default:
          break; /* Quiet the compiler */
      }
      x = sc->args;
      v = ts_num_val(car(x));
      x = cdr(x);

      for (; x != sc->nil; x = cdr(x)) {
        if (!comp_func(v, ts_num_val(car(x)))) {
          s_retbool(0);
        }
        v = ts_num_val(car(x));
      }
      s_retbool(1);
    case OP_SYMBOLP: /* symbol? */
      s_retbool(ts_is_sym(car(sc->args)));
    case OP_NUMBERP: /* number? */
      s_retbool(ts_is_num(car(sc->args)));
    case OP_STRINGP: /* string? */
      s_retbool(ts_is_str(car(sc->args)));
    case OP_INTEGERP: /* integer? */
      s_retbool(ts_is_int(car(sc->args)));
    case OP_REALP:                         /* real? */
      s_retbool(ts_is_num(car(sc->args))); /* All numbers are real */
    case OP_CHARP:                         /* char? */
      s_retbool(ts_is_char(car(sc->args)));
#if USE_CHAR_CLASSIFIERS
    case OP_CHARAP: /* char-alphabetic? */
      s_retbool(Cisalpha(ts_int_val(car(sc->args))));
    case OP_CHARNP: /* char-numeric? */
      s_retbool(Cisdigit(ts_int_val(car(sc->args))));
    case OP_CHARWP: /* char-whitespace? */
      s_retbool(Cisspace(ts_int_val(car(sc->args))));
    case OP_CHARUP: /* char-upper-case? */
      s_retbool(Cisupper(ts_int_val(car(sc->args))));
    case OP_CHARLP: /* char-lower-case? */
      s_retbool(Cislower(ts_int_val(car(sc->args))));
#endif
    case OP_PORTP: /* port? */
      s_retbool(ts_is_port(car(sc->args)));
    case OP_INPORTP: /* input-port? */
      s_retbool(is_inport(car(sc->args)));
    case OP_OUTPORTP: /* output-port? */
      s_retbool(is_outport(car(sc->args)));
    case OP_PROCP: /* procedure? */
      /*--
       * continuation should be procedure by the example
       * (call-with-current-continuation procedure?) ==> #t
       * in R^3 report sec. 6.9
       */
      s_retbool(ts_is_proc(car(sc->args)) || ts_is_closure(car(sc->args)) ||
                ts_is_continuation(car(sc->args)) ||
                ts_is_foreign(car(sc->args)));
    case OP_PAIRP: /* pair? */
      s_retbool(ts_is_pair(car(sc->args)));
    case OP_LISTP: /* list? */
      s_retbool(ts_list_len(sc, car(sc->args)) >= 0);

    case OP_ENVP: /* environment? */
      s_retbool(ts_is_env(car(sc->args)));
    case OP_VECTORP: /* vector? */
      s_retbool(ts_is_vec(car(sc->args)));
    case OP_EQ: /* eq? */
      s_retbool(car(sc->args) == cadr(sc->args));
    case OP_EQV: /* eqv? */
      s_retbool(ts_eqv(car(sc->args), cadr(sc->args)));
    default:
      snprintf(sc->strbuff, TS_STRBUFFSIZE, "%d: illegal operator", sc->op);
      Error_0(sc, sc->strbuff);
  }
  return sc->t;
}

static ts_ptr opexe_4(ts_interp *sc, enum opcodes op) {
  ts_ptr x, y;

  switch (op) {
    case OP_FORCE: /* force */
      sc->code = car(sc->args);
      if (ts_is_promise(sc->code)) {
        /* Should change type to closure here */
        s_save(sc, OP_SAVE_FORCED, sc->nil, sc->code);
        sc->args = sc->nil;
        s_goto(sc, OP_APPLY);
      } else {
        s_return(sc, sc->code);
      }

    case OP_SAVE_FORCED: /* Save forced value replacing promise */
      memcpy(sc->code, sc->value, sizeof(struct ts_cell));
      s_return(sc, sc->value);

    case OP_WRITE:      /* write */
    case OP_DISPLAY:    /* display */
    case OP_WRITE_CHAR: /* write-char */
      if (ts_is_pair(cdr(sc->args))) {
        if (cadr(sc->args) != sc->outport) {
          x = ts_cons(sc, sc->outport, sc->nil);
          s_save(sc, OP_SET_OUTPORT, x, sc->nil);
          sc->outport = cadr(sc->args);
        }
      }
      sc->args = car(sc->args);
      if (op == OP_WRITE) {
        sc->print_flag = 1;
      } else {
        sc->print_flag = 0;
      }
      s_goto(sc, OP_P0LIST);

    case OP_NEWLINE: /* newline */
      if (ts_is_pair(sc->args)) {
        if (car(sc->args) != sc->outport) {
          x = ts_cons(sc, sc->outport, sc->nil);
          s_save(sc, OP_SET_OUTPORT, x, sc->nil);
          sc->outport = car(sc->args);
        }
      }
      ts_put_str(sc, "\n");
      s_return(sc, sc->t);

    case OP_ERR0: /* error */
      sc->retcode = -1;
      if (!ts_is_str(car(sc->args))) {
        sc->args = ts_cons(sc, ts_mk_str(sc, " -- "), sc->args);
        ts_set_immutable(car(sc->args));
      }
      ts_put_str(sc, "Error: ");
      ts_put_str(sc, strvalue(car(sc->args)));
      sc->args = cdr(sc->args);
      s_goto(sc, OP_ERR1);

    case OP_ERR1: /* error */
      ts_put_str(sc, " ");
      if (sc->args != sc->nil) {
        s_save(sc, OP_ERR1, cdr(sc->args), sc->nil);
        sc->args = car(sc->args);
        sc->print_flag = 1;
        s_goto(sc, OP_P0LIST);
      } else {
        ts_put_str(sc, "\n");
        if (sc->interactive_repl) {
          s_goto(sc, OP_T0LVL);
        } else {
          return sc->nil;
        }
      }

    case OP_REVERSE: /* reverse */
      s_return(sc, reverse(sc, car(sc->args)));

    case OP_LIST_STAR: /* list* */
      s_return(sc, list_star(sc, sc->args));

    case OP_APPEND: /* append */
      x = sc->nil;
      y = sc->args;
      if (y == x) {
        s_return(sc, x);
      }

      /* cdr() in the while condition is not a typo. If car() */
      /* is used (append '() 'a) will return the wrong result.*/
      while (cdr(y) != sc->nil) {
        x = revappend(sc, x, car(y));
        y = cdr(y);
        if (x == sc->f) {
          Error_0(sc, "non-list argument to append");
        }
      }

      s_return(sc, reverse_in_place(sc, car(y), x));

#if USE_PLIST
    case OP_PUT: /* put */
      if (!ts_has_prop(car(sc->args)) || !ts_has_prop(cadr(sc->args))) {
        Error_0(sc, "illegal use of put");
      }
      for (x = symprop(car(sc->args)), y = cadr(sc->args); x != sc->nil;
           x = cdr(x)) {
        if (caar(x) == y) {
          break;
        }
      }
      if (x != sc->nil)
        cdar(x) = caddr(sc->args);
      else
        symprop(car(sc->args)) = ts_cons(sc, ts_cons(sc, y, caddr(sc->args)),
                                         symprop(car(sc->args)));
      s_return(sc, sc->t);

    case OP_GET: /* get */
      if (!ts_has_prop(car(sc->args)) || !ts_has_prop(cadr(sc->args))) {
        Error_0(sc, "illegal use of get");
      }
      for (x = symprop(car(sc->args)), y = cadr(sc->args); x != sc->nil;
           x = cdr(x)) {
        if (caar(x) == y) {
          break;
        }
      }
      if (x != sc->nil) {
        s_return(sc, cdar(x));
      } else {
        s_return(sc, sc->nil);
      }
#endif            /* USE_PLIST */
    case OP_QUIT: /* quit */
      if (ts_is_pair(sc->args)) {
        sc->retcode = ts_int_val(car(sc->args));
      }
      return (sc->nil);

    case OP_GC: /* gc */
      gc(sc, sc->nil, sc->nil);
      s_return(sc, sc->t);

    case OP_GCVERB: /* gc-verbose */
    {
      bool was = sc->gc_verbose;

      sc->gc_verbose = (car(sc->args) != sc->f);
      s_retbool(was);
    }

    case OP_NEWSEGMENT: /* new-segment */
      if (!ts_is_pair(sc->args) || !ts_is_num(car(sc->args))) {
        Error_0(sc, "new-segment: argument must be a number");
      }
      alloc_cellseg(sc, (int)ts_int_val(car(sc->args)));
      s_return(sc, sc->t);

    case OP_OBLIST: /* oblist */
      s_return(sc, oblist_all_symbols(sc));

    case OP_CURR_INPORT: /* current-input-port */
      s_return(sc, sc->inport);

    case OP_CURR_OUTPORT: /* current-output-port */
      s_return(sc, sc->outport);

    case OP_OPEN_INFILE:  /* open-input-file */
    case OP_OPEN_OUTFILE: /* open-output-file */
    case OP_OPEN_INOUTFILE: /* open-input-output-file */ {
      int prop = 0;
      ts_ptr p;
      switch (op) {
        case OP_OPEN_INFILE:
          prop = TS_PORT_INPUT;
          break;
        case OP_OPEN_OUTFILE:
          prop = TS_PORT_OUTPUT;
          break;
        case OP_OPEN_INOUTFILE:
          prop = TS_PORT_INPUT | TS_PORT_OUTPUT;
          break;
        default:
          break; /* Quiet the compiler */
      }
      p = port_from_filename(sc, strvalue(car(sc->args)), prop);
      if (p == sc->nil) {
        s_return(sc, sc->f);
      }
      s_return(sc, p);
    }

#if USE_STRING_PORTS
    case OP_OPEN_INSTRING: /* open-input-string */
    case OP_OPEN_INOUTSTRING: /* open-input-output-string */ {
      int prop = 0;
      ts_ptr p;
      switch (op) {
        case OP_OPEN_INSTRING:
          prop = TS_PORT_INPUT;
          break;
        case OP_OPEN_INOUTSTRING:
          prop = TS_PORT_INPUT | TS_PORT_OUTPUT;
          break;
        default:
          break; /* Quiet the compiler */
      }
      p = port_from_string(sc, strvalue(car(sc->args)),
                           strvalue(car(sc->args)) + strlength(car(sc->args)),
                           prop);
      if (p == sc->nil) {
        s_return(sc, sc->f);
      }
      s_return(sc, p);
    }
    case OP_OPEN_OUTSTRING: /* open-output-string */ {
      ts_ptr p;
      if (car(sc->args) == sc->nil) {
        p = port_from_scratch(sc);
        if (p == sc->nil) {
          s_return(sc, sc->f);
        }
      } else {
        p = port_from_string(sc, strvalue(car(sc->args)),
                             strvalue(car(sc->args)) + strlength(car(sc->args)),
                             TS_PORT_OUTPUT);
        if (p == sc->nil) {
          s_return(sc, sc->f);
        }
      }
      s_return(sc, p);
    }
    case OP_GET_OUTSTRING: /* get-output-string */ {
      ts_port *p;

      if ((p = car(sc->args)->_object._port)->kind & TS_PORT_STRING) {
        off_t size;
        char *str;

        size = p->rep.string.curr - p->rep.string.start + 1;
        str = sc->malloc(size);
        if (str != NULL) {
          ts_ptr s;

          memcpy(str, p->rep.string.start, size - 1);
          str[size - 1] = '\0';
          s = ts_mk_str(sc, str);
          sc->free(str);
          s_return(sc, s);
        }
      }
      s_return(sc, sc->f);
    }
#endif

    case OP_CLOSE_INPORT: /* close-input-port */
      port_close(sc, car(sc->args), TS_PORT_INPUT);
      s_return(sc, sc->t);

    case OP_CLOSE_OUTPORT: /* close-output-port */
      port_close(sc, car(sc->args), TS_PORT_OUTPUT);
      s_return(sc, sc->t);

    case OP_INT_ENV: /* interaction-environment */
      s_return(sc, sc->global_env);

    case OP_CURR_ENV: /* current-environment */
      s_return(sc, sc->envir);

    default:
      snprintf(sc->strbuff, TS_STRBUFFSIZE, "%d: illegal operator", sc->op);
      Error_0(sc, sc->strbuff);
  }
  return sc->t;
}

static ts_ptr opexe_5(ts_interp *sc, enum opcodes op) {
  ts_ptr x;

  if (sc->nesting != 0) {
    int n = sc->nesting;
    sc->nesting = 0;
    sc->retcode = -1;
    Error_1(sc, "unmatched parentheses:", ts_mk_int(sc, n));
  }

  switch (op) {
    /* ========== reading part ========== */
    case OP_READ:
      if (!ts_is_pair(sc->args)) {
        s_goto(sc, OP_READ_INTERNAL);
      }
      if (!is_inport(car(sc->args))) {
        Error_1(sc, "read: not an input port:", car(sc->args));
      }
      if (car(sc->args) == sc->inport) {
        s_goto(sc, OP_READ_INTERNAL);
      }
      x = sc->inport;
      sc->inport = car(sc->args);
      x = ts_cons(sc, x, sc->nil);
      s_save(sc, OP_SET_INPORT, x, sc->nil);
      s_goto(sc, OP_READ_INTERNAL);

    case OP_READ_CHAR: /* read-char */
    case OP_PEEK_CHAR: /* peek-char */ {
      char c;
      if (ts_is_pair(sc->args)) {
        if (car(sc->args) != sc->inport) {
          x = sc->inport;
          x = ts_cons(sc, x, sc->nil);
          s_save(sc, OP_SET_INPORT, x, sc->nil);
          sc->inport = car(sc->args);
        }
      }
      c = inchar(sc);
      if (c == EOF) {
        s_return(sc, sc->eof_obj);
      }
      if (sc->op == OP_PEEK_CHAR) {
        backchar(sc, c);
      }
      s_return(sc, ts_mk_char(sc, c));
    }

    case OP_CHAR_READY: /* char-ready? */ {
      ts_ptr p = sc->inport;
      int res;
      if (ts_is_pair(sc->args)) {
        p = car(sc->args);
      }
      res = p->_object._port->kind & TS_PORT_STRING;
      s_retbool(res);
    }

    case OP_SET_INPORT: /* set-input-port */
      sc->inport = car(sc->args);
      s_return(sc, sc->value);

    case OP_SET_OUTPORT: /* set-output-port */
      sc->outport = car(sc->args);
      s_return(sc, sc->value);

    case OP_RDSEXPR:
      switch (sc->tok) {
        case TOK_EOF:
          s_return(sc, sc->eof_obj);
          /* NOTREACHED */
          /*
           * Commented out because we now skip comments in the scanner
           *
                    case TOK_COMMENT: {
                         char c;
                         while ((c=inchar(sc)) != '\n' && c!=EOF)
                              ;
                         sc->tok = token(sc);
                         s_goto(sc,OP_RDSEXPR);
                    }
          */
        case TOK_VEC:
          s_save(sc, OP_RDVEC, sc->nil, sc->nil);
          /* fall through */
        case TOK_LPAREN:
          sc->tok = token(sc);
          if (sc->tok == TOK_RPAREN) {
            s_return(sc, sc->nil);
          } else if (sc->tok == TOK_DOT) {
            Error_0(sc, "syntax error: illegal dot expression");
          } else {
            sc->nesting_stack[sc->file_i]++;
            s_save(sc, OP_RDLIST, sc->nil, sc->nil);
            s_goto(sc, OP_RDSEXPR);
          }
        case TOK_QUOTE:
          s_save(sc, OP_RDQUOTE, sc->nil, sc->nil);
          sc->tok = token(sc);
          s_goto(sc, OP_RDSEXPR);
        case TOK_BQUOTE:
          sc->tok = token(sc);
          if (sc->tok == TOK_VEC) {
            s_save(sc, OP_RDQQUOTEVEC, sc->nil, sc->nil);
            sc->tok = TOK_LPAREN;
            s_goto(sc, OP_RDSEXPR);
          } else {
            s_save(sc, OP_RDQQUOTE, sc->nil, sc->nil);
          }
          s_goto(sc, OP_RDSEXPR);
        case TOK_COMMA:
          s_save(sc, OP_RDUNQUOTE, sc->nil, sc->nil);
          sc->tok = token(sc);
          s_goto(sc, OP_RDSEXPR);
        case TOK_ATMARK:
          s_save(sc, OP_RDUQTSP, sc->nil, sc->nil);
          sc->tok = token(sc);
          s_goto(sc, OP_RDSEXPR);
        case TOK_ATOM:
          s_return(sc, mk_atom(sc, readstr_upto(sc, DELIMITERS)));
        case TOK_DQUOTE:
          x = readstrexp(sc);
          if (x == sc->f) {
            Error_0(sc, "Error reading string");
          }
          ts_set_immutable(x);
          s_return(sc, x);
        case TOK_SHARP: {
          ts_ptr f = find_slot_in_env(sc, sc->envir, sc->sharp_hook, 1);
          if (f == sc->nil) {
            Error_0(sc, "undefined sharp expression");
          } else {
            sc->code = ts_cons(sc, slot_value_in_env(f), sc->nil);
            s_goto(sc, OP_EVAL);
          }
        }
        case TOK_SHARP_CONST:
          if ((x = mk_sharp_const(sc, readstr_upto(sc, DELIMITERS))) ==
              sc->nil) {
            Error_0(sc, "undefined sharp expression");
          } else {
            s_return(sc, x);
          }
        default:
          Error_0(sc, "syntax error: illegal token");
      }
      break;

    case OP_RDLIST: {
      sc->args = ts_cons(sc, sc->value, sc->args);
      sc->tok = token(sc);
      /* We now skip comments in the scanner
                while (sc->tok == TOK_COMMENT) {
                     char c;
                     while ((c=inchar(sc)) != '\n' && c!=EOF)
                          ;
                     sc->tok = token(sc);
                }
      */
      if (sc->tok == TOK_EOF) {
        s_return(sc, sc->eof_obj);
      } else if (sc->tok == TOK_RPAREN) {
        char c = inchar(sc);
        if (c != '\n') backchar(sc, c);
#if SHOW_ERROR_LINE
        else if (sc->load_stack[sc->file_i].kind & TS_PORT_FILE)
          sc->load_stack[sc->file_i].rep.stdio.curr_line++;
#endif
        sc->nesting_stack[sc->file_i]--;
        s_return(sc, reverse_in_place(sc, sc->nil, sc->args));
      } else if (sc->tok == TOK_DOT) {
        s_save(sc, OP_RDDOT, sc->args, sc->nil);
        sc->tok = token(sc);
        s_goto(sc, OP_RDSEXPR);
      } else {
        s_save(sc, OP_RDLIST, sc->args, sc->nil);
        ;
        s_goto(sc, OP_RDSEXPR);
      }
    }

    case OP_RDDOT:
      if (token(sc) != TOK_RPAREN) {
        Error_0(sc, "syntax error: illegal dot expression");
      } else {
        sc->nesting_stack[sc->file_i]--;
        s_return(sc, reverse_in_place(sc, sc->value, sc->args));
      }

    case OP_RDQUOTE:
      s_return(sc, ts_cons(sc, sc->quote, ts_cons(sc, sc->value, sc->nil)));

    case OP_RDQQUOTE:
      s_return(sc, ts_cons(sc, sc->qquote, ts_cons(sc, sc->value, sc->nil)));

    case OP_RDQQUOTEVEC:
      s_return(sc,
               ts_cons(sc, ts_mk_sym(sc, "apply"),
                       ts_cons(sc, ts_mk_sym(sc, "vector"),
                               ts_cons(sc,
                                       ts_cons(sc, sc->qquote,
                                               ts_cons(sc, sc->value, sc->nil)),
                                       sc->nil))));

    case OP_RDUNQUOTE:
      s_return(sc, ts_cons(sc, sc->unquote, ts_cons(sc, sc->value, sc->nil)));

    case OP_RDUQTSP:
      s_return(sc, ts_cons(sc, sc->unquotesp, ts_cons(sc, sc->value, sc->nil)));

    case OP_RDVEC:
      /*sc->code=ts_cons(sc,mk_proc(sc,OP_VECTOR),sc->value);
      s_goto(sc,OP_EVAL); Cannot be quoted*/
      /*x=ts_cons(sc,mk_proc(sc,OP_VECTOR),sc->value);
      s_return(sc,x); Cannot be part of pairs*/
      /*sc->code=mk_proc(sc,OP_VECTOR);
      sc->args=sc->value;
      s_goto(sc,OP_APPLY);*/
      sc->args = sc->value;
      s_goto(sc, OP_VECTOR);

    /* ========== printing part ========== */
    case OP_P0LIST:
      if (ts_is_vec(sc->args)) {
        ts_put_str(sc, "#(");
        sc->args = ts_cons(sc, sc->args, ts_mk_int(sc, 0));
        s_goto(sc, OP_PVECFROM);
      } else if (ts_is_env(sc->args)) {
        ts_put_str(sc, "#<ENVIRONMENT>");
        s_return(sc, sc->t);
      } else if (!ts_is_pair(sc->args)) {
        printatom(sc, sc->args, sc->print_flag);
        s_return(sc, sc->t);
      } else if (car(sc->args) == sc->quote && ok_abbrev(cdr(sc->args))) {
        ts_put_str(sc, "'");
        sc->args = cadr(sc->args);
        s_goto(sc, OP_P0LIST);
      } else if (car(sc->args) == sc->qquote && ok_abbrev(cdr(sc->args))) {
        ts_put_str(sc, "`");
        sc->args = cadr(sc->args);
        s_goto(sc, OP_P0LIST);
      } else if (car(sc->args) == sc->unquote && ok_abbrev(cdr(sc->args))) {
        ts_put_str(sc, ",");
        sc->args = cadr(sc->args);
        s_goto(sc, OP_P0LIST);
      } else if (car(sc->args) == sc->unquotesp && ok_abbrev(cdr(sc->args))) {
        ts_put_str(sc, ",@");
        sc->args = cadr(sc->args);
        s_goto(sc, OP_P0LIST);
      } else {
        ts_put_str(sc, "(");
        s_save(sc, OP_P1LIST, cdr(sc->args), sc->nil);
        sc->args = car(sc->args);
        s_goto(sc, OP_P0LIST);
      }

    case OP_P1LIST:
      if (ts_is_pair(sc->args)) {
        s_save(sc, OP_P1LIST, cdr(sc->args), sc->nil);
        ts_put_str(sc, " ");
        sc->args = car(sc->args);
        s_goto(sc, OP_P0LIST);
      } else if (ts_is_vec(sc->args)) {
        s_save(sc, OP_P1LIST, sc->nil, sc->nil);
        ts_put_str(sc, " . ");
        s_goto(sc, OP_P0LIST);
      } else {
        if (sc->args != sc->nil) {
          ts_put_str(sc, " . ");
          printatom(sc, sc->args, sc->print_flag);
        }
        ts_put_str(sc, ")");
        s_return(sc, sc->t);
      }
    case OP_PVECFROM: {
      int i = ivalue_unchecked(cdr(sc->args));
      ts_ptr vec = car(sc->args);
      int len = ivalue_unchecked(vec);
      if (i == len) {
        ts_put_str(sc, ")");
        s_return(sc, sc->t);
      } else {
        ts_ptr elem = ts_vec_elem(vec, i);
        ivalue_unchecked(cdr(sc->args)) = i + 1;
        s_save(sc, OP_PVECFROM, sc->args, sc->nil);
        sc->args = elem;
        if (i > 0) ts_put_str(sc, " ");
        s_goto(sc, OP_P0LIST);
      }
    }

    default:
      snprintf(sc->strbuff, TS_STRBUFFSIZE, "%d: illegal operator", sc->op);
      Error_0(sc, sc->strbuff);
  }
  return sc->t;
}

static ts_ptr opexe_6(ts_interp *sc, enum opcodes op) {
  ts_ptr x, y;
  int v;

  switch (op) {
    case OP_LIST_LENGTH: /* length */ /* a.k */
      v = ts_list_len(sc, car(sc->args));
      if (v < 0) {
        Error_1(sc, "length: not a list:", car(sc->args));
      }
      s_return(sc, ts_mk_int(sc, v));

    case OP_ASSQ: /* assq */ /* a.k */
      x = car(sc->args);
      for (y = cadr(sc->args); ts_is_pair(y); y = cdr(y)) {
        if (!ts_is_pair(car(y))) {
          Error_0(sc, "unable to handle non pair element");
        }
        if (x == caar(y)) break;
      }
      if (ts_is_pair(y)) {
        s_return(sc, car(y));
      } else {
        s_return(sc, sc->f);
      }

    case OP_GET_CLOSURE: /* get-closure-code */ /* a.k */
      sc->args = car(sc->args);
      if (sc->args == sc->nil) {
        s_return(sc, sc->f);
      } else if (ts_is_closure(sc->args)) {
        s_return(sc, ts_cons(sc, sc->lambda, ts_closure_code(sc->value)));
      } else if (ts_is_macro(sc->args)) {
        s_return(sc, ts_cons(sc, sc->lambda, ts_closure_code(sc->value)));
      } else {
        s_return(sc, sc->f);
      }
    case OP_CLOSUREP: /* closure? */
      /*
       * Note, macro object is also a closure.
       * Therefore, (closure? <#MACRO>) ==> #t
       */
      s_retbool(ts_is_closure(car(sc->args)));
    case OP_MACROP: /* macro? */
      s_retbool(ts_is_macro(car(sc->args)));
    default:
      snprintf(sc->strbuff, TS_STRBUFFSIZE, "%d: illegal operator", sc->op);
      Error_0(sc, sc->strbuff);
  }
  return sc->t; /* NOTREACHED */
}

typedef ts_ptr (*dispatch_func)(ts_interp *, enum opcodes);

typedef bool (*test_predicate)(ts_ptr);
static bool is_any(ts_ptr p) { return true; }

static bool is_nonneg(ts_ptr p) { return ts_int_val(p) >= 0 && ts_is_int(p); }

/* Correspond carefully with following defines! */
static struct {
  test_predicate fct;
  const char *kind;
} tests[] = {{0, 0}, /* unused */
             {is_any, 0},
             {ts_is_str, "string"},
             {ts_is_sym, "symbol"},
             {ts_is_port, "port"},
             {is_inport, "input port"},
             {is_outport, "output port"},
             {ts_is_env, "environment"},
             {ts_is_pair, "pair"},
             {0, "pair or '()"},
             {ts_is_char, "character"},
             {ts_is_vec, "vector"},
             {ts_is_num, "number"},
             {ts_is_int, "integer"},
             {is_nonneg, "non-negative integer"}};

#define TST_NONE 0
#define TST_ANY "\001"
#define TST_STRING "\002"
#define TST_SYMBOL "\003"
#define TST_PORT "\004"
#define TST_INPORT "\005"
#define TST_OUTPORT "\006"
#define TST_ENVIRONMENT "\007"
#define TST_PAIR "\010"
#define TST_LIST "\011"
#define TST_CHAR "\012"
#define TST_VECTOR "\013"
#define TST_NUMBER "\014"
#define TST_INTEGER "\015"
#define TST_NATURAL "\016"

typedef struct {
  dispatch_func func;
  char *name;
  int min_arity;
  int max_arity;
  char *arg_tests_encoding;
} op_code_info;

#define INF_ARG 0xffff

static op_code_info dispatch_table[] = {
#define _OP_DEF(A, B, C, D, E, OP) {A, B, C, D, E},
#include "opdefines.h"
    {0}};

static const char *procname(ts_ptr x) {
  int n = procnum(x);
  const char *name = dispatch_table[n].name;
  if (name == 0) {
    name = "ILLEGAL!";
  }
  return name;
}

/* kernel of this interpreter */
void Eval_Cycle(ts_interp *sc, enum opcodes op) {
  sc->op = op;
  for (;;) {
    op_code_info *pcd = dispatch_table + sc->op;
    if (pcd->name != 0) { /* if built-in function, check arguments */
      char msg[TS_STRBUFFSIZE];
      int ok = 1;
      int n = ts_list_len(sc, sc->args);

      /* Check number of arguments */
      if (n < pcd->min_arity) {
        ok = 0;
        snprintf(msg, TS_STRBUFFSIZE, "%s: needs%s %d argument(s)", pcd->name,
                 pcd->min_arity == pcd->max_arity ? "" : " at least",
                 pcd->min_arity);
      }
      if (ok && n > pcd->max_arity) {
        ok = 0;
        snprintf(msg, TS_STRBUFFSIZE, "%s: needs%s %d argument(s)", pcd->name,
                 pcd->min_arity == pcd->max_arity ? "" : " at most",
                 pcd->max_arity);
      }
      if (ok) {
        if (pcd->arg_tests_encoding != 0) {
          int i = 0;
          int j;
          const char *t = pcd->arg_tests_encoding;
          ts_ptr arglist = sc->args;
          do {
            ts_ptr arg = car(arglist);
            j = (int)t[0];
            if (j == TST_LIST[0]) {
              if (arg != sc->nil && !ts_is_pair(arg)) break;
            } else {
              if (!tests[j].fct(arg)) break;
            }

            if (t[1] != 0) { /* last test is replicated as necessary */
              t++;
            }
            arglist = cdr(arglist);
            i++;
          } while (i < n);
          if (i < n) {
            ok = 0;
            snprintf(msg, TS_STRBUFFSIZE, "%s: argument %d must be: %s",
                     pcd->name, i + 1, tests[j].kind);
          }
        }
      }
      if (!ok) {
        if (_Error_1(sc, msg, 0) == sc->nil) {
          return;
        }
        pcd = dispatch_table + sc->op;
      }
    }
    ok_to_freely_gc(sc);
    if (pcd->func(sc, (enum opcodes)sc->op) == sc->nil) {
      return;
    }
    if (sc->no_memory) {
      fprintf(stderr, "No memory!\n");
      return;
    }
  }
}

/* ========== Initialization of internal keywords ========== */

static void assign_syntax(ts_interp *sc, char *name) {
  ts_ptr x;

  x = oblist_add_by_name(sc, name);
  typeflag(x) |= T_SYNTAX;
}

static void assign_proc(ts_interp *sc, enum opcodes op, char *name) {
  ts_ptr x, y;

  x = ts_mk_sym(sc, name);
  y = mk_proc(sc, op);
  new_slot_in_env(sc, x, y);
}

static ts_ptr mk_proc(ts_interp *sc, enum opcodes op) {
  ts_ptr y;

  y = get_cell(sc, sc->nil, sc->nil);
  typeflag(y) = (T_PROC | T_ATOM);
  ivalue_unchecked(y) = (int)op;
  set_num_integer(y);
  return y;
}

/* Hard-coded for the given keywords. Remember to rewrite if more are added! */
static int syntaxnum(ts_ptr p) {
  const char *s = strvalue(car(p));
  switch (strlength(car(p))) {
    case 2:
      if (s[0] == 'i')
        return OP_IF0; /* if */
      else
        return OP_OR0; /* or */
    case 3:
      if (s[0] == 'a')
        return OP_AND0; /* and */
      else
        return OP_LET0; /* let */
    case 4:
      switch (s[3]) {
        case 'e':
          return OP_CASE0; /* case */
        case 'd':
          return OP_COND0; /* cond */
        case '*':
          return OP_LET0AST; /* let* */
        default:
          return OP_SET0; /* set! */
      }
    case 5:
      switch (s[2]) {
        case 'g':
          return OP_BEGIN; /* begin */
        case 'l':
          return OP_DELAY; /* delay */
        case 'c':
          return OP_MACRO0; /* macro */
        default:
          return OP_QUOTE; /* quote */
      }
    case 6:
      switch (s[2]) {
        case 'm':
          return OP_LAMBDA; /* lambda */
        case 'f':
          return OP_DEF0; /* define */
        default:
          return OP_LET0REC; /* letrec */
      }
    default:
      return OP_C0STREAM; /* cons-stream */
  }
}

/* initialization of TinyScheme */
#if USE_INTERFACE
static struct ts_interface vtbl = {
    ts_def,
    ts_cons,
    ts_immutable_cons,
    ts_reserve_cells,
    ts_mk_int,
    ts_mk_real,
    ts_mk_sym,
    ts_gen_sym,
    ts_mk_str,
    ts_mk_counted_str,
    ts_mk_char,
    ts_mk_vec,
    ts_mk_foreign_func,
    ts_put_str,
    ts_put_char,

    ts_is_str,
    ts_str_val,
    ts_is_num,
    ts_num_val,
    ts_int_val,
    ts_real_val,
    ts_is_int,
    ts_is_real,
    ts_is_char,
    ts_char_val,
    ts_is_list,
    ts_is_vec,
    ts_list_len,
    ts_fill_vec,
    ts_vec_elem,
    ts_set_vec_elem,
    ts_is_port,
    ts_is_pair,
    ts_pair_car,
    ts_pair_cdr,
    ts_set_car,
    ts_set_cdr,

    ts_is_sym,
    ts_sym_name,

    ts_is_syntax,
    ts_is_proc,
    ts_is_foreign,
    ts_syntax_name,
    ts_is_closure,
    ts_is_macro,
    ts_closure_code,
    ts_closure_env,

    ts_is_continuation,
    ts_is_promise,
    ts_is_env,
    ts_is_immutable,
    ts_set_immutable,

    ts_load_str,
    ts_deinit,
    ts_set_in_port_file,
    ts_set_in_port_str,
    ts_set_out_port_file,
    ts_set_out_port_str,
    ts_set_extern_data,
    ts_eqv,
    ts_mk_empty_str,
#if USE_DL
    ts_load_ext,
#else
    NULL,
#endif

#if USE_PLIST
    ts_has_prop,
#else
    NULL,
#endif

#if !STANDALONE
    ts_load_file,
    ts_apply0,
    ts_call,
    ts_eval,
    ts_vec_len,
    ts_get_global,
    ts_register_foreign_func_list,
    ts_mk_userdata,
    ts_is_userdata,
    ts_userdata_set_finalizer,
    ts_mk_bool,
#else
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#endif
};
#endif

ts_interp *ts_init_new() {
  ts_interp *sc = (ts_interp *)malloc(sizeof(ts_interp));
  if (!ts_init(sc)) {
    free(sc);
    return 0;
  } else {
    return sc;
  }
}

ts_interp *ts_init_new_custom_alloc(ts_func_alloc malloc, ts_func_dealloc free) {
  ts_interp *sc = (ts_interp *)malloc(sizeof(ts_interp));
  if (!ts_init_custom_alloc(sc, malloc, free)) {
    free(sc);
    return 0;
  } else {
    return sc;
  }
}

int ts_init(ts_interp *sc) { return ts_init_custom_alloc(sc, malloc, free); }

bool ts_init_custom_alloc(ts_interp *sc, ts_func_alloc malloc,
                          ts_func_dealloc free) {
  int i, n = sizeof(dispatch_table) / sizeof(dispatch_table[0]);
  ts_ptr x;

  num_zero.is_fixnum = 1;
  num_zero.value.ivalue = 0;
  num_one.is_fixnum = 1;
  num_one.value.ivalue = 1;

#if USE_INTERFACE
  sc->vptr = &vtbl;
#endif
  sc->gensym_cnt = 0;
  sc->malloc = malloc;
  sc->free = free;
  sc->last_cell_seg = -1;
  sc->sink = &sc->_sink;
  sc->nil = &sc->_nil;
  sc->t = &sc->_hasht;
  sc->f = &sc->_hashf;
  sc->eof_obj = &sc->_eof_obj;
  sc->free_cell = &sc->_nil;
  sc->fcells = 0;
  sc->no_memory = false;
  sc->inport = sc->nil;
  sc->outport = sc->nil;
  sc->save_inport = sc->nil;
  sc->loadport = sc->nil;
  sc->nesting = 0;
  sc->interactive_repl = false;

  if (alloc_cellseg(sc, FIRST_CELLSEGS) != FIRST_CELLSEGS) {
    sc->no_memory = true;
    return false;
  }
  sc->gc_verbose = 0;
  dump_stack_initialize(sc);
  sc->code = sc->nil;
  sc->tracing = 0;

  /* init sc->nil */
  typeflag(sc->nil) = (T_ATOM | MARK);
  car(sc->nil) = cdr(sc->nil) = sc->nil;
  /* init T */
  typeflag(sc->t) = (T_ATOM | MARK);
  car(sc->t) = cdr(sc->t) = sc->t;
  /* init F */
  typeflag(sc->f) = (T_ATOM | MARK);
  car(sc->f) = cdr(sc->f) = sc->f;
  /* init sink */
  typeflag(sc->sink) = (T_PAIR | MARK);
  car(sc->sink) = sc->nil;
  /* init c_nest */
  sc->c_nest = sc->nil;

  sc->oblist = oblist_initial_value(sc);
  /* init global_env */
  new_frame_in_env(sc, sc->nil);
  sc->global_env = sc->envir;
  /* init else */
  x = ts_mk_sym(sc, "else");
  new_slot_in_env(sc, x, sc->t);

  assign_syntax(sc, "lambda");
  assign_syntax(sc, "quote");
  assign_syntax(sc, "define");
  assign_syntax(sc, "if");
  assign_syntax(sc, "begin");
  assign_syntax(sc, "set!");
  assign_syntax(sc, "let");
  assign_syntax(sc, "let*");
  assign_syntax(sc, "letrec");
  assign_syntax(sc, "cond");
  assign_syntax(sc, "delay");
  assign_syntax(sc, "and");
  assign_syntax(sc, "or");
  assign_syntax(sc, "cons-stream");
  assign_syntax(sc, "macro");
  assign_syntax(sc, "case");

  for (i = 0; i < n; i++) {
    if (dispatch_table[i].name != 0) {
      assign_proc(sc, (enum opcodes)i, dispatch_table[i].name);
    }
  }

  /* initialization of global pointers to special symbols */
  sc->lambda = ts_mk_sym(sc, "lambda");
  sc->quote = ts_mk_sym(sc, "quote");
  sc->qquote = ts_mk_sym(sc, "quasiquote");
  sc->unquote = ts_mk_sym(sc, "unquote");
  sc->unquotesp = ts_mk_sym(sc, "unquote-splicing");
  sc->feed_to = ts_mk_sym(sc, "=>");
  sc->colon_hook = ts_mk_sym(sc, "*colon-hook*");
  sc->error_hook = ts_mk_sym(sc, "*error-hook*");
  sc->sharp_hook = ts_mk_sym(sc, "*sharp-hook*");
  sc->compile_hook = ts_mk_sym(sc, "*compile-hook*");

  return !sc->no_memory;
}

void ts_set_in_port_file(ts_interp *sc, FILE *fin) {
  sc->inport = port_from_file(sc, fin, TS_PORT_INPUT);
}

void ts_set_in_port_str(ts_interp *sc, char *start, char *past_the_end) {
  sc->inport = port_from_string(sc, start, past_the_end, TS_PORT_INPUT);
}

void ts_set_out_port_file(ts_interp *sc, FILE *fout) {
  sc->outport = port_from_file(sc, fout, TS_PORT_OUTPUT);
}

void ts_set_out_port_str(ts_interp *sc, char *start, char *past_the_end) {
  sc->outport = port_from_string(sc, start, past_the_end, TS_PORT_OUTPUT);
}

void ts_set_extern_data(ts_interp *sc, void *p) { sc->ext_data = p; }

void ts_deinit(ts_interp *sc) {
  int i;

#if SHOW_ERROR_LINE
  char *fname;
#endif

  sc->oblist = sc->nil;
  sc->global_env = sc->nil;
  dump_stack_free(sc);
  sc->envir = sc->nil;
  sc->code = sc->nil;
  sc->args = sc->nil;
  sc->value = sc->nil;
  if (ts_is_port(sc->inport)) {
    typeflag(sc->inport) = T_ATOM;
  }
  sc->inport = sc->nil;
  sc->outport = sc->nil;
  if (ts_is_port(sc->save_inport)) {
    typeflag(sc->save_inport) = T_ATOM;
  }
  sc->save_inport = sc->nil;
  if (ts_is_port(sc->loadport)) {
    typeflag(sc->loadport) = T_ATOM;
  }
  sc->loadport = sc->nil;
  sc->gc_verbose = false;
  gc(sc, sc->nil, sc->nil);

  for (i = 0; i <= sc->last_cell_seg; i++) {
    sc->free(sc->alloc_seg[i]);
  }

#if SHOW_ERROR_LINE
  for (i = 0; i <= sc->file_i; i++) {
    if (sc->load_stack[i].kind & TS_PORT_FILE) {
      fname = sc->load_stack[i].rep.stdio.filename;
      if (fname) sc->free(fname);
    }
  }
#endif
}

bool is_file_exist(const char *fname) {
  FILE *file = fopen(fname, "r");

  if (file) {
    fclose(file);

    return true;
  }

  return false;
}

void load_file(ts_interp *sc, FILE *fin) { load_named_file(sc, fin, 0); }

void load_named_file(ts_interp *sc, FILE *fin, const char *filename) {
  if (fin == NULL) {
    fprintf(stderr, "File pointer can not be NULL when loading a file\n");
    return;
  }
  dump_stack_reset(sc);
  sc->envir = sc->global_env;
  sc->file_i = 0;
  sc->load_stack[0].kind = TS_PORT_INPUT | TS_PORT_FILE;
  sc->load_stack[0].rep.stdio.file = fin;
  sc->loadport = mk_port(sc, sc->load_stack);
  sc->retcode = 0;
  if (fin == stdin) {
    sc->interactive_repl = true;
  }

#if SHOW_ERROR_LINE
  sc->load_stack[0].rep.stdio.curr_line = 0;
  if (fin != stdin && filename)
    sc->load_stack[0].rep.stdio.filename =
        store_string(sc, strlen(filename), filename, 0);
  else
    sc->load_stack[0].rep.stdio.filename = NULL;
#endif

  sc->inport = sc->loadport;
  sc->args = ts_mk_int(sc, sc->file_i);
  Eval_Cycle(sc, OP_T0LVL);
  typeflag(sc->loadport) = T_ATOM;
  if (sc->retcode == 0) {
    sc->retcode = sc->nesting != 0;
  }
}

void ts_load_str(ts_interp *sc, const char *cmd) {
  dump_stack_reset(sc);
  sc->envir = sc->global_env;
  sc->file_i = 0;
  sc->load_stack[0].kind = TS_PORT_INPUT | TS_PORT_STRING;
  sc->load_stack[0].rep.string.start =
      (char *)cmd; /* This func respects const */
  sc->load_stack[0].rep.string.past_the_end = (char *)cmd + strlen(cmd);
  sc->load_stack[0].rep.string.curr = (char *)cmd;
  sc->loadport = mk_port(sc, sc->load_stack);
  sc->retcode = 0;
  sc->interactive_repl = false;
  sc->inport = sc->loadport;
  sc->args = ts_mk_int(sc, sc->file_i);
  Eval_Cycle(sc, OP_T0LVL);
  typeflag(sc->loadport) = T_ATOM;
  if (sc->retcode == 0) {
    sc->retcode = sc->nesting != 0;
  }
}

void ts_def(ts_interp *sc, ts_ptr envir, ts_ptr symbol, ts_ptr value) {
  ts_ptr x;

  x = find_slot_in_env(sc, envir, symbol, 0);
  if (x != sc->nil) {
    set_slot_in_env(sc, x, value);
  } else {
    new_slot_spec_in_env(sc, envir, symbol, value);
  }
}
