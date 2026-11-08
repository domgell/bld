#define BLD_IMPL
#include "../../bld.h"

int main() {
    const char* out = "/tmp/main.so";
    const char* src = "main.c";

    // Compile the app for the first time
    BldCmd build_cmd = bld_cmd_new();
    bld_cmd_add(&build_cmd, "cc");
    bld_cmd_add(&build_cmd, src);
    bld_cmd_add(&build_cmd, "-shared");
    bld_cmd_add(&build_cmd, "-fPIC");
    bld_cmd_add(&build_cmd, "-lX11 -lXi"); // Windowing
    bld_cmd_add(&build_cmd, "-o");
    bld_cmd_add(&build_cmd, out);
    bld_assert(bld_cmd_run(&build_cmd) == 0);

    // Load app functions
    BldLib lib = bld_lib_new(out);
    void* (*init)() = bld_lib_load_fn(&lib, "init");
    bool (*update)(void*) = bld_lib_load_fn(&lib, "update");

    // Watch for file changes
    BldWatcher src_watcher = bld_watcher_new(src);
    u32 reload_count = 0;

    void* state = init();
    while (update(state)) {
        // Perform hot reload if source file changes
        if (bld_watcher_check(&src_watcher)) {
            bld_log_info("Reloading...");

            // Set new out path and rebuild library
            const char* lib_path = bld_fmt("/tmp/main%d.so", reload_count++);
            bld_cmd_pop(&build_cmd);
            bld_cmd_add(&build_cmd, lib_path);
            if (bld_cmd_run(&build_cmd) != 0) {
                bld_log_error("Reload failed");
                continue;
            }

            // Reload the library
            bld_lib_unload(&lib);
            lib = bld_lib_new(lib_path);
            update = bld_lib_load_fn(&lib, "update");
        }
    }
}