# TSRA 0.0.2

`hasprop` function renamed to `ts_has_prop`  
`USE_SCHEME_STACK` define renamed to `USE_STACK`  
`ts_load_named_file` function removed from public API  
`ts_load_file` function implementation changed  
`vec_len` function added  
`ts_get_global` function added  
`ts_apply0` function added `ts_interface` struct  
`ts_call` function added `ts_interface` struct  
`ts_eval` function added `ts_interface` struct 

Mac OS 9 support code removed 

for integers the `int` type is now used instead of the `long` type  

`ts_eqv` function added to `ts_interface` struct  
`ts_set_extern_data` function added to `ts_interface` struct  
`ts_set_out_port_str` function added to `ts_interface` struct  
`ts_set_out_port_file` function added to `ts_interface` struct  
`ts_set_in_port_str` function added to `ts_interface` struct  
`ts_set_in_port_file` function added to `ts_interface` struct  
`ts_deinit` function added to `ts_interface` struct  

# TSRA 0.0.1

`set_vector_elem` function renamed to `ts_set_vec_elem`  
`vector_elem` function renamed to `ts_vec_elem`  
`fill_vector` function renamed to `ts_fill_vec`  
`is_list` function renamed to `ts_is_list`  
`_cons` function renamed to `ts_cons`  
`is_macro` function renamed to `ts_is_macro`  
`is_symbol` function renamed to `ts_is_sym`  
`is_pair` function renamed to `ts_is_pair`  
`ivalue` function renamed to `ts_int_val`  
`set_immutable` function renamed to `ts_set_immutable`  
`is_immutable` function renamed to `ts_is_immutable`  
`is_environment` function renamed to `ts_is_env`  
`is_promise` function renamed to `ts_is_promise`  
`closure_env` function renamed to `ts_closure_env`  
`closure_code` function renamed to `ts_closure_code`  
`is_closure` function renamed to `ts_is_closure`  
`syntaxname` function renamed to `ts_syntax_name`  
`is_foreign` function renamed to `ts_is_foreign`  
`is_proc` function renamed to `ts_is_proc`  
`is_syntax` function renamed to `ts_is_syntax`  
`symname` function renamed to `ts_sym_name`  
`set_cdr` function renamed to `ts_set_cdr`  
`set_car` function renamed to `ts_set_car`  
`pair_cdr` function renamed to `ts_pair_cdr`  
`pair_car` function renamed to `ts_pair_car`  
`is_port` function renamed to `ts_is_port`  
`is_vector` function renamed to `ts_is_vec`  
`charvalue` function renamed to `ts_char_val`  
`is_character` function renamed to `ts_is_char`  
`is_real` function renamed to `ts_is_real`  
`is_integer` function renamed to `ts_is_int`  
`rvalue` function renamed to `ts_real_val`  
`nvalue` function renamed to `ts_num_val`  
`is_number` function renamed to `ts_is_num`  
`string_value` function renamed to `ts_str_val`  
`is_string` function renamed to `ts_is_str`  
`putcharacter` function renamed to `ts_put_char`  
`mk_vector` function renamed to `ts_mk_vec`  
`reserve_cells` function renamed to `ts_reserve_cells`  
`eqv` function renamed to `ts_eqv`  
`list_length` function renamed to `ts_list_len`  
`putstr` function renamed to `ts_put_str`  
`mk_foreign_func` function renamed to `ts_mk_foreign_func`  
`mk_character` function renamed to `ts_mk_char`  
`mk_empty_string` function renamed to `ts_mk_empty_str`  
`mk_counted_string` function renamed to `ts_mk_counted_str`  
`mk_string` function renamed to `ts_mk_str`  
`gensym` function renamed to `ts_gen_sym`  
`mk_symbol` function renamed to `ts_mk_sym`  
`mk_real` function renamed to `ts_mk_real`  
`mk_integer` function renamed to `ts_mk_int`  
`scheme_define` function renamed to `ts_def`  
`scheme_set_external_data` function renamed to `ts_set_extern_data`  
`scheme_eval` function renamed to `ts_eval`  
`scheme_call` function renamed to `ts_call`  
`scheme_apply0` function renamed to `ts_apply0`  
`scheme_load_string` function renamed to `ts_load_str`  
`scheme_load_named_file` function renamed to `ts_load_named_file`  
`scheme_load_file` function renamed to `ts_load_file`  
`scheme_set_output_port_string` function renamed to `ts_set_out_port_str`  
`scheme_set_output_port_file` function renamed to `ts_set_out_port_file`  
`scheme_set_input_port_string` function renamed to `ts_set_in_port_str`  
`scheme_set_input_port_file` function renamed to `ts_set_in_port_file`  
`scheme_deinit` function renamed to `ts_deinit`  
`scheme_init_custom_alloc` function renamed to `ts_init_custom_alloc`  
`scheme_init` function renamed to `ts_init`  
`scheme_init_new_custom_alloc` function renamed to `ts_init_new_custom_alloc`  
`scheme_init_new` function renamed to `ts_init_new`  
`scheme_register_foreign_func_list` function renamed to `ts_register_foreign_func_list`  
  
`port_kind` stuct declaration moved to scheme.h  
`cell` stuct declaration moved to scheme.h  
`port` stuct declaration moved to scheme.h  
`scheme` stuct declaration moved to scheme.h  
  
`port_kind` enum renamed to `ts_port_kind`  
`num` struct renamed to `ts_num`  
`port` struct renamed to `ts_port`  
`cell` struct renamed to `ts_cell`  
`scheme_interface` struct renamed to `ts_interface`  
`scheme_registerable` struct renamed to `ts_registerable`

`features defines` moved to scheme.c
  
`EXPORT` define renamed to `TS_EXPORT`  
`STRBUFFSIZE` define renamed to `TS_STRBUFFSIZE`  
`LINESIZE` define renamed to `TS_LINESIZE`  
`MAXFIL` define renamed to `TS_MAXFIL`  
`CELL_NSEGMENT` define renamed to `TS_CELL_NSEGMENT`  

`INLINE` defined as `inline` keyword by default  
  
`pointer` typedef renamed to `ts_ptr`  
`func_alloc` typedef renamed to `ts_func_alloc`  
`func_dealloc` typedef renamed to `ts_func_dealloc`  
`foreign_func` typedef renamed to `ts_foreign_func`  
