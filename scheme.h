/* SCHEME.H */

#ifndef _SCHEME_H
#define _SCHEME_H

#include <stdbool.h>
#include <stdio.h>

#ifndef _MSC_VER
#define TS_EXPORT
#else
#ifdef _SCHEME_SOURCE
#define TS_EXPORT __declspec(dllexport)
#else
#define TS_EXPORT __declspec(dllimport)
#endif
#endif

typedef struct ts_interp ts_interp;
typedef struct ts_cell *ts_ptr;

typedef void *(*ts_func_alloc)(size_t);
typedef void (*ts_func_dealloc)(void *);
typedef ts_ptr (*ts_foreign_func)(ts_interp *, ts_ptr);

/* num, for generic arithmetic */
typedef struct ts_num {
  char is_fixnum;
  union {
    int ivalue;
    double rvalue;
  } value;
} ts_num;

typedef enum ts_port_kind {
  TS_PORT_FREE = 0,
  TS_PORT_FILE = 1,
  TS_PORT_STRING = 2,
  TS_PORT_SRFI6 = 4,
  TS_PORT_INPUT = 16,
  TS_PORT_OUTPUT = 32,
  TS_PORT_SAW_EOF = 64
} ts_port_kind;

typedef struct ts_port {
  ts_port_kind kind;
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
} ts_port;

/* cell structure */
struct ts_cell {
  unsigned int _flag;
  union {
    struct {
      char *_svalue;
      int _length;
    } _string;
    ts_num _number;
    ts_port *_port;
    ts_foreign_func _ff;
    struct {
      struct ts_cell *_car;
      struct ts_cell *_cdr;
    } _cons;
  } _object;
  union {
    void *ptr;
    void (*finalizer)(void *);
  } userdata;
};

struct ts_interp {
  /* arrays for segments */
  ts_func_alloc malloc;
  ts_func_dealloc free;

  /* return code */
  int retcode;
  int tracing;

#ifndef TS_CELL_SEGSIZE
#define TS_CELL_SEGSIZE 5000 /* # of cells in one segment */
#endif
#ifndef TS_CELL_NSEGMENT
#define TS_CELL_NSEGMENT 10 /* # of segments for cells */
#endif
  char *alloc_seg[TS_CELL_NSEGMENT];
  ts_ptr cell_seg[TS_CELL_NSEGMENT];
  int last_cell_seg;

  /* We use 4 registers. */
  ts_ptr args;  /* register for arguments of function */
  ts_ptr envir; /* stack register for current environment */
  ts_ptr code;  /* register for current code */
  ts_ptr dump;  /* stack register for next evaluation */

  bool interactive_repl; /* are we in an interactive REPL? */

  struct ts_cell _sink;
  ts_ptr sink; /* when mem. alloc. fails */
  struct ts_cell _nil;
  ts_ptr nil; /* special cell representing empty cell */
  struct ts_cell _hasht;
  ts_ptr T; /* special cell representing #t */
  struct ts_cell _hashf;
  ts_ptr F; /* special cell representing #f */
  struct ts_cell _eof_obj;
  ts_ptr eof_obj;    /* special cell representing end-of-file object */
  ts_ptr oblist;     /* pointer to symbol table */
  ts_ptr global_env; /* pointer to global environment */
  ts_ptr c_nest;     /* stack for nested calls from C */

  /* global pointers to special symbols */
  ts_ptr lambda; /* pointer to syntax lambda */
  ts_ptr quote;  /* pointer to syntax quote */

  ts_ptr qquote;       /* pointer to symbol quasiquote */
  ts_ptr unquote;      /* pointer to symbol unquote */
  ts_ptr unquotesp;    /* pointer to symbol unquote-splicing */
  ts_ptr feed_to;      /* => */
  ts_ptr colon_hook;   /* *colon-hook* */
  ts_ptr error_hook;   /* *error-hook* */
  ts_ptr sharp_hook;   /* *sharp-hook* */
  ts_ptr compile_hook; /* *compile-hook* */

  ts_ptr free_cell; /* pointer to top of free cells */
  int fcells;       /* # of free cells */

  ts_ptr inport;
  ts_ptr outport;
  ts_ptr save_inport;
  ts_ptr loadport;

#ifndef TS_MAXFIL
#define TS_MAXFIL 64
#endif
  ts_port load_stack[TS_MAXFIL]; /* Stack of open files for port -1 (LOADing) */
  int nesting_stack[TS_MAXFIL];
  int file_i;
  int nesting;

  bool gc_verbose; /* if gc_verbose is not zero, print gc status */
  bool no_memory;  /* Whether mem. alloc. has failed */

#ifndef TS_LINESIZE
#define TS_LINESIZE 1024
#endif
  char linebuff[TS_LINESIZE];
#ifndef TS_STRBUFFSIZE
#define TS_STRBUFFSIZE 256
#endif
  char strbuff[TS_STRBUFFSIZE];

  FILE *tmpfp;
  int tok;
  int print_flag;
  ts_ptr value;
  int op;

  void *ext_data; /* For the benefit of foreign functions */
  int gensym_cnt;

  struct ts_interface *vptr;
  void *dump_base; /* pointer to base of allocated dump stack */
  int dump_size;   /* number of frames allocated for dump stack */
};

typedef enum ts_err {
  ts_fopen_err = -1,
  ts_fclose_err = -2,
  ts_type_err = -3,
} ts_err;

typedef struct ts_registerable {
  ts_foreign_func f;
  const char *name;
} ts_registerable;

TS_EXPORT ts_interp *ts_init_new(void);
TS_EXPORT ts_interp *ts_init_new_custom_alloc(ts_func_alloc malloc,
                                           ts_func_dealloc free);
TS_EXPORT int ts_init(ts_interp *sc);
TS_EXPORT bool ts_init_custom_alloc(ts_interp *sc, ts_func_alloc, ts_func_dealloc);
TS_EXPORT void ts_deinit(ts_interp *sc);
TS_EXPORT void ts_set_in_port_file(ts_interp *sc, FILE *fin);
TS_EXPORT void ts_set_in_port_str(ts_interp *sc, char *start, char *past_the_end);
TS_EXPORT void ts_set_out_port_file(ts_interp *sc, FILE *fin);
TS_EXPORT void ts_set_out_port_str(ts_interp *sc, char *start, char *past_the_end);
TS_EXPORT ts_err ts_load_file(ts_interp *sc, const char *name);
TS_EXPORT void ts_load_str(ts_interp *sc, const char *cmd);
TS_EXPORT ts_ptr ts_apply0(ts_interp *sc, const char *procname);
TS_EXPORT ts_ptr ts_call(ts_interp *sc, ts_ptr func, ts_ptr args);
TS_EXPORT ts_ptr ts_eval(ts_interp *sc, ts_ptr obj);
TS_EXPORT void ts_set_extern_data(ts_interp *sc, void *p);
TS_EXPORT void ts_def(ts_interp *sc, ts_ptr env, ts_ptr symbol, ts_ptr value);

TS_EXPORT ts_ptr ts_cons(ts_interp *sc, ts_ptr a, ts_ptr b);
TS_EXPORT ts_ptr ts_immutable_cons(ts_interp *sc, ts_ptr a, ts_ptr b);
TS_EXPORT ts_ptr ts_mk_int(ts_interp *sc, int num);
TS_EXPORT ts_ptr ts_mk_real(ts_interp *sc, double num);
TS_EXPORT ts_ptr ts_mk_sym(ts_interp *sc, const char *name);
TS_EXPORT ts_ptr ts_gen_sym(ts_interp *sc);
TS_EXPORT ts_ptr ts_mk_str(ts_interp *sc, const char *str);
TS_EXPORT ts_ptr ts_mk_counted_str(ts_interp *sc, const char *str, int len);
TS_EXPORT ts_ptr ts_mk_empty_str(ts_interp *sc, int len, char fill);
TS_EXPORT ts_ptr ts_mk_char(ts_interp *sc, char c);
TS_EXPORT ts_ptr ts_mk_foreign_func(ts_interp *sc, ts_foreign_func f);
TS_EXPORT void ts_put_str(ts_interp *sc, const char *s);
TS_EXPORT int ts_list_len(ts_interp *sc, ts_ptr a);
TS_EXPORT int ts_eqv(ts_ptr a, ts_ptr b);

TS_EXPORT bool ts_is_str(ts_ptr p);
TS_EXPORT char *ts_str_val(ts_ptr p);
TS_EXPORT bool ts_is_num(ts_ptr p);
TS_EXPORT ts_num ts_num_val(ts_ptr p);
TS_EXPORT int ts_int_val(ts_ptr p);
TS_EXPORT double ts_real_val(ts_ptr p);
TS_EXPORT bool ts_is_int(ts_ptr p);
TS_EXPORT bool ts_is_real(ts_ptr p);
TS_EXPORT bool ts_is_char(ts_ptr p);
TS_EXPORT int ts_char_val(ts_ptr p);
TS_EXPORT bool ts_is_vec(ts_ptr p);

TS_EXPORT bool ts_is_port(ts_ptr p);

TS_EXPORT bool ts_is_pair(ts_ptr p);
TS_EXPORT ts_ptr ts_pair_car(ts_ptr p);
TS_EXPORT ts_ptr ts_pair_cdr(ts_ptr p);
TS_EXPORT ts_ptr ts_set_car(ts_ptr p, ts_ptr q);
TS_EXPORT ts_ptr ts_set_cdr(ts_ptr p, ts_ptr q);

TS_EXPORT bool ts_is_sym(ts_ptr p);
TS_EXPORT char *ts_sym_name(ts_ptr p);
TS_EXPORT bool ts_has_prop(ts_ptr p);

TS_EXPORT bool ts_is_syntax(ts_ptr p);
TS_EXPORT bool ts_is_proc(ts_ptr p);
TS_EXPORT bool ts_is_foreign(ts_ptr p);
TS_EXPORT char *ts_syntax_name(ts_ptr p);
TS_EXPORT bool ts_is_closure(ts_ptr p);
TS_EXPORT bool ts_is_macro(ts_ptr p);
TS_EXPORT ts_ptr ts_closure_code(ts_ptr p);
TS_EXPORT ts_ptr ts_closure_env(ts_ptr p);

TS_EXPORT bool ts_is_continuation(ts_ptr p);
TS_EXPORT bool ts_is_promise(ts_ptr p);
TS_EXPORT bool ts_is_env(ts_ptr p);
TS_EXPORT bool ts_is_immutable(ts_ptr p);
TS_EXPORT void ts_set_immutable(ts_ptr p);
TS_EXPORT int ts_vec_len(ts_ptr vec);
TS_EXPORT ts_ptr ts_get_global(ts_interp *sc, ts_ptr env, const char *name);
TS_EXPORT void ts_fill_vec(ts_ptr vec, ts_ptr elem);
TS_EXPORT bool ts_is_list(ts_interp *sc, ts_ptr p);
TS_EXPORT ts_ptr ts_mk_vec(ts_interp *sc, int len);
TS_EXPORT void ts_put_char(ts_interp *sc, char c);
TS_EXPORT ts_ptr ts_reserve_cells(ts_interp *sc, int n);
TS_EXPORT ts_ptr ts_set_vec_elem(ts_ptr vec, int ielem, ts_ptr newel);
TS_EXPORT ts_ptr ts_vec_elem(ts_ptr vec, int ielem);
TS_EXPORT void ts_register_foreign_func_list(ts_interp *sc, ts_registerable *list,
                                             int n);
TS_EXPORT ts_ptr ts_mk_userdata(ts_interp *sc, void *ptr);
TS_EXPORT bool ts_is_userdata(ts_ptr ptr);
TS_EXPORT void ts_userdata_set_finalizer(ts_ptr userdata,
                                         void (*finalizer)(void *));

struct ts_interface {
  void (*def)(ts_interp *sc, ts_ptr env, ts_ptr symbol, ts_ptr value);
  ts_ptr (*cons)(ts_interp *sc, ts_ptr a, ts_ptr b);
  ts_ptr (*immutable_cons)(ts_interp *sc, ts_ptr a, ts_ptr b);
  ts_ptr (*reserve_cells)(ts_interp *sc, int n);
  ts_ptr (*mk_int)(ts_interp *sc, int num);
  ts_ptr (*mk_real)(ts_interp *sc, double num);
  ts_ptr (*mk_sym)(ts_interp *sc, const char *name);
  ts_ptr (*gen_sym)(ts_interp *sc);
  ts_ptr (*mk_str)(ts_interp *sc, const char *str);
  ts_ptr (*mk_counted_str)(ts_interp *sc, const char *str, int len);
  ts_ptr (*mk_char)(ts_interp *sc, char c);
  ts_ptr (*mk_vec)(ts_interp *sc, int len);
  ts_ptr (*mk_foreign_func)(ts_interp *sc, ts_foreign_func f);
  void (*put_str)(ts_interp *sc, const char *s);
  void (*put_char)(ts_interp *sc, char c);

  bool (*is_str)(ts_ptr p);
  char *(*str_val)(ts_ptr p);
  bool (*is_num)(ts_ptr p);
  ts_num (*num_val)(ts_ptr p);
  int (*int_val)(ts_ptr p);
  double (*real_val)(ts_ptr p);
  bool (*is_int)(ts_ptr p);
  bool (*is_real)(ts_ptr p);
  bool (*is_char)(ts_ptr p);
  int (*char_val)(ts_ptr p);
  bool (*is_list)(ts_interp *sc, ts_ptr p);
  bool (*is_vec)(ts_ptr p);
  int (*list_len)(ts_interp *sc, ts_ptr vec);
  void (*fill_vec)(ts_ptr vec, ts_ptr elem);
  ts_ptr (*vec_elem)(ts_ptr vec, int ielem);
  ts_ptr (*set_vec_elem)(ts_ptr vec, int ielem, ts_ptr newel);
  bool (*is_port)(ts_ptr p);

  bool (*is_pair)(ts_ptr p);
  ts_ptr (*pair_car)(ts_ptr p);
  ts_ptr (*pair_cdr)(ts_ptr p);
  ts_ptr (*set_car)(ts_ptr p, ts_ptr q);
  ts_ptr (*set_cdr)(ts_ptr p, ts_ptr q);

  bool (*is_sym)(ts_ptr p);
  char *(*sym_name)(ts_ptr p);

  bool (*is_syntax)(ts_ptr p);
  bool (*is_proc)(ts_ptr p);
  bool (*is_foreign)(ts_ptr p);
  char *(*syntax_name)(ts_ptr p);
  bool (*is_closure)(ts_ptr p);
  bool (*is_macro)(ts_ptr p);
  ts_ptr (*closure_code)(ts_ptr p);
  ts_ptr (*closure_env)(ts_ptr p);

  bool (*is_continuation)(ts_ptr p);
  bool (*is_promise)(ts_ptr p);
  bool (*is_env)(ts_ptr p);
  bool (*is_immutable)(ts_ptr p);
  void (*set_immutable)(ts_ptr p);
  void (*load_str)(ts_interp *sc, const char *input);
  void (*deinit)(ts_interp *sc);
  void (*set_in_port_file)(ts_interp *sc, FILE *fin);
  void (*set_in_port_str)(ts_interp *sc, char *start, char *past_the_end);
  void (*set_out_port_file)(ts_interp *sc, FILE *fin);
  void (*set_out_port_str)(ts_interp *sc, char *start, char *past_the_end);
  void (*set_extern_data)(ts_interp *sc, void *p);
  int (*eqv)(ts_ptr a, ts_ptr b);
  ts_ptr (*mk_empty_str)(ts_interp *sc, int len, char fill);
  ts_ptr (*load_ext)(ts_interp *sc, ts_ptr arglist);
  bool (*has_prop)(ts_ptr p);
  ts_err (*load_file)(ts_interp *sc, const char *name);
  ts_ptr (*apply0)(ts_interp *sc, const char *procname);
  ts_ptr (*call)(ts_interp *sc, ts_ptr func, ts_ptr args);
  ts_ptr (*eval)(ts_interp *sc, ts_ptr obj);
  int (*vec_len)(ts_ptr vec);
  ts_ptr (*get_global)(ts_interp *sc, ts_ptr env, const char *name);
  void (*register_foreign_func_list)(ts_interp *sc, ts_registerable *list, int n);
  ts_ptr (*mk_userdata)(ts_interp *sc, void *ptr);
  bool (*is_userdata)(ts_ptr ptr);
  void (*userdata_set_finalizer)(ts_ptr userdata, void (*finalizer)(void *));
};

#endif
