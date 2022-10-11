#pragma once

typedef enum {
    Severity_Debug,
    Severity_Info,
    Severity_Warn,
    Severity_Error,
    Severity_Fatal,
} Severity;

void setSeverity(Severity severity);

void vlog(char *file, int line, Severity severity,
    const char *format, ...);

#define logDebug(format, ...) vlog(__FILE__, __LINE__,\
    Severity_Debug, format, ## __VA_ARGS__)
#define logInfo(format, ...) vlog(__FILE__, __LINE__,\
    Severity_Info, format, ## __VA_ARGS__)
#define logWarn(format, ...) vlog(__FILE__, __LINE__,\
    Severity_Warn, format, ## __VA_ARGS__)
#define logError(format, ...) vlog(__FILE__, __LINE__,\
    Severity_Error, format, ## __VA_ARGS__)
#define logFatal(format, ...) vlog(__FILE__, __LINE__,\
    Severity_Fatal, format, ## __VA_ARGS__)
