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
    bld_cmd_add(&build_cmd, "-lX11"); // Windowing
    bld_cmd_add(&build_cmd, "-o");
    bld_cmd_add(&build_cmd, out);
    bld_assert(bld_cmd_run(&build_cmd) == 0);

    // Run app as separate process
    BldProc app_proc = bld_proc_run(out);

    int reload_count = 0;

    while (bld_proc_is_running(&app_proc)) {
        // Watch for file changes
        if (bld_path_is_modified(src)) {
            bld_log_info("Rebuilding...");

            // Set new out path and rebuild app
            const char* new_out_path = bld_fmt("/tmp/main%d.so", reload_count++);
            bld_cmd_pop(&build_cmd);
            bld_cmd_add(&build_cmd, new_out_path);
            if (bld_cmd_run(&build_cmd) != 0) {
                bld_log_error("Rebuild failed");
                continue;
            }

            // Restart the app
            bld_proc_kill(&app_proc);
            app_proc = bld_proc_run(new_out_path);
        }
    }
}
