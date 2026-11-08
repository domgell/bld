#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef const char* cstr;

#define BLD_MAX_CMD_LEN 4096

cstr bld_fmt(cstr str, ...);
void bld_panic(cstr msg, ...);
void bld_log_error(cstr msg, ...);
void bld_log_info(cstr msg, ...);

// void bld_run_self(int argc, char** argv);

// --------------------------------------- :cmd ----------------------------------------

typedef struct {
    char str[BLD_MAX_CMD_LEN];
    u32 len;
} BldCmd;

BldCmd bld_cmd_new();
void bld_cmd_add(BldCmd* cmd, cstr string);
void bld_cmd_pop(BldCmd* cmd);
i32 bld_cmd_run(BldCmd* cmd);

// void bld_cmd_add_fmt(BldCmd* cmd, cstr fmt, ...);
// void bld_cmd_clear(BldCmd* cmd);

// typedef struct {
//     cstr error;
//     cstr output;
//     i32 code;
// } BldCmdOut;
//
// BldCmdOut bld_cmd_run_output(BldCmd* cmd);

// --------------------------------------- :lib ----------------------------------------

typedef struct {
    void* handle;
} BldLib;

BldLib bld_lib_new(cstr path);
void bld_lib_unload(BldLib* lib);
void* bld_lib_load_fn(BldLib* lib, cstr fn_name);

// ------------------------------------- :watcher --------------------------------------

typedef struct {
    cstr path;
    u64 last_modified_time;
    u64 last_check_time;
    f32 check_interval_seconds;
} BldWatcher;

BldWatcher bld_watcher_new(cstr path);
bool bld_watcher_check(BldWatcher* watcher);

// --------------------------------------- :dir ----------------------------------------

// bool bld_dir_exists(cstr path);
// bool bld_dir_remove(cstr path);
// bool bld_dir_watch_changes(cstr path);

// --------------------------------------- :impl ---------------------------------------

#ifdef BLD_IMPL

// libc
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

// Platform
// TODO: platform macro
#include <dlfcn.h>
#include <sys/stat.h>

// ----------------------------------- :impl.macros ------------------------------------

#ifndef BLD_MAX_TEMP_BYTES
#define BLD_MAX_TEMP_BYTES 1024 * 1024 * 4
#endif

// TODO: format assert condition in bld_panic message
#define bld_assert(condition) do { if (!(condition)) bld_panic("assertion failed"); } while(0);

// --------------------------------------- :impl ---------------------------------------

void* _bld_temp_alloc(usize byte_count) {
    static u8 bytes[BLD_MAX_TEMP_BYTES] = {};
    static usize next_byte = 0;
    bld_assert(byte_count <= BLD_MAX_TEMP_BYTES
        && "Temp allocation too large, consider increasing BLD_MAX_TEMP_BYTES");

    // Wrap around to start
    if (byte_count > BLD_MAX_TEMP_BYTES - next_byte) next_byte = 0;

    u8* ptr = bytes + next_byte;
    next_byte += byte_count;
    return ptr;
}

cstr _bld_fmt_va(cstr str, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);

    int len = vsnprintf(NULL, 0, str, args_copy);
    va_end(args_copy);
    if (len < 0) return "";

    char* buf = _bld_temp_alloc((usize)len + 1);
    va_copy(args_copy, args);
    vsnprintf(buf, (usize)len + 1, str, args_copy);
    va_end(args_copy);

    return buf;
}

cstr bld_fmt(cstr str, ...) {
    va_list args;
    va_start(args, str);

    cstr result = _bld_fmt_va(str, args);

    va_end(args);
    return result;
}

void bld_panic(cstr msg, ...) {
    va_list args;
    va_start(args, msg);

    cstr formatted_msg = _bld_fmt_va(msg, args);

    va_end(args);

    fprintf(stderr, "[bld panic]: %s\n", formatted_msg);
    exit(1);
}

void bld_log_error(cstr msg, ...) {
    va_list args;
    va_start(args, msg);

    cstr formatted_msg = _bld_fmt_va(msg, args);

    va_end(args);

    fprintf(stderr, "[bld error]: %s\n", formatted_msg);
}

void bld_log_info(cstr msg, ...) {
    va_list args;
    va_start(args, msg);

    cstr formatted_msg = _bld_fmt_va(msg, args);

    va_end(args);

    fprintf(stdout, "[bld]: %s\n", formatted_msg);
}

// ------------------------------------- :impl.cmd -------------------------------------

BldCmd bld_cmd_new() {
    return (BldCmd){};
}

void bld_cmd_add(BldCmd* cmd, cstr string) {
    usize len = strlen(string);
    bld_assert(cmd->len + len + 1 < BLD_MAX_CMD_LEN
        && "Command string too long, consider increasing BLD_MAX_CMD_LEN");
    if (cmd->len > 0) cmd->str[cmd->len++] = ' ';
    strncpy(cmd->str + cmd->len, string, len);
    cmd->len += len;
}

void bld_cmd_pop(BldCmd* cmd) {
    for (i32 i = cmd->len - 1; i >= 0; i--) {
        if (cmd->str[i] == ' ') {
            cmd->len = i;
            return;
        }
    }
    cmd->len = 0;
}

i32 bld_cmd_run(BldCmd* cmd) {
    return system(cmd->str);
}

// ------------------------------------- :impl.lib -------------------------------------

BldLib bld_lib_new(cstr path) {
    BldLib lib = {};
    lib.handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!lib.handle) bld_panic("Failed to load library '%s'\n%s", path, dlerror());
    return lib;
}

void bld_lib_unload(BldLib* lib) {
    dlclose(lib->handle);
    lib->handle = NULL;
}

void* bld_lib_load_fn(BldLib* lib, cstr fn_name) {
    void* fn = dlsym(lib->handle, fn_name);
    if (!fn) bld_panic("Failed to load function '%s'\n%s", fn_name, dlerror());
    return fn;
}

// ----------------------------------- :impl.watcher -----------------------------------

u64 _bld_path_last_modified_time(cstr path) {
    struct stat result;
    if (stat(path, &result) != 0) bld_panic("Failed to get last modified time for path '%s'", path);
    return result.st_mtime;
}

// u64 _bld_time_now() {
// }

BldWatcher bld_watcher_new(cstr path) {
    BldWatcher watcher = {};
    watcher.path = path;
    watcher.check_interval_seconds = 0.5f;
    watcher.last_modified_time = _bld_path_last_modified_time(path);
    return watcher;
}

bool bld_watcher_check(BldWatcher* watcher) {
    // TODO: check interval, check recursive

    u64 last_modified_time = _bld_path_last_modified_time(watcher->path);
    if (last_modified_time > watcher->last_modified_time) {
        watcher->last_modified_time = last_modified_time;
        return true;
    }

    return false;
}

// ------------------------------------- :impl.dir -------------------------------------

// bool bld_dir_exists(cstr path) {
//     struct stat info;
//     if (stat(path, &info) != 0) return false;
//     return (info.st_mode & S_IFDIR) != 0;
// }

// bool bld_dir_remove(cstr path) {
//     char cmd[BLD_MAX_CMD_LEN];
//     snprintf(cmd, sizeof(cmd), "rm -rf %s", path);
//     return system(cmd) == 0;
// }

#endif
