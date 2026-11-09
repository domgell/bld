// cc build.c -o build && ./build

#define BLD_IMPL
#include "../../bld.h"

int main(int argc, const char** argv) {
    bld_self_rebuild(argc, argv);

    const char* out = "/tmp/main.so";
    const char* src = "main.c";

    // Compile the app for the first time
    BldCmd build_cmd = bld_cmd_new();
    bld_cmd_add(&build_cmd, "cc");
    bld_cmd_add(&build_cmd, src);
    bld_cmd_add(&build_cmd, "-shared");
    bld_cmd_add(&build_cmd, "-fPIC");
    bld_cmd_add(&build_cmd, "-lX11"); // Windowing
    bld_cmd_add(&build_cmd, "-o");
    bld_cmd_add(&build_cmd, out);
    bld_assert(bld_cmd_run(&build_cmd) == 0);

    // Load app functions
    BldLib lib = bld_lib_new(out);
    void* (*init)() = bld_lib_load_fn(&lib, "init");
    bool (*update)(void*) = bld_lib_load_fn(&lib, "update");

    int reload_count = 0;

    void* state = init();
    while (update(state)) {
        // Watch for file changes
        if (bld_path_is_modified(src)) {
            bld_log_info("Reloading...");

            // Set new out path and rebuild library
            const char* new_lib_path = bld_fmt("/tmp/main%d.so", reload_count++);
            bld_cmd_pop(&build_cmd);
            bld_cmd_add(&build_cmd, new_lib_path);
            if (bld_cmd_run(&build_cmd) != 0) {
                bld_log_error("Reload failed");
                continue;
            }

            // Reload the library
            lib = bld_lib_new(new_lib_path);
            update = bld_lib_load_fn(&lib, "update");
        }
    }
}