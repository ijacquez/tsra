#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "dynload.h"

bool is_file_exist(const char *fname);

int main(int argc, char **argv) {
  ts_interp sc;
  FILE *fin = NULL;
  const char *file_name = "init.scm";
  int retcode;
  int isfile = 1;

  if (argc == 1) {
    printf("%s", "TinyScheme 1.42");
  }
  if (argc == 2 && strcmp(argv[1], "-?") == 0) {
    printf("Usage: tinyscheme -?\n");
    printf("or:    tinyscheme [<file1> <file2> ...]\n");
    printf("followed by\n");
    printf("          -1 <file> [<arg1> <arg2> ...]\n");
    printf("          -c <Scheme commands> [<arg1> <arg2> ...]\n");
    printf("assuming that the executable is named tinyscheme.\n");
    printf("Use - as filename for stdin.\n");
    return 1;
  }
  if (!ts_init(&sc)) {
    fprintf(stderr, "Could not initialize!\n");
    return 2;
  }
  ts_set_in_port_file(&sc, stdin);
  ts_set_out_port_file(&sc, stdout);
#if USE_DL
  ts_def(&sc, sc.global_env, ts_mk_sym(&sc, "load-extension"),
         ts_mk_foreign_func(&sc, ts_load_ext));
#endif
  argv++;
  if (!is_file_exist(file_name)) {
    char *p = getenv("TINYSCHEMEINIT");
    if (p != 0) {
      file_name = p;
    }
  }
  do {
    if (strcmp(file_name, "-") == 0) {
      fin = stdin;
    } else if (strcmp(file_name, "-1") == 0 || strcmp(file_name, "-c") == 0) {
      ts_ptr args = sc.nil;
      isfile = file_name[1] == '1';
      file_name = *argv++;
      if (strcmp(file_name, "-") == 0) {
        fin = stdin;
      } else if (isfile) {
        fin = fopen(file_name, "r");
      }
      for (; *argv; argv++) {
        ts_ptr value = ts_mk_str(&sc, *argv);
        args = ts_cons(&sc, value, args);
      }
      args = reverse_in_place(&sc, sc.nil, args);
      ts_def(&sc, sc.global_env, ts_mk_sym(&sc, "*args*"), args);

    } else {
      fin = fopen(file_name, "r");
    }
    if (isfile && fin == 0) {
      fprintf(stderr, "Could not open file %s\n", file_name);
    } else {
      if (isfile) {
        load_named_file(&sc, fin, file_name);
      } else {
        ts_load_str(&sc, file_name);
      }
      if (!isfile || fin != stdin) {
        if (sc.retcode != 0) {
          fprintf(stderr, "Errors encountered reading %s\n", file_name);
        }
        if (isfile) {
          fclose(fin);
        }
      }
    }
    file_name = *argv++;
  } while (file_name != 0);
  if (argc == 1) {
    load_named_file(&sc, stdin, 0);
  }
  retcode = sc.retcode;
  ts_deinit(&sc);

  return retcode;
}
