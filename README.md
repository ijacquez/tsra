# Tinyscheme revised API
Tinyscheme fork focused on extending API and fixing bugs. 

## Building
1) Clone the repository
```
$ git clone git@github.com:iZarif/tsra.git
``` 

2) Change directory to `tsra`
```
$ cd tsra
```

3) Download [premake5](https://premake.github.io/download.html#v5)  
You can see the available options by entering into the terminal
```
$ [path_to_premake_binary] --help
```

There are switches (like --use-plist) that enable some additional features.

4) Generate "project files"  
This command, for example, will generate makefiles.
```
$ [path_to_premake_binary] gmake2
```

Now you can see the list of available targets by entering into the terminal
```
$ make help
```
5) For example, to build a static library, run the following command
```
$ make libtsra
```

The output file will be in the `bin/debug/` folder

## Code example
```C
#include "scheme.h"

// display -- scheme function
// example: (display "Hello")
// this version only displays strings
ts_ptr display(scheme *sc, ts_ptr args) {
  if (args != sc->nil) {
    if (ts_is_str(ts_pair_car(args))) {
      char *str = ts_str_val(ts_pair_car(args));
      printf("%s", str);
    }
  }
  return sc->nil;
}

// square -- scheme function
// example: (square 3)
ts_ptr square(scheme *sc, ts_ptr args) {
  if (args != sc->nil) {
    if (ts_is_num(ts_pair_car(args))) {
      double v = ts_real_val(ts_pair_car(args));
      return ts_mk_real(sc, v * v);
    }
  }
  return sc->nil;
}

int main(void) {
  scheme *sc;

  // init scheme interpreter
  sc = ts_init_new();

  // load init.scm
  ts_load_file(sc, "init.scm");

  // define square
  ts_def(sc, sc->global_env, ts_mk_sym(sc, "square"),
         ts_mk_foreign_func(sc, square));

  // define display
  ts_def(sc, sc->global_env, ts_mk_sym(sc, "display"),
         ts_mk_foreign_func(sc, display));

  // run first example
  char *hello_scm = "(display \"Hello, world!\n\")";
  ts_load_str(sc, hello_scm);

  // run second example
  char *square_scm =
      "(display "
      "  (string-append \"Answer: \" "
      "    (number->string (square 6.480740698407859)) \"\\n\"))";
  ts_load_str(sc, square_scm);

  // bye!
  ts_deinit(sc);
  return 0;
}
```

Code of this example is taken from https://github.com/dchest/tinyscheme/blob/master/example.c and modified.

## Building example code
Save the code above to a file named `example.c`

Compile `example.c` file
```
$ gcc example.c libtinyscheme.a -ldl -lm
```  

Run `a.out` file  
```
$ ./a.out
```  

## License 
This work is free. You can redistribute it and/or modify it under the
terms of the MIT license. See the [license](LICENSE) file for more details. 
