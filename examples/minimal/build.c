// cc build.c -o build && ./build

#define BLD_IMPL
#include "../../bld.h"

int main() {
    const char* out = "main";
    const char* src = "main.c";

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