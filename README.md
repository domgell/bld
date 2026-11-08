Small C99 utilities for compiling and running programs in C/C++ source files instead of separate build scripts.

## Usage
In your build file, define how you want to compile and run your program.
```c
#define BLD_IMPL
#include "bld.h"

int main() {
    const char* src = "myapp.c";
    const char* out = "myapp";

    BldCmd build = bld_cmd_new();
    bld_cmd_add(&build, "cc");
    bld_cmd_add(&build, src);
    bld_cmd_add(&build, "-o");
    bld_cmd_add(&build, out);
    bld_assert(bld_cmd_run(&build) == 0);

    BldCmd run = bld_cmd_new();
    bld_cmd_add(&run, bld_fmt("./%s", out));
    bld_assert(bld_cmd_run(&run) == 0);
}
```

Then compile and run the build file, which in turn compiles and runs your actual program.
```bash
cc build.c -o build && ./build
```
