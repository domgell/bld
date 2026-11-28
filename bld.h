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

// Attribute to get proper compiler warnings for printf-style format string literals
#ifndef BLD_NO_FMT_ATTRIBUTE
#if defined(__GNUC__) || defined(__clang__)
#define BLD_FMT_VARAGS(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define BLD_FMT_VARAGS(fmt, args)
#endif
#endif

// Format a string with printf-style formatting and return a temporary string.
cstr bld_fmt(cstr str, ...) BLD_FMT_VARAGS(1, 2);
void bld_panic(cstr msg, ...) BLD_FMT_VARAGS(1, 2);

// --------------------------------------- :log ----------------------------------------

void bld_log_info(cstr msg, ...) BLD_FMT_VARAGS(1, 2);
void bld_log_error(cstr msg, ...) BLD_FMT_VARAGS(1, 2);
void bld_log_warning(cstr msg, ...) BLD_FMT_VARAGS(1, 2);

void bld_timer_begin(cstr msg);
u64 bld_timer_end();
// Set to enable/disable timer logging from `bld_timer_end()`.
// Enabled by default
void bld_timer_log_enable(bool enable);

// Rebuild and rerun the current build file if the build file was modified
// after the build executable was created.
// This means after compiling the build file once, it will always rebuild itself
// whenever it has been editted.
#define bld_self_rebuild(argc, argv) _bld_self_rebuild(argc, argv, __FILE__)

// --------------------------------------- :cmd ----------------------------------------

// Maximum number of characters allowed per command.
// Includes spaces added by `bld_cmd_add()`
#ifndef BLD_MAX_CMD_LEN
#define BLD_MAX_CMD_LEN (4096 - 4)
#endif

typedef struct {
    char str[BLD_MAX_CMD_LEN];
    u32 len;
} BldCmd;

BldCmd bld_cmd_new();
void bld_cmd_add(BldCmd* cmd, cstr string);
void bld_cmd_pop(BldCmd* cmd);
i32 bld_cmd_run(BldCmd* cmd);

// TODO: capture command output
// typedef struct {
//     cstr error;
//     cstr output;
//     i32 code;
// } BldCmdOut;
//
// BldCmdOut bld_cmd_run_output(BldCmd* cmd);

// TODO: run commands async
// bld_cmd_run_async
// bld_cmd_run_async_output

// --------------------------------------- :lib ----------------------------------------

typedef struct {
    void* handle;
} BldLib;

BldLib bld_lib_new(cstr path);
void bld_lib_unload(BldLib* lib);
void* bld_lib_load_fn(BldLib* lib, cstr fn_name);

// --------------------------------------- :proc ---------------------------------------

typedef struct {
    void* handle;
} BldProc;

// Start a new process asynchronously.
BldProc bld_proc_run(cstr path);
// Start a new process asynchronously with null terminated arguments list.
BldProc bld_proc_run_argv(cstr path, cstr* argv);

// Force the process to terminate.
// Waits until the process has terminated.
void bld_proc_kill(BldProc* proc);

// Returns true if the process is running.
// Returns false if the process has exited or has been killed.
bool bld_proc_is_running(BldProc* proc);

// Wait indefinitely for the process to exit and return its exit code.
i32 bld_proc_wait(BldProc* proc);
// Wait for the process to exit with a timeout in nanoseconds.
// Returns true if the process exited before the timeout and sets `exit_code`.
// Returns false if the process is still running after the timeout and does not set `exit_code`.
bool bld_proc_wait_timeout(BldProc* proc, u64 timeout_nano, i32* exit_code);

// --------------------------------------- :path ----------------------------------------

// Returns true if a file/folder exists at the given path.
bool bld_path_exists(cstr path);
// Returns true if the path is a file (not a folder).
bool bld_path_is_file(cstr path);
// Returns temporary string of the absolute path of the given path.
cstr bld_path_absolute(cstr path);

// TODO: path utilities
// bool bld_path_create(cstr path);
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

// Recursively check to get the maximum (most recent) path modified time.
// This can be slow since it must check every inner folder and file.
u64 bld_path_latest_modified_time(cstr path);

// Returns the last modified time of the file/folder at the given path in nanoseconds.
// Unlike `bld_path_latest_modified_time()` it does not recursively check inner folders.
u64 bld_path_modified_time(cstr path);

// Recursively checks if the path was modified since 
// the last call to `bld_path_is_modified` for the same path.
// Relative paths are converted to absolute paths.
bool bld_path_is_modified(cstr path);

// --------------------------------------- :time ---------------------------------------

// Returns the current time in nanoseconds since unix epoch.
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

// Platform includes for using files, dynamic libraries, processes, etc.
#if defined(BLD_PLATFORM_LINUX) || defined(BLD_PLATFORM_APPLE)
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <spawn.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#elif defined(BLD_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sys/stat.h>
#include <process.h>
#include <io.h>
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

#ifndef bld_assert
#define bld_assert(condition) do { if (!(condition)) bld_panic("%s [%d]\nassertion failed: %s", __FILE__, __LINE__, #condition); } while(0);
#endif

#ifndef BLD_CC
#ifdef BLD_PLATFORM_LINUX
#define BLD_CC "cc"
#elif defined BLD_PLATFORM_WINDOWS
// Ideally, use clang or gcc on windows instead of MSVC
// to be able to use the same CLI syntax as on other platforms.
#define BLD_CC "clang"
// #define BLD_CC "cl"
#else
#define BLD_CC "cc"
#endif
#endif

#ifndef BLD_TEMP_FOLDER
#ifdef BLD_PLATFORM_LINUX
#define BLD_TEMP_FOLDER "/tmp"
#elif defined BLD_PLATFORM_WINDOWS
#define BLD_TEMP_FOLDER "C:\\Windows\\Temp"
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

    // Check if source file was modified since last time
    cstr src_path = bld_path_absolute(current_file);
    u64 src_modified_time = bld_path_modified_time(src_path);
    if (src_modified_time > exe_modified_time) {
        bld_log_info("%s modified, performing self rebuild...", src_path);
        goto self_rebuild;
    }

    // Check if the bld header was modified since last time
    cstr bld_header_path = bld_path_absolute(__FILE__);
    u64 bld_header_modified_time = bld_path_modified_time(bld_header_path);
    if (bld_header_modified_time > exe_modified_time) {
        bld_log_info("%s modified, performing self rebuild...", bld_header_path);
        goto self_rebuild;
    }

    // No rebuild needed
    return;

self_rebuild:;
    // TODO: allow custom self rebuild command
    BldCmd rebuild_cmd = bld_cmd_new();
    bld_cmd_add(&rebuild_cmd, BLD_CC);
    bld_cmd_add(&rebuild_cmd, src_path);
    bld_cmd_add(&rebuild_cmd, "-g");
    bld_cmd_add(&rebuild_cmd, "-o");
    bld_cmd_add(&rebuild_cmd, exe_path);
    bld_assert(bld_cmd_run(&rebuild_cmd) == 0 && "Failed to self rebuild");

    // Relaunch the program
    execv(exe_path, (char* const*)argv);
}

// ------------------------------------- :impl.log -------------------------------------

#ifndef BLD_LOG_REPEAT_COUNTER_MAX_STR_LEN
#define BLD_LOG_REPEAT_COUNTER_MAX_STR_LEN 512
#endif

static char _bld_previous_log[BLD_LOG_REPEAT_COUNTER_MAX_STR_LEN] = {};
static u32 _bld_previous_log_increment = 0;

#define _bld_ansi_line_clear "\033[1A\033[2K"
#define _bld_ansi_text_bold "\033[1m"
#define _bld_ansi_text_reset "\033[0m"
#define _bld_ansi_color_red "\033[31m"
#define _bld_ansi_color_yellow "\033[33m"
#define _bld_ansi_color_green "\033[32m"

// Counter for repeated log messages.
// Enabled by default.
// Might not work correctly in some terminals.
#ifndef BLD_LOG_DISABLE_REPEAT_COUNTER
void _bld_log_str(cstr msg, FILE* stream) {
    if (strcmp(msg, _bld_previous_log) == 0) {
        _bld_previous_log_increment++;

        // Clear previous line
        fprintf(stream, _bld_ansi_line_clear);
        fflush(stream);

        fprintf(stream, "%s (x%d)\n", msg, _bld_previous_log_increment + 1);
    }
    else {
        fprintf(stream, "%s\n", msg);
        strcpy(_bld_previous_log, msg);
        _bld_previous_log_increment = 0;
    }
}
#else
void _bld_log_str(cstr msg, FILE* stream) {
    fprintf(stream, "%s\n", msg);
}
#endif

void bld_log_error(cstr msg, ...) {
    va_list args;
    va_start(args, msg);
    cstr fmt_msg = _bld_fmt_va(msg, args);
    va_end(args);

    cstr log_str = bld_fmt(_bld_ansi_color_red "[bld error]" _bld_ansi_text_reset " %s", fmt_msg);
    _bld_log_str(log_str, stderr);
}

void bld_log_info(cstr msg, ...) {
    va_list args;
    va_start(args, msg);
    cstr fmt_msg = _bld_fmt_va(msg, args);
    va_end(args);

    cstr log_str = bld_fmt(_bld_ansi_color_green "[bld]" _bld_ansi_text_reset " %s", fmt_msg);
    _bld_log_str(log_str, stdout);
}

void bld_log_warning(cstr msg, ...) {
    va_list args;
    va_start(args, msg);
    cstr fmt_msg = _bld_fmt_va(msg, args);
    va_end(args);

    cstr log_str = bld_fmt(_bld_ansi_color_yellow "[bld warning]" _bld_ansi_text_reset " %s", fmt_msg);
    _bld_log_str(log_str, stdout);
}

// TODO: stack to allow multiple timers, increase print/log indentation with each stack entry
static u64 _bld_timer_start_time;
static cstr _bld_timer_msg;
static bool _bld_timer_log_enabled = true;

void bld_timer_begin(cstr msg) {
    _bld_timer_msg = msg;
    _bld_timer_start_time = bld_time_now_nano();
}

u64 bld_timer_end() {
    u64 time_ns = bld_time_now_nano() - _bld_timer_start_time;

    f32 time_ms = (f32)time_ns / 1000000.0f;
    if (_bld_timer_log_enabled) bld_log_info("%s (%.3f ms)", _bld_timer_msg, time_ms);

    return time_ns;
}

void bld_timer_log_enable(bool enable) {
    _bld_timer_log_enabled = enable;
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

#if defined(BLD_PLATFORM_LINUX) || defined(BLD_PLATFORM_APPLE)

#define _bld_platform_lib_load(path)                    dlopen(path, RTLD_NOW | RTLD_LOCAL)
#define _bld_platform_lib_error_str()                   dlerror()
#define _bld_platform_lib_unload(handle)                dlclose(handle)
#define _bld_platform_lib_load_fn(handle, name)         dlsym(handle, name)

#elif defined(BLD_PLATFORM_WINDOWS)

#define _bld_platform_lib_load(path)                    LoadLibraryA(path)
// TODO: convert to proper string error message
#define _bld_platform_lib_error_str()                   bld_fmt("Win32 error code: %d", GetLastError())
#define _bld_platform_lib_unload(handle)                FreeLibrary(handle)
#define _bld_platform_lib_load_fn(handle, name)         GetProcAddress(handle, name)

#endif

BldLib bld_lib_new(cstr path) {
    BldLib lib = {};
    lib.handle = _bld_platform_lib_load(path);
    if (!lib.handle) bld_panic("Failed to load library '%s'\n%s", path, _bld_platform_lib_error_str());
    return lib;
}

void bld_lib_unload(BldLib* lib) {
    bld_assert(lib->handle && "Lib handle is NULL or unloaded");
    _bld_platform_lib_unload(lib->handle);
    lib->handle = NULL;
}

void* bld_lib_load_fn(BldLib* lib, cstr fn_name) {
    bld_assert(lib->handle && "Lib handle is NULL or unloaded");
    void* fn = _bld_platform_lib_load_fn(lib->handle, fn_name);
    if (!fn) bld_panic("Failed to load function '%s'\n%s", fn_name, _bld_platform_lib_error_str());
    return fn;
}

// ------------------------------------ :impl.proc -------------------------------------

#if defined(BLD_PLATFORM_LINUX) || defined(BLD_PLATFORM_APPLE)

extern char** environ;

BldProc bld_proc_run_argv(cstr path, cstr* argv) {
    pid_t pid;

    int error = posix_spawn(&pid, path, NULL, NULL, (char*const *)argv, environ);
    bld_assert(error == 0 && bld_fmt("Failed to spawn process %s: %s", path, strerror(error)));

    BldProc proc = {};
    proc.handle = (void*)(uintptr_t)pid;
    return proc;
}

void bld_proc_kill(BldProc* proc) {
    bld_assert(proc->handle && "Proc handle is NULL or process is not running");

    pid_t pid = (pid_t)(uintptr_t)proc->handle;
    kill(pid, SIGKILL);
    proc->handle = NULL;
}

bool bld_proc_is_running(BldProc* proc) {
    bld_assert(proc->handle && "Proc handle is NULL or process is not running");

    pid_t pid = (pid_t)(uintptr_t)proc->handle;
    int status;
    pid_t result;
    do {
        result = waitpid(pid, &status, WNOHANG);
    }
    while (result < 0 && errno == EINTR);

    // Not running
    if (result != 0) {
        proc->handle = NULL;
        return false;
    }

    return true;
}

bool bld_proc_wait_timeout(BldProc* proc, u64 timeout_nano, i32* exit_code) {
    bld_assert(proc->handle && "Proc handle is NULL or process is not running");

    pid_t pid = (pid_t)(uintptr_t)proc->handle;
    int status;
    u64 start_time = bld_time_now_nano();

    while (true) {
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result < 0 && errno != EINTR) {
            bld_log_error("Failed to wait for process %d: %s", pid, strerror(errno));
            return false;
        }

        // Process exited
        if (result != 0) {
            proc->handle = NULL;
            if (exit_code) *exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            return true;
        }

        // Check timeout
        u64 current_time = bld_time_now_nano();
        if (current_time - start_time >= timeout_nano) {
            return false;
        }
    }
}

#elif defined(BLD_PLATFORM_WINDOWS)

BldProc bld_proc_run_argv(cstr path, cstr* argv) {
    bld_panic("TODO: proc not implemented for Win32");
    return (BldProc){};
}

void bld_proc_kill(BldProc* proc) {
    bld_panic("TODO: proc not implemented for Win32");
}

bool bld_proc_is_running(BldProc* proc) {
    bld_panic("TODO: proc not implemented for Win32");
    return false;
}

bool bld_proc_wait_timeout(BldProc* proc, u64 timeout_nano, i32* exit_code) {
    bld_panic("TODO: proc not implemented for Win32");
    return false;
}

#endif

BldProc bld_proc_run(cstr path) {
    cstr argv[] = {path, NULL};
    return bld_proc_run_argv(path, argv);
}

i32 bld_proc_wait(BldProc* proc) {
    i32 exit_code;
    bool ok = bld_proc_wait_timeout(proc, UINT64_MAX, &exit_code);

    if (!ok) return -1; // Failed to wait for process
    return exit_code;
}

// ------------------------------------- :impl.path -------------------------------------

// *DEPRECATED*
#define _bld_path_latest_modified_time(path) bld_path_latest_modified_time(path)

#if defined(BLD_PLATFORM_LINUX) || defined(BLD_PLATFORM_APPLE)

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
    while ((entry = readdir(dir))) {
        // Skip
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Increase capacity
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

#elif defined(BLD_PLATFORM_WINDOWS)

// Number of 100-nanosecond intervals between January 1, 1606 (Win32 epoch) and January 1, 1970 (Unix epoch)
#define _bld_time_win32_epoch_magic 116444736000000000ULL

u64 bld_path_modified_time(cstr path) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    bool ok = GetFileAttributesExA(path, GetFileExInfoStandard, &data);
    if (!ok) bld_panic("Failed to get last modified time for path '%s'", path);

    ULARGE_INTEGER time;
    time.LowPart = data.ftLastWriteTime.dwLowDateTime;
    time.HighPart = data.ftLastWriteTime.dwHighDateTime;

    return (time.QuadPart - _bld_time_win32_epoch_magic) * 100;
}

bool bld_path_exists(cstr path) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    return GetFileAttributesExA(path, GetFileExInfoStandard, &data);
}

bool bld_path_is_file(cstr path) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    bool ok = GetFileAttributesExA(path, GetFileExInfoStandard, &data);
    if (!ok) bld_panic("Failed to access path '%s'", path);

    return !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

cstr bld_path_absolute(cstr path) {
    DWORD result_len = GetFullPathNameA(path, 0, NULL, NULL);
    if (result_len == 0) bld_panic("Failed to resolve absolute path '%s'", path);

    char* result = _bld_temp_alloc(result_len * sizeof(char));
    DWORD write_len = GetFullPathNameA(path, result_len, result, NULL);
    bld_assert(write_len > 0 && write_len < result_len);

    return result;
}

char** _bld_path_get_entries(cstr path, u32* count) {
    // Append wildcard to directory path.
    usize path_len = strlen(path);
    char* search_path = _bld_temp_alloc(path_len + 3);
    memcpy(search_path, path, path_len);
    search_path[path_len] = '\\';
    search_path[path_len + 1] = '*';
    search_path[path_len + 2] = '\0';

    WIN32_FIND_DATAA find_data;
    HANDLE find_handle = FindFirstFileA(search_path, &find_data);
    bld_assert(find_handle != INVALID_HANDLE_VALUE && "Failed to open directory");

    u32 capacity = 16;
    u32 len = 0;
    char** entries = _bld_temp_alloc(capacity * sizeof(char*));

    do {
        const char* name = find_data.cFileName;

        // Skip
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        // Increase capacity
        if (len >= capacity) {
            capacity *= 2;

            char** new_entries = _bld_temp_alloc(capacity * sizeof(char*));
            memcpy(new_entries, entries, len * sizeof(char*));
            entries = new_entries;
        }

        usize name_len = strlen(name) + 1;
        char* entry_name = _bld_temp_alloc(name_len);
        memcpy(entry_name, name, name_len);

        entries[len++] = entry_name;
    }
    while (FindNextFileA(find_handle, &find_data));

    FindClose(find_handle);

    *count = len;
    return entries;
}

#endif

BldPathIter bld_path_iter_new(cstr path) {
    BldPathIter iter = {};

    if (bld_path_is_file(path)) {
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

u64 bld_path_latest_modified_time(cstr path) {
    // Path is file, no need to check recursively
    if (bld_path_is_file(path)) {
        return bld_path_modified_time(path);
    }

    // Path is folder, search all inner files/folders
    u64 max_time = 0;
    BldPathIter iter = bld_path_iter_new(path);
    cstr next;
    while ((next = bld_path_iter_next(&iter))) {
        u64 last_modified_time = bld_path_modified_time(next);
        if (last_modified_time > max_time) {
            max_time = last_modified_time;
        }
    }

    return max_time;
}

typedef struct {
    u64 last_modified_time;
    u64 last_check_time;
    char* path;
} _BldPathTimes;

// Maximum number of unique absolute paths for `bld_path_is_modified()`
#ifndef BLD_MAX_PATH_TIMES
#define BLD_MAX_PATH_TIMES 1024
#endif

_BldPathTimes* _bld_path_time_map_insert(cstr abs_path) {
    static _BldPathTimes* path_times_map = NULL;

    // Init path times map once
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

            entry->last_check_time = bld_time_now_nano();
            entry->last_modified_time = bld_path_latest_modified_time(abs_path);

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

// Time interval between `bld_path_is_modified()` checks, for a unique path, in nanoseconds.
// This is to avoid excessive file api calls for folders with many subfolders.
// Default is 100,000,000 nanoseconds (0.1 seconds).
#ifndef BLD_PATH_MODIFIED_CHECK_INTERVAL_NANO
#define BLD_PATH_MODIFIED_CHECK_INTERVAL_NANO 100000000
#endif

bool bld_path_is_modified(cstr path) {
    cstr abs_path = bld_path_absolute(path);
    _BldPathTimes* entry = _bld_path_time_map_insert(abs_path);
    if (!entry) return false; // Path map is full

    // Check interval
    u64 current_time = bld_time_now_nano();
    if (current_time - entry->last_check_time < (u64)(BLD_PATH_MODIFIED_CHECK_INTERVAL_NANO)) {
        return false;
    }
    entry->last_check_time = current_time;

    // Path is file, no need to search recursively
    if (bld_path_is_file(abs_path)) {
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

#if defined(BLD_PLATFORM_LINUX) || defined(BLD_PLATFORM_APPLE)

u64 bld_time_now_nano() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec;
}

#elif defined(BLD_PLATFORM_WINDOWS)

u64 bld_time_now_nano() {
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);

    ULARGE_INTEGER time;
    time.LowPart = ft.dwLowDateTime;
    time.HighPart = ft.dwHighDateTime;

    return (time.QuadPart - _bld_time_win32_epoch_magic) * 100;
}

#endif

#endif // BLD_IMPL
#endif // BLD_H