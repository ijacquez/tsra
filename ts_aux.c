#include "common.h"

/* auxiliary 'module'
 * this module contains functions that are not needed to build the interpreter 
   in 'standalone' mode */

void scheme_register_foreign_func(ts_interp *sc, ts_registerable *sr) {
  ts_def(sc, sc->global_env, ts_mk_sym(sc, sr->name),
         ts_mk_foreign_func(sc, sr->f));
}

void ts_register_foreign_func_list(ts_interp *sc, ts_registerable *list,
                                   int count) {
  int i;
  for (i = 0; i < count; i++) {
    scheme_register_foreign_func(sc, list + i);
  }
}

ts_ptr ts_apply0(ts_interp *sc, const char *procname) {
  return ts_eval(sc, ts_cons(sc, ts_mk_sym(sc, procname), sc->nil));
}

void save_from_C_call(ts_interp *sc) {
  ts_ptr saved_data =
      ts_cons(sc, car(sc->sink), ts_cons(sc, sc->envir, sc->dump));
  /* Push */
  sc->c_nest = ts_cons(sc, saved_data, sc->c_nest);
  /* Truncate the dump stack so TS will return here when done, not
     directly resume pre-C-call operations. */
  dump_stack_reset(sc);
}

void restore_from_C_call(ts_interp *sc) {
  car(sc->sink) = caar(sc->c_nest);
  sc->envir = cadar(sc->c_nest);
  sc->dump = cdr(cdar(sc->c_nest));
  /* Pop */
  sc->c_nest = cdr(sc->c_nest);
}

/* "func" and "args" are assumed to be already eval'ed. */
ts_ptr ts_call(ts_interp *sc, ts_ptr func, ts_ptr args) {
  bool old_repl = sc->interactive_repl;
  sc->interactive_repl = false;
  save_from_C_call(sc);
  sc->envir = sc->global_env;
  sc->args = args;
  sc->code = func;
  sc->retcode = 0;
  Eval_Cycle(sc, OP_APPLY);
  sc->interactive_repl = old_repl;
  restore_from_C_call(sc);
  return sc->value;
}

ts_ptr ts_eval(ts_interp *sc, ts_ptr obj) {
  bool old_repl = sc->interactive_repl;
  sc->interactive_repl = false;
  save_from_C_call(sc);
  sc->args = sc->nil;
  sc->code = obj;
  sc->retcode = 0;
  Eval_Cycle(sc, OP_EVAL);
  sc->interactive_repl = old_repl;
  restore_from_C_call(sc);
  return sc->value;
}

ts_err ts_load_file(ts_interp *sc, const char *name) {
  int status;
  FILE *file = fopen(name, "r");

  if (file == NULL) {
    return ts_fopen_err;
  }

  load_file(sc, file);
  status = fclose(file);

  if (status == EOF) {
    return ts_fclose_err;
  }

  return 0;
}

int ts_vec_len(ts_ptr vec) {
  if (ts_is_vec(vec)) {
    return ts_int_val(vec);
  }

  return ts_type_err;
}

ts_ptr ts_get_global(ts_interp *sc, ts_ptr env, const char *name) {
  ts_ptr sym = ts_mk_sym(sc, name);
  ts_ptr slot = find_slot_in_env(sc, env, sym, 0);

  return slot_value_in_env(slot);
}

bool ts_is_userdata(ts_ptr ptr) { return type(ptr) == T_USERDATA; }

void default_userdata_finalizer(void *ptr) {}

void ts_userdata_set_finalizer(ts_ptr userdata, void (*finalizer)(void *)) {
  userdata->userdata.finalizer = finalizer;
}

ts_ptr ts_mk_userdata(ts_interp *sc, void *ptr) {
  ts_ptr cell = get_cell(sc, sc->nil, sc->nil);

  typeflag(cell) = (T_USERDATA | T_ATOM);
  cell->userdata.ptr = ptr;
  cell->userdata.finalizer = default_userdata_finalizer;

  return cell;
}
