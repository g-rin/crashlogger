#include "backtrace.h"
#include "backtrace-supported.h"
#include <cxxabi.h>
#include <errno.h>
#include <execinfo.h>
#include <inttypes.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <QFileInfo>
#include <QDir>

#ifndef PRIxPTR
#    if __WORDSIZE == 64
#        define PRIxPTR "lx"
#    else
#        define PRIxPTR "x"
#    endif
#endif

static void sigHandler( int sig ) __attribute((noreturn));
static void writeCrashLog( const char* why, const int stackFramesToIgnore );

static FILE* reportFile = nullptr;
static char* demangleBuffer;
static size_t demangleBufferSize;
static char SIGSEGV_BUFFER[ SIGSTKSZ ];
static char SIGABRT_BUFFER[ SIGSTKSZ ];
static char SIGFPE_BUFFER[ SIGSTKSZ ];
static char SIGPIPE_BUFFER[ SIGSTKSZ ];
static char LOGBUFFER[ SIGSTKSZ ];

static struct timeval TIME_VAL;
static struct tm TIMESTAMP;
static char TIMESTAMP_BUFFER[64];

#pragma GCC diagnostic ignored "-Wformat-nonliteral"
static void printToLog( const char* format, ...) {
   static va_list arguments;
   static int length;
   va_start(arguments, format);
   length = vsprintf(LOGBUFFER, format, arguments);
   va_end(arguments);
   LOGBUFFER[ length ] = '\0';
   fprintf( stderr, "%s", LOGBUFFER );
   if ( reportFile ) {
       fprintf( reportFile, "%s", LOGBUFFER );
   }
}
#pragma GCC diagnostic warning "-Wformat-nonliteral"

static void printCurrentTime() {
    gettimeofday( &TIME_VAL, nullptr );
    gmtime_r( &TIME_VAL.tv_sec, &TIMESTAMP );
    strftime( TIMESTAMP_BUFFER, sizeof( TIMESTAMP_BUFFER ), "[%Y-%m-%d %H:%M:%S UTC] ", &TIMESTAMP );
    printToLog( "%s", TIMESTAMP_BUFFER );
}

void releaseCrashLogger() {
    fflush( stderr );

    if ( not reportFile ) {
        return;
    }

    fflush( reportFile );
    if ( 0 != fclose( reportFile ) ) {
        const int error_code = errno;
        printToLog( "Could not close crashlogger's file. Cause: %s\n", strerror( error_code ) );
    }
    reportFile = nullptr;

    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
}

static void sigHandler( int sig ) {
    // this lambda is the low-level signal handler multiplexer
    // resetToDefault(sig);
    static char buffer[ 256 ];
    snprintf( buffer, sizeof( buffer ), "uncaught signal %d (%s)", sig, sys_siglist[ sig ] );

    // 3 means to remove 3 stack frames: this way the backtrace starts at the point where
    // the signal reception interrupted the normal program flow
    writeCrashLog( buffer, 3 );
    releaseCrashLogger();
    abort();
}

static void sigPipeHandler( int ) {
    writeCrashLog( "SIGPIPE has been received!", 3 );
}

static void install( int sig, char*const buffer ) {
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
}

void reinitSignalHandlers() {
    install( SIGSEGV, SIGSEGV_BUFFER );
    install( SIGABRT, SIGABRT_BUFFER );
    install( SIGFPE, SIGFPE_BUFFER );

    stack_t sigpipestack;
    sigpipestack.ss_sp = SIGPIPE_BUFFER;
    sigpipestack.ss_size = SIGSTKSZ;
    sigpipestack.ss_flags = 0;
    sigaltstack(&sigpipestack, nullptr);
    struct sigaction sigpipe_action;
    sigpipe_action.sa_flags = SA_ONSTACK;
    sigpipe_action.sa_handler = sigPipeHandler;
    sigemptyset(&sigpipe_action.sa_mask);
    sigaction(SIGPIPE, &sigpipe_action, nullptr);
}

void initCrashLogger(  const char* report_filename ) {
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

    QFileInfo finfo( report_filename );
    auto logdir = finfo.dir();
    if ( not logdir.exists() ) {
        if ( not logdir.mkpath( logdir.path() ) ) {
            const int error_code = errno;
            printToLog( "Could create logdir '%s' for the crashlog. Cause: %s\n",
                        qPrintable( logdir.path() ),
                        strerror( error_code ) );
            return;
        }
    }

    reportFile = fopen( report_filename, "wa" );
    if ( not reportFile ) {
        const int error_code = errno;
        printToLog( "Could not open file '%s'. Cause: %s\n", report_filename, strerror( error_code ) );
        return;
    }

    demangleBufferSize = 512;
    demangleBuffer = reinterpret_cast< char* >( malloc( demangleBufferSize ) );

    install( SIGSEGV, SIGSEGV_BUFFER );
    install( SIGABRT, SIGABRT_BUFFER );
    install( SIGFPE, SIGFPE_BUFFER );

    reinitSignalHandlers();

    std::set_terminate([]() {
        static char buffer[ 1024 ];

        auto type = abi::__cxa_current_exception_type();
        if ( not type ) {
            // 3 means to remove 3 stack frames: this way the backtrace starts at std::terminate
            writeCrashLog( "terminate() was called although no exception was thrown", 3 );
            releaseCrashLogger();
            exit( 0 );
        }

        const char* typeName = type->name();
        if ( typeName ) {
            int status;
            abi::__cxa_demangle( typeName, demangleBuffer, &demangleBufferSize, &status );
            if ( status == 0 and *demangleBuffer ) {
                typeName = demangleBuffer;
            }
        }

        try {
            throw;
        } catch ( const std::exception &exc ) {
            snprintf( buffer, sizeof( buffer ), "uncaught exception of type %s (%s)",
                     typeName, exc.what() );
        } catch (...) {
            snprintf( buffer, sizeof( buffer ), "uncaught exception of type %s", typeName );
        }

        // 4 means to remove 4 stack frames: this way the backtrace starts at std::terminate
        writeCrashLog( buffer, 4 );
        releaseCrashLogger();
        exit( 0 );
    });
}

static void writeCrashLog( const char* why, const int stackFramesToIgnore ) {
    printCurrentTime();
    printToLog( "%s\n", why );

    static auto printBacktraceLine = []( int level,
                                         const char* symbol,
                                         uintptr_t offset,
                                         const char* file = nullptr,
                                         int line = -1)
    {
        printToLog( " %3d: ", level );

        if ( not symbol ) {
            printToLog( "--there is no valid data--\n" );
            return;
        }
        printToLog( "%s [%" PRIxPTR "]", symbol, offset );

        if ( not file ) {
            printToLog( "\n" );
            return;
        }

        printToLog( " in %s:%d\n", file, line );
    };

    struct btData {
        backtrace_state *state;
        int level;
    };

    static auto errorCallback = []( void *data,
                                    const char *msg,
                                    int errnum )
    {
        if ( data && msg ) {
            auto bt = static_cast< btData* >( data );
            printToLog( " %3d: ERROR: %s (%d)\n", bt->level, msg, errnum );
        } else {
            printToLog( " --level-not-defined--: ERROR: --not-defined-- (%d)\n", errnum );
        }
    };

    static auto syminfoCallback = []( void *data,
                                      uintptr_t pc,
                                      const char *symname,
                                      uintptr_t,
                                      uintptr_t )
    {
        if ( not data ) {
            printToLog( "Internal error - an invalid 'data' for syminfoCallback." );
            return;
        }

        int level = static_cast< btData* >( data )->level;
        if ( not symname ) {
            printBacktraceLine( level, nullptr, pc);
            return;
        }

        int status;
        abi::__cxa_demangle(symname, demangleBuffer, &demangleBufferSize, &status);

        if (status == 0 && *demangleBuffer) {
            printBacktraceLine(level, demangleBuffer, pc);
            return;
        }

        printBacktraceLine(level, symname, pc);
    };

    static auto fullCallback = []( void *data,
                                   uintptr_t pc,
                                   const char *filename,
                                   int lineno,
                                   const char *function) -> int
    {
        if ( not data ) {
            printToLog( "Internal error - an invalid 'data' for fullCallback.\n" );
            return 0;
        }

        auto bt = static_cast< btData* >( data );

        if ( not function ) {
            backtrace_syminfo( bt->state, pc, syminfoCallback, errorCallback, data );
            return 0;
        }

        int status;
        abi::__cxa_demangle(function, demangleBuffer, &demangleBufferSize, &status);

        printBacktraceLine( bt->level,
                           (status == 0 && *demangleBuffer) ? demangleBuffer : function, pc,
                           filename ? filename : "<unknown>", lineno);

        return 0;
    };

    static auto simpleCallback = []( void *data, uintptr_t pc ) -> int {
        if ( not data ) {
            printToLog( "Internal error - an invalid 'data' for simpleCallback.\n" );
            return 0;
        }

        auto bt = static_cast< btData* >( data );

        const int stack_frames_limit = 100;
        if ( bt->level > stack_frames_limit ) {
            return 1;
        }

        backtrace_pcinfo( bt->state, pc, fullCallback, errorCallback, data);
        bt->level++;
        return 0;
    };

    auto state = backtrace_create_state( nullptr, BACKTRACE_SUPPORTS_THREADS, errorCallback, nullptr );
    printToLog( "\n > backtrace:\n" );
    btData data = { state, 0 };
    backtrace_simple( state, stackFramesToIgnore, simpleCallback, errorCallback, &data );
}
