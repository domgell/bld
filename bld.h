#ifndef BLD_H
#define BLD_H

// Try to infer platform
#if defined _WIN32
#define BLD_PLATFORM_WINDOWS
#elif defined __linux__
#define BLD_PLATFORM_LINUX
#elif defined __APPLE__
#define BLD_PLATFORM_APPLE
#else
#define BLD_PLATFORM_UNKNOWN
#warning "Cannot to infer BLD_PLATFORM macro"
#endif

#ifdef BLD_PLATFORM_LINUX
#define _XOPEN_SOURCE 700
#endif

#ifndef BLD_NO_TYPEDEF
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
#endif

cstr bld_fmt(cstr str, ...);
void bld_panic(cstr msg, ...);

// --------------------------------------- :log ----------------------------------------

void bld_log_info(cstr msg, ...);
void bld_log_error(cstr msg, ...);
void bld_log_warning(cstr msg, ...);

void bld_timer_begin(cstr msg);
void bld_timer_end();

// Rebuild and rerun the current build file if the build file was modified
// after the build executable was created.
// This means after compiling the build file once, it will always rebuild itself
// whenever it has been editted.
#define bld_self_rebuild(argc, argv) _bld_self_rebuild(argc, argv, __FILE__)

// --------------------------------------- :cmd ----------------------------------------

// Maximum number of characters allowed per command.
// Includes spaces added by `bld_cmd_add()`
#ifndef BLD_MAX_CMD_LEN
#define BLD_MAX_CMD_LEN 4096
#endif

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

// --------------------------------------- :path ----------------------------------------

// Returns true if a file/folder exists at the given path.
bool bld_path_exists(cstr path);
// Returns true if the path is a file (not a folder).
bool bld_path_is_file(cstr path);
// Returns temporary string of the absolute path of the given path.
cstr bld_path_absolute(cstr path);

// bool bld_path_remove(cstr path);
// cstr bld_path_file_extension(cstr path);

#ifndef BLD_PATH_ITER_MAX_DEPTH
#define BLD_PATH_ITER_MAX_DEPTH 32
#endif

typedef struct {
    cstr path;
    char** entries;
    u32 entry_count;
    u32 next_entry;
} _BldPathIterFrame;

typedef struct {
    _BldPathIterFrame stack[BLD_PATH_ITER_MAX_DEPTH];
    u32 stack_len;
} BldPathIter;

BldPathIter bld_path_iter_new(cstr path);
// Returns a temporary string of the next path
// Returns NULL when the iterator is finished.
cstr bld_path_iter_next(BldPathIter* iter);

// Returns the last modified time of the file/folder
// at the given path in nanoseconds since unspecified epoch.
u64 bld_path_modified_time(cstr path);

// Recursively checks if the path was modified since 
// the last call to `bld_path_is_modified` with the same path.
bool bld_path_is_modified(cstr path);

// --------------------------------------- :time ---------------------------------------

// Returns the current time in nanoseconds since unspecified epoch.
u64 bld_time_now_nano();

// --------------------------------------- :impl ---------------------------------------

#ifdef BLD_IMPL

// C standard library includes
#ifndef BLD_NO_CSTD
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#endif

// Platform includes for using files, dynamic libraries, time, etc.
#ifdef BLD_PLATFORM_LINUX
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#elif BLD_PLATFORM_WINDOWS
// TODO
#endif

// ----------------------------------- :impl.macros ------------------------------------

// Maximum size of the temp allocator until it wraps around to the start
// Default 4 MB
#ifndef BLD_MAX_TEMP_BYTES
#define BLD_MAX_TEMP_BYTES 1024 * 1024 * 4
#endif

// Maximum size of the persistent allocator which exists for the entire program lifetime
// Default 4 MB
#ifndef BLD_MAX_PERSISTENT_BYTES
#define BLD_MAX_PERSISTENT_BYTES 1024 * 1024 * 4
#endif

// TODO: format assert condition in bld_panic message
#ifndef bld_assert
#define bld_assert(condition) do { if (!(condition)) bld_panic("assertion failed"); } while(0);
#endif

#ifndef BLD_CC
#ifdef BLD_PLATFORM_LINUX
#define BLD_CC "cc"
#elif defined BLD_PLATFORM_WINDOWS
#define BLD_CC "cl"
#else
#define BLD_CC "cc"
#endif
#endif

#ifndef BLD_TEMP_FOLDER
#ifdef BLD_PLATFORM_LINUX
#define BLD_TEMP_FOLDER "/tmp"
#elif defined BLD_PLATFORM_WINDOWS
#define BLD_TEMP_FOLDER "C:\\Temp"
#else
#define BLD_TEMP_FOLDER "/tmp"
#endif
#endif

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

void* _bld_persistent_alloc(usize byte_count) {
    static u8 bytes[BLD_MAX_PERSISTENT_BYTES] = {};
    static usize next_byte = 0;

    bld_assert(next_byte + byte_count <= BLD_MAX_PERSISTENT_BYTES
        && "Ran out of persistent allocation, consider increasing BLD_MAX_PERSISTENT_BYTES");

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

void _bld_self_rebuild(i32 argc, const char** argv, cstr current_file) {
    cstr exe_path = bld_path_absolute(argv[0]);
    u64 exe_modified_time = bld_path_modified_time(exe_path);

    cstr src_path = bld_path_absolute(current_file);
    u64 src_modified_time = bld_path_modified_time(src_path);

    if (src_modified_time <= exe_modified_time) return;

    // TODO: custom self rebuild command
    bld_log_info("Build file modified, performing self rebuild...");
    BldCmd cmd = bld_cmd_new();
    bld_cmd_add(&cmd, "cc");
    bld_cmd_add(&cmd, src_path);
    bld_cmd_add(&cmd, "-g");
    bld_cmd_add(&cmd, "-o");
    bld_cmd_add(&cmd, exe_path);
    bld_assert(bld_cmd_run(&cmd) == 0 && "Failed to self rebuild");

    // Relaunch the program
    execv(exe_path, (char* const*)argv);
}

// ------------------------------------- :impl.log -------------------------------------

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

void bld_log_warning(cstr msg, ...) {
    va_list args;
    va_start(args, msg);
    cstr formatted_msg = _bld_fmt_va(msg, args);
    va_end(args);

    fprintf(stdout, "[bld warning]: %s\n", formatted_msg);
}

// TODO: allow multiple timers
static u64 _bld_timer_start_time;
static cstr _bld_timer_msg;

void bld_timer_begin(cstr msg) {
    _bld_timer_msg = msg;
    _bld_timer_start_time = bld_time_now_nano();
}

void bld_timer_end() {
    u64 time = bld_time_now_nano() - _bld_timer_start_time;

    f32 time_ms = (f32)time / 1000000.0f;
    bld_log_info("%s (%.3f ms)", _bld_timer_msg, time_ms);
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

// ------------------------------------- :impl.path -------------------------------------

u64 bld_path_modified_time(cstr path) {
    struct stat result;
    if (stat(path, &result) != 0) bld_panic("Failed to get last modified time for path '%s'", path);
    return result.st_mtime;
}

bool bld_path_exists(cstr path) {
    return access(path, F_OK) == 0;
}

bool bld_path_is_file(cstr path) {
    struct stat result;
    if (stat(path, &result) != 0) bld_panic("Failed to access path '%s'", path);
    return S_ISREG(result.st_mode);
}

cstr bld_path_absolute(cstr path) {
    char* result = realpath(path, NULL);
    if (!result) bld_panic("Failed to resolve absolute path '%s'", path);

    usize byte_len = strlen(result) + 1;
    char* absolute_path = _bld_temp_alloc(byte_len);
    memcpy(absolute_path, result, byte_len);
    free(result);

    return absolute_path;
}

// TODO: dedicated temp alloc for path iteration
char** _bld_path_get_entries(cstr path, u32* count) {
    DIR* dir = opendir(path);
    bld_assert(dir != NULL && "Failed to open directory");

    u32 capacity = 16;
    u32 len = 0;
    char** entries = _bld_temp_alloc(capacity * sizeof(char*));

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0
        ) {
            continue;
        }

        if (len >= capacity) {
            capacity *= 2;

            char** new_entries = _bld_temp_alloc(capacity * sizeof(char*));
            memcpy(new_entries, entries, len * sizeof(char*));
            entries = new_entries;
        }

        usize name_len = strlen(entry->d_name) + 1;
        char* name = _bld_temp_alloc(name_len);
        memcpy(name, entry->d_name, name_len);

        entries[len++] = name;
    }

    closedir(dir);

    *count = len;
    return entries;
}

BldPathIter bld_path_iter_new(cstr path) {
    BldPathIter iter = {};

    if (!bld_path_is_file(path)) {
        bld_log_error("Path '%s' is not a folder, returning empty path iterator", path);
        return iter;
    }

    _BldPathIterFrame* frame = &iter.stack[0];
    frame->path = path;
    frame->entries = _bld_path_get_entries(path, &frame->entry_count);
    frame->next_entry = 0;
    iter.stack_len = 1;
    return iter;
}

cstr bld_path_iter_next(BldPathIter* iter) {
    while (iter->stack_len > 0) {
        _BldPathIterFrame* frame = &iter->stack[iter->stack_len - 1];

        // Current folder has no more entries
        if (frame->next_entry >= frame->entry_count) {
            iter->stack_len--;
            continue;
        }

        // Next entry
        cstr name = frame->entries[frame->next_entry++];
        cstr path = bld_fmt("%s/%s", frame->path, name);

        if (!bld_path_exists(path)) {
            bld_log_warning("Failed to access path '%s', skipping", path);
            continue;
        }

        // Go into subdirectories
        if (!bld_path_is_file(path) && iter->stack_len < BLD_PATH_ITER_MAX_DEPTH) {
            _BldPathIterFrame* child = &iter->stack[iter->stack_len++];

            child->path = path;
            child->entries = _bld_path_get_entries(path, &child->entry_count);
            child->next_entry = 0;
        }

        return path;
    }

    return NULL;
}

u64 _bld_path_hash(cstr str, usize len) {
    u64 hash = 5381;
    for (usize i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + (u8)str[i];
    }
    return hash;
}

typedef struct {
    u64 last_modified_time;
    char* path;
} _BldPathTimes;

// Maximum number of unique absolute paths for `bld_path_is_modified()`
#ifndef BLD_MAX_PATH_TIMES
#define BLD_MAX_PATH_TIMES 1024
#endif

// Time interval between `bld_path_is_modified()` checks in nanoseconds.
// This is to avoid excessive file api calls for folders with many subfolders.
// Default is 100,000,000 nanoseconds (0.1 seconds).
#ifndef BLD_PATH_MODIFIED_CHECK_INTERVAL_NANO
#define BLD_PATH_MODIFIED_CHECK_INTERVAL_NANO 100000000
#endif

_BldPathTimes* _bld_path_modified_time_map(cstr abs_path) {
    static _BldPathTimes* path_times_map = NULL;
    static u64 last_check_time = 0;

    u64 time = bld_time_now_nano();
    if (time - last_check_time < (u64)(BLD_PATH_MODIFIED_CHECK_INTERVAL_NANO)) {
        return NULL;
    }
    last_check_time = time;

    if (!path_times_map) {
        usize size = sizeof(_BldPathTimes) * BLD_MAX_PATH_TIMES;
        path_times_map = _bld_persistent_alloc(size);
        memset(path_times_map, 0, size);
    }

    u32 path_len = strlen(abs_path);
    u64 hash = _bld_path_hash(abs_path, path_len);

    usize index = hash % BLD_MAX_PATH_TIMES;

    for (usize i = 0; i < BLD_MAX_PATH_TIMES; i++) {
        _BldPathTimes* entry = &path_times_map[index];

        // Insert new entry
        if (entry->path == NULL) {
            entry->path = _bld_persistent_alloc(path_len + 1);
            memcpy(entry->path, abs_path, path_len + 1);

            entry->last_modified_time = bld_path_modified_time(abs_path);

            return entry;
        }

        // Return existing entry
        if (strncmp(entry->path, abs_path, path_len) == 0) {
            return entry;
        }

        index = (index + 1) % BLD_MAX_PATH_TIMES;
    }

    bld_log_error("Path map is full, consider increasing BLD_MAX_PATH_TIMES");
    return NULL;
}

bool bld_path_is_modified(cstr path) {
    cstr abs_path = bld_path_absolute(path);
    _BldPathTimes* entry = _bld_path_modified_time_map(abs_path);
    if (!entry) return false;

    // Path is file, no need to search recursively
    if (bld_path_is_file(path)) {
        u64 last_modified_time = bld_path_modified_time(abs_path);
        if (last_modified_time > entry->last_modified_time) {
            entry->last_modified_time = last_modified_time;
            return true;
        }
        return false;
    }

    // Path is folder, search all inner files/folders
    BldPathIter iter = bld_path_iter_new(abs_path);
    cstr next;
    while ((next = bld_path_iter_next(&iter))) {
        u64 last_modified_time = bld_path_modified_time(next);
        if (last_modified_time > entry->last_modified_time) {
            entry->last_modified_time = last_modified_time;
            return true;
        }
    }

    return false;
}

// ------------------------------------ :impl.time -------------------------------------

u64 bld_time_now_nano() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec;
}

#endif // BLD_IMPL
#endif // BLD_H