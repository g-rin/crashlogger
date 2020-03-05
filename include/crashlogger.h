#ifndef CRASHLOGGER_H
#define CRASHLOGGER_H

void initCrashLogger( const char* report_filename );
void releaseCrashLogger();

void reinitSignalHandlers();

#endif  // CRASHLOGGER_H
