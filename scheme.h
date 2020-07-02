/* SCHEME.H */

#ifndef _SCHEME_H
#define _SCHEME_H

#include <stdio.h>

#ifndef _MSC_VER
# define TS_EXPORT
#else
# ifdef _SCHEME_SOURCE
#  define TS_EXPORT __declspec(dllexport)
# else
#  define TS_EXPORT __declspec(dllimport)
# endif
#endif

typedef struct scheme scheme;
typedef struct cell *pointer;

typedef void * (*func_alloc)(size_t);
typedef void (*func_dealloc)(void *);
typedef pointer (*foreign_func)(scheme *, pointer);

/* num, for generic arithmetic */
typedef struct num {
     char is_fixnum;
     union {
          long ivalue;
          double rvalue;
     } value;
} num;

enum scheme_port_kind {
  port_free=0,
  port_file=1,
  port_string=2,
  port_srfi6=4,
  port_input=16,
  port_output=32,
  port_saw_EOF=64
};

typedef struct port {
  unsigned char kind;
  union {
    struct {
      FILE *file;
      int closeit;
      int curr_line;
      char *filename;
    } stdio;
    struct {
      char *start;
      char *past_the_end;
      char *curr;
    } string;
  } rep;
} port;

/* cell structure */
struct cell {
  unsigned int _flag;
  union {
    struct {
      char   *_svalue;
      int   _length;
    } _string;
    num _number;
    port *_port;
    foreign_func _ff;
    struct {
      struct cell *_car;
      struct cell *_cdr;
    } _cons;
  } _object;
};

struct scheme {
/* arrays for segments */
func_alloc malloc;
func_dealloc free;

/* return code */
int retcode;
int tracing;


#ifndef TS_CELL_SEGSIZE
#define TS_CELL_SEGSIZE    5000  /* # of cells in one segment */
#endif
#ifndef TS_CELL_NSEGMENT
#define TS_CELL_NSEGMENT   10    /* # of segments for cells */
#endif
char *alloc_seg[TS_CELL_NSEGMENT];
pointer cell_seg[TS_CELL_NSEGMENT];
int     last_cell_seg;

/* We use 4 registers. */
pointer args;            /* register for arguments of function */
pointer envir;           /* stack register for current environment */
pointer code;            /* register for current code */
pointer dump;            /* stack register for next evaluation */

int interactive_repl;    /* are we in an interactive REPL? */

struct cell _sink;
pointer sink;            /* when mem. alloc. fails */
struct cell _NIL;
pointer NIL;             /* special cell representing empty cell */
struct cell _HASHT;
pointer T;               /* special cell representing #t */
struct cell _HASHF;
pointer F;               /* special cell representing #f */
struct cell _EOF_OBJ;
pointer EOF_OBJ;         /* special cell representing end-of-file object */
pointer oblist;          /* pointer to symbol table */
pointer global_env;      /* pointer to global environment */
pointer c_nest;          /* stack for nested calls from C */

/* global pointers to special symbols */
pointer LAMBDA;               /* pointer to syntax lambda */
pointer QUOTE;           /* pointer to syntax quote */

pointer QQUOTE;               /* pointer to symbol quasiquote */
pointer UNQUOTE;         /* pointer to symbol unquote */
pointer UNQUOTESP;       /* pointer to symbol unquote-splicing */
pointer FEED_TO;         /* => */
pointer COLON_HOOK;      /* *colon-hook* */
pointer ERROR_HOOK;      /* *error-hook* */
pointer SHARP_HOOK;  /* *sharp-hook* */
pointer COMPILE_HOOK;  /* *compile-hook* */

pointer free_cell;       /* pointer to top of free cells */
long    fcells;          /* # of free cells */

pointer inport;
pointer outport;
pointer save_inport;
pointer loadport;

#ifndef TS_MAXFIL
#define TS_MAXFIL 64
#endif
port load_stack[TS_MAXFIL];     /* Stack of open files for port -1 (LOADing) */
int nesting_stack[TS_MAXFIL];
int file_i;
int nesting;

char    gc_verbose;      /* if gc_verbose is not zero, print gc status */
char    no_memory;       /* Whether mem. alloc. has failed */

#ifndef TS_LINESIZE
#define TS_LINESIZE 1024
#endif
char    linebuff[TS_LINESIZE];
#ifndef TS_STRBUFFSIZE
#define TS_STRBUFFSIZE 256
#endif
char    strbuff[TS_STRBUFFSIZE];

FILE *tmpfp;
int tok;
int print_flag;
pointer value;
int op;

void *ext_data;     /* For the benefit of foreign functions */
long gensym_cnt;

struct scheme_interface *vptr;
void *dump_base;    /* pointer to base of allocated dump stack */
int dump_size;      /* number of frames allocated for dump stack */
};

TS_EXPORT scheme *ts_init_new(void);
TS_EXPORT scheme *ts_init_new_custom_alloc(func_alloc malloc, func_dealloc free);
TS_EXPORT int ts_init(scheme *sc);
TS_EXPORT int ts_init_custom_alloc(scheme *sc, func_alloc, func_dealloc);
TS_EXPORT void ts_deinit(scheme *sc);
TS_EXPORT void ts_set_in_port_file(scheme *sc, FILE *fin);
TS_EXPORT void ts_set_in_port_str(scheme *sc, char *start, char *past_the_end);
TS_EXPORT void ts_set_out_port_file(scheme *sc, FILE *fin);
TS_EXPORT void ts_set_out_port_str(scheme *sc, char *start, char *past_the_end);
TS_EXPORT void ts_load_file(scheme *sc, FILE *fin);
TS_EXPORT void ts_load_named_file(scheme *sc, FILE *fin, const char *filename);
TS_EXPORT void ts_load_str(scheme *sc, const char *cmd);
TS_EXPORT pointer ts_apply0(scheme *sc, const char *procname);
TS_EXPORT pointer ts_call(scheme *sc, pointer func, pointer args);
TS_EXPORT pointer ts_eval(scheme *sc, pointer obj);
TS_EXPORT void ts_set_extern_data(scheme *sc, void *p);
TS_EXPORT void ts_def(scheme *sc, pointer env, pointer symbol, pointer value);

TS_EXPORT pointer ts_cons(scheme *sc, pointer a, pointer b, int immutable);
TS_EXPORT pointer ts_mk_int(scheme *sc, long num);
TS_EXPORT pointer ts_mk_real(scheme *sc, double num);
TS_EXPORT pointer ts_mk_sym(scheme *sc, const char *name);
TS_EXPORT pointer ts_gen_sym(scheme *sc);
TS_EXPORT pointer ts_mk_str(scheme *sc, const char *str);
TS_EXPORT pointer ts_mk_counted_str(scheme *sc, const char *str, int len);
TS_EXPORT pointer ts_mk_empty_str(scheme *sc, int len, char fill);
TS_EXPORT pointer ts_mk_char(scheme *sc, int c);
TS_EXPORT pointer ts_mk_foreign_func(scheme *sc, foreign_func f);
TS_EXPORT void ts_put_str(scheme *sc, const char *s);
TS_EXPORT int ts_list_len(scheme *sc, pointer a);
TS_EXPORT int ts_eqv(pointer a, pointer b);

TS_EXPORT int ts_is_str(pointer p);
TS_EXPORT char *ts_str_val(pointer p);
TS_EXPORT int ts_is_num(pointer p);
TS_EXPORT num ts_num_val(pointer p);
TS_EXPORT long ts_int_val(pointer p);
TS_EXPORT double ts_real_val(pointer p);
TS_EXPORT int ts_is_int(pointer p);
TS_EXPORT int ts_is_real(pointer p);
TS_EXPORT int ts_is_char(pointer p);
TS_EXPORT long ts_char_val(pointer p);
TS_EXPORT int ts_is_vec(pointer p);

TS_EXPORT int ts_is_port(pointer p);

TS_EXPORT int ts_is_pair(pointer p);
TS_EXPORT pointer ts_pair_car(pointer p);
TS_EXPORT pointer ts_pair_cdr(pointer p);
TS_EXPORT pointer ts_set_car(pointer p, pointer q);
TS_EXPORT pointer ts_set_cdr(pointer p, pointer q);

TS_EXPORT int ts_is_sym(pointer p);
TS_EXPORT char *ts_sym_name(pointer p);
TS_EXPORT int hasprop(pointer p);

TS_EXPORT int ts_is_syntax(pointer p);
TS_EXPORT int ts_is_proc(pointer p);
TS_EXPORT int ts_is_foreign(pointer p);
TS_EXPORT char *ts_syntax_name(pointer p);
TS_EXPORT int ts_is_closure(pointer p);
TS_EXPORT int ts_is_macro(pointer p);
TS_EXPORT pointer ts_closure_code(pointer p);
TS_EXPORT pointer ts_closure_env(pointer p);

TS_EXPORT int is_continuation(pointer p);
TS_EXPORT int ts_is_promise(pointer p);
TS_EXPORT int ts_is_env(pointer p);
TS_EXPORT int ts_is_immutable(pointer p);
TS_EXPORT void ts_set_immutable(pointer p);

struct scheme_interface {
  void (*scheme_define)(scheme *sc, pointer env, pointer symbol, pointer value);
  pointer (*cons)(scheme *sc, pointer a, pointer b);
  pointer (*immutable_cons)(scheme *sc, pointer a, pointer b);
  pointer (*reserve_cells)(scheme *sc, int n);
  pointer (*mk_integer)(scheme *sc, long num);
  pointer (*mk_real)(scheme *sc, double num);
  pointer (*mk_symbol)(scheme *sc, const char *name);
  pointer (*gensym)(scheme *sc);
  pointer (*mk_string)(scheme *sc, const char *str);
  pointer (*mk_counted_string)(scheme *sc, const char *str, int len);
  pointer (*mk_character)(scheme *sc, int c);
  pointer (*mk_vector)(scheme *sc, int len);
  pointer (*mk_foreign_func)(scheme *sc, foreign_func f);
  void (*putstr)(scheme *sc, const char *s);
  void (*putcharacter)(scheme *sc, int c);

  int (*is_string)(pointer p);
  char *(*string_value)(pointer p);
  int (*is_number)(pointer p);
  num (*nvalue)(pointer p);
  long (*ivalue)(pointer p);
  double (*rvalue)(pointer p);
  int (*is_integer)(pointer p);
  int (*is_real)(pointer p);
  int (*is_character)(pointer p);
  long (*charvalue)(pointer p);
  int (*is_list)(scheme *sc, pointer p);
  int (*is_vector)(pointer p);
  int (*list_length)(scheme *sc, pointer vec);
  long (*vector_length)(pointer vec);
  void (*fill_vector)(pointer vec, pointer elem);
  pointer (*vector_elem)(pointer vec, int ielem);
  pointer (*set_vector_elem)(pointer vec, int ielem, pointer newel);
  int (*is_port)(pointer p);

  int (*is_pair)(pointer p);
  pointer (*pair_car)(pointer p);
  pointer (*pair_cdr)(pointer p);
  pointer (*set_car)(pointer p, pointer q);
  pointer (*set_cdr)(pointer p, pointer q);

  int (*is_symbol)(pointer p);
  char *(*symname)(pointer p);

  int (*is_syntax)(pointer p);
  int (*is_proc)(pointer p);
  int (*is_foreign)(pointer p);
  char *(*syntaxname)(pointer p);
  int (*is_closure)(pointer p);
  int (*is_macro)(pointer p);
  pointer (*closure_code)(pointer p);
  pointer (*closure_env)(pointer p);

  int (*is_continuation)(pointer p);
  int (*is_promise)(pointer p);
  int (*is_environment)(pointer p);
  int (*is_immutable)(pointer p);
  void (*setimmutable)(pointer p);
  void (*load_file)(scheme *sc, FILE *fin);
  void (*load_string)(scheme *sc, const char *input);
};

typedef struct scheme_registerable
{
  foreign_func  f;
  const char *  name;
}
scheme_registerable;

TS_EXPORT void ts_register_foreign_func_list(scheme * sc,
                                       scheme_registerable * list,
                                       int n);
#endif
