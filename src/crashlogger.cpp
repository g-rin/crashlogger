#include "backtrace.h"
#include "backtrace-supported.h"
#include <cxxabi.h>
#include <signal.h>
#include <unistd.h>
#include <qglobal.h>
#ifndef PRIxPTR
#    if __WORDSIZE == 64
#        define PRIxPTR "lx"
#    else
#        define PRIxPTR "x"
#    endif
#endif

static bool printBacktrace;
static bool useAnsiColor;
static bool dumpCore;

static char *demangleBuffer;
static size_t demangleBufferSize;

static int stackFramesLimit = 100;

static char SIGSEGV_BUFFER[SIGSTKSZ];
static char SIGABRT_BUFFER[SIGSTKSZ];
static char SIGFPE_BUFFER[SIGSTKSZ];

static void writeCrashLog(const char *why, int stackFramesToIgnore) __attribute__((noreturn));

static void sigHandler(int sig)
{
    // this lambda is the low-level signal handler multiplexer
    // resetToDefault(sig);
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), "uncaught signal %d (%s)", sig, sys_siglist[sig]);
    // 3 means to remove 3 stack frames: this way the backtrace starts at the point where
    // the signal reception interrupted the normal program flow
    writeCrashLog(buffer, 3);
}

static void install(int sig, char*const buffer)
{
#if defined(Q_OS_UNIX)
    // Use alternate signal stack to get backtrace for stack overflow
    stack_t sigstack;
    sigstack.ss_sp = buffer;
    sigstack.ss_size = SIGSTKSZ;
    sigstack.ss_flags = 0;
    sigaltstack(&sigstack, nullptr);

    struct sigaction sigact;
    sigact.sa_flags = SA_ONSTACK;
    sigact.sa_handler = sigHandler;

    sigemptyset(&sigact.sa_mask);
    sigset_t unblockSet;
    sigemptyset(&unblockSet);

    sigaddset(&unblockSet, sig);
    sigaction(sig, &sigact, nullptr);

    sigprocmask(SIG_UNBLOCK, &unblockSet, nullptr);
#else
    signal(sig, sigHandler);
#endif
}

void initCrashlogger()
{
    // This can catch and pretty-print all of the following:

    // SIGFPE
    // volatile int i = 2;
    // int zero = 0;
    // i /= zero;

    // SIGSEGV
    // *((int *)1) = 1;

    // uncaught arbitrary exception
    // throw 42;

    // uncaught std::exception derived exception (prints what())
    // throw std::logic_error("test output");

    printBacktrace = true;
    dumpCore = true;

#if defined(Q_OS_UNIX)
    if (::isatty(STDERR_FILENO))
        useAnsiColor = true;
#else
    useAnsiColor = false;
#endif

    demangleBufferSize = 512;
    demangleBuffer = (char *)malloc(demangleBufferSize);

    // handling signals
    //{ SIGFPE, SIGSEGV, SIGILL, SIGBUS, SIGPIPE, SIGABRT },
    install(SIGSEGV, SIGSEGV_BUFFER );
    install(SIGABRT, SIGABRT_BUFFER);
    install(SIGFPE, SIGFPE_BUFFER);

    std::set_terminate([]() {
        static char buffer[1024];

        auto type = abi::__cxa_current_exception_type();
        if (!type)
        {
            // 3 means to remove 3 stack frames: this way the backtrace starts at std::terminate
            writeCrashLog("terminate was called although no exception was thrown", 3);
        }

        const char *typeName = type->name();
        if (typeName)
        {
            int status;
            abi::__cxa_demangle(typeName, demangleBuffer, &demangleBufferSize, &status);
            if (status == 0 && *demangleBuffer)
            {
                typeName = demangleBuffer;
            }
        }
        try
        {
            throw;
        }
        catch (const std::exception &exc)
        {
            snprintf(buffer, sizeof(buffer), "uncaught exception of type %s (%s)",
                     typeName, exc.what());
        }
        catch (...)
        {
            snprintf(buffer, sizeof(buffer), "uncaught exception of type %s", typeName);
        }

        // 4 means to remove 4 stack frames: this way the backtrace starts at std::terminate
        writeCrashLog(buffer, 4);
    });
}

static void writeCrashLog(const char *why, int stackFramesToIgnore)
{
    pid_t pid = getpid();
    char who[256];
    int whoLen = readlink("/proc/self/exe", who, sizeof(who) - 1);
    who[qMax(0, whoLen)] = '\0';

    fprintf(stderr, "\n*** process %s (%d) crashed ***\n\n > why: %s\n", who, pid, why);

    if (printBacktrace)
    {
#ifdef BACKTRACE_SUPPORTED
        struct btData
        {
            backtrace_state *state;
            int level;
        };

        static auto printBacktraceLine = [](int level, const char *symbol, uintptr_t offset, const char *file = nullptr,
                                            int line = -1) {
            const char *fmt1 = " %3d: %s [%" PRIxPTR "]";
            const char *fmt2 = " in %s:%d";
            if (useAnsiColor)
            {
                fmt1 = " %3d: \x1b[1m%s\x1b[0m [\x1b[36m%" PRIxPTR "\x1b[0m]";
                fmt2 = " in \x1b[35m%s\x1b[0m:\x1b[35;1m%d\x1b[0m";
            }

            fprintf(stderr, fmt1, level, symbol, offset);
            if (file)
                fprintf(stderr, fmt2, file, line);
            fputs("\n", stderr);
        };

        static auto errorCallback = [](void *data, const char *msg, int errnum) {
            const char *fmt = " %3d: ERROR: %s (%d)\n";
            if (useAnsiColor)
                fmt = " %3d: \x1b[31;1mERROR: \x1b[0;1m%s (%d)\x1b[0m\n";

            fprintf(stderr, fmt, static_cast<btData *>(data)->level, msg, errnum);
        };

        static auto syminfoCallback = [](void *data, uintptr_t pc, const char *symname, uintptr_t symval,
                                         uintptr_t symsize) {
            Q_UNUSED(symval)
            Q_UNUSED(symsize)

            int level = static_cast<btData *>(data)->level;
            if (symname)
            {
                int status;
                abi::__cxa_demangle(symname, demangleBuffer, &demangleBufferSize, &status);

                if (status == 0 && *demangleBuffer)
                    printBacktraceLine(level, demangleBuffer, pc);
                else
                    printBacktraceLine(level, symname, pc);
            }
            else
            {
                printBacktraceLine(level, nullptr, pc);
            }
        };

        static auto fullCallback = [](void *data, uintptr_t pc, const char *filename, int lineno,
                                      const char *function) -> int {
            if (function)
            {
                int status;
                abi::__cxa_demangle(function, demangleBuffer, &demangleBufferSize, &status);

                printBacktraceLine(static_cast<btData *>(data)->level,
                                   (status == 0 && *demangleBuffer) ? demangleBuffer : function, pc,
                                   filename ? filename : "<unknown>", lineno);
            }
            else
            {
                backtrace_syminfo(static_cast<btData *>(data)->state, pc, syminfoCallback, errorCallback, data);
            }
            return 0;
        };

        static auto simpleCallback = [](void *data, uintptr_t pc) -> int {
            if (static_cast<btData *>(data)->level > stackFramesLimit)
                return 1;

            backtrace_pcinfo(static_cast<btData *>(data)->state, pc, fullCallback, errorCallback, data);
            static_cast<btData *>(data)->level++;
            return 0;
        };

        struct backtrace_state *state =
            backtrace_create_state(nullptr, BACKTRACE_SUPPORTS_THREADS, errorCallback, nullptr);

        fprintf(stderr, "\n > backtrace:\n");
        btData data = {state, 0};
        // backtrace_print(state, stackFramesToIgnore, stderr);
        backtrace_simple(state, stackFramesToIgnore, simpleCallback, errorCallback, &data);
#endif  // defined(BACKTRACE_SUPPORTED)
    }
    if (dumpCore)
    {
        fprintf(stderr, "\n > the process will be aborted (core dump)\n\n");

        //
        //{ SIGFPE, SIGSEGV, SIGILL, SIGBUS, SIGPIPE, SIGABRT };
        signal(SIGSEGV, SIG_DFL);
        signal(SIGABRT, SIG_DFL);
        signal(SIGFPE, SIG_DFL);
        //
        abort();
    }
    _exit(-1);
}
