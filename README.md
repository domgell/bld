Header only C99 library for compiling and running programs directly via C source files instead of separate build scripts.

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
    bld_cmd_run(&build);

    BldCmd run = bld_cmd_new();
    bld_cmd_add(&run, bld_fmt("./%s", out));
    bld_cmd_run(&run);
}
```

Then compile and run the build file, which in turn compiles and runs your actual program.
```bash
cc build.c -o build && ./build
```

### Hotreloading Example
```c
// Build game DLL
BldCmd build_cmd = bld_cmd_new();
bld_cmd_add(&build_cmd, ...);
bld_cmd_run(&build_cmd);

// Load game DLL
BldLib lib = bld_lib_new(dll_path);
GameInitFn init_fn = bld_lib_load_fn(&lib, "init");
GameUpdateFn update_fn = bld_lib_load_fn(&lib, "update");

// Run game
GameState* game = init_fn();
while (update_fn(game)) {
    // Automatically rebuild game DLL when source files change
    if (bld_dir_modified(src_path)) {
        bld_cmd_run(&build_cmd);
        lib = bld_lib_new(dll_path);
        update_fn = bld_lib_load_fn(&lib, "update");
    }
}
```

